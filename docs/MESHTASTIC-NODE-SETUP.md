# Configure a Meshtastic node (with **no PIN**)

TastyPiece talks to Meshtastic over BLE and needs no pairing. To make that
stick, put the node into `NO_PIN` mode. Do this once per node.

## 1. Flash / get a Meshtastic node

Any Meshtastic device works (RAK4631, Heltec V3, T-Echo, …). Official images:
<https://flasher.meshtastic.org>. Back up first if the board currently runs
something else:

```bash
esptool --port /dev/cu.usbmodemXXXX --baud 460800 read_flash 0x0 0x1000000 backup.bin
```

## 2. Set the region (required before the radio works)

```bash
meshtastic --port /dev/cu.usbmodemXXXX --set lora.region US
```

## 3. Turn pairing off

```bash
meshtastic --port /dev/cu.usbmodemXXXX --set bluetooth.mode NO_PIN
```

`Config.BluetoothConfig.mode` values: `RANDOM_PIN` (0), `FIXED_PIN` (1),
`NO_PIN` (2). With `NO_PIN`, TastyPiece connects and reads/writes immediately —
no code to read off a screen.

## 4. Give the primary channel a name (optional, nicer UI)

```bash
meshtastic --port /dev/cu.usbmodemXXXX --ch-set name "Public" --ch-index 0
```

## 5. Verify

```bash
meshtastic --port /dev/cu.usbmodemXXXX --info
```

Look for `bluetooth.mode = NO_PIN` (or `2`). Then power the node and pick it on
the TastyPiece **Nodes** tab.

### macOS note (ESP32-S3 USB-CDC)

On macOS, opening an ESP32-S3 USB-CDC port can reset/re-enumerate the device, so
a `meshtastic` CLI connection may time out. If that happens: unplug/replug, run
the CLI once more, or configure over **BLE** instead:

```bash
meshtastic --ble "Meshtastic_xxxx" --set bluetooth.mode NO_PIN
```
