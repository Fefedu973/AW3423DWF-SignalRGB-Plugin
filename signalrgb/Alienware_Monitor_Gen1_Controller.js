// Alienware monitor protocol work: Fefedu973, WhirlwindFX, Thanatos and Bruno St. John.
// Profiles generated from the research matrix; see the accompanying sources and tests.
export function Name() { return "Alienware Monitor"; }
export function VendorId() { return PLUGIN_VID; }
export function ProductId() { return Object.keys(PROFILES).map(Number); }
export function Publisher() { return "WhirlwindFX & Fefe_du_973"; }
export function Documentation() { return "troubleshooting/alienware"; }
export function Size() { return [1, 1]; }
export function DeviceType() { return "other"; }
export function Type() { return PLUGIN_TYPE; }
export function ImageUrl() { return "https://assets.signalrgb.com/devices/brands/alienware/misc/aw3423dw-monitor.png"; }
export function Validate(endpoint) {
    return endpoint.interface === 0 && endpoint.usage === PLUGIN_USAGE && endpoint.usage_page === PLUGIN_USAGE_PAGE;
}
/* global shutdownColor:readonly, LightingMode:readonly, forcedColor:readonly,
   IndividualLedControl:readonly, TimingMode:readonly, delayRGB:readonly */
export function ControllableParameters() {
    return [
        {property:"shutdownColor", group:"lighting", label:"Shutdown Color", type:"color", default:"#000000"},
        {property:"LightingMode", group:"lighting", label:"Lighting Mode", type:"combobox", values:["Canvas","Forced"], default:"Canvas"},
        {property:"forcedColor", group:"lighting", label:"Forced Color", type:"color", default:"#009bde"},
        {property:"IndividualLedControl", group:"lighting", label:"Individual LED Control", type:"boolean", default:false,
            description:"Control each lighting zone separately. Identical colors share one command."},
        {property:"TimingMode", group:"lighting", label:"Refresh Timing", type:"combobox", values:["Recommended","Custom"], default:"Recommended",
            description:"Recommended uses the interval selected for this monitor model."},
        {property:"delayRGB", group:"lighting", label:"Custom Minimum Interval (ms)", type:"number", step:5, min:0, max:500, default:50,
            description:"Used with Custom timing. Increase if lighting skips colors."},
    ];
}

export function Initialize() { monitor.initialize(); }
export function Render() { monitor.render(); }
export function Shutdown(suspending) { monitor.shutdown(suspending ? "#000000" : shutdownColor); }
export function onIndividualLedControlChanged() { monitor.configureZones(); }
export function onLightingModeChanged() { monitor.invalidate(); }
export function onforcedColorChanged() { monitor.invalidate(); }

const OEM_KEYS = [
    "f53fc139443a31790db18276", // Common OEM key; DWF capture fixture.
    "7e7f18002b33fbfde364d0c004b0dac7b5", // AW2724DM capture fixture; current AWCC key 1.
    "66cba370083ae41d34235e24cff60d57",
    "333a3b140985ea277dcd6ae85f8570",
    "b9f1b4fb0ff536ef77abf70d72",
].map(hexBytes);

function hexBytes(hex) { return hex.match(/../g).map(value => parseInt(value,16)); }
function hexToRgb(hex) {
    const match = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(String(hex));
    return match ? match.slice(1).map(value => parseInt(value,16)) : [0,0,0];
}
function normalizeColor(color) {
    return [0,1,2].map(i => Math.max(0,Math.min(255,Math.round(Number(color[i]) || 0))));
}
function checksum(data) { return data.reduce((value,byte) => value ^ byte,0x6e); }
function authenticationResponse(challenge, oem) {
    if (!challenge || challenge.length !== 16) { throw new Error("Authentication requires exactly 16 challenge bytes"); }
    const synthetic = [];
    for (let p=0; p<oem.length && synthetic.length<8; p++) {
        const count = (oem[p] & 1) + ((oem[p] & 0x10) >> 4);
        for (let i=0; i<count && synthetic.length<8; i++) {
            if (p+1 >= oem.length) { throw new Error("Truncated OEM key"); }
            synthetic.push(oem[p] ^ oem[p+1]);
            p++;
        }
    }
    if (synthetic.length !== 8) { throw new Error("Invalid OEM key"); }
    let parity=0;
    for (let value=(challenge[0]<<8)|challenge[15]; value; value>>>=1) { parity ^= value & 1; }
    const mask = parity ? challenge.slice(0,8) : challenge.slice(8,16);
    const index = challenge[parity ? 14 : 6] & 7;
    mask[index] ^= challenge[index + (parity ? 8 : 0)];
    return synthetic.map((value,i) => value ^ mask[i]);
}
function realtekPacket(command, speed) {
    const packet = new Array(193).fill(0);
    packet[1]=0x40; packet[2]=0xc6; packet[7]=command.length;
    packet[9]=0x6e; packet[11]=0x80 | speed;
    command.forEach((value,i) => { packet[65+i]=value; });
    return packet;
}
function colorPacket(profile, mask, color) {
    if (profile.protocol === "legacy") {
        const packet = new Array(65).fill(0xff);
        packet[0]=0;
        [0x92,0x48,0x05,0x00,0x04,mask].concat(color).forEach((value,i) => { packet[1+i]=value; });
        return packet;
    }
    const command = [0x51,0x87,0xd0,0x04,mask].concat(color,[100]);
    command.push(checksum(command));
    if (profile.protocol === "realtek") { return realtekPacket(command,2); }
    const packet = new Array(65).fill(0xff);
    packet[0]=0;
    [0x92,0x37,command.length,0x00].concat(command).forEach((value,i) => { packet[1+i]=value; });
    return packet;
}

