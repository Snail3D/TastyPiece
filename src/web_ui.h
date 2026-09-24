#pragma once

// TastyPiece captive-portal web client.
// Self-contained SPA served from flash — no internet, no install.
// Works with both MeshCore and Meshtastic nodes.
static const char TP_HTML[] PROGMEM = R"TP_HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0d1117">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<title>TastyPiece</title>
<style>
:root{--bg:#0d1117;--card:#161b22;--card2:#1c2230;--line:#243040;--txt:#e6edf3;--dim:#8b949e;--accent:#3fb950;--warn:#d29922;--bad:#f85149;--blue:#58a6ff}
[data-theme="light"]{--bg:#f6f8fa;--card:#ffffff;--card2:#eef2f6;--line:#d6dee6;--txt:#0d1117;--dim:#57606a}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html,body{margin:0;height:100%}
body{background:var(--bg);color:var(--txt);font:15px/1.45 -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;
  padding-bottom:calc(74px + env(safe-area-inset-bottom))}
header{position:sticky;top:0;z-index:10;background:color-mix(in srgb,var(--bg) 85%,transparent);backdrop-filter:saturate(180%) blur(16px);
  border-bottom:1px solid var(--line);padding:calc(8px + env(safe-area-inset-top)) 14px 8px;display:flex;align-items:center;gap:10px}
.logo{font-weight:800;letter-spacing:.3px}.logo b{color:var(--accent)}
.pill{margin-left:auto;font-size:12px;padding:4px 9px;border-radius:999px;background:var(--card2);
  border:1px solid var(--line);color:var(--dim);display:flex;align-items:center;gap:6px;white-space:nowrap}
.dot{width:7px;height:7px;border-radius:50%;background:var(--dim);flex:0 0 auto}
.dot.ok{background:var(--accent)}.dot.warn{background:var(--warn)}.dot.off{background:var(--bad)}
main{padding:12px}.view{display:none}.view.active{display:block}
.card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:12px;margin-bottom:10px}
.card h3{margin:0 0 6px;font-size:12px;color:var(--dim);font-weight:800;letter-spacing:.6px;text-transform:uppercase}
.kv{display:flex;justify-content:space-between;gap:10px;padding:9px 0;border-bottom:1px solid var(--line);font-size:14px}
.kv:last-child{border-bottom:none}.kv span:first-child{color:var(--dim)}
.kv span:last-child{text-align:right;overflow-wrap:anywhere}
.chat-tile{display:flex;align-items:center;gap:12px;padding:12px;border-radius:14px;background:var(--card);
  border:1px solid var(--line);margin-bottom:8px}
.chat-tile.sel{border-color:var(--accent);background:#132018}
[data-theme="light"] .chat-tile.sel{background:#eafff1}
.avatar{width:40px;height:40px;border-radius:12px;background:linear-gradient(135deg,#3fb950,#1f6feb);
  display:flex;align-items:center;justify-content:center;font-size:17px;flex:0 0 auto;color:#fff}
.grow{flex:1;min-width:0}.sub{font-size:12px;color:var(--dim);white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.chip{font-size:11px;padding:3px 8px;border-radius:999px;background:var(--card2);color:var(--dim);border:1px solid var(--line);white-space:nowrap}
.thread{display:flex;flex-direction:column;gap:6px;padding-bottom:8px}
.msg{max-width:86%;padding:8px 12px;border-radius:16px;font-size:14px;overflow-wrap:anywhere}
.msg.in{background:var(--card2);border-top-left-radius:4px;align-self:flex-start}
.msg.out{background:#1f6feb;color:#fff;margin-left:auto;border-top-right-radius:4px;align-self:flex-end}
.msg .meta{font-size:11px;opacity:.75;margin-bottom:2px}
.compose{display:flex;gap:8px;position:sticky;bottom:calc(74px + env(safe-area-inset-bottom));padding:8px 0 4px}
.compose input{flex:1;min-width:0;background:var(--card);border:1px solid var(--line);border-radius:12px;color:var(--txt);padding:12px;font-size:15px}
.compose .send{background:var(--accent);border:none;color:#08130a;font-weight:800;border-radius:12px;padding:0 18px;min-height:44px}
.mic{width:48px;min-height:44px;border-radius:12px;border:1px solid var(--line);background:var(--card2);color:var(--txt);font-size:20px}
.mic.on{background:var(--bad);color:#fff;border-color:var(--bad)}
.thread-head{display:flex;align-items:center;gap:10px;margin-bottom:8px}
.back{background:none;border:none;color:var(--blue);font-size:15px;padding:6px 4px}
.pair input{width:100%;background:var(--card);border:1px solid var(--line);border-radius:12px;color:var(--txt);
  padding:12px;font-size:20px;letter-spacing:5px;text-align:center;margin:8px 0}
.pair button{width:100%;background:var(--accent);border:none;color:#08130a;font-weight:800;border-radius:12px;padding:12px}
.mini{flex:1;background:var(--accent);border:none;color:#08130a;font-weight:800;border-radius:12px;padding:11px 16px;font-size:14px}
.mini.ghost{background:var(--card2);color:var(--txt);border:1px solid var(--line)}
.rowbtn{display:flex;gap:8px;margin-top:8px}
.sw{display:flex;align-items:center;justify-content:space-between;padding:10px 0;border-bottom:1px solid var(--line)}
.sw:last-child{border-bottom:none}
.sw small{display:block;color:var(--dim);font-size:12px;font-weight:400}
.tog{width:46px;height:28px;border-radius:999px;background:var(--card2);border:1px solid var(--line);position:relative;flex:0 0 auto}
.tog:after{content:"";position:absolute;top:3px;left:3px;width:20px;height:20px;border-radius:50%;background:var(--dim);transition:.15s}
.tog.on{background:var(--accent);border-color:var(--accent)}
.tog.on:after{left:23px;background:#fff}
nav{position:fixed;left:0;right:0;bottom:0;z-index:20;background:color-mix(in srgb,var(--bg) 92%,transparent);backdrop-filter:blur(16px);
  border-top:1px solid var(--line);display:flex;padding:6px 0 calc(6px + env(safe-area-inset-bottom))}
nav button{flex:1;background:none;border:none;color:var(--dim);font-size:11px;display:flex;flex-direction:column;
  align-items:center;gap:3px;padding:6px 0}
nav button svg{width:22px;height:22px;fill:currentColor}nav button.on{color:var(--accent)}
.notice{font-size:12px;color:var(--dim);text-align:center;padding:10px}
.banner{background:#2a1f06;border:1px solid #5c4409;color:#e3b341;border-radius:12px;padding:10px;font-size:13px;margin-bottom:10px}
[data-theme="light"] .banner{background:#fff8e1;color:#7a5b00}
.hint{font-size:11px;color:var(--dim);text-align:center;padding:4px 0}
</style>
</head>
<body>
<header>
  <div class="logo">Tasty<b>Piece</b></div>
  <div class="pill"><span id="dot" class="dot"></span><span id="pilltxt">starting…</span></div>
</header>

<main>
  <section id="v-chats" class="view active">
    <div id="connbanner" class="banner" style="display:none"></div>

    <div id="listview">
      <div class="card">
        <h3>Group chats</h3>
        <div id="channels"></div>
      </div>
      <div class="card">
        <h3>Direct messages</h3>
        <div id="dms"></div>
      </div>
    </div>

    <div id="threadview" style="display:none">
      <div class="thread-head">
        <button class="back" onclick="closeThread()">‹ Chats</button>
        <div class="grow"><div id="t-title">—</div><div class="sub" id="t-sub">—</div></div>
      </div>
      <div class="card"><div id="thread" class="thread"><div class="notice">…</div></div></div>
      <form class="compose" onsubmit="return doSend(event)">
        <button type="button" id="mic" class="mic" onpointerdown="pttStart(event)" onpointerup="pttEnd(event)" onpointercancel="pttEnd(event)">🎤</button>
        <input id="sendtext" placeholder="Message…" autocomplete="off" enterkeyhint="send">
        <button type="submit" class="send">Send</button>
      </form>
      <div class="hint" id="ptthint">Hold 🎤 to dictate (uses your phone's dictation)</div>
    </div>
  </section>

  <section id="v-nodes" class="view">
    <div class="card">
      <h3>This gateway</h3>
      <div class="kv"><span>AP</span><span id="n-ap">—</span></div>
      <div class="kv"><span>AP address</span><span id="n-apip">—</span></div>
      <div class="kv"><span>Clients</span><span id="n-clients">0</span></div>
      <div class="kv"><span>Uptime</span><span id="n-uptime">—</span></div>
    </div>
    <div class="card">
      <h3>Connected node</h3>
      <div class="kv"><span>Protocol</span><span id="n-proto">—</span></div>
      <div class="kv"><span>Bridge</span><span id="n-bridge">—</span></div>
      <div class="kv"><span>Bluetooth name</span><span id="n-peer">—</span></div>
      <div class="kv"><span>Node name</span><span id="n-name">—</span></div>
      <div class="kv"><span>Model</span><span id="n-model">—</span></div>
      <div class="kv"><span>Firmware</span><span id="n-ver">—</span></div>
      <div class="kv"><span>Battery</span><span id="n-batt">—</span></div>
      <div class="kv"><span>My node #</span><span id="n-mynum">—</span></div>
      <div class="kv"><span>TX / RX</span><span id="n-counts">0 / 0</span></div>
    </div>
    <div class="card">
      <h3>Mesh nodes</h3>
      <div id="mesh-nodes"><div class="notice">—</div></div>
    </div>
    <div class="card">
      <h3>Choose a node</h3>
      <div class="kv"><span>Target</span><span id="n-target">—</span></div>
      <div id="n-picker"><div class="notice">Searching…</div></div>
      <div class="rowbtn">
        <button class="mini" onclick="rescan()">Rescan</button>
        <button class="mini ghost" onclick="forgetNode()">Forget</button>
      </div>
    </div>
  </section>

  <section id="v-settings" class="view">
    <div class="card pair" id="pairbox" style="display:none">
      <h3>Pair with node</h3>
      <div class="sub">Type the PIN shown on the mesh node's screen.</div>
      <input id="pin" inputmode="numeric" maxlength="6" placeholder="000000">
      <button onclick="doPair()">Pair</button>
    </div>
    <details open><summary>📱 App</summary>
      <div class="sw"><div>Dark theme<small>Match your phone or force dark</small></div><div id="sw-theme" class="tog" onclick="toggleTheme()"></div></div>
      <div class="sw"><div>Notification sound<small>On new messages</small></div><div id="sw-sound" class="tog on" onclick="toggle('sound')"></div></div>
      <div class="sw"><div>Compact bubbles<small>Smaller chat text</small></div><div id="sw-compact" class="tog" onclick="toggle('compact')"></div></div>
      <div class="sw"><div>Show signal (SNR)<small>Per-message radio quality</small></div><div id="sw-snr" class="tog on" onclick="toggle('snr')"></div></div>
    </details>
    <details><summary>🎙 Dictation</summary>
      <div class="kv"><span>Language</span><span id="s-lang">en-US</span></div>
      <div class="sub">Hold the 🎤 button to dictate. If your browser lacks speech recognition, the input still uses your keyboard's mic key.</div>
    </details>
    <details><summary>📡 Connected node</summary>
      <div class="kv"><span>Protocol</span><span id="s-proto">—</span></div>
      <div class="kv"><span>Firmware</span><span id="s-ver">—</span></div>
      <div class="kv"><span>Bridge link</span><span id="s-bridge">—</span></div>
      <div class="kv"><span>Pairing</span><span id="s-pair">—</span></div>
    </details>
    <details><summary>🔋 Power</summary>
      <div class="kv"><span>Node battery</span><span id="s-batt">—</span></div>
    </details>
    <details><summary>💾 Storage (light device)</summary>
      <div class="kv"><span>Gateway RAM</span><span id="s-heap">—</span></div>
      <div class="kv"><span>Messages kept</span><span id="s-kept">—</span></div>
      <div class="sub">This little gateway keeps the last 60 messages in RAM and forgets them on power loss. Your phone keeps the full history.</div>
    </details>
    <div class="notice">TastyPiece gateway · <span id="s-fw">—</span></div>
  </section>
</main>

<nav>
  <button id="t-chats" class="on" onclick="tab('chats')">
    <svg viewBox="0 0 24 24"><path d="M4 4h16v12H5.2L4 17.2V4zm2 2v9h12V6H6z"/></svg>Chats</button>
  <button id="t-nodes" onclick="tab('nodes')">
    <svg viewBox="0 0 24 24"><path d="M12 2a4 4 0 0 1 4 4c0 1.6-.9 3-2.3 3.7L15 18h-2l-1-8.3A4 4 0 0 1 8 6a4 4 0 0 1 4-4zm0 2a2 2 0 1 0 0 4 2 2 0 0 0 0-4zM6 20a6 6 0 0 1 12 0h-2a4 4 0 0 0-8 0H6z"/></svg>Nodes</button>
  <button id="t-settings" onclick="tab('settings')">
    <svg viewBox="0 0 24 24"><path d="M12 8a4 4 0 1 0 0 8 4 4 0 0 0 0-8zm8.9 4a7.9 7.9 0 0 0-.1-1.2l2-1.5-2-3.4-2.3 1a8 8 0 0 0-2-1.2L16.2 3h-4l-.6 2.4a8 8 0 0 0-2 1.2l-2.3-1-2 3.4 2 1.5a8 8 0 0 0 0 2.4l-2 1.5 2 3.4 2.3-1a8 8 0 0 0 2 1.2l.6 2.4h4l.6-2.4a8 8 0 0 0 2-1.2l2.3 1 2-3.4-2-1.5c.1-.4.1-.8.1-1.2z"/></svg>Settings</button>
</nav>

<script>
const $=id=>document.getElementById(id);
let STATUS={},MESH={},NODES={devices:[]},SEL=null,timed=false,lastMsgCount=-1;
const LS=(k,v)=>v===undefined?localStorage.getItem('tp_'+k):localStorage.setItem('tp_'+k,v);
let SET={sound:LS('sound')!=='0',snr:LS('snr')!=='0',compact:LS('compact')==='1',theme:LS('theme')||'dark',lang:LS('lang')||'en-US'};

function applyTheme(){document.documentElement.setAttribute('data-theme',SET.theme);
  $('sw-theme').className='tog'+(SET.theme==='dark'?' on':'');
  $('s-lang').textContent=SET.lang;
  $('sw-sound').className='tog'+(SET.sound?' on':'');
  $('sw-compact').className='tog'+(SET.compact?' on':'');
  $('sw-snr').className='tog'+(SET.snr?' on':'');}
function toggle(k){SET[k]=!SET[k];LS(k,SET[k]?'1':'0');applyTheme();render();}
function toggleTheme(){SET.theme=SET.theme==='dark'?'light':'dark';LS('theme',SET.theme);applyTheme();}
function tab(n){for(const v of['chats','nodes','settings']){$('v-'+v).classList.toggle('active',v===n);$('t-'+v).classList.toggle('on',v===n);}poll();}
function fmtUptime(s){s=+s||0;const h=(s/3600)|0,m=((s%3600)/60)|0,x=s%60;return (h?h+'h ':'')+(m?m+'m ':'')+x+'s';}
function esc(t){return (t||'').replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));}
function ago(ts){if(!ts)return'';const d=Math.max(0,(Date.now()/1000|0)-ts);if(d<60)return d+'s';if(d<3600)return (d/60|0)+'m';if(d<86400)return (d/3600|0)+'h';return (d/86400|0)+'d';}

function conversations(){
  const ch=(MESH.channels||[]).map(c=>({kind:'channel',id:c.index,name:c.name||('CH'+(c.index+1)),sub:'Group'}));
  const seen={}; (MESH.messages||[]).forEach(m=>{if(m.kind==='direct')seen[m.from]=1;});
  (MESH.mesh_nodes||[]).forEach(n=>{const nm=n.name||('!'+n.num);seen[nm]=1;});
  const dms=Object.keys(seen).map(k=>({kind:'direct',id:k,name:k,sub:'Direct'}));
  return {ch,dms};
}
function openThread(kind,id,name){SEL={kind,id,name};window._lastCount=-1;$('listview').style.display='none';$('threadview').style.display='block';
  $('t-title').textContent=name;$('t-sub').textContent=(kind==='channel'?'Group chat':'Direct message')+(MESH.protocol?' · '+MESH.protocol:'');renderThread();}
function closeThread(){SEL=null;$('listview').style.display='block';$('threadview').style.display='none';}

function render(){
  applyTheme();
  const bs=MESH.status||'unknown';
  $('dot').className='dot '+(bs==='connected'?'ok':(bs==='need_pin'?'warn':'off'));
  $('pilltxt').textContent = bs==='connected' ? (MESH.name||'node')
    : (bs==='need_pin'?'PIN needed':(bs==='scanning'?'searching…':(bs==='not_found'?'node not found':bs)));

  const cb=$('connbanner');
  if(bs==='need_pin'){cb.style.display='block';cb.textContent='Node needs a PIN — open Settings and enter the code on its screen.';}
  else if(bs!=='connected'){cb.style.display='block';cb.textContent='Not connected to a mesh node yet — see the Nodes tab to pick one.';}
  else cb.style.display='none';
  $('pairbox').style.display = bs==='need_pin' ? 'block' : 'none';

  const {ch,dms}=conversations();
  const lastOf=arr=>arr.length?arr[arr.length-1]:null;
  const tile=(o,kind,id)=>{
    const ms=(MESH.messages||[]).filter(m=>(kind==='channel'?(m.kind==='channel'&&m.channel===id):(m.kind==='direct'&&m.from===id)));
    const l=lastOf(ms); const n=ms.length;
    return `<div class="chat-tile" onclick="openThread('${kind}','${esc(String(id))}','${esc(o.name)}')">
      <div class="avatar">${kind==='channel'?'#':'@'}</div>
      <div class="grow"><div>${esc(o.name)}</div><div class="sub">${l?esc(l.text):o.sub}</div></div>
      <div class="chip">${n?n:''}${l?' · '+ago(l.ts):''}</div></div>`;};
  $('channels').innerHTML=ch.length?ch.map(c=>tile(c,'channel',c.id)).join(''):'<div class="notice">No channels yet.</div>';
  $('dms').innerHTML=dms.length?dms.map(d=>tile(d,'direct',d.id)).join(''):'<div class="notice">No direct chats yet.</div>';

  $('n-ap').textContent=STATUS.ap_ssid||'—';$('n-apip').textContent=STATUS.ap_ip||'—';
  $('n-clients').textContent=STATUS.clients??0;$('n-uptime').textContent=fmtUptime(STATUS.uptime_s);
  $('n-proto').textContent=MESH.protocol||'—';$('n-bridge').textContent=bs;$('n-peer').textContent=MESH.peer||'—';
  $('n-name').textContent=MESH.name||'—';$('n-model').textContent=MESH.model||'—';$('n-ver').textContent=MESH.version||'—';
  $('n-batt').textContent=MESH.battery_mv?(MESH.battery_mv+' mV'):'—';
  $('n-mynum').textContent=MESH.my_num||'—';
  $('n-counts').textContent=(STATUS.tx||0)+' / '+(STATUS.rx||0);

  const mn=MESH.mesh_nodes||[];
  $('mesh-nodes').innerHTML=mn.length?mn.map(n=>`<div class="kv"><span>${esc(n.name||('!'+n.num))}</span>
    <span>${n.hops!=null?n.hops+' hop'+(n.hops===1?'':'s'):''}${n.snr!=null&&SET.snr?' · '+n.snr.toFixed(1)+'dB':''}${n.last_heard?' · '+ago(n.last_heard):''}${n.via_mqtt?' · MQTT':''}</span></div>`).join(''):'<div class="notice">No nodes reported (MeshCore may not expose a node list).</div>';

  const devs=NODES.devices||[], target=NODES.target||'';
  $('n-target').textContent=target||'(auto — strongest mesh node)';
  const picker=devs.length?devs.map(d=>{
    const sel=target&&d.address&&d.address.toLowerCase()===target.toLowerCase();
    return `<div class="chat-tile ${sel?'sel':''}" onclick="pickNode('${d.address}')">
      <div class="avatar">${d.proto?'📡':'·'}</div>
      <div class="grow"><div>${esc(d.name)} ${d.proto?'':'<span class="chip">not mesh</span>'}</div>
      <div class="sub">${esc(d.address)}${d.proto?' · '+esc(d.proto):''}</div></div>
      <div class="chip">${d.rssi} dBm${sel?' · picked':''}</div></div>`;}).join('')
    :'<div class="notice">No nodes yet — is the mesh node powered on?</div>';
  $('n-picker').innerHTML=picker;

  $('s-fw').textContent=(STATUS.fw||'TastyPiece')+' '+(STATUS.version||'');
  $('s-proto').textContent=MESH.protocol||'—';$('s-ver').textContent=MESH.version||'—';
  $('s-bridge').textContent=bs;$('s-pair').textContent=bs==='connected'?'bonded':'not paired';
  $('s-batt').textContent=MESH.battery_mv?(MESH.battery_mv+' mV'):'—';
  $('s-heap').textContent=STATUS.heap?((STATUS.heap/1024|0)+' KB free'):'—';
  $('s-kept').textContent=(MESH.messages||[]).length+' in RAM';

  if(SEL)renderThread();
}
function renderThread(){
  const ms=(MESH.messages||[]).filter(m=>SEL.kind==='channel'?(m.kind==='channel'&&m.channel===SEL.id):(m.kind==='direct'&&m.from===SEL.id));
  const view=ms.slice(-40);
  $('thread').innerHTML=view.length?view.map(m=>
    `<div class="msg ${m.out?'out':'in'}" style="${SET.compact?'font-size:13px;padding:6px 10px':''}">
      <div class="meta">${esc(m.out?'me':m.from)}${SET.snr&&m.snr?(' · '+m.snr.toFixed(1)+'dB'):''}${m.ts?' · '+ago(m.ts):''}</div>${esc(m.text)}</div>`
  ).join(''):'<div class="notice">No messages yet. Say hello 👋</div>';
  const el=$('thread').parentElement.parentElement; // card
  const box=$('thread'); if(box)box.scrollIntoView&&0;
}

async function poll(){
  try{
    const [s,m,n]=await Promise.all([
      fetch('/api/status',{cache:'no-store'}).then(r=>r.json()),
      fetch('/api/mesh',{cache:'no-store'}).then(r=>r.json()),
      fetch('/api/nodes',{cache:'no-store'}).then(r=>r.json()).catch(()=>({devices:[]}))
    ]);
    STATUS=s;MESH=m;NODES=n;
    if(!timed){timed=true;fetch('/api/time',{method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify({epoch:Math.floor(Date.now()/1000)})}).catch(()=>{});}
    const c=(MESH.messages||[]).length;
    if(c!==lastMsgCount){if(lastMsgCount>=0&&c>lastMsgCount&&SET.sound)beep();lastMsgCount=c;}
    render();
  }catch(e){$('dot').className='dot off';$('pilltxt').textContent='offline';}
}
let AC=null;
function beep(){try{AC=AC||new (window.AudioContext||window.webkitAudioContext)();const o=AC.createOscillator(),g=AC.createGain();
  o.frequency.value=880;g.gain.value=.04;o.connect(g);g.connect(AC.destination);o.start();setTimeout(()=>o.stop(),120);}catch(e){}}

async function doSend(ev){
  ev.preventDefault();
  const text=$('sendtext').value.trim(); if(!text||!SEL)return false;
  $('sendtext').value='';
  const body=SEL.kind==='channel'?{channel:SEL.id,text}:{to:Number(SEL.id)||SEL.id,text};
  await fetch('/api/send',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)}).catch(()=>{});
  poll();return false;
}
async function doPair(){const pin=$('pin').value.trim();if(!pin)return;
  await fetch('/api/pair',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({pin})}).catch(()=>{});poll();}
function pickNode(addr){fetch('/api/node',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({address:addr})}).catch(()=>{});poll();}
function forgetNode(){fetch('/api/forget',{method:'POST'}).catch(()=>{});poll();}
function rescan(){fetch('/api/scan',{method:'POST'}).catch(()=>{});poll();}

// ---- hold-to-talk dictation ----
let REC=null,recing=false;
function pttStart(e){e.preventDefault();
  if(!('webkitSpeechRecognition' in window)&&!('SpeechRecognition' in window)){ $('sendtext').focus(); $('ptthint').textContent='Use your keyboard mic 🎤'; return; }
  $('mic').classList.add('on'); $('ptthint').textContent='Listening…';
  const SR=window.SpeechRecognition||window.webkitSpeechRecognition;
  REC=new SR(); REC.lang=SET.lang; REC.continuous=true; REC.interimResults=true;
  const base=$('sendtext').value;
  REC.onresult=ev=>{let t=base;for(let i=ev.resultIndex;i<ev.results.length;i++)t+=ev.results[i][0].transcript;$('sendtext').value=t;};
  REC.onerror=()=>{$('ptthint').textContent='Dictation unavailable — use keyboard mic';};
  try{REC.start();recing=true;}catch(_){}
}
function pttEnd(e){e.preventDefault();$('mic').classList.remove('on');
  if(REC&&recing){try{REC.stop();}catch(_){};recing=false;}
  $('ptthint').textContent='Hold 🎤 to dictate (uses your phone\'s dictation)';}

applyTheme();tab('chats');poll();setInterval(poll,2500);
</script>
</body>
</html>
)TP_HTML";
