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
.cfgrow{display:flex;align-items:center;justify-content:space-between;gap:10px;padding:8px 0;border-bottom:1px solid var(--line)}
.cfgrow small{display:block;color:var(--dim);font-size:12px;font-weight:400}
.cfgrow .ctl{flex:0 0 auto;max-width:52%}
.cfgrow select,.cfgrow input{background:#0d1117;color:inherit;border:1px solid var(--line);border-radius:8px;padding:5px 7px;font:inherit;max-width:100%}
.cfgrow input[type=number]{width:110px}
.cfgrow input[type=text]{width:170px}
#cfg-groups details{margin:6px 0}#cfg-groups summary{font-weight:600;padding:8px 0;cursor:pointer}
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
    </div>
  </section>

  <section id="v-settings" class="view">
    <div class="card pair" id="pairbox" style="display:none">
      <h3>Pair with node</h3>
      <div class="sub">Type the PIN shown on the mesh node's screen.</div>
      <input id="pin" inputmode="numeric" maxlength="6" placeholder="000000">
      <button onclick="doPair()">Pair</button>
    </div>
    <details open id="meshcfg"><summary>⚙️ Node settings</summary>
      <div class="sub" id="cfg-note">Loading node settings…</div>
      <div id="cfg-groups"></div>
    </details>
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
/* Meshtastic settings schema (generated from meshtastic/protobufs). */
const MT_SCHEMA=/*MTSCHEMA_START*/{"config":[{"id":0,"n":"Device","flds":[{"f":1,"k":"role","kind":5,"en":17},{"f":2,"k":"serial_enabled","kind":0},{"f":4,"k":"button_gpio","kind":1},{"f":5,"k":"buzzer_gpio","kind":1},{"f":6,"k":"rebroadcast_mode","kind":5,"en":15},{"f":7,"k":"node_info_broadcast_secs","kind":1},{"f":8,"k":"double_tap_as_button_press","kind":0},{"f":9,"k":"is_managed","kind":0},{"f":10,"k":"disable_triple_click","kind":0},{"f":11,"k":"tzdef","kind":4},{"f":12,"k":"led_heartbeat_disabled","kind":0},{"f":13,"k":"buzzer_mode","kind":5,"en":2}]},{"id":1,"n":"Position","flds":[{"f":1,"k":"position_broadcast_secs","kind":1},{"f":2,"k":"position_broadcast_smart_enabled","kind":0},{"f":3,"k":"fixed_position","kind":0},{"f":4,"k":"gps_enabled","kind":0},{"f":5,"k":"gps_update_interval","kind":1},{"f":6,"k":"gps_attempt_time","kind":1},{"f":7,"k":"position_flags","kind":1},{"f":8,"k":"rx_gpio","kind":1},{"f":9,"k":"tx_gpio","kind":1},{"f":10,"k":"broadcast_smart_minimum_distance","kind":1},{"f":11,"k":"broadcast_smart_minimum_interval_secs","kind":1},{"f":12,"k":"gps_en_gpio","kind":1},{"f":13,"k":"gps_mode","kind":5,"en":8}]},{"id":2,"n":"Power","flds":[{"f":1,"k":"is_power_saving","kind":0},{"f":2,"k":"on_battery_shutdown_after_secs","kind":1},{"f":3,"k":"adc_multiplier_override","kind":3},{"f":4,"k":"wait_bluetooth_secs","kind":1},{"f":6,"k":"sds_secs","kind":1},{"f":7,"k":"ls_secs","kind":1},{"f":8,"k":"min_wake_secs","kind":1},{"f":9,"k":"device_battery_ina_address","kind":1},{"f":32,"k":"powermon_enables","kind":1}]},{"id":3,"n":"Network","flds":[{"f":1,"k":"wifi_enabled","kind":0},{"f":3,"k":"wifi_ssid","kind":4},{"f":4,"k":"wifi_psk","kind":4},{"f":5,"k":"ntp_server","kind":4},{"f":6,"k":"eth_enabled","kind":0},{"f":7,"k":"address_mode","kind":5,"en":0},{"f":9,"k":"rsyslog_server","kind":4},{"f":10,"k":"enabled_protocols","kind":1},{"f":11,"k":"ipv6_enabled","kind":0}]},{"id":4,"n":"Display","flds":[{"f":1,"k":"screen_on_secs","kind":1},{"f":2,"k":"gps_format","kind":5,"en":4},{"f":3,"k":"auto_screen_carousel_secs","kind":1},{"f":4,"k":"compass_north_top","kind":0},{"f":5,"k":"flip_screen","kind":0},{"f":6,"k":"units","kind":5,"en":6},{"f":7,"k":"oled","kind":5,"en":12},{"f":8,"k":"displaymode","kind":5,"en":5},{"f":9,"k":"heading_bold","kind":0},{"f":10,"k":"wake_on_tap_or_motion","kind":0},{"f":11,"k":"compass_orientation","kind":5,"en":3},{"f":12,"k":"use_12h_clock","kind":0},{"f":13,"k":"use_long_node_name","kind":0},{"f":14,"k":"enable_message_bubbles","kind":0}]},{"id":5,"n":"LoRa","flds":[{"f":1,"k":"use_preset","kind":0},{"f":2,"k":"modem_preset","kind":5,"en":11},{"f":3,"k":"bandwidth","kind":1},{"f":4,"k":"spread_factor","kind":1},{"f":5,"k":"coding_rate","kind":1},{"f":6,"k":"frequency_offset","kind":3},{"f":7,"k":"region","kind":5,"en":16},{"f":8,"k":"hop_limit","kind":1},{"f":9,"k":"tx_enabled","kind":0},{"f":10,"k":"tx_power","kind":2},{"f":11,"k":"channel_num","kind":1},{"f":12,"k":"override_duty_cycle","kind":0},{"f":13,"k":"sx126x_rx_boosted_gain","kind":0},{"f":14,"k":"override_frequency","kind":3},{"f":15,"k":"pa_fan_disabled","kind":0},{"f":103,"k":"ignore_incoming","kind":1,"rep":1},{"f":104,"k":"ignore_mqtt","kind":0},{"f":105,"k":"config_ok_to_mqtt","kind":0},{"f":106,"k":"fem_lna_mode","kind":5,"en":7},{"f":107,"k":"serial_hal_only","kind":0}]},{"id":6,"n":"Bluetooth","flds":[{"f":1,"k":"enabled","kind":0},{"f":2,"k":"mode","kind":5,"en":14},{"f":3,"k":"fixed_pin","kind":1}]},{"id":7,"n":"Security","flds":[{"f":1,"k":"public_key","kind":4},{"f":2,"k":"private_key","kind":4},{"f":3,"k":"admin_key","kind":4,"rep":1},{"f":4,"k":"is_managed","kind":0},{"f":5,"k":"serial_enabled","kind":0},{"f":6,"k":"debug_log_api_enabled","kind":0},{"f":8,"k":"admin_channel_enabled","kind":0},{"f":9,"k":"packet_signature_policy","kind":5,"en":13}]},{"id":8,"n":"Sessionkey","flds":[]}],"module":[{"id":0,"n":"MQTT","flds":[{"f":1,"k":"enabled","kind":0},{"f":2,"k":"address","kind":4},{"f":3,"k":"username","kind":4},{"f":4,"k":"password","kind":4},{"f":5,"k":"encryption_enabled","kind":0},{"f":6,"k":"json_enabled","kind":0},{"f":7,"k":"tls_enabled","kind":0},{"f":8,"k":"root","kind":4},{"f":9,"k":"proxy_to_client_enabled","kind":0},{"f":10,"k":"map_reporting_enabled","kind":0}]},{"id":1,"n":"Serial","flds":[{"f":1,"k":"enabled","kind":0},{"f":2,"k":"echo","kind":0},{"f":3,"k":"rxd","kind":1},{"f":4,"k":"txd","kind":1},{"f":5,"k":"baud","kind":5,"en":18},{"f":6,"k":"timeout","kind":1},{"f":7,"k":"mode","kind":5,"en":19},{"f":8,"k":"override_console_serial_port","kind":0}]},{"id":2,"n":"ExternalNotification","flds":[{"f":1,"k":"enabled","kind":0},{"f":2,"k":"output_ms","kind":1},{"f":3,"k":"output","kind":1},{"f":8,"k":"output_vibra","kind":1},{"f":9,"k":"output_buzzer","kind":1},{"f":4,"k":"active","kind":0},{"f":5,"k":"alert_message","kind":0},{"f":10,"k":"alert_message_vibra","kind":0},{"f":11,"k":"alert_message_buzzer","kind":0},{"f":6,"k":"alert_bell","kind":0},{"f":12,"k":"alert_bell_vibra","kind":0},{"f":13,"k":"alert_bell_buzzer","kind":0},{"f":7,"k":"use_pwm","kind":0},{"f":14,"k":"nag_timeout","kind":1},{"f":15,"k":"use_i2s_as_buzzer","kind":0}]},{"id":3,"n":"StoreForward","flds":[{"f":1,"k":"enabled","kind":0},{"f":2,"k":"heartbeat","kind":0},{"f":3,"k":"records","kind":1},{"f":4,"k":"history_return_max","kind":1},{"f":5,"k":"history_return_window","kind":1},{"f":6,"k":"is_server","kind":0}]},{"id":4,"n":"RangeTest","flds":[{"f":1,"k":"enabled","kind":0},{"f":2,"k":"sender","kind":1},{"f":3,"k":"save","kind":0},{"f":4,"k":"clear_on_reboot","kind":0}]},{"id":5,"n":"Telemetry","flds":[{"f":1,"k":"device_update_interval","kind":1},{"f":2,"k":"environment_update_interval","kind":1},{"f":3,"k":"environment_measurement_enabled","kind":0},{"f":4,"k":"environment_screen_enabled","kind":0},{"f":5,"k":"environment_display_fahrenheit","kind":0},{"f":6,"k":"air_quality_enabled","kind":0},{"f":7,"k":"air_quality_interval","kind":1},{"f":8,"k":"power_measurement_enabled","kind":0},{"f":9,"k":"power_update_interval","kind":1},{"f":10,"k":"power_screen_enabled","kind":0},{"f":11,"k":"health_measurement_enabled","kind":0},{"f":12,"k":"health_update_interval","kind":1},{"f":13,"k":"health_screen_enabled","kind":0},{"f":14,"k":"device_telemetry_enabled","kind":0},{"f":15,"k":"air_quality_screen_enabled","kind":0}]},{"id":6,"n":"CannedMessage","flds":[{"f":1,"k":"rotary1_enabled","kind":0},{"f":2,"k":"inputbroker_pin_a","kind":1},{"f":3,"k":"inputbroker_pin_b","kind":1},{"f":4,"k":"inputbroker_pin_press","kind":1},{"f":5,"k":"inputbroker_event_cw","kind":5,"en":10},{"f":6,"k":"inputbroker_event_ccw","kind":5,"en":10},{"f":7,"k":"inputbroker_event_press","kind":5,"en":10},{"f":8,"k":"updown1_enabled","kind":0},{"f":9,"k":"enabled","kind":0},{"f":10,"k":"allow_input_source","kind":4},{"f":11,"k":"send_bell","kind":0}]},{"id":7,"n":"Audio","flds":[{"f":1,"k":"codec2_enabled","kind":0},{"f":2,"k":"ptt_pin","kind":1},{"f":3,"k":"bitrate","kind":5,"en":1},{"f":4,"k":"i2s_ws","kind":1},{"f":5,"k":"i2s_sd","kind":1},{"f":6,"k":"i2s_din","kind":1},{"f":7,"k":"i2s_sck","kind":1}]},{"id":8,"n":"RemoteHardware","flds":[{"f":1,"k":"enabled","kind":0},{"f":2,"k":"allow_undefined_pin_access","kind":0}]},{"id":9,"n":"NeighborInfo","flds":[{"f":1,"k":"enabled","kind":0},{"f":2,"k":"update_interval","kind":1},{"f":3,"k":"transmit_over_lora","kind":0}]},{"id":10,"n":"AmbientLighting","flds":[{"f":1,"k":"led_state","kind":0},{"f":2,"k":"current","kind":1},{"f":3,"k":"red","kind":1},{"f":4,"k":"green","kind":1},{"f":5,"k":"blue","kind":1}]},{"id":11,"n":"DetectionSensor","flds":[{"f":1,"k":"enabled","kind":0},{"f":2,"k":"minimum_broadcast_secs","kind":1},{"f":3,"k":"state_broadcast_secs","kind":1},{"f":4,"k":"send_bell","kind":0},{"f":5,"k":"name","kind":4},{"f":6,"k":"monitor_pin","kind":1},{"f":7,"k":"detection_trigger_type","kind":5,"en":20},{"f":8,"k":"use_pullup","kind":0}]},{"id":12,"n":"Paxcounter","flds":[{"f":1,"k":"enabled","kind":0},{"f":2,"k":"paxcounter_update_interval","kind":1},{"f":3,"k":"wifi_threshold","kind":2},{"f":4,"k":"ble_threshold","kind":2}]}],"channel":[{"f":1,"k":"channel_num","kind":1},{"f":2,"k":"psk","kind":4},{"f":3,"k":"name","kind":4},{"f":5,"k":"uplink_enabled","kind":0},{"f":6,"k":"downlink_enabled","kind":0},{"f":8,"k":"use_aead","kind":0}],"owner":[{"f":1,"k":"id","kind":4},{"f":2,"k":"long_name","kind":4},{"f":3,"k":"short_name","kind":4},{"f":4,"k":"macaddr","kind":4},{"f":5,"k":"hw_model","kind":5,"en":9},{"f":6,"k":"is_licensed","kind":0},{"f":7,"k":"role","kind":5,"en":17},{"f":8,"k":"public_key","kind":4},{"f":9,"k":"is_unmessagable","kind":0,"rep":1}],"enums":[{"name":"Addressmode","v":[0,1],"l":["Dhcp","Static"]},{"name":"Audio Baud","v":[0,1,2,3,4,5,6,7,8,9,10],"l":["Codec2 Default","Codec2 3200","Codec2 2400","Codec2 1600","Codec2 1400","Codec2 1300","Codec2 1200","Codec2 700","Codec2 700b","Codec2 700c","Codec2 450"]},{"name":"Buzzermode","v":[0,1,2,3,4],"l":["All Enabled","Disabled","Notifications Only","System Only","Direct Msg Only"]},{"name":"Compassorientation","v":[0,1,2,3,4,5,6,7],"l":["Degrees 0","Degrees 90","Degrees 180","Degrees 270","Degrees 0 Inverted","Degrees 90 Inverted","Degrees 180 Inverted","Degrees 270 Inverted"]},{"name":"Deprecatedgpscoordinateformat","v":[0],"l":["Unused"]},{"name":"Displaymode","v":[0,1,2,3],"l":["Default","Twocolor","Inverted","Color"]},{"name":"Displayunits","v":[0,1],"l":["Metric","Imperial"]},{"name":"Fem Lna Mode","v":[0,1,2],"l":["Disabled","Enabled","Not Present"]},{"name":"Gpsmode","v":[0,1,2],"l":["Disabled","Enabled","Not Present"]},{"name":"Hardwaremodel","v":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,255],"l":["Unset","Tlora V2","Tlora V1","Tlora V2 1 1p6","Tbeam","Heltec V2 0","Tbeam V0p7","T Echo","Tlora V1 1p3","Rak4631","Heltec V2 1","Heltec V1","Lilygo Tbeam S3 Core","Rak11200","Nano G1","Tlora V2 1 1p8","Tlora T3 S3","Nano G1 Explorer","Nano G2 Ultra","Lora Type","Wiphone","Wio Wm1110","Rak2560","Heltec Hru 3601","Heltec Wireless Bridge","Station G1","Rak11310","Makerfabs Tracker","Makerfabs Reserved","Canaryone","Rp2040 Lora","Station G2","Lora Relay V1","T Echo Plus","Ppr","Genieblocks","Nrf52 Unknown","Portduino","Android Sim","Diy V1","Nrf52840 Pca10059","Dr Dev","M5stack","Heltec V3","Heltec Wsl V3","Betafpv 2400 Tx","Betafpv 900 Nano Tx","Rpi Pico","Heltec Wireless Tracker","Heltec Wireless Paper","T Deck","T Watch S3","Picomputer S3","Heltec Ht62","Ebyte Esp32 S3","Esp32 S3 Pico","Chatter 2","Heltec Wireless Paper V1 0","Heltec Wireless Tracker V1 0","Unphone","Td Lorac","Cdebyte Eora S3","Twc Mesh V4","Nrf52 Promicro Diy","Radiomaster 900 Bandit Nano","Heltec Capsule Sensor V3","Heltec Vision Master T190","Heltec Vision Master E213","Heltec Vision Master E290","Heltec Mesh Node T114","Sensecap Indicator","Tracker T1000 E","Rak3172","Wio E5","Radiomaster 900 Bandit","Me25ls01 4y10td","Rp2040 Feather Rfm95","M5stack Corebasic","M5stack Core2","Rpi Pico2","M5stack Cores3","Seeed Xiao S3","Ms24sf1","Tlora C6","Wismesh Tap","Routastic","Mesh Tab","Meshlink","Xiao Nrf52 Kit","Thinknode M1","Thinknode M2","T Eth Elite","Heltec Sensor Hub","Muzi Base","Heltec Mesh Pocket","Seeed Solar Node","Nomadstar Meteor Pro","Crowpanel","Link 32","Seeed Wio Tracker L1","Seeed Wio Tracker L1 Eink","Muzi R1 Neo","T Deck Pro","T Lora Pager","M5stack Reserved","Wismesh Tag","Rak3312","Thinknode M5","Heltec Mesh Solar","T Echo Lite","Heltec V4","M5stack C6l","M5stack Cardputer Adv","Heltec Wireless Tracker V2","T Watch Ultra","Thinknode M3","Wismesh Tap V2","Rak3401","Rak6421","Thinknode M4","Thinknode M6","Meshstick 1262","Tbeam 1 Watt","T5 S3 Epaper Pro","Tbeam Bpf","Mini Epaper S3","Tdisplay S3 Pro","Heltec Mesh Node T096","Mesh Tracker X1","Thinknode M7","Thinknode M8","Thinknode M9","Heltec V4 R8","Heltec Mesh Node T1","Station G3","T Impulse Plus","T Echo Card","Seeed Wio Tracker L2","Crowpanel P4","Heltec Mesh Tower V2","Meshnology W10","Heltec Rc32","Heltec Rc52","Heltec Rcc6","Seeed Wio Tracker L1 Pro 1w","Meshnology W12","Meshpager X2","T Connect Pro","Axiometa Genesis Mini","Private Hw"]},{"name":"Inputeventchar","v":[0,10,17,18,19,20,24,27],"l":["None","Select","Up","Down","Left","Right","Cancel","Back"]},{"name":"Modempreset","v":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16],"l":["Long Fast","Long Slow","Very Long Slow","Medium Slow","Medium Fast","Short Slow","Short Fast","Long Moderate","Short Turbo","Long Turbo","Lite Fast","Lite Slow","Narrow Fast","Narrow Slow","Tiny Fast","Tiny Slow","Medium Turbo"]},{"name":"Oledtype","v":[0,1,2,3,4,5],"l":["Oled Auto","Oled Ssd1306","Oled Sh1106","Oled Sh1107","Oled Sh1107 128 128","Oled Sh1107 Rotated"]},{"name":"Packetsignaturepolicy","v":[0,1,2],"l":["Packet Signature Policy Compatible","Packet Signature Policy Balanced","Packet Signature Policy Strict"]},{"name":"Pairingmode","v":[0,1,2],"l":["Random Pin","Fixed Pin","No Pin"]},{"name":"Rebroadcastmode","v":[0,1,2,3,4,5],"l":["All","All Skip Decoding","Local Only","Known Only","None","Core Portnums Only"]},{"name":"Regioncode","v":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37],"l":["Unset","Us","Eu 433","Eu 868","Cn","Jp","Anz","Kr","Tw","Ru","In","Nz 865","Th","Lora 24","Ua 433","Ua 868","My 433","My 919","Sg 923","Ph 433","Ph 868","Ph 915","Anz 433","Kz 433","Kz 863","Np 865","Br 902","Itu1 2m","Itu2 2m","Eu 866","Eu 874","Eu 917","Eu N 868","Itu3 2m","Itu1 70cm","Itu2 70cm","Itu3 70cm","Itu2 125cm"]},{"name":"Role","v":[0,1,2,3,4,5,6,7,8,9,10,11,12],"l":["Client","Client Mute","Router","Router Client","Repeater","Tracker","Sensor","Tak","Client Hidden","Lost And Found","Tak Tracker","Router Late","Client Base"]},{"name":"Serial Baud","v":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15],"l":["Baud Default","Baud 110","Baud 300","Baud 600","Baud 1200","Baud 2400","Baud 4800","Baud 9600","Baud 19200","Baud 38400","Baud 57600","Baud 115200","Baud 230400","Baud 460800","Baud 576000","Baud 921600"]},{"name":"Serial Mode","v":[0,1,2,3,4,5,6,7,8,9,10],"l":["Default","Simple","Proto","Textmsg","Nmea","Caltopo","Ws85","Ve Direct","Ms Config","Log","Logtext"]},{"name":"Triggertype","v":[0,1,2,3,4,5],"l":["Logic Low","Logic High","Falling Edge","Rising Edge","Either Edge Active Low","Either Edge Active High"]}]}/*MTSCHEMA_END*/;
const $=id=>document.getElementById(id);
let STATUS={},MESH={},NODES={devices:[]},SETTINGS={ready:false},SEL=null,timed=false,lastMsgCount=-1;
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

  const devs=NODES.devices||[], target=NODES.target||'', locked=!!NODES.locked, pi=NODES.peer_info||null;
  $('n-target').textContent=target||'(auto — strongest mesh node)';
  let picker='';
  if(locked && pi && pi.address){
    picker=`<div class="chat-tile sel">
      <div class="avatar">🔒</div>
      <div class="grow"><div>${esc(pi.name||pi.address)}</div>
      <div class="sub">${esc(pi.address)}${pi.proto?' · '+esc(pi.proto):''} · connected</div></div>
      <div class="chip">${pi.rssi?pi.rssi+' dBm':''}</div></div>
      <div class="notice" style="margin-top:8px">Locked to this node. Disconnect to choose another — the selection stays saved.</div>
      <div class="rowbtn" style="margin-top:8px">
        <button class="mini" style="background:#f85149;color:#fff" onclick="disconnectNode()">Disconnect</button>
        <button class="mini ghost" onclick="forgetNode()">Forget</button></div>`;
  } else if(devs.length){
    picker=devs.map(d=>{
      const sel=target&&d.address&&d.address.toLowerCase()===target.toLowerCase();
      return `<div class="chat-tile ${sel?'sel':''}" onclick="pickNode('${d.address}')">
        <div class="avatar">${d.proto?'📡':'·'}</div>
        <div class="grow"><div>${esc(d.name)} ${d.saved?'<span class="chip">saved</span>':''} ${d.proto?'':'<span class="chip">not mesh</span>'}</div>
        <div class="sub">${esc(d.address)}${d.proto?' · '+esc(d.proto):''}</div></div>
        <div class="chip">${d.rssi} dBm${sel?' · picked':''}</div></div>`;}).join('')
      +`<div class="rowbtn" style="margin-top:8px">
        <button class="mini" onclick="rescan()">Rescan</button>
        ${target?'<button class="mini" onclick="reconnectNode()">Reconnect</button>':''}
        <button class="mini ghost" onclick="forgetNode()">Forget</button></div>`;
  } else {
    picker='<div class="notice">No nodes yet — is the mesh node powered on?</div>'
      +`<div class="rowbtn" style="margin-top:8px">
        <button class="mini" onclick="rescan()">Rescan</button>
        ${target?'<button class="mini" onclick="reconnectNode()">Reconnect</button>':''}</div>`;
  }
  $('n-picker').innerHTML=picker;

  $('s-fw').textContent=(STATUS.fw||'TastyPiece')+' '+(STATUS.version||'');
  $('s-proto').textContent=MESH.protocol||'—';$('s-ver').textContent=MESH.version||'—';
  $('s-bridge').textContent=bs;$('s-pair').textContent=bs==='connected'?'bonded':'not paired';
  $('s-batt').textContent=MESH.battery_mv?(MESH.battery_mv+' mV'):'—';
  $('s-heap').textContent=STATUS.heap?((STATUS.heap/1024|0)+' KB free'):'—';
  $('s-kept').textContent=(MESH.messages||[]).length+' in RAM';

  if(SEL)renderThread();
  renderSettings();
}
function renderThread(){
  const ms=(MESH.messages||[]).filter(m=>SEL.kind==='channel'?(m.kind==='channel'&&String(m.channel)===String(SEL.id)):(m.kind==='direct'&&m.from===SEL.id));
  const view=ms.slice(-40);
  $('thread').innerHTML=view.length?view.map(m=>
    `<div class="msg ${m.out?'out':'in'}" style="${SET.compact?'font-size:13px;padding:6px 10px':''}">
      <div class="meta">${esc(m.out?'me':m.from)}${SET.snr&&m.snr?(' · '+m.snr.toFixed(1)+'dB'):''}${m.ts?' · '+ago(m.ts):''}</div>${esc(m.text)}</div>`
  ).join(''):'<div class="notice">No messages yet. Say hello 👋</div>';
  const el=$('thread').parentElement.parentElement; // card
  const box=$('thread'); if(box)box.scrollIntoView&&0;
}

