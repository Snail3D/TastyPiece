# Node settings

TastyPiece exposes the **same settings the official Meshtastic apps do**, read
and written straight over Bluetooth — no app, no account.

## How it works

The Meshtastic companion protocol carries an `AdminMessage` on
`PortNum.ADMIN_APP`. TastyPiece:

1. Reads the device's `Config`, `ModuleConfig` and `Channel` records as they
   stream out during the `want_config` handshake.
2. Fetches the `DeviceOwner` (`User`) with a `get_owner_request` (retried until
   the node answers — the reply is larger than the default 23-byte BLE MTU, so
   the gateway negotiates a 517-byte MTU).
3. Decodes every field into JSON using a schema generated from the upstream
   `meshtastic/protobufs` files (`src/mt_settings_gen.h`, and a matching JS
   schema embedded in the web UI).
4. Writes a change as a **read-modify-write of the whole sub-message**:
   `begin_edit_settings` → `set_config` / `set_module_config` / `set_channel` /
   `set_owner` → `commit_edit_settings`. Unknown and repeated fields in the
   stored bytes are preserved, so the edit never clobbers parts of the config
   the gateway doesn't understand.

## Coverage

| Scope | Types |
| --- | --- |
| Config | Device, Position, Power, Network, Display, LoRa, Bluetooth, Security, Sessionkey, DeviceUI |
| Modules | MQTT, Serial, External notification, Store & forward, Range test, Telemetry, Canned messages, Audio, Remote hardware, Neighbor info, Ambient lighting, Detection sensor, Paxcounter |
| Channels | name, PSK, uplink/downlink, module settings, AEAD |
| Owner | long/short name, role, licensed, public key |

That is **201 fields** and **21 enum pickers**, generated from the firmware's own
protobuf definitions — so it tracks whatever the connected node actually
supports.

## API

```
GET  /api/settings           # { ready, config:{...}, module:{...}, channel:{...}, owner:{...} }
POST /api/setting            # { scope, type, field, value }
```

Scope: `0=Config`, `1=ModuleConfig`, `2=Channel (settings)`, `3=Owner`.

Serial equivalents (debug): `cfg`, `cfgtest`, `own`, `set <scope>:<type> <field> <value>`.

## Regenerating the schema

```bash
python3 /tmp/gen_mt_settings.py   # reads /tmp/mtproto/*.proto, rewrites src/mt_settings_gen.h + web_ui.h
```

Copy the protos first:

```bash
mkdir -p /tmp/mtproto && cd /tmp/mtproto
for f in admin.proto config.proto module_config.proto channel.proto mesh.proto portnums.proto; do
  curl -sS -o "$f" "https://raw.githubusercontent.com/meshtastic/protobufs/master/meshtastic/$f"
done
```

## MeshCore

MeshCore's companion protocol uses fixed binary frames instead of protobuf, so
it gets its own field set (scope numbers `100+`):

| Scope | What | Fields |
| --- | --- | --- |
| 100 | Node & radio | node name, TX power, frequency (kHz), bandwidth (Hz), spreading factor, coding rate, latitude/longitude, multi-acks, advert location policy, telemetry modes (base/loc/env), manual-add contacts, RX delay base, airtime factor, auto-add config, auto-add max hops, path-hash mode, BLE PIN |
| 102 | Channel (`type` = channel index) | name, secret (32 hex chars) |
| 104 | Custom variable (`type` = index) | value |

Reads come from `SELF_INFO` (which carries name, radio params, position, TX
power and the other-params byte), plus `CMD_GET_TUNING_PARAMS`, `CMD_GET_CUSTOM_VARS`
and `CMD_GET_AUTOADD_CONFIG`. Writes use `CMD_SET_ADVERT_NAME`,
`CMD_SET_RADIO_PARAMS`, `CMD_SET_RADIO_TX_POWER`, `CMD_SET_ADVERT_LATLON`,
`CMD_SET_TUNING_PARAMS`, `CMD_SET_OTHER_PARAMS`, `CMD_SET_CHANNEL`,
`CMD_SET_CUSTOM_VAR`, `CMD_SET_AUTOADD_CONFIG`, `CMD_SET_PATH_HASH_MODE` and
`CMD_SET_DEVICE_PIN`.

Note: `SET_CHANNEL` frames are 50 bytes, which is why the gateway negotiates a
517-byte BLE MTU.

## Notes

- Meshtastic channels are edited through their `ChannelSettings` sub-message;
  the channel `role` (Disabled / Primary / Secondary) is shown read-only.
- The MeshCore BLE PIN field writes a fixed PIN to the node (`0` = random each
  boot). The current session PIN is only shown on the node's own screen.
