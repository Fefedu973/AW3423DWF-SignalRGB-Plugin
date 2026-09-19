const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
const assert=require('node:assert/strict');
const root=path.resolve(__dirname,'../..');
const out=path.resolve(__dirname,'..');
const fixturePath=path.join(out,'tests/fixtures.json');
fs.mkdirSync(path.dirname(fixturePath),{recursive:true});
const profiles=JSON.parse(fs.readFileSync(path.join(out,'profiles.json'))).profiles;
const checks=[];
function check(label,body){ body(); checks.push(label); }
function load(pid,options={}){
  const item=profiles.find(p=>Number(p.pid)===pid);
  const filename=item?item.plugin:'Alienware_Monitor_Gen3_Controller.js';
  const source=fs.readFileSync(path.join(out,filename),'utf8');
  let clock=10000;
  const writes=[],logs=[];
  const challenge=Array.from(Buffer.from('2c207684f155909167e480659d5236ef','hex'));
  const context=vm.createContext({console,Date:{now:()=>clock},IndividualLedControl:!!options.individual,
    LightingMode:options.canvas?'Canvas':'Forced',forcedColor:'#123456',shutdownColor:'#000000',TimingMode:'Recommended',delayRGB:200,
    device:{productId:()=>pid,getHidEndpoints:()=>options.missingEndpoint?[]:[{interface:0,usage:item?.protocol==='realtek'?0xda:1,usage_page:item?.protocol==='realtek'?0xffda:0xff00,collection:0}],
      set_endpoint(){},setSize(){},setName(){},setImageFromUrl(){},setControllableLeds(){},log:x=>logs.push(x),
      color:(x,y)=>options.color?options.color(x,y,clock):[10+x,20+y,30],pause:ms=>{clock+=ms;},
      write:(data,length)=>{ assert.equal(data.length,length); assert.equal(data[0],0); writes.push({data:Array.from(data),length,time:clock}); return options.write?options.write(Array.from(data),writes):length; },
      control_transfer:(...args)=>{assert.deepEqual(args.slice(0,4),[0xa1,1,0x0100,0]); return options.shortRead?challenge:challenge.concat(new Array(176).fill(0));}
    }});
  vm.runInContext(source.replace(/^export /gm,'')+'\nthis.api={Initialize,Render,Shutdown,monitor,PROFILES,colorPacket,authenticationResponse,OEM_KEYS,Validate};',context);
  return {context,api:context.api,writes,logs,tick:ms=>{clock+=ms;}};
}
const isColor=w=>w.data[2]===0xc6 && w.data[68]===4 || w.data[1]===0x92 && (w.data[8]===4 || w.data[5]===4);
for(const item of profiles){
  check('Initialize and render '+item.model,()=>{
    const run=load(Number(item.pid)); run.api.Initialize(); run.tick(1000); run.api.Render();
    const colors=run.writes.filter(isColor); assert.equal(colors.length,1);
    assert.equal(colors[0].length,item.protocol==='realtek'?193:65);
    const handshake=run.writes.some(w=>w.data[2]===0xe1);
    assert.equal(handshake,item.auth);
  });
}
check('Unknown PID and missing endpoint never write',()=>{
  for(const run of [load(0xeeee),load(0x101d,{missingEndpoint:true})]){run.api.Initialize();run.api.Render();assert.equal(run.writes.length,0);}
});
check('Short challenge never emits response or color',()=>{
  const run=load(0x100e,{shortRead:true});run.api.Initialize();run.api.Render();
  assert.equal(run.writes.filter(isColor).length,0);
  assert.equal(run.writes.filter(w=>w.data[2]===0xe1&&w.data[3]===2).length,0);
  const count=run.writes.length;run.api.Render();assert.equal(run.writes.length,count);
});
check('Fallback key probe is bounded, cached per instance',()=>{
  let probes=0;
  const run=load(0x100e,{write:(data)=>data[2]===0xc6&&data[7]===5?(++probes===1?-1:data.length):data.length});
  run.api.Initialize();run.api.Render();assert.equal(run.api.monitor.keyIndex,1);assert.equal(probes,2);
  run.tick(2200);run.api.Render();assert.equal(probes,2);
  const other=load(0x100e);other.api.Initialize();assert.equal(other.api.monitor.keyIndex,null);
});
check('All five rejected probes cause backoff and no color',()=>{
  const run=load(0x100e,{write:data=>data[2]===0xc6&&data[7]===5?-1:data.length});
  run.api.Initialize();run.api.Render();assert.equal(run.writes.filter(w=>w.data[2]===0xc6&&w.data[7]===5).length,5);
  assert.equal(run.writes.filter(isColor).length,0);
});
check('Unchanged colors skip output, keepalive reasserts, shutdown bypasses cache',()=>{
  const run=load(0x101d);run.api.Initialize();run.api.Render();run.tick(100);run.api.Render();assert.equal(run.writes.length,1);
  run.tick(2000);run.api.Render();assert.equal(run.writes.length,2);
  run.api.Shutdown(true);assert.equal(run.writes.length,3);assert.deepEqual(run.writes[2].data.slice(70,73),[0,0,0]);
});
check('Scheduler preserves interval without blocking, and does not starve a zone',()=>{
  const run=load(0x101d,{individual:true,canvas:true,color:(x,y,t)=>[x,Math.floor(t/20)%255,y]});
  run.api.Initialize();run.api.Render();run.tick(1);run.api.Render();assert.equal(run.writes.length,1);
  for(let i=0;i<5;i++){run.tick(100);run.api.Render();}
  assert.deepEqual(run.writes.map(w=>w.data[69]),[1,8,1,8,1,8]);
});
check('AW3426DW throttles at 100ms and drops intermediate animation colors',()=>{
  const run=load(0x101d,{canvas:true,color:(x,y,t)=>[Math.floor(t/10)%255,0,0]});
  run.api.Initialize();run.api.Render();
  const first=run.writes[0].data[70];
  for(let i=0;i<9;i++){run.tick(10);run.api.Render();}
  assert.equal(run.writes.length,1);
  run.tick(10);run.api.Render();
  assert.equal(run.writes.length,2);
  assert.equal(run.writes[1].data[70],(first+10)%255);
});
check('Same-color individual zones combine into one bitmap',()=>{
  const run=load(0x101d,{individual:true});run.api.Initialize();run.api.Render();assert.equal(run.writes.length,1);assert.equal(run.writes[0].data[69],9);
});
check('Partial writes invalidate cache and trigger retry backoff',()=>{
  const run=load(0x101d,{write:()=>12});run.api.Initialize();run.api.Render();assert.equal(Object.keys(run.api.monitor.sent).length,0);
  const count=run.writes.length;run.tick(20);run.api.Render();assert.equal(run.writes.length,count);
});

