# Testing

## Automated (host-independent)

```bash
# compile both protocol paths for the gateway
pio run -e esp32dev
pio run -e esp32c3-supermini
pio run -e heltec-v3
```

The web app's JavaScript is syntax-checked:

```bash
python - <<'PY'
import re
src=open('src/web_ui.h').read()
open('/tmp/tp_ui.js','w').write(re.search(r'<script>(.*?)</script>',src,re.S).group(1))
PY
node --check /tmp/tp_ui.js
```

## On-device self-test

The firmware can fetch its own API from the AP interface:

```
> selftest
[TP] selftest: HTTP/1.1 200 OK body=... bytes
```

## Manual test plan

| # | Step | Expected |
|---|---|---|
| 1 | Power gateway | Serial: `SoftAP 'TastyPiece' up -> 192.168.4.1` |
| 2 | Phone joins `TastyPiece` | Captive portal opens (or http://192.168.4.1) |
| 3 | Chats tab loads | Conversation list, no console errors |
| 4 | Nodes tab | Gateway info + connected node + node DB |
| 5 | Send group message | Appears as outgoing bubble; node TX log |
| 6 | Another node replies | Appears as incoming bubble; OLED ticker |
| 7 | Hold 🎤 and speak | Text lands in the input (or keyboard-mic fallback) |
| 8 | Nodes → Rescan | Nearby nodes list refreshes |
| 9 | Pick a different node | Bridge disconnects and reconnects to it |
| 10 | Reboot gateway | Reconnects to remembered node automatically |
| 11 | Settings toggles | Persist across reloads (localStorage) |

## Protocol-specific checks

**MeshCore**

- `status` serial command prints name/model/fw/battery/channels.
- After pairing, `auth complete encrypted=1 bonded=1`.
- Sending: `tx 0x03 … ok=1`, then node `rx 0x06 MSG_SENT`.

**Meshtastic**

- After connect: `toRadio … ok=1` (want_config), then a stream of
  `FromRadio` reads; `config complete id=<n>` in the log.
- Node DB appears with names/short names/hops.
- Sending: `toRadio` write with a `MeshPacket`; message echoes back if the node
  relays to the app, otherwise it appears optimistically.

## Verified results (this build)

```
MeshCore  ............ PASS  (identity, battery, channels, send, reconnect)
Meshtastic ..........  CODE OK / DEVICE TEST PENDING
Wi-Fi AP ............. PASS
Captive portal ....... PASS
Web app JS syntax .... PASS
OLED (C3 target) ..... PASS (SDA5 SCL6)
```

## The dead-C3 finding (regression guard)

A specific ESP32-C3 SuperMini could scan Wi-Fi (RX) but never transmit:
its SoftAP produced no beacons and station joins failed. Verified by making a
Heltec act as an independent Wi-Fi scanner — a bare `TESTAP` at max power on
ch 6 never appeared while every other AP did. If a gateway's AP is invisible,
rule this out before debugging firmware.