class AlienwareMonitor {
    constructor() {
        this.profile=null;
        this.initialized=false;
        this.lastWrite=-Infinity;
        this.nextRetry=0;
        this.lastError=-Infinity;
        this.keyIndex=null;
        this.cursor=0;
        this.sent={};
        this.zones=[];
    }
    initialize() {
        this.initialized=false;
        this.keyIndex=null;
        this.profile=PROFILES[device.productId()];
        if (!this.profile) { device.log("Unsupported Alienware PID: " + device.productId()); return; }
        const endpoint=device.getHidEndpoints().find(value => Validate(value) && value.collection === 0);
        if (!endpoint) { device.log("Alienware monitor HID endpoint not found"); return; }
        device.set_endpoint(endpoint.interface,endpoint.usage,endpoint.usage_page,endpoint.collection);
        device.setName("Alienware " + this.profile.name);
        device.setImageFromUrl(ImageUrl());
        this.configureZones();
        try {
            if (this.profile.protocol === "legacy") {
                const packet=new Array(65).fill(0xff);
                packet[0]=0; packet[1]=0x95; packet[2]=0; packet[3]=0; packet[4]=0;
                if (!this.write(packet)) { throw new Error("Legacy initialization write failed"); }
            } else if (this.profile.protocol === "microchip") {
                // Preserve the existing SignalRGB initialization request (not a proven mode switch).
                const packet=new Array(65).fill(0);
                [0,0x92,0x37,0x05,0,0x51,0x82,0xd0,0xf4,0x99].forEach((value,i)=>{packet[i]=value;});
                if (!this.write(packet)) { throw new Error("Microchip initialization write failed"); }
            }
            this.initialized=true;
            device.log(this.profile.name + ": " + this.profile.protocol + ", authentication=" + this.profile.auth + ", effective interval=" + this.interval() + " ms", {toFile:true});
        } catch (error) { this.fail(error); }
    }
    configureZones() {
        if (!this.profile) { return; }
        const individual=IndividualLedControl === true;
        this.zones=individual ? this.profile.zones : [{name:"Monitor",mask:this.profile.all,position:[0,0]}];
        device.setSize(individual ? this.profile.size : [1,1]);
        device.setControllableLeds(this.zones.map(zone=>zone.name),this.zones.map(zone=>zone.position));
        this.invalidate();
    }
    invalidate() { this.sent={}; this.cursor=0; }
    interval() {
        const custom=Number(delayRGB);
        return TimingMode === "Custom" && isFinite(custom) ? Math.max(0,Math.min(500,custom)) : this.profile.interval;
    }
    write(packet) {
        const result=device.write(packet,packet.length);
        // Some transports report wire length; Windows HID normally includes report ID 0.
        return result === true || result === packet.length || result === packet.length-1;
    }
    authenticate(index) {
        const request=new Array(193).fill(0);
        request[1]=0x40; request[2]=0xe1; request[3]=1;
        if (!this.write(request)) { throw new Error("Challenge request failed"); }
        const report=device.control_transfer(0xa1,1,0x0100,0,[],192,1000);
        if (!report || report.length !== 192) { throw new Error("Incomplete authentication input report"); }
        const response=authenticationResponse(Array.from(report).slice(0,16),OEM_KEYS[index]);
        request[3]=2;
        response.forEach((value,i)=>{request[65+i]=value;});
        return this.write(request);
    }
    chooseKey() {
        const order=device.productId() === 0x1010 ? [1,0,2,3,4] : [0,1,2,3,4];
        for (const index of order) {
            if (!this.authenticate(index)) { continue; }
            device.pause(100);
            // Same bounded I/O probe as AWCC. This is not a cryptographic success response.
            if (this.write(realtekPacket([0x51,0x82,0x01,0xc8,0x74],0))) {
                this.keyIndex=index;
                return true;
            }
        }
        return false;
    }
    send(mask,color) {
        if (this.profile.auth) {
            if (this.keyIndex === null && !this.chooseKey()) { throw new Error("No OEM key passed the I/O probe"); }
            device.pause(10);
            if (!this.authenticate(this.keyIndex)) { throw new Error("Authentication write failed"); }
            device.pause(100);
        }
        if (!this.write(colorPacket(this.profile,mask,color))) { throw new Error("Color write failed or incomplete"); }
        this.lastWrite=Date.now();
    }
    fail(error) {
        this.keyIndex=null;
        this.invalidate();
        this.nextRetry=Date.now()+2000;
        if (Date.now()-this.lastError>=5000) {
            device.log("Alienware transport: " + String(error));
            this.lastError=Date.now();
        }
    }
    render() {
        const now=Date.now();
        if (!this.profile || now<this.nextRetry) { return; }
        if (!this.initialized) { this.initialize(); return; }
        if (now-this.lastWrite<this.interval()) { return; }
        const colors=this.zones.map(zone=>normalizeColor(LightingMode === "Forced" ? hexToRgb(forcedColor) : device.color(zone.position[0],zone.position[1])));
        // One current-state command per call prevents queued obsolete animation frames.
        for (let offset=0; offset<this.zones.length; offset++) {
            const index=(this.cursor+offset)%this.zones.length;
            const zone=this.zones[index];
            const color=colors[index];
            const key=color.join(',');
            const previous=this.sent[zone.mask];
            if (previous && previous.color===key && now-previous.time<2000) { continue; }
            let mask=0;
            const members=[];
            for (let i=0; i<this.zones.length; i++) {
                if (colors[i].join(',')===key) { mask |= this.zones[i].mask; members.push(i); }
            }
            try {
                this.send(mask,color);
                for (const member of members) { this.sent[this.zones[member].mask]={color:key,time:Date.now()}; }
                this.cursor=(index+1)%this.zones.length;
            } catch (error) { this.fail(error); }
            return;
        }
    }
    shutdown(color) {
        if (!this.initialized) { return; }
        try {
            const remaining=this.interval()-(Date.now()-this.lastWrite);
            if (remaining>0) { device.pause(remaining); }
            this.send(this.profile.all,hexToRgb(color));
        } catch (error) { this.fail(error); }
    }
}

