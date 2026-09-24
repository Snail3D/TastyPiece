# TastyPiece — Architecture

## 1. Problem

A LoRa mesh node (MeshCore or Meshtastic) is normally configured and used from a
vendor phone app over Bluetooth. Visitors at an event, or anyone without the app
installed, can't participate.

TastyPiece bridges that gap: the ESP32 speaks the node's **companion protocol**
over BLE and re-serves it as an ordinary web app over its own Wi-Fi AP.

## 2. Layers

```
┌──────────────────────────────────────────────┐
│ Browser (phone)                              │
│   captive portal SPA, polls JSON APIs        │
└───────────────▲──────────────────────────────┘
                │ HTTP (app served from flash)
┌───────────────┴──────────────────────────────┐
│ TastyPiece firmware (ESP32)                  │
│  ├─ WiFi softAP + DNSServer (captive portal) │
│  ├─ WebServer + JSON API                     │
│  ├─ MeshState (channels, messages, device)   │
│  └─ MeshBridge task (BLE central)            │
└───────────────▲──────────────────────────────┘
                │ BLE GATT (Nordic UART Service), paired/encrypted
┌───────────────┴──────────────────────────────┐
│ Mesh node firmware (radio, crypto, routing)  │
└──────────────────────────────────────────────┘
```

## 3. Why BLE, not Wi-Fi STA

- BLE is the **universal** companion transport: every MeshCore/Meshtastic node
  exposes it. Wi-Fi/TCP is optional and model-dependent.
- The product is meant to work **offline**. An AP-only gateway needs no upstream
  network; phones just join TastyPiece.
- We don't touch the node's firmware — it stays the source of truth for the
  radio and keys.

## 4. MeshCore companion protocol (as implemented)

Transport: BLE, NUS UUIDs `6E400001/2/3-B5A3-F393-E0A9-E50E24DCCA9E`.
One protocol frame per GATT write/notification.

### Commands (app → node, write to RX `...0002`)

| Bytes | Command |
|---|---|
| `01 03 <6 pad> <app name>` | `CMD_APP_START` → `SELF_INFO` |
| `16 03` | `CMD_DEVICE_QUERY` → `DEVICE_INFO` |
| `14` | `CMD_GET_BATTERY` → `BATTERY` |
| `1F <idx>` | `CMD_GET_CHANNEL` → `CHANNEL_INFO` |
| `0A` | `CMD_SYNC_NEXT_MESSAGE` → a message or `NO_MORE_MSGS` |
| `03 00 <ch> <ts:4 LE> <text>` | `SEND_CHANNEL_MESSAGE` → `OK` |

### Responses (node → app, notify on TX `...0003`)

| Byte | Packet | Decoded fields |
|---|---|---|
| `0x05` | `SELF_INFO` | adv type, tx power, public key, radio freq/bw/sf/cr, **name** |
| `0x0D` | `DEVICE_INFO` | fw version, max channels, BLE PIN, build, model, version |
| `0x0C` | `BATTERY` | millivolts, storage used/total |
| `0x12` | `CHANNEL_INFO` | index, 32-byte name, 16-byte secret |
| `0x08`/`0x11` | `CHANNEL_MSG_RECV` | channel, text type, timestamp, text (+SNR in V3) |
| `0x07`/`0x10` | `CONTACT_MSG_RECV` | pubkey prefix, path, timestamp, text (+SNR in V3) |
| `0x0A` | `NO_MORE_MSGS` | queue drained |
| `0x83` | `MSG_WAITING` | poll with `0x0A` |
| `0x88` | `LOG_DATA` | RF logs (ignored) |

All multi-byte integers little-endian.

### Security

MeshCore marks its characteristics `ENC_MITM`, so the link must be **paired and
bonded with MITM**. The node's PIN comes from `BLE_PIN_CODE`, but when a node has
a display it generates a **random 6-digit PIN each boot** and shows it on screen.
TastyPiece therefore:

1. tries to pair with the stored PIN (NVS) or `123456`,
2. on `BLE_SM_ERR_CONFIRM_MISMATCH` enters `need_pin` state and waits,
3. accepts a PIN from the web UI (`POST /api/pair`) or serial (`pin NNNNNN`),
4. persists it in NVS once accepted.

## 5. Concurrency model

A blocking BLE GATT connect must never stall the HTTP server, so the bridge runs
on its own FreeRTOS task (`tp_bridge`). `loop()` stays responsive for the web
server and captive portal; the bridge task scans, connects, pairs, and polls.

## 6. HTTP API

| Method | Path | Purpose |
|---|---|---|
| GET | `/` | the SPA |
| GET | `/api/status` | gateway: AP, clients, uptime, counters, bridge state |
| GET | `/api/mesh` | node: name, model, version, battery, channels, messages |
| GET | `/api/log` | ring buffer of gateway events |
| POST | `/api/send` | `{channel, text}` → transmit |
| POST | `/api/pair` | `{pin}` → retry pairing |
| POST | `/api/time` | `{epoch}` → align message timestamps with the browser |

Captive-portal probes (`/generate_204`, `/hotspot-detect.html`, `/connecttest.txt`,
…) 302 to `/`.

## 7. Known gaps / next steps

- **Meshtastic**: different protocol (protobuf `ToRadio`/`FromRadio`). Add a
  transport interface so the bridge can speak either.
- **Config writes**: settings UI is read-only; wire `SET_*` commands.
- **Contacts/DMs/telemetry**: decode the contact list and contact messages.
- **MTU**: default BLE MTU is 23; long channel messages need MTU negotiation or
  chunking.
- **Live updates**: replace polling with WebSocket/SSE.
- **Multi-node**: currently one node per gateway (one BLE link).
