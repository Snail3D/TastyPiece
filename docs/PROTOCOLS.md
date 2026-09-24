# Wire protocols TastyPiece speaks

TastyPiece is a BLE **central** (client) to a LoRa mesh node that is a BLE
**peripheral** (server). It detects the node's protocol from its advertised /
discovered GATT services, then runs the matching transport.

## MeshCore (Nordic UART Service)

- Service `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- RX (app → firmware, write): `6E400002-…`
- TX (firmware → app, notify): `6E400003-…`

Frames are compact binary. Each packet starts with a type byte, then a version
byte for some types.

| Direction | Type | Payload |
|---|---|---|
| app → node | `0x01` CMD_APP_START | `03` + 6 pad bytes + app name |
| app → node | `0x16` CMD_DEVICE_QUERY | `03` |
| app → node | `0x14` CMD_GET_BATTERY | — |
| app → node | `0x1F` CMD_GET_CHANNEL | channel index |
| app → node | `0x0A` CMD_SYNC_NEXT_MESSAGE | — |
| app → node | `0x03` CMD_SEND_CHANNEL_MESSAGE | `00` + channel + ts(4 LE) + utf8 text |
| node → app | `0x05` SELF_INFO | name at offset 58 |
| node → app | `0x0D` DEVICE_INFO | fw version, max channels, model, version strings |
| node → app | `0x0C` BATTERY | mV (u16 LE) |
| node → app | `0x12` CHANNEL_INFO | index + 32-byte name |
| node → app | `0x08` / `0x11` CHANNEL_MSG(_V3) | channel, timestamp, text, SNR |
| node → app | `0x0A` NO_MORE_MSGS | — |

The MeshCore characteristics require an **encrypted + MITM** link, so the bridge
performs `secureConnection()` with the PIN shown on the node's OLED. The PIN is
stored in NVS so later reconnects reuse it. (MeshCore regenerates a random PIN
on each boot for display-equipped nodes, so a fresh PIN may occasionally be
needed after a node reboot.)

## Meshtastic (protobuf over BLE)

- Service `6ba1b218-15a8-461f-9fa8-5dcae273eafd`
- TORADIO (app → firmware, write): `f75c76d2-129e-4dad-a1dd-7866124401e7`
- FROMRADIO (firmware → app, read): `2c55e69e-4993-11ed-b878-0242ac120002`
- FROMNUM (firmware → app, notify): `ed9da18c-a800-4f66-a670-aa7547e34453`

Over BLE there is **no `0x94 0xC3` framing** (that exists only on serial/TCP).
Messages are raw protobufs:

- **app → node:** `ToRadio{ packet | want_config_id | ... }`
- **node → app:** `FromRadio{ id | packet | my_info | node_info | config |
  config_complete_id | channel | metadata | ... }`
- `FROMNUM` notifies "data is ready"; the client then reads `FROMRADIO` until it
  returns empty.

Handshake: connect → subscribe `FROMNUM` → write `ToRadio{want_config_id: n}` →
read `FROMRADIO` until `config_complete_id`. The node streams `my_info`, every
`node_info`, `channel`, `config`, then `config_complete_id`.

Text messages ride inside `MeshPacket.decoded`:

- `MeshPacket{ from, to, channel, decoded, id, hop_limit, want_ack, rx_snr, rx_time }`
- `Data{ portnum=TEXT_MESSAGE_APP(1), payload=<utf8> }`
- Broadcast destination is `0xFFFFFFFF`; direct messages set `to = node number`.

Sending a channel message = `ToRadio.packet` with `to = 0xFFFFFFFF`,
`channel = <index>`, `decoded.portnum = 1`, `decoded.payload = text`. The node
encrypts with the channel PSK on its side.

**No PIN:** Meshtastic's GATT characteristics are usable without bonding, so the
bridge never calls `secureConnection()` for Meshtastic. To be extra safe, set the
node to `bluetooth.mode = NO_PIN` (see the setup doc).

## Protocol detection

After the GATT link is up the bridge calls `getService()` for each service UUID:

- Meshtastic service present → Meshtastic transport
- NUS present → MeshCore transport

The scan list also tags each discovered device with its protocol so the UI can
show 📡 next to real mesh nodes.