const fixtures=JSON.parse(fs.readFileSync(fixturePath,'utf8'));
const realtek=load(0x101d).api;
check('Captured authentication responses reproduced including zero bytes',()=>{
  for(const fixture of fixtures.authentication){
    const result=realtek.authenticationResponse(Array.from(Buffer.from(fixture.challenge,'hex')),realtek.OEM_KEYS[fixture.key]);
    assert.equal(Buffer.from(result).toString('hex'),fixture.response);
  }
  assert.equal(Buffer.from(realtek.authenticationResponse(new Array(16).fill(0),realtek.OEM_KEYS[0])).toString('hex'),'cafe7d7e487433f4');
  assert.throws(()=>realtek.authenticationResponse(new Array(15).fill(0),realtek.OEM_KEYS[0]));
});
check('All 16 full AWCC wire packets reproduced exactly',()=>{
  assert.equal(fixtures.wire.length,16);
  for(const fixture of fixtures.wire){
    const packet=realtek.colorPacket(realtek.PROFILES[0x101d],fixture.mask,fixture.rgb);
    assert.equal(Buffer.from(packet.slice(1)).toString('hex'),fixture.hex);
  }
});

const result={passed:checks.length,profiles:profiles.length,authentication_fixtures:fixtures.authentication.length,full_wire_fixtures:fixtures.wire.length,checks};
fs.writeFileSync(path.join(out,'tests/results.json'),JSON.stringify(result,null,2));
console.log(JSON.stringify(result,null,2));