const PLUGIN_VID=1060;
const PLUGIN_TYPE="hid";
const PLUGIN_USAGE=1;
const PLUGIN_USAGE_PAGE=65280;
const PROFILES={
  "10049": {
    "name": "AW5520QF",
    "protocol": "microchip",
    "auth": false,
    "zones": [
      {
        "name": "Back",
        "mask": 1,
        "position": [
          0,
          2
        ]
      }
    ],
    "all": 1,
    "size": [
      9,
      7
    ],
    "interval": 50
  },
  "10053": {
    "name": "AW3420DW",
    "protocol": "microchip",
    "auth": false,
    "zones": [
      {
        "name": "Logo",
        "mask": 1,
        "position": [
          0,
          1
        ]
      },
      {
        "name": "Stand",
        "mask": 2,
        "position": [
          4,
          2
        ]
      },
      {
        "name": "Downlight",
        "mask": 4,
        "position": [
          4,
          6
        ]
      },
      {
        "name": "Power Button",
        "mask": 8,
        "position": [
          8,
          6
        ]
      }
    ],
    "all": 15,
    "size": [
      9,
      7
    ],
    "interval": 50
  },
  "10058": {
    "name": "AW3418DW",
    "protocol": "legacy",
    "auth": false,
    "zones": [
      {
        "name": "Upper Left Backside",
        "mask": 1,
        "position": [
          0,
          1
        ]
      },
      {
        "name": "Stand Backside",
        "mask": 2,
        "position": [
          4,
          2
        ]
      },
      {
        "name": "Middle Bottom",
        "mask": 4,
        "position": [
          4,
          6
        ]
      },
      {
        "name": "Switch",
        "mask": 8,
        "position": [
          8,
          6
        ]
      }
    ],
    "all": 15,
    "size": [
      9,
      7
    ],
    "interval": 50
  },
  "10059": {
    "name": "AW3418HW",
    "protocol": "legacy",
    "auth": false,
    "zones": [
      {
        "name": "Upper Left Backside",
        "mask": 1,
        "position": [
          0,
          1
        ]
      },
      {
        "name": "Stand Backside",
        "mask": 2,
        "position": [
          4,
          2
        ]
      },
      {
        "name": "Middle Bottom",
        "mask": 4,
        "position": [
          4,
          6
        ]
      },
      {
        "name": "Switch",
        "mask": 8,
        "position": [
          8,
          6
        ]
      }
    ],
    "all": 15,
    "size": [
      9,
      7
    ],
    "interval": 50
  },
  "10060": {
    "name": "AW2518H",
    "protocol": "legacy",
    "auth": false,
    "zones": [
      {
        "name": "Back Logo",
        "mask": 1,
        "position": [
          0,
          1
        ]
      }
    ],
    "all": 1,
    "size": [
      9,
      7
    ],
    "interval": 50
  }
};
const monitor=new AlienwareMonitor();
