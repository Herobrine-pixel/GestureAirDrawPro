
// GestureAirDrawPro - script.js
const connectBtn = document.getElementById('connectBtn');
const disconnectBtn = document.getElementById('disconnectBtn');
const statusText = document.getElementById('statusText');
const rawJsonEl = document.getElementById('rawJson');
const drawCanvas = document.getElementById('drawCanvas');
const ctx = drawCanvas.getContext('2d');
const anglesEl = document.getElementById('angles');
const segmentsEl = document.getElementById('segments');
const lengthEl = document.getElementById('length');
const dtwEl = document.getElementById('dtw');
const rType = document.getElementById('r_type');
const rName = document.getElementById('r_name');
const rConf = document.getElementById('r_conf');
const alts = document.getElementById('alts');
const cube = document.getElementById('cube');
const scaleRange = document.getElementById('scaleRange');

let port = null;
let reader = null;
let keepReading = false;
let buffer = '';
let scale = parseFloat(scaleRange.value);

scaleRange.addEventListener('input', ()=> scale = parseFloat(scaleRange.value));

connectBtn.addEventListener('click', async ()=>{
  try{
    port = await navigator.serial.requestPort();
    await port.open({ baudRate: 115200 });
    statusText.textContent = 'Connected';
    connectBtn.disabled = true;
    disconnectBtn.disabled = false;
    keepReading = true;
    readLoop();
  }catch(e){
    statusText.textContent = 'Connection failed: ' + e.message;
    console.error(e);
  }
});

disconnectBtn.addEventListener('click', async ()=>{
  keepReading = false;
  if(reader) try{ await reader.cancel(); }catch(e){}
  if(port) try{ await port.close(); }catch(e){}
  port = null;
  connectBtn.disabled = false;
  disconnectBtn.disabled = true;
  statusText.textContent = 'Disconnected';
});

async function readLoop(){
  const textDecoder = new TextDecoderStream();
  const readableStreamClosed = port.readable.pipeTo(textDecoder.writable);
  reader = textDecoder.readable.getReader();
  try{
    while(keepReading){
      const { value, done } = await reader.read();
      if(done) break;
      if(value){
        buffer += value;
        processBuffer();
      }
    }
  }catch(e){
    console.error(e);
    statusText.textContent = 'Read error';
  }finally{
    reader.releaseLock && reader.releaseLock();
  }
}

function processBuffer(){
  // Expect JSON objects separated by newlines. We'll try to extract complete braces.
  while(true){
    const start = buffer.indexOf('{');
    const end = buffer.indexOf('}\n', start);
    if(start === -1) break;
    let candidate;
    if(end !== -1){
      candidate = buffer.slice(start, end+1);
      buffer = buffer.slice(end+3);
    } else {
      // try to find end by matching braces count
      let depth = 0;
      let found = -1;
      for(let i=start;i<buffer.length;i++){
        if(buffer[i]==='{') depth++;
        else if(buffer[i]==='}') { depth--; if(depth===0){ found = i; break; } }
      }
      if(found===-1) break;
      candidate = buffer.slice(start, found+1);
      buffer = buffer.slice(found+1);
    }
    try{
      const obj = JSON.parse(candidate);
      handleGestureJSON(obj);
    }catch(e){
      console.warn('JSON parse failed', e, candidate.slice(0,200));
    }
  }
}

function handleGestureJSON(obj){
  // display pretty JSON
  rawJsonEl.textContent = JSON.stringify(obj, null, 2);
  // draw path using normalized points if available, else try to build from input.raw
  const pts = (obj.input && obj.input.normalized && obj.input.normalized.length>0)
    ? obj.input.normalized.map(p=>({x:p.x,y:p.y,t:p.t}))
    : (obj.input && obj.input.raw && obj.input.raw.length>0)
      ? obj.input.raw.map(p=>({x:p.ax*100,y:p.ay*100,t:p.t}))
      : [];
  drawGesture(pts);
  // features
  if(obj.features){
    anglesEl.textContent = (obj.features.angles || []).map(a=>a.toFixed(1)).join(', ');
    segmentsEl.innerHTML = '';
    (obj.features.segments||[]).forEach(s=>{
      const li = document.createElement('li');
      li.textContent = `dx:${s.dx}, dy:${s.dy}, len:${s.len}`;
      segmentsEl.appendChild(li);
    });
    lengthEl.textContent = (obj.features.length || '—');
    dtwEl.textContent = (obj.features.dtw_distance || '—');
  }
  // result
  if(obj.result){
    rType.textContent = obj.result.type || '—';
    rName.textContent = obj.result.name || '—';
    rConf.textContent = (obj.result.confidence!=null) ? obj.result.confidence : '—';
    alts.innerHTML = '';
    (obj.result.alternatives || []).forEach(a=>{
      const li = document.createElement('li');
      li.textContent = `${a.name} — ${a.confidence}`;
      alts.appendChild(li);
    });
  }

  // 3D preview: compute roll/pitch from first raw accel if available
  let ax=0,ay=0,az=1;
  if(obj.input && obj.input.raw && obj.input.raw.length>0){
    const r = obj.input.raw[0];
    ax = r.ax || ax; ay = r.ay || ay; az = r.az || az;
  }
  // compute pitch and roll (degrees)
  const roll = Math.atan2(ay, az) * 180/Math.PI;
  const pitch = Math.atan2(-ax, Math.sqrt(ay*ay+az*az)) * 180/Math.PI;
  cube.style.transform = `rotateX(${pitch}deg) rotateY(${roll}deg) rotateZ(0deg)`;
  statusText.textContent = 'Received JSON';
}

