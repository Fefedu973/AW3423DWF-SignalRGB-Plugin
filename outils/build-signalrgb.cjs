const fs=require('node:fs');
const path=require('node:path');
const root=path.resolve(__dirname,'..');
const matrix=JSON.parse(fs.readFileSync(path.join(root,'models.json'),'utf8'));
const template=fs.readFileSync(path.join(__dirname,'controller-template.js'),'utf8');
const out=path.join(root,'signalrgb');
fs.mkdirSync(out,{recursive:true});
const groups=[{gen:1,vid:0x424,realtek:false},{gen:2,vid:0x187c,realtek:false},{gen:3,vid:0x187c,realtek:true}];
const selected=[];
const position=(zone,index)=>{
  const name=zone.name.toLowerCase();
  if (/power|switch/.test(name)) return [8,6];
  if (/down|bottom/.test(name)) return [4,6];
  if (/stand/.test(name)) return [4,2];
  if (/number/.test(name)) return [4,1];
  if (/logo|upper/.test(name)) return [0,1];
  return [Math.min(index*2,8),2];
};
for(const group of groups){
  const profiles={};
  for(const model of matrix.models){
    const vid=Number(model.vid),pid=Number(model.pid);
    if(vid!==group.vid || !pid || !model.zones.length || model.auth===null) continue;
    const protocol=model.protocol_family==='legacy_92_48'?'legacy':model.controller==='RTS5409S'?'realtek':model.controller==='MicroChip5734'?'microchip':null;
    if(!protocol || (protocol==='realtek')!==group.realtek) continue;
    if(profiles[pid]) throw new Error('Duplicate PID '+model.pid);
    const zones=model.zones.map((zone,index)=>({name:zone.name,mask:zone.mask,position:position(zone,index)}));
    if(zones.some(z=>!z.mask || (z.mask&(z.mask-1)))) throw new Error('Invalid zone mask '+model.model);
    profiles[pid]={name:model.model,protocol,auth:model.auth,zones,all:zones.reduce((m,z)=>m|z.mask,0),size:[9,7],
      interval:pid===0x101d?100:protocol==='realtek'?200:50};
    selected.push({model:model.model,vid:model.vid,pid:model.pid,plugin:'Alienware_Monitor_Gen'+group.gen+'_Controller.js',protocol,auth:model.auth,
      hardware_tested:model.hardware_tested_current_session===true});
  }
  const constants=`const PLUGIN_VID=${group.vid};\nconst PLUGIN_TYPE=${JSON.stringify(group.realtek?'hybrid':'hid')};\nconst PLUGIN_USAGE=${group.realtek?0xda:1};\nconst PLUGIN_USAGE_PAGE=${group.realtek?0xffda:0xff00};\nconst PROFILES=${JSON.stringify(profiles,null,2)};`;
  const source=template.replace('// GENERATED_PROFILES',constants);
  fs.writeFileSync(path.join(out,'Alienware_Monitor_Gen'+group.gen+'_Controller.js'),source);
}
fs.writeFileSync(path.join(out,'profiles.json'),JSON.stringify({generated:'2026-09-19',source:'../models.json',profiles:selected},null,2));
console.log(JSON.stringify(selected,null,2));
