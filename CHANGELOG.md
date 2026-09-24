# Changelog

## v0.1.0 — 2026-09-24

### Added
- **Meshtastic transport**: Meshtastic BLE service, hand-rolled protobuf codec
  (ToRadio/FromRadio/MeshPacket/Data/NodeInfo/Channel), `want_config`
  handshake, node DB, channels, text send + receive, direct (DM) send.
- **Protocol auto-detection** from GATT services; per-device protocol tag in scan.
- **Node picker**: scan nearby mesh nodes, choose one, remembered in NVS;
  `/api/nodes`, `/api/node`, `/api/forget`, `/api/scan`.
- **New web app**: Chats (group + direct threads), Nodes (node DB + picker),
  Settings (theme/sound/compact/SNR, dictation language, storage note).
- **Hold-to-talk dictation** (SpeechRecognition with keyboard fallback).
- Serial console: `nodes`, `use <addr>`, `forget`.
- Docs: PROTOCOLS, TESTING, MESHTASTIC-NODE-SETUP, RECEIPT.
- ESP32 (classic) board target; OLED I²C probe safety (avoid flash pins).

### Fixed
- BT controller core-affinity assert on multi-core ESP32 (bridge now runs only
  on the core-0 task; removed the duplicate `meshBridgeLoop()` call).
- Scan result cap no longer evicts mesh nodes.

### Verified
- MeshCore: identity, battery, channels, send, reconnect.
- Meshtastic: code complete; device test pending a Meshtastic node.

## v0.1.0 initial
- Wi-Fi AP + captive portal + MeshCore companion protocol + BLE pairing.