function pretty(k){return k.replace(/_/g,' ').replace(/\b\w/g,c=>c.toUpperCase());}
function cfgLookup(scope,type,key){
  if(scope===0)return ((SETTINGS.config||{})[type]||{})[key];
  if(scope===1)return ((SETTINGS.module||{})[type]||{})[key];
  if(scope===2)return ((SETTINGS.channel||{})[type]||{})[key];
  if(scope===3)return (SETTINGS.owner||{})[key];
  if(scope===100)return (SETTINGS.device||{})[key];
  if(scope===102)return ((SETTINGS.channel||{})[type]||{})[key];
  if(scope===104)return ((SETTINGS.vars||[])[type]||{}).v;
}
const MC_FIELDS=[
 {f:0,k:'name',kind:4},{f:1,k:'tx_power',kind:2},{f:2,k:'freq_khz',kind:1},
 {f:3,k:'bw_hz',kind:1},{f:4,k:'spreading_factor',kind:1},{f:5,k:'coding_rate',kind:1},
 {f:6,k:'latitude',kind:3},{f:7,k:'longitude',kind:3},{f:8,k:'multi_acks',kind:0},
 {f:9,k:'advert_location_policy',kind:1},{f:10,k:'telemetry_base',kind:1},
 {f:11,k:'telemetry_location',kind:1},{f:12,k:'telemetry_environment',kind:1},
 {f:13,k:'manual_add_contacts',kind:0},{f:14,k:'rx_delay_base',kind:3},
 {f:15,k:'airtime_factor',kind:3},{f:16,k:'autoadd_config',kind:1},
 {f:17,k:'autoadd_max_hops',kind:1},{f:18,k:'path_hash_mode',kind:1},{f:19,k:'ble_pin',kind:1}];