function drawGesture(points){
  ctx.clearRect(0,0,drawCanvas.width,drawCanvas.height);
  if(!points || points.length===0) return;
  // normalize to center
  const xs = points.map(p=>p.x), ys = points.map(p=>p.y);
  const minX = Math.min(...xs), maxX = Math.max(...xs);
  const minY = Math.min(...ys), maxY = Math.max(...ys);
  const w = maxX - minX || 1, h = maxY - minY || 1;
  const margin = 40;
  const scaleToCanvas = Math.min((drawCanvas.width-2*margin)/w, (drawCanvas.height-2*margin)/h) * (scale/12);
  const cx = drawCanvas.width/2, cy = drawCanvas.height/2;
  ctx.lineWidth = 3;
  ctx.lineJoin = ctx.lineCap = 'round';
  ctx.strokeStyle = '#06b6d4';
  ctx.beginPath();
  points.forEach((p,i)=>{
    const x = (p.x - (minX+maxX)/2) * scaleToCanvas + cx;
    const y = (p.y - (minY+maxY)/2) * scaleToCanvas + cy;
    if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
  });
  ctx.stroke();
  // draw points
  ctx.fillStyle = '#a7f3d0';
  points.forEach((p,i)=>{
    const x = (p.x - (minX+maxX)/2) * scaleToCanvas + cx;
    const y = (p.y - (minY+maxY)/2) * scaleToCanvas + cy;
    ctx.beginPath(); ctx.arc(x,y,4,0,Math.PI*2); ctx.fill();
  });
}

// Simulation buttons
document.querySelectorAll('.simBtn').forEach(b=>{
  b.addEventListener('click', ()=> {
    const type = b.dataset.sim;
    const obj = sampleJSON(type);
    handleGestureJSON(obj);
  });
});

document.getElementById('clearBtn').addEventListener('click', ()=>{
  ctx.clearRect(0,0,drawCanvas.width,drawCanvas.height);
  rawJsonEl.textContent = '{}';
  anglesEl.textContent = '—';
  segmentsEl.innerHTML = '';
  lengthEl.textContent = '—';
  dtwEl.textContent = '—';
  rType.textContent = '—';
  rName.textContent = '—';
  rConf.textContent = '—';
  alts.innerHTML = '';
  cube.style.transform = 'rotateX(0deg) rotateY(0deg) rotateZ(0deg)';
  statusText.textContent = 'Idle';
});

function sampleJSON(type){
  // Return a JSON structure matching the Ultra JSON required in prompt.
  if(type==='triangle'){
    return {
      input:{
        raw:[{ax:0.12,ay:0.05,az:0.98,gx:-3.2,gy:1.8,gz:0.5,t:0}],
        normalized:[
          {x:0,y:0,t:0},{x:60,y:0,t:10},{x:30,y:52,t:20},{x:0,y:0,t:30}
        ]
      },
      features:{
        angles:[60,60,60],
        segments:[
          {dx:60,dy:0,len:60},
          {dx:-30,dy:52,len:60},
          {dx:-30,dy:-52,len:60}
        ],
        length:180,
        dtw_distance:5.2
      },
      result:{
        type:'shape',
        name:'Triangle',
        confidence:0.95,
        alternatives:[{name:'V',confidence:0.4},{name:'A',confidence:0.34}]
      }
    };
  } else if(type==='circle'){
    const pts = [];
    for(let a=0;a<360;a+=15){
      pts.push({x:50+40*Math.cos(a*Math.PI/180), y:50+40*Math.sin(a*Math.PI/180), t:a});
    }
    return {
      input:{ raw:[{ax:0.01,ay:0.0,az:1.0,gx:0,gy:0,gz:0,t:0}], normalized: pts },
      features:{ angles:[0,0,0], segments:[], length:250, dtw_distance:3.1 },
      result:{ type:'shape', name:'Circle', confidence:0.97, alternatives:[] }
    };
  } else {
    // Letter A
    return {
      input:{ raw:[{ax:0.05,ay:0.02,az:0.99,gx:0.2,gy:-0.1,gz:0,t:0}], normalized:[
        {x:0,y:80,t:0},{x:25,y:0,t:10},{x:50,y:80,t:20},{x:12,y:40,t:15},{x:38,y:40,t:15}
      ]},
      features:{ angles:[25,45,25], segments:[{dx:25,dy:-80,len:85},{dx:25,dy:80,len:85}], length:170, dtw_distance:8.6 },
      result:{ type:'letter', name:'A', confidence:0.92, alternatives:[{name:'4',confidence:0.2}] }
    };
  }
}

// Try to gracefully handle missing Web Serial support
if(!('serial' in navigator)){
  connectBtn.disabled = true;
  statusText.textContent = 'Web Serial API not available in this browser.';
}
