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

## Notes

- Channels are edited through their `ChannelSettings` sub-message; the channel
  `role` (Disabled / Primary / Secondary) is shown read-only for now.
- MeshCore node settings (its `SET_*` companion commands) are not wired up yet —
  the Settings tab says so plainly instead of showing dead controls.
