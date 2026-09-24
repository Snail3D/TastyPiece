# TastyPiece — delivery receipt

```
==================================================
              TASTYPIECE  ·  RECEIPT
==================================================
  project : TastyPiece  (mesh hotspot for phones)
  repo    : github.com/Snail3D/TastyPiece
  version : v0.1.0
  date    : 2026-09-24
  build   : ESP32 (classic) gateway  +  Heltec V3 node
==================================================
```

## What was built

```
[+] Gateway firmware (ESP32)
    Wi-Fi AP "TastyPiece" (192.168.4.1)
    Captive portal (iOS / Android / Windows probes)
    Mobile web app served from flash
    BLE central + auto protocol detection
    Node picker (scan / choose / remember)
    Serial console + HTTP JSON API

[+] MESHCORE transport
    NUS GATT (6E400001)
    APP_START / DEVICE_QUERY / BATTERY
    GET_CHANNEL / SYNC_NEXT_MESSAGE
    SEND_CHANNEL_MESSAGE
    MITM pairing (PIN persisted in NVS)
    channel messages, device info, battery

[+] MESHTASTIC transport
    Meshtastic GATT (6ba1b218)
    ToRadio / FromRadio protobuf (hand-rolled codec)
    want_config handshake + config_complete
    Node DB (name / hops / SNR / last heard)
    Channel list
    TEXT_MESSAGE_APP send + receive
    DIRECT (DM) send
    no PIN required (no secureConnection)

[+] Web app
    Chats tab: group + direct conversation list
    Thread view + compose
    Hold-to-talk dictation (SpeechRecognition)
    Nodes tab: gateway / node / node picker / node DB
    Settings tab: theme, sound, compact, SNR,
                  dictation language, storage note

[+] Docs
    README.md
    docs/ARCHITECTURE.md
    docs/PROTOCOLS.md
    docs/MESHTASTIC-NODE-SETUP.md
    docs/TESTING.md
    docs/RECEIPT.md
```

## Verified on hardware (MeshCore)

```
  gateway board ............ ESP32 Dev Module  (MAC e4:65:b8:76:f2:ec)
  gateway AP ............... TastyPiece -> 192.168.4.1   [OK]
  captive portal ........... HTTP 200, 308-byte self-test [OK]
  node discovered .......... MeshCore-SnailShack  -29 dBm  [OK]
  connect + GATT ........... [OK]
  pairing (MITM) ........... encrypted=1 bonded=1          [OK]
  node identity ............ SnailShack                     [OK]
  node model / firmware .... Heltec V3 / v1.16.0-07a3ca9    [OK]
  battery .................. 4226 mV                        [OK]
  channels ................. [0] Public                     [OK]
  send channel message ..... node accepted (tx ok=1)        [OK]
  reconnect after reboot ... selection + PIN persisted      [OK]
```

## Meshtastic status

```
  transport implemented ..... YES
  protobuf codec ............ YES (ToRadio/FromRadio/MeshPacket/Data/NodeInfo/Channel)
  compiles for esp32dev ..... YES
  device test ............... PENDING (needs a Meshtastic node in range)
  PIN required .............. NO  (set node bluetooth.mode = NO_PIN)
```

To test: flash a node with Meshtastic, set `lora.region`,
`bluetooth.mode = NO_PIN`, power it, then pick it on the Nodes tab.
See `docs/MESHTASTIC-NODE-SETUP.md`.

## HTTP API

```
GET  /            -> web app
GET  /api/status  -> gateway (uptime, clients, heap)
GET  /api/mesh    -> node (protocol, name, model, firmware,
                            battery, channels, messages, mesh_nodes)
GET  /api/nodes   -> nearby BLE candidates + current target
GET  /api/log     -> on-device event log
POST /api/send    -> {channel,text} | {to,text}
POST /api/pair    -> {pin}
POST /api/node    -> {address}      (choose node)
POST /api/forget  -> clear selection
POST /api/scan    -> request a scan
POST /api/time    -> {epoch}
```

## Use it

```
  1. Power the gateway and the mesh node.
  2. Join Wi-Fi  "TastyPiece"   (password "tastypiece")
  3. Open        http://192.168.4.1
  4. Nodes tab -> pick your node (MeshCore PIN if asked)
  5. Chats tab -> pick a channel or DM, hold 🎤 to talk
```

## Notes / known limits

```
  * Messages: gateway keeps last 60 in RAM (light device).
  * MeshCore PIN regenerates per node boot; re-enter if asked.
  * Meshtastic NULL: config writes not implemented yet (read-only + send).
  * Tested gateway: generic ESP32. Some ESP32-C3 clones have a dead
    Wi-Fi transmitter (see README warning).
```

## Links

```
  store / docs ..... snail3d.com/lab
  source access .... $5, private repo invite
                     buy.polar.sh/polar_cl_JKiVqefFRid6LfUoj8PzARYhBSHkqZGpkiI061JqyBR
  demo (9:16 Short)  youtu.be/_db3JFUq4_E
  repo ............. github.com/Snail3D/TastyPiece
  support .......... snailmail3d@gmail.com
```

```
==================================================
  Built by Snail3D · snail3d.com/lab
  support: snailmail3d@gmail.com
==================================================
```