function cfgRow(scope,type,fd){
  const v=cfgLookup(scope,type,fd.k), name=pretty(fd.k);
  if(fd.kind===0){
    const on=(v===1||v==='1'||v===true);
    return `<div class="cfgrow"><div>${name}</div><div class="ctl"><div class="tog ${on?'on':''}" onclick="setCfg(${scope},${type},${fd.f},${on?0:1})"></div></div></div>`;
  }
  if(fd.kind===5){
    const e=MT_SCHEMA.enums[fd.en]||{v:[],l:[]}, cur=(v===undefined?e.v[0]:Number(v));
    return `<div class="cfgrow"><div>${name}</div><div class="ctl"><select onchange="setCfg(${scope},${type},${fd.f},this.value)">${e.v.map((val,i)=>`<option value="${val}" ${val===cur?'selected':''}>${esc(e.l[i])}</option>`).join('')}</select></div></div>`;
  }
  if(fd.kind===1||fd.kind===2||fd.kind===3){
    const cur=(v===undefined?0:v);
    return `<div class="cfgrow"><div>${name}</div><div class="ctl"><input type="number" value="${cur}" onchange="setCfg(${scope},${type},${fd.f},this.value)"></div></div>`;
  }
  const cur=(v===undefined?'':v);
  return `<div class="cfgrow"><div>${name}</div><div class="ctl"><input type="text" value="${esc(String(cur))}" onchange="setCfg(${scope},${type},${fd.f},this.value)"></div></div>`;
}
function cfgGroup(label,rows,open){
  const body=Array.isArray(rows)?rows.join(''):(rows||'');
  return body?`<details ${open?'open':''}><summary>${esc(label)}</summary>${body}</details>`:'';
}
function renderSettings(){
  const note=$('cfg-note'), box=$('cfg-groups');
  if(!note||!box)return;
  const proto=MESH.protocol||'';
  if(proto==='meshcore'){
    if(!SETTINGS.ready){note.textContent='Reading settings from the node…';box.innerHTML='';return;}
    note.textContent='Edits are written straight to the node.';
    let html=cfgGroup('📻 Radio & node',MC_FIELDS.map(fd=>cfgRow(100,0,fd)),true);
    const chs=SETTINGS.channel||{};
    Object.keys(chs).forEach(i=>{
      const rows=cfgRow(102,Number(i),{f:0,k:'name',kind:4})+cfgRow(102,Number(i),{f:1,k:'secret',kind:4});
      html+=cfgGroup('📶 Channel '+i,rows,false);
    });
    const vars=SETTINGS.vars||[];
    if(vars.length){
      html+=cfgGroup('🧩 Custom variables',vars.map((v,idx)=>`<div class="cfgrow"><div>${esc(v.n)}</div><div class="ctl"><input type="text" value="${esc(v.v)}" onchange="setCfg(104,${idx},0,this.value)"></div></div>`),false);
    }
    box.innerHTML=html;return;
  }
  if(proto!=='meshtastic'){
    note.textContent='Connect to a Meshtastic or MeshCore node to edit its settings here.';
    box.innerHTML='';return;
  }
  if(!SETTINGS.ready){note.textContent='Reading settings from the node…';box.innerHTML='';return;}
  note.textContent='Edits are written straight to the node.';
  let html='';
  html+=cfgGroup('👤 Owner',(MT_SCHEMA.owner||[]).map(fd=>cfgRow(3,0,fd)),true);
  (MT_SCHEMA.config||[]).forEach(g=>{if((SETTINGS.config||{})[g.id]===undefined)return;
    html+=cfgGroup('🔧 '+pretty(g.n),g.flds.map(fd=>cfgRow(0,g.id,fd)),false);});
  (MT_SCHEMA.module||[]).forEach(g=>{if((SETTINGS.module||{})[g.id]===undefined)return;
    html+=cfgGroup('🧩 '+pretty(g.n),g.flds.map(fd=>cfgRow(1,g.id,fd)),false);});
  const chs=SETTINGS.channel||{};
  Object.keys(chs).forEach(i=>{
    const role=Number(chs[i].role||0), rl=['Disabled','Primary','Secondary'][role]||role;
    const rows=`<div class="cfgrow"><div>Role</div><div class="ctl"><span class="chip">${rl}</span></div></div>`+
      (MT_SCHEMA.channel||[]).map(fd=>cfgRow(2,Number(i),fd)).join('');
    html+=cfgGroup('📶 Channel '+i,rows,false);
  });
  box.innerHTML=html;
}
async function setCfg(scope,type,field,value){
  try{await fetch('/api/setting',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({scope:scope,type:type,field:field,value:String(value)})});}catch(e){}
  poll();
}

