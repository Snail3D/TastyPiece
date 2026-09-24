#pragma once

// TastyPiece captive-portal web client.
// Self-contained SPA served from flash — no internet, no install.
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
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html,body{margin:0;height:100%}
body{background:var(--bg);color:var(--txt);font:15px/1.4 -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;
  padding-bottom:calc(70px + env(safe-area-inset-bottom))}
header{position:sticky;top:0;z-index:10;background:rgba(13,17,23,.85);backdrop-filter:saturate(180%) blur(16px);
  border-bottom:1px solid var(--line);padding:calc(8px + env(safe-area-inset-top)) 14px 8px;display:flex;align-items:center;gap:10px}
.logo{font-weight:700;letter-spacing:.3px}.logo b{color:var(--accent)}
.pill{margin-left:auto;font-size:12px;padding:4px 9px;border-radius:999px;background:var(--card2);
  border:1px solid var(--line);color:var(--dim);display:flex;align-items:center;gap:6px}
.dot{width:7px;height:7px;border-radius:50%;background:var(--dim)}
.dot.ok{background:var(--accent)}.dot.warn{background:var(--warn)}.dot.off{background:var(--bad)}
main{padding:12px}.view{display:none}.view.active{display:block}
.card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:12px;margin-bottom:10px}
.card h3{margin:0 0 6px;font-size:13px;color:var(--dim);font-weight:700;letter-spacing:.4px}
.kv{display:flex;justify-content:space-between;gap:10px;padding:9px 0;border-bottom:1px solid var(--line);font-size:14px}
.kv:last-child{border-bottom:none}.kv span:first-child{color:var(--dim)}
.kv span:last-child{text-align:right;overflow-wrap:anywhere}
.chat-tile{display:flex;align-items:center;gap:12px;padding:12px;border-radius:14px;background:var(--card);
  border:1px solid var(--line);margin-bottom:8px}
