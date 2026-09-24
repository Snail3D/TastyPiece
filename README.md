# TastyPiece

**TastyPiece** turns a cheap ESP32 into a pocket mesh hotspot: it connects to a
LoRa mesh node over Bluetooth and casts its own Wi-Fi access point serving a
mobile web app — so anyone can read and send mesh messages **without installing
the vendor app**.

Join the `TastyPiece` Wi-Fi network, open the captive portal, and you're on the
mesh. No accounts, no app store, no internet.

```
        phone / tablet                ESP32 (TastyPiece)             LoRa mesh node
   ┌───────────────────────┐      ┌────────────────────────┐      ┌──────────────────┐
   │ Wi-Fi client          │◄────►│ Wi-Fi softAP           │      │ MeshCore /       │
   │ captive-portal web UI │ HTTP │ + captive portal       │ BLE  │ Meshtastic node  │
   │ (no install)          │      │ + mesh bridge          │◄────►│ (has the radio)  │
   └───────────────────────┘      └────────────────────────┘      └──────────────────┘
```

The mesh node keeps running the real firmware (radio, crypto, routing).
TastyPiece is a **companion**: it speaks the node's companion protocol, keeps a
local copy of channels/messages, and re-exposes it over HTTP to the browser.

---

## Hardware

| Role | Board | Notes |
|---|---|---|
| **TastyPiece gateway** | ESP32-C3 SuperMini + 0.42" OLED (SSD1306 72×40) | 4 MB flash, OLED on I²C `SDA5 / SCL6` |
| **Mesh node** | Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262) running MeshCore | stays untouched |

Also builds for ESP32-S3 (`-e heltec-v3`) — e.g. if you want the gateway itself
to have a big screen / PSRAM.

The little OLED shows live status: AP address, bridge state, TX/RX counters, and
the last message ("little sent and received scrolling statuses").

---

## Status

**v0.1.0 — working on hardware.**

- [x] Wi-Fi AP `TastyPiece` @ `192.168.4.1` + captive portal (iOS/Android/Windows probes handled)
- [x] Mobile web app served from flash (channels, messages, node info, settings preview)
- [x] BLE central: scan → connect → pair (bonding + MITM passkey)
- [x] MeshCore companion protocol: `APP_START`, `DEVICE_QUERY`, `BATTERY`,
      `GET_CHANNEL`, `GET_MESSAGE`, `SEND_CHANNEL_MESSAGE`
- [x] Send a channel message end-to-end (verified: node accepted + transmitted)
- [x] HTTP JSON API consumed by the UI (`/api/status`, `/api/mesh`, `/api/log`, `/api/send`, `/api/pair`, `/api/time`)
- [x] PIN persistence in NVS
- [ ] Meshtastic (protobuf) transport alongside MeshCore
- [ ] Config writes (device/radio/channel settings), contacts, DMs, telemetry
- [ ] Browser push / live updates (currently 2.5 s polling)

---

## Build & flash

Requires [PlatformIO](https://platformio.org/).

```bash
pio run -e esp32c3-supermini -t upload --upload-port /dev/cu.usbmodemXXXX
pio device monitor
```

Enable USB CDC on boot is already set for the C3 (see `platformio.ini`), so
`Serial` logs appear over USB.

---

## Using it

1. Power TastyPiece next to your mesh node (or power the node on first).
2. On the mesh node's screen, read the **BLE PIN** (MeshCore generates a random
   6-digit PIN each boot when the node has a display).
3. Connect a phone to the `TastyPiece` Wi-Fi network (password `tastypiece`);
   a captive-portal page opens automatically.
4. If prompted, type the node's PIN. TastyPiece pairs and bonds, then pulls
   channels and messages. It's remembered in NVS.
5. Read/send messages; watch the counters move on the OLED.

### Serial console

TastyPiece exposes a small console for bring-up and debugging:

| Command | Action |
|---|---|
| `status` | print bridge + node + channel + message state |
| `selftest` | HTTP GET `/api/mesh` from the device (end-to-end check) |
| `pin 269067` | set the node's BLE PIN and retry pairing |
| `send 0 hello` | send `hello` to channel 0 |

---

## Configuration

`src/config.h` holds the AP identity (`TP_AP_SSID`, `TP_AP_PASS`, channel,
address). `src/secrets.h` (git-ignored) is only for optional upstream Wi-Fi
(STA) development and is not required in normal use — the AP works fully
offline.

---

## Why "TastyPiece"

Because the mesh shouldn't need an app store to be useful. Hand someone the
piece and they're talking.

Not affiliated with MeshCore or Meshtastic.
