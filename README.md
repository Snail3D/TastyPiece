# TastyPiece

[![License: MIT](https://img.shields.io/badge/license-MIT-3FB950.svg)](LICENSE)
[![Build: PlatformIO](https://img.shields.io/badge/build-PlatformIO-orange.svg)](platformio.ini)
[![MeshCore: supported](https://img.shields.io/badge/MeshCore-supported-3FB950.svg)](docs/PROTOCOLS.md)
[![Meshtastic: supported](https://img.shields.io/badge/Meshtastic-supported-3FB950.svg)](docs/PROTOCOLS.md)
[![Source access](https://img.shields.io/badge/source%20access-%245-3FB950.svg)](https://buy.polar.sh/polar_cl_JKiVqefFRid6LfUoj8PzARYhBSHkqZGpkiI061JqyBR)

**TastyPiece** turns a cheap ESP32 into a pocket mesh hotspot: it connects to a
LoRa mesh node over Bluetooth and casts its own Wi-Fi access point serving a
mobile web app — so anyone can read and send mesh messages **without installing
the vendor app**.

Join the `TastyPiece` Wi-Fi network, open the captive portal, and you're on the
mesh. No accounts, no app store, no internet.

## Node settings

The Settings tab mirrors the vendor apps: device role, LoRa region/preset/hop
limit/TX power, channels (name + PSK), position, power, display, Bluetooth,
security, and all the module configs (MQTT, telemetry, telemetry, store &
forward, range test, canned messages, and more). It reads them from the node
and writes changes straight back over Bluetooth. See [docs/SETTINGS.md](docs/SETTINGS.md).


```
        phone / tablet                ESP32 (TastyPiece)             LoRa mesh node
   ┌───────────────────────┐      ┌────────────────────────┐      ┌──────────────────┐
   │ Wi-Fi client          │◄────►│ Wi-Fi softAP           │      │ MeshCore /       │
   │ captive-portal web UI │ HTTP │ + captive portal       │ BLE  │ Meshtastic node  │
   │ (no install)          │      │ + mesh bridge          │◄────►│ (has the radio)  │
   └───────────────────────┘      └────────────────────────┘      └──────────────────┘
```

The mesh node keeps running the real firmware (radio, crypto, routing).
TastyPiece is a **companion**: it speaks the node's companion protocol over BLE,
keeps a small local copy of channels/messages, and re-exposes it over HTTP to
the browser.

---

## Two protocols, one gateway

| Node firmware | BLE profile | Wire format | Pairing |
|---|---|---|---|
| **MeshCore** | Nordic UART Service (`6E400001-…`) | framed companion protocol (`0x01` APP_START, `0x03` SEND, …) | bonded + MITM passkey (shown on node screen) |
| **Meshtastic** | Meshtastic service (`6ba1b218-…`) | raw protobuf `ToRadio` / `FromRadio`, `FROMNUM` = "data ready" | none needed (works with `bluetooth.mode = NO_PIN`) |

The bridge auto-detects which one it is by looking at the connected device's
GATT services — no configuration needed. The web UI adapts: channels, node DB,
signal (SNR), hop count and direct messages all appear when the node reports
them.

---

## Hardware

| Role | Board | Notes |
|---|---|---|
| **TastyPiece gateway** | generic **ESP32 Dev Module** (classic ESP32) | proven Wi-Fi TX + BLE; 4 MB flash |
| | or ESP32-C3 SuperMini + 0.42" OLED | 4 MB flash, OLED on I²C `SDA5 / SCL6` |
| | or ESP32-S3 (`-e heltec-v3`) | big screen / PSRAM |
| **Mesh node** | Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262) | MeshCore — stays untouched |
| | RAK4631 / T-Echo / Heltec V3 | Meshtastic |

The optional little OLED shows live status: AP address, bridge state, TX/RX
counters, and the last message.

> **Board warning.** Cheap ESP32-C3 SuperMini units vary. One unit here could
> *receive* Wi-Fi but never *transmit* (proved with an independent scanner) —
> its SoftAP never beaconed and its station joins always failed. If your AP is
> invisible, test the board's TX with a second device before blaming the code.

---

## App

The phone app is a single self-contained page served from flash. It gives you:

- **Chats** — a conversation list with *group* channels and *direct* messages,
  last-message previews and timestamps; tap to open a thread.
- **Hold-to-talk dictation** — hold the 🎤 button and speak. Uses the browser's
  SpeechRecognition where available; otherwise falls back to the keyboard mic.
- **Nodes** — gateway info, connected node (protocol, model, firmware, battery),
  the node DB (hops / SNR / last heard), and a **node picker**: scan for nearby
  mesh nodes and tap one to bridge to it (remembered across reboots).
- **Settings** — dark/light theme, notification sound, compact bubbles, show
  SNR; dictation language; node/power info; and a storage note tuned for a light
  device (the gateway keeps the last 60 messages in RAM).

---

## Status

**v0.1.0 — working on hardware (MeshCore verified end-to-end).**

- [x] Wi-Fi AP `TastyPiece` @ `192.168.4.1` + captive portal (iOS/Android/Windows probes)
- [x] BLE central: scan → pick → connect → pair; auto-reconnect; selection persisted in NVS
- [x] **MeshCore**: companion protocol (APP_START, DEVICE_QUERY, BATTERY, GET_CHANNEL, SYNC_NEXT_MESSAGE, SEND_CHANNEL_MESSAGE)
- [x] **Meshtastic**: protobuf ToRadio/FromRadio, `want_config` handshake, node DB, channels, text send + receive
- [x] Send channel messages on both protocols; direct (DM) send on Meshtastic
- [x] Web app: chats (group + direct), hold-to-talk, node picker, settings toggles
- [x] HTTP JSON API (`/api/status`, `/api/mesh`, `/api/nodes`, `/api/node`, `/api/forget`, `/api/scan`, `/api/send`, `/api/pair`, `/api/time`, `/api/log`)
- [x] Serial console (`status`, `nodes`, `use <addr>`, `forget`, `scan`, `selftest`, `pin`, `send`)
- [ ] Config **writes** (device/radio/channel settings), telemetry, telemetry graphs
- [ ] Live updates (currently 2.5 s polling)

---

## Build & flash

```bash
# gateway (classic ESP32 — proven Wi-Fi TX)
pio run -e esp32dev -t upload

# ESP32-C3 SuperMini + 0.42" OLED
pio run -e esp32c3-supermini -t upload

# ESP32-S3 (e.g. Heltec V3 as the gateway itself)
pio run -e heltec-v3 -t upload
```

Then on the gateway:

```
AP:       TastyPiece
Password: tastypiece
Portal:   http://192.168.4.1
```

Pick your mesh node on the **Nodes** tab. If the node is MeshCore and asks for a
PIN, type the 6-digit code shown on the node's screen (the bridge remembers it).

### Configure a Meshtastic node with **no PIN**

See [`docs/MESHTASTIC-NODE-SETUP.md`](docs/MESHTASTIC-NODE-SETUP.md).

---

## Repo layout

```
platformio.ini          board targets (esp32dev, esp32c3-supermini, heltec-v3)
src/main.cpp            Wi-Fi AP, captive portal, HTTP API, serial console
src/mesh_bridge.*       BLE central + MeshCore & Meshtastic transports
src/web_ui.h            the whole mobile web app (HTML/CSS/JS, from flash)
src/oled_status.*       optional status OLED
src/config.h            build config
docs/                   architecture, protocols, testing, receipt
```

---

## Docs

| Document | What it covers |
|---|---|
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | how the AP, captive portal, bridge task, and RAM budget fit together |
| [`docs/PROTOCOLS.md`](docs/PROTOCOLS.md) | MeshCore companion framing and Meshtastic protobuf, side by side |
| [`docs/MESHTASTIC-NODE-SETUP.md`](docs/MESHTASTIC-NODE-SETUP.md) | get a Meshtastic node onto BLE with `NO_PIN` |
| [`docs/TESTING.md`](docs/TESTING.md) | flash-and-verify checklist for both protocols |
| [`docs/RECEIPT.md`](docs/RECEIPT.md) | the printed setup receipt |
| [`CHANGELOG.md`](CHANGELOG.md) | what changed, release by release |

---

## Get the source

The gateway, both protocol transports, and the whole web app are available as a
$5 source-access purchase (private repo invite, free updates by `git pull`):

**<https://snail3d.com/lab/>** · direct checkout: <https://buy.polar.sh/polar_cl_JKiVqefFRid6LfUoj8PzARYhBSHkqZGpkiI061JqyBR>

---

## License

MIT — see [`LICENSE`](LICENSE).

Built by Snail3D · [snail3d.com/lab](https://snail3d.com/lab/) ·
support: snailmail3d@gmail.com