.chat-tile.sel{border-color:var(--accent);background:#132018}
.avatar{width:38px;height:38px;border-radius:11px;background:linear-gradient(135deg,#3fb950,#1f6feb);
  display:flex;align-items:center;justify-content:center;font-size:17px;flex:0 0 auto}
.grow{flex:1;min-width:0}.sub{font-size:12px;color:var(--dim)}
.chip{font-size:11px;padding:2px 8px;border-radius:999px;background:var(--card2);color:var(--dim);border:1px solid var(--line)}
.bubble-row{display:flex;flex-direction:column}
.msg{max-width:86%;padding:8px 12px;border-radius:16px;margin:4px 0;font-size:14px;overflow-wrap:anywhere}
.msg.in{background:var(--card2);border-top-left-radius:4px}
.msg.out{background:#1f6feb;margin-left:auto;border-top-right-radius:4px}
.msg .meta{font-size:11px;opacity:.75;margin-bottom:2px}
.compose{display:flex;gap:8px;position:sticky;bottom:calc(70px + env(safe-area-inset-bottom));padding:8px 0}
.compose input{flex:1;min-width:0;background:var(--card);border:1px solid var(--line);border-radius:12px;color:var(--txt);padding:11px 12px;font-size:15px}
.compose button{background:var(--accent);border:none;color:#08130a;font-weight:700;border-radius:12px;padding:0 18px}
.pair input{width:100%;background:var(--card);border:1px solid var(--line);border-radius:12px;color:var(--txt);
  padding:12px;font-size:20px;letter-spacing:5px;text-align:center;margin:8px 0}
.pair button{width:100%;background:var(--accent);border:none;color:#08130a;font-weight:700;border-radius:12px;padding:12px}
details{border:1px solid var(--line);border-radius:12px;margin-bottom:8px;background:var(--card);overflow:hidden}
details summary{padding:12px;font-weight:600;cursor:pointer;list-style:none;display:flex;gap:8px}
details summary::-webkit-details-marker{display:none}
details summary:after{content:"›";margin-left:auto;color:var(--dim);transform:rotate(90deg);transition:.2s}
details[open] summary:after{transform:rotate(-90deg)}
nav{position:fixed;left:0;right:0;bottom:0;z-index:20;background:rgba(13,17,23,.92);backdrop-filter:blur(16px);
  border-top:1px solid var(--line);display:flex;padding:6px 0 calc(6px + env(safe-area-inset-bottom))}
nav button{flex:1;background:none;border:none;color:var(--dim);font-size:11px;display:flex;flex-direction:column;
  align-items:center;gap:3px;padding:6px 0}
nav button svg{width:22px;height:22px;fill:currentColor}nav button.on{color:var(--accent)}
.notice{font-size:12px;color:var(--dim);text-align:center;padding:10px}
.banner{background:#2a1f06;border:1px solid #5c4409;color:#e3b341;border-radius:12px;padding:10px;font-size:13px;margin-bottom:10px}
</style>
</head>
<body>
<header>
  <div class="logo">Tasty<b>Piece</b></div>
  <div class="pill"><span id="dot" class="dot"></span><span id="pilltxt">starting…</span></div>
</header>

<main>
  <section id="v-mesh" class="view active">
    <div id="pairbox" class="card pair" style="display:none">
      <h3>PAIR WITH NODE</h3>
      <div class="sub">Type the PIN shown on the mesh node's screen.</div>
      <input id="pin" inputmode="numeric" maxlength="6" placeholder="000000">
      <button onclick="doPair()">Pair</button>
    </div>
    <div id="channels"></div>
    <div class="card">
      <h3>MESSAGES</h3>
      <div id="traffic" class="bubble-row"><div class="notice">…</div></div>
    </div>
    <form class="compose" onsubmit="return doSend(event)">
      <input id="sendtext" placeholder="Message to channel…" autocomplete="off">
      <button type="submit">Send</button>
    </form>
  </section>

  <section id="v-nodes" class="view">
    <div class="card">
      <h3>THIS GATEWAY</h3>
      <div class="kv"><span>AP</span><span id="n-ap">—</span></div>
      <div class="kv"><span>AP address</span><span id="n-apip">—</span></div>
      <div class="kv"><span>Clients</span><span id="n-clients">0</span></div>
      <div class="kv"><span>Uptime</span><span id="n-uptime">—</span></div>
    </div>
    <div class="card">
      <h3>MESH NODE</h3>
      <div class="kv"><span>Bridge</span><span id="n-bridge">—</span></div>
      <div class="kv"><span>Bluetooth name</span><span id="n-peer">—</span></div>
      <div class="kv"><span>Node name</span><span id="n-name">—</span></div>
      <div class="kv"><span>Model</span><span id="n-model">—</span></div>
      <div class="kv"><span>Firmware</span><span id="n-ver">—</span></div>
      <div class="kv"><span>Battery</span><span id="n-batt">—</span></div>
      <div class="kv"><span>TX / RX</span><span id="n-counts">0 / 0</span></div>
    </div>
  </section>

  <section id="v-settings" class="view">
    <details open><summary>📱 Device</summary>
      <div class="kv"><span>Role</span><span>COMPANION</span></div>
      <div class="kv"><span>Gateway firmware</span><span id="s-fw">—</span></div>
    </details>
    <details><summary>📡 Radio (LoRa)</summary>
      <div class="kv"><span>Region</span><span id="s-freq">—</span></div>
      <div class="kv"><span>Node firmware</span><span id="s-ver">—</span></div>
    </details>
    <details><summary>🔑 Channels</summary>
      <div class="kv"><span>Public</span><span>joined</span></div>
      <div class="kv"><span>Private slots</span><span id="s-chans">—</span></div>
    </details>
    <details><summary>🔋 Power</summary>
      <div class="kv"><span>Node battery</span><span id="s-batt">—</span></div>
    </details>
    <details><summary>🔵 Bluetooth</summary>
      <div class="kv"><span>Bridge link</span><span id="s-bridge">—</span></div>
      <div class="kv"><span>Pairing</span><span id="s-pair">—</span></div>
    </details>
    <div class="notice">Read-only in v0.1 — config writes land next.</div>
  </section>
</main>

<nav>
  <button id="t-mesh" class="on" onclick="tab('mesh')">
    <svg viewBox="0 0 24 24"><path d="M4 4h16v12H5.2L4 17.2V4zm2 2v9h12V6H6z"/></svg>Mesh</button>
  <button id="t-nodes" onclick="tab('nodes')">
    <svg viewBox="0 0 24 24"><path d="M12 2a4 4 0 0 1 4 4c0 1.6-.9 3-2.3 3.7L15 18h-2l-1-8.3A4 4 0 0 1 8 6a4 4 0 0 1 4-4zm0 2a2 2 0 1 0 0 4 2 2 0 0 0 0-4zM6 20a6 6 0 0 1 12 0h-2a4 4 0 0 0-8 0H6z"/></svg>Nodes</button>
  <button id="t-settings" onclick="tab('settings')">
    <svg viewBox="0 0 24 24"><path d="M12 8a4 4 0 1 0 0 8 4 4 0 0 0 0-8zm8.9 4a7.9 7.9 0 0 0-.1-1.2l2-1.5-2-3.4-2.3 1a8 8 0 0 0-2-1.2L16.2 3h-4l-.6 2.4a8 8 0 0 0-2 1.2l-2.3-1-2 3.4 2 1.5a8 8 0 0 0 0 2.4l-2 1.5 2 3.4 2.3-1a8 8 0 0 0 2 1.2l.6 2.4h4l.6-2.4a8 8 0 0 0 2-1.2l2.3 1 2-3.4-2-1.5c.1-.4.1-.8.1-1.2z"/></svg>Settings</button>
</nav>

<script>
const $=id=>document.getElementById(id);
let SEL=0, MESH={}, STATUS={}, timed=false;
function tab(n){for(const v of['mesh','nodes','settings']){$('v-'+v).classList.toggle('active',v===n);$('t-'+v).classList.toggle('on',v===n);}poll();}
function fmtUptime(s){s=+s||0;const h=(s/3600)|0,m=((s%3600)/60)|0,x=s%60;return (h?h+'h ':'')+(m?m+'m ':'')+x+'s';}
function esc(t){return (t||'').replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));}

function render(){
  const bs=MESH.status||'unknown';
  $('dot').className='dot '+(bs==='connected'?'ok':(bs==='need_pin'?'warn':'off'));
  $('pilltxt').textContent = bs==='connected' ? ((MESH.name||'node')+(MESH.version?(' · '+MESH.version):''))
    : (bs==='need_pin'?'PIN needed':(bs==='scanning'?'searching…':bs));
  $('pairbox').style.display = bs==='need_pin' ? 'block' : 'none';

  // channels
  const chs=MESH.channels||[];
  let html='';
  if(!chs.length){html='<div class="notice">No channels (bridge not ready).</div>';}
  chs.forEach(c=>{
    const n=(MESH.messages||[]).filter(m=>m.kind==='channel'&&m.channel===c.index).length;
    html+=`<div class="chat-tile ${SEL===c.index?'sel':''}" onclick="sel(${c.index})">
      <div class="avatar">#</div>
      <div class="grow"><div>${esc(c.name||('CH'+(c.index+1)))}</div><div class="sub">index ${c.index}</div></div>
      <div class="chip">${n}</div></div>`;
  });
  $('channels').innerHTML=html;

  // messages for selected channel
  const ms=(MESH.messages||[]).filter(m=>m.kind==='channel'&&m.channel===SEL);
  $('traffic').innerHTML = ms.length ? ms.map(m=>
    `<div class="msg ${m.out?'out':'in'}"><div class="meta">${esc(m.from)}${m.snr?(' · '+m.snr.toFixed(1)+'dB'):''}</div>${esc(m.text)}</div>`
  ).join('') : '<div class="notice">No messages yet.</div>';

  // gateway / node
  $('n-ap').textContent=STATUS.ap_ssid||'—';
  $('n-apip').textContent=STATUS.ap_ip||'—';
  $('n-clients').textContent=STATUS.clients??0;
  $('n-uptime').textContent=fmtUptime(STATUS.uptime_s);
  $('n-bridge').textContent=bs;
  $('n-peer').textContent=MESH.peer||'—';
  $('n-name').textContent=MESH.name||'—';
  $('n-model').textContent=MESH.model||'—';
  $('n-ver').textContent=MESH.version||'—';
  $('n-batt').textContent=MESH.battery_mv?(MESH.battery_mv+' mV'):'—';
  $('n-counts').textContent=(STATUS.tx||0)+' / '+(STATUS.rx||0);

  $('s-fw').textContent=(STATUS.fw||'TastyPiece')+' '+(STATUS.version||'');
  $('s-ver').textContent=MESH.version||'—';
  $('s-freq').textContent='—';
  $('s-chans').textContent=(chs.length?chs.length-1:0)+' available';
  $('s-batt').textContent=MESH.battery_mv?(MESH.battery_mv+' mV'):'—';
  $('s-bridge').textContent=bs;
  $('s-pair').textContent=bs==='connected'?'bonded':'not paired';
}
function sel(i){SEL=i;render();}
async function poll(){
  try{
    const [s,m]=await Promise.all([
      fetch('/api/status',{cache:'no-store'}).then(r=>r.json()),
      fetch('/api/mesh',{cache:'no-store'}).then(r=>r.json())
    ]);
    STATUS=s; MESH=m;
    if(!timed){timed=true;fetch('/api/time',{method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify({epoch:Math.floor(Date.now()/1000)})}).catch(()=>{});}
    render();
  }catch(e){$('dot').className='dot off';$('pilltxt').textContent='offline';}
}
async function doSend(ev){
  ev.preventDefault();
  const text=$('sendtext').value.trim();
  if(!text) return false;
  $('sendtext').value='';
  await fetch('/api/send',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({channel:SEL,text})}).catch(()=>{});
  poll();
  return false;
}
async function doPair(){
  const pin=$('pin').value.trim();
  if(!pin) return;
  await fetch('/api/pair',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({pin})}).catch(()=>{});
  poll();
}
tab('mesh');
poll(); setInterval(poll,2500);
</script>
</body>
</html>
)TP_HTML";