async function poll(){
  try{
    const [s,m,n,st]=await Promise.all([
      fetch('/api/status',{cache:'no-store'}).then(r=>r.json()),
      fetch('/api/mesh',{cache:'no-store'}).then(r=>r.json()),
      fetch('/api/nodes',{cache:'no-store'}).then(r=>r.json()).catch(()=>({devices:[]})),
      fetch('/api/settings',{cache:'no-store'}).then(r=>r.json()).catch(()=>({ready:false}))
    ]);
    STATUS=s;MESH=m;NODES=n;SETTINGS=st;
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
  const body=SEL.kind==='channel'?{channel:Number(SEL.id),text}:{to:Number(SEL.id)||SEL.id,text};
  await fetch('/api/send',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)}).catch(()=>{});
  poll();return false;
}
async function doPair(){const pin=$('pin').value.trim();if(!pin)return;
  await fetch('/api/pair',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({pin})}).catch(()=>{});poll();}
function pickNode(addr){fetch('/api/node',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({address:addr})}).catch(()=>{});poll();}
function forgetNode(){fetch('/api/forget',{method:'POST'}).catch(()=>{});poll();}
function rescan(){fetch('/api/scan',{method:'POST'}).catch(()=>{});poll();}
function disconnectNode(){fetch('/api/disconnect',{method:'POST'}).catch(()=>{});poll();}
function reconnectNode(){fetch('/api/reconnect',{method:'POST'}).catch(()=>{});poll();}

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
