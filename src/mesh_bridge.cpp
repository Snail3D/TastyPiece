#include "mesh_bridge.h"
#include "config.h"

#include <NimBLEDevice.h>
#include <Preferences.h>
#include <vector>
#include <cstring>
#include <ctime>

// ---------------------------------------------------------------------------
// MeshCore BLE companion (Nordic UART Service)
// ---------------------------------------------------------------------------
#define SVC_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define RX_UUID  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"   // app -> firmware
#define TX_UUID  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"   // firmware -> app

// Companion packet types
enum : uint8_t {
  PKT_OK = 0x00, PKT_ERROR = 0x01,
  PKT_CONTACT_START = 0x02, PKT_CONTACT = 0x03, PKT_CONTACT_END = 0x04,
  PKT_SELF_INFO = 0x05, PKT_MSG_SENT = 0x06,
  PKT_CONTACT_MSG = 0x07, PKT_CHANNEL_MSG = 0x08,
  PKT_NO_MORE_MSGS = 0x0A, PKT_BATTERY = 0x0C, PKT_DEVICE_INFO = 0x0D,
  PKT_CONTACT_MSG_V3 = 0x10, PKT_CHANNEL_MSG_V3 = 0x11, PKT_CHANNEL_INFO = 0x12,
  PKT_CHANNEL_DATA = 0x1B, PKT_ADVERTISEMENT = 0x80, PKT_ACK = 0x82,
  PKT_MSGS_WAITING = 0x83, PKT_LOG_DATA = 0x88
};

static NimBLEScan*   s_scan      = nullptr;
static NimBLEClient* s_client    = nullptr;
static NimBLERemoteCharacteristic* s_rx = nullptr;
static NimBLERemoteCharacteristic* s_tx = nullptr;

static volatile bool s_connected  = false;
static volatile bool s_connecting = false;
static volatile bool s_secure     = false;
static volatile bool s_needPin    = false;
static uint32_t s_pin = 123456;
static String s_status = "idle";
static String s_peer;

static uint32_t s_t0        = 0;     // millis at (re)connect
static int      s_initStage = 0;
static uint32_t s_chNext    = 0;
static uint8_t  s_chIdx     = 0;
static uint32_t s_lastPoll  = 0;
static bool     s_wantPoll  = false;
static uint32_t s_lastScanEnd = 0;
static std::vector<MeshNodeInfo> s_found;   // scanned candidates
static String   s_target;                  // pinned BLE address ("" = auto-pick)
static String   s_peerAddr;                // address currently bridged
static bool     s_scanRequest = false;     // force a scan next tick

static bool     s_epochSet = false;
static uint32_t s_epochBase = 0, s_epochAt = 0;
static Preferences s_prefs;

// Decoded device state
static String   s_devName, s_model, s_version;
static uint16_t s_batteryMv = 0;
static uint8_t  s_maxChannels = 0;
static String   s_channels[8];
static std::vector<MeshMessage> s_msgs;

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------
static uint32_t rdU32(const uint8_t* p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static String rdStr(const uint8_t* p, size_t n) {
  String s;
  for (size_t i = 0; i < n && p[i] != 0; i++) s += (char)p[i];
  s.trim();
  return s;
}

static uint32_t nowEpoch() {
  if (s_epochSet) return s_epochBase + (millis() - s_epochAt) / 1000;
  time_t t = time(nullptr);
  if (t > 1000000000) return (uint32_t)t;
  return (uint32_t)(millis() / 1000);
}

static void pushMessage(const MeshMessage& m) {
  s_msgs.push_back(m);
  if (s_msgs.size() > 40) s_msgs.erase(s_msgs.begin(), s_msgs.begin() + (s_msgs.size() - 40));
}

static bool writeFrame(const uint8_t* d, size_t n) {
  if (!s_connected || !s_rx) return false;
  bool ok = s_rx->writeValue(d, n, true);
  Serial.printf("[BRIDGE] tx 0x%02X len=%u ok=%d\n", d[0], (unsigned)n, (int)ok);
  return ok;
}

static String channelName(uint8_t idx) {
  if (idx >= 8 || s_channels[idx].isEmpty()) return String("CH") + (idx + 1);
  return s_channels[idx];
}

// ---------------------------------------------------------------------------
// frame parsing
// ---------------------------------------------------------------------------
static void handleFrame(const uint8_t* d, size_t n) {
  uint8_t t = d[0];
  Serial.printf("[BRIDGE] rx 0x%02X len=%u\n", t, (unsigned)n);
  switch (t) {
    case PKT_SELF_INFO: {
      if (n < 36) break;
      size_t o = 58;
      if (o < n) s_devName = rdStr(d + o, n - o);
      if (s_devName.isEmpty()) s_devName = "MeshCore node";
      Serial.printf("[BRIDGE] self name='%s'\n", s_devName.c_str());
      break;
    }
    case PKT_DEVICE_INFO: {
      if (n < 4) break;
      uint8_t fw = d[1];
      if (fw >= 3 && n >= 80) {
        s_maxChannels = d[3];
        s_model   = rdStr(d + 20, 40);
        s_version = rdStr(d + 60, 20);
      }
      Serial.printf("[BRIDGE] device fw=%u maxch=%u model='%s' ver='%s'\n",
                    fw, s_maxChannels, s_model.c_str(), s_version.c_str());
      break;
    }
    case PKT_BATTERY:
      if (n >= 3) s_batteryMv = (uint16_t)d[1] | ((uint16_t)d[2] << 8);
      Serial.printf("[BRIDGE] battery %umV\n", s_batteryMv);
      break;
    case PKT_CHANNEL_INFO: {
      if (n < 34) break;
      uint8_t idx = d[1];
      if (idx < 8) s_channels[idx] = rdStr(d + 2, 32);
      Serial.printf("[BRIDGE] channel[%u]='%s'\n", idx, s_channels[idx].c_str());
      break;
    }
    case PKT_CHANNEL_MSG:
    case PKT_CHANNEL_MSG_V3: {
      size_t o = (t == PKT_CHANNEL_MSG_V3) ? 4 : 1;
      MeshMessage m;
      m.kind = "channel";
      m.outgoing = false;
      if (t == PKT_CHANNEL_MSG_V3 && n >= 2)
        m.snr = (float)(int8_t)d[1] / 4.0f;
      if (n < o + 7) break;
      m.channel   = d[o];
      m.timestamp = rdU32(d + o + 3);
      m.text      = rdStr(d + o + 7, n - (o + 7));
      m.from      = channelName(m.channel);
      pushMessage(m);
      break;
    }
    case PKT_CONTACT_MSG:
    case PKT_CONTACT_MSG_V3: {
      size_t o = (t == PKT_CONTACT_MSG_V3) ? 4 : 1;
      MeshMessage m;
      m.kind = "direct";
      m.outgoing = false;
      if (t == PKT_CONTACT_MSG_V3 && n >= 2) m.snr = (float)(int8_t)d[1] / 4.0f;
      if (n < o + 11) break;
      String pk;
      char b[3];
      for (int i = 0; i < 6; i++) { snprintf(b, sizeof(b), "%02x", d[o + i]); pk += b; }
      m.from = pk;
      uint8_t txtType = d[o + 7];
      m.timestamp = rdU32(d + o + 8);
      size_t textOff = o + 12 + (txtType == 2 ? 4 : 0);
      if (textOff < n) m.text = rdStr(d + textOff, n - textOff);
      pushMessage(m);
      break;
    }
    case PKT_MSG_SENT:
      // direct echo of our own send; already recorded optimistically
      break;
    case PKT_MSGS_WAITING:
      s_wantPoll = true;
      break;
    case PKT_ERROR:
      break;
    default:
      break;
  }
}

// ---------------------------------------------------------------------------
// BLE plumbing
// ---------------------------------------------------------------------------
static void notifyCB(NimBLERemoteCharacteristic*, uint8_t* data, size_t len, bool) {
  if (len == 0) return;
  handleFrame(data, len);
}

static void disconnectCB(NimBLEClient*) {
  s_connected = false;
  s_secure    = false;
  s_rx = nullptr;
  s_tx = nullptr;
  s_peer = "";
  s_peerAddr = "";
  s_status = s_target.length() ? "connecting" : "scanning";
}

static void disconnectCB(NimBLEClient*);
static void bridgeTask(void* arg);

class SecCB : public NimBLESecurityCallbacks {
  uint32_t onPassKeyRequest() override { return s_pin; }
  void     onPassKeyNotify(uint32_t pass_key) override {
    Serial.printf("[BRIDGE] peripheral passkey notify %06u\n", pass_key);
  }
  bool     onSecurityRequest() override { return true; }
  bool     onConfirmPIN(uint32_t) override { return true; }
  void     onAuthenticationComplete(ble_gap_conn_desc* desc) override {
    s_secure = desc && desc->sec_state.encrypted;
    Serial.printf("[BRIDGE] auth complete encrypted=%d bonded=%d\n",
                  desc ? (int)desc->sec_state.encrypted : 0,
                  desc ? (int)desc->sec_state.bonded : 0);
  }
};
static SecCB s_sec;

class ClientCB : public NimBLEClientCallbacks {
  void onDisconnect(NimBLEClient* pClient) override { disconnectCB(pClient); }
};

class ScanCB : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* dev) override {
    if (s_connecting || s_connected) return;
    String addr = dev->getAddress().toString().c_str();
    String name = dev->getName().c_str();
    bool svc = dev->isAdvertisingService(NimBLEUUID(SVC_UUID));
    bool nm  = dev->getName().find("MeshCore") != std::string::npos;
    bool mc  = svc || nm;
    for (auto& d : s_found) {                 // de-dup, refresh RSSI
      if (d.address.equalsIgnoreCase(addr)) {
        d.rssi = dev->getRSSI(); d.name = name; d.meshcore = mc; return;
      }
    }
    MeshNodeInfo info;
    info.name    = name.isEmpty() ? addr : name;
    info.address = addr;
    info.rssi    = dev->getRSSI();
    info.meshcore = mc;
    s_found.push_back(info);
    if (s_found.size() > 32) {            // never evict a mesh node
      for (size_t i = 0; i < s_found.size(); i++) {
        if (!s_found[i].meshcore) { s_found.erase(s_found.begin() + i); break; }
      }
      if (s_found.size() > 32) s_found.erase(s_found.begin());
    }
    if (mc) Serial.printf("[BRIDGE] found '%s' %s\n", info.name.c_str(), addr.c_str());
  }
};

static bool connectToFoundDevice(const NimBLEAddress& addr, const String& name) {
  s_connecting = true;
  s_status = "connecting";
  s_peer = name;

  if (!s_client) {
    s_client = NimBLEDevice::createClient();
    static ClientCB clientCB;
    s_client->setClientCallbacks(&clientCB);
    s_client->setConnectTimeout(15);
  }
  Serial.printf("[BRIDGE] connecting to '%s' %s ...\n", name.c_str(),
                addr.toString().c_str());
  if (!s_client->connect(addr, true)) {
    Serial.println("[BRIDGE] connect failed");
    s_connecting = false;
    s_status = "scanning";
    if (s_client->isConnected()) s_client->disconnect();
    return false;
  }
  Serial.println("[BRIDGE] link up, resolving GATT");

  NimBLERemoteService* svc = s_client->getService(NimBLEUUID(SVC_UUID));
  if (!svc) {
    Serial.println("[BRIDGE] service missing");
    s_client->disconnect();
    s_connecting = false;
    return false;
  }
  s_rx = svc->getCharacteristic(NimBLEUUID(RX_UUID));
  s_tx = svc->getCharacteristic(NimBLEUUID(TX_UUID));
  if (!s_rx || !s_tx) {
    Serial.println("[BRIDGE] characteristics missing");
    s_client->disconnect();
    s_connecting = false;
    return false;
  }
  if (!s_tx->subscribe(true, notifyCB, true)) {
    Serial.println("[BRIDGE] subscribe failed");
    s_client->disconnect();
    s_connecting = false;
    s_status = "scanning";
    return false;
  }

  // MeshCore chars require an encrypted + MITM link; initiate pairing.
  s_secure = false;
  Serial.printf("[BRIDGE] pairing (MITM passkey %06u) ...\n", s_pin);
  if (!s_client->secureConnection()) {
    Serial.printf("[BRIDGE] pairing failed err=%d (need PIN)\n", s_client->getLastError());
    s_client->disconnect();
    s_connecting = false;
    s_needPin = true;
    s_status = "need_pin";
    return false;
  }

  s_connected = true;
  s_connecting = false;
  s_status = "connected";
  s_peerAddr = addr.toString().c_str();
  s_t0 = millis();
  s_initStage = 0;
  Serial.printf("[BRIDGE] paired + connected to '%s'\n", s_peer.c_str());
  return true;
}

// ---------------------------------------------------------------------------
// public API
// ---------------------------------------------------------------------------
void meshBridgeBegin() {
  s_prefs.begin("tastypiece", false);
  uint32_t savedPin = s_prefs.getUInt("pin", 0);
  if (savedPin) s_pin = savedPin;
  s_target = s_prefs.isKey("target") ? s_prefs.getString("target", "") : String("");

  NimBLEDevice::init("TastyPiece");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  s_scan = NimBLEDevice::getScan();
  static ScanCB cb;
  s_scan->setAdvertisedDeviceCallbacks(&cb, false);
  s_scan->setActiveScan(true);
  s_scan->setInterval(100);
  s_scan->setWindow(80);
  s_status = "scanning";

  NimBLEDevice::setSecurityCallbacks(&s_sec);
  NimBLEDevice::setSecurityAuth(true, true, true);   // bonding, MITM, secure connections
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_DISPLAY);

  // Own task so a blocking GATT connect can never stall the web server.
  xTaskCreatePinnedToCore(bridgeTask, "tp_bridge", 8192, nullptr, 2, nullptr, 0);
}

static void bridgeTask(void* arg) {
  (void)arg;
  for (;;) {
    meshBridgeLoop();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void meshBridgeLoop() {
  uint32_t now = millis();

  // 1) not connected -> scan, then connect to the chosen / strongest node
  if (!s_connected && !s_connecting) {
    if (s_needPin) {                       // wait for a PIN from the user
      s_status = "need_pin";
      vTaskDelay(pdMS_TO_TICKS(500));
      return;
    }

    s_status = s_target.length() ? "connecting" : "scanning";
    s_found.clear();
    s_scanRequest = false;
    Serial.println("[BRIDGE] scanning 5s ...");
    s_scan->start(5, false);          // seconds; blocks this task only
    s_scan->stop();
    Serial.printf("[BRIDGE] scan done: %u device(s)\n", (unsigned)s_found.size());
    for (auto& d : s_found)
      if (d.meshcore)
        Serial.printf("[BRIDGE]   %-22s %-17s %4d dBm [meshcore]\n", d.name.c_str(),
                      d.address.c_str(), d.rssi);

    // Choose the node: explicit target first, else strongest MeshCore device.
    const MeshNodeInfo* pick = nullptr;
    if (s_target.length()) {
      for (auto& d : s_found)
        if (d.address.equalsIgnoreCase(s_target)) { pick = &d; break; }
      if (!pick) {
        s_status = "not_found";
        Serial.printf("[BRIDGE] target %s not in range\n", s_target.c_str());
        vTaskDelay(pdMS_TO_TICKS(3000));
        return;
      }
    } else {
      for (auto& d : s_found)
        if (d.meshcore && (!pick || d.rssi > pick->rssi)) pick = &d;
      if (!pick) {                        // nothing suitable yet
        vTaskDelay(pdMS_TO_TICKS(2500));
        return;
      }
    }

    if (connectToFoundDevice(NimBLEAddress(pick->address.c_str()), pick->name)) {
      if (!s_target.length()) {           // remember the auto-picked node
        s_target = pick->address;
        s_prefs.putString("target", s_target);
      }
    } else {
      vTaskDelay(pdMS_TO_TICKS(1500));
    }
    return;
  }

  if (!s_connected) return;

  // 2) one-time init sequence
  uint32_t el = now - s_t0;
  if (s_initStage == 0 && s_secure && el > 400) {
    // Mirrors the reference client: CMD_APP_START, protocol 0x03, 6 pad bytes, app name.
    uint8_t f[8] = {0x01, 0x03, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
    std::vector<uint8_t> frame(f, f + 8);
    const char* app = "TastyPiece";
    for (size_t i = 0; app[i]; i++) frame.push_back((uint8_t)app[i]);
    writeFrame(frame.data(), frame.size());
    s_initStage = 1;
  } else if (s_initStage == 1 && el > 900) {
    uint8_t f[2] = {0x16, 0x03};
    writeFrame(f, 2);
    s_initStage = 2;
  } else if (s_initStage == 2 && el > 1400) {
    uint8_t f[1] = {0x14};
    writeFrame(f, 1);
    s_initStage = 3;
    s_chNext = now + 200;
  } else if (s_initStage == 3) {
    if (now >= s_chNext) {
      uint8_t f[2] = {0x1F, s_chIdx};
      writeFrame(f, 2);
      s_chIdx++;
      s_chNext = now + 200;
      if (s_chIdx >= 8) s_initStage = 4;
    }
  }

  // 3) poll for queued messages
  if (s_initStage >= 4) {
    if (s_wantPoll || now - s_lastPoll > 3000) {
      uint8_t f[1] = {0x0A};
      writeFrame(f, 1);
      s_lastPoll = now;
      s_wantPoll = false;
    }
  }
}

bool meshBridgeConnected() { return s_connected; }
String meshBridgeStatus()  { return s_status; }
String meshBridgePeer()    { return s_peer; }

String meshBridgeDeviceName() {
  if (!s_devName.isEmpty()) return s_devName;
  return s_peer.length() ? s_peer : String("—");
}
String   meshBridgeModel()      { return s_model; }
String   meshBridgeVersion()    { return s_version; }
uint16_t meshBridgeBatteryMv()  { return s_batteryMv; }
uint8_t  meshBridgeChannelCount() {
  uint8_t c = 0;
  for (uint8_t i = 0; i < 8; i++) if (s_channels[i].length()) c = i + 1;
  return c ? c : (s_maxChannels ? s_maxChannels : 1);
}
String meshBridgeChannelName(uint8_t idx) { return channelName(idx); }
std::vector<MeshMessage>& meshBridgeMessages() { return s_msgs; }

bool meshBridgeSendChannel(uint8_t ch, const String& text, String& err) {
  if (!s_connected) { err = "not connected"; return false; }
  if (ch > 7) { err = "bad channel"; return false; }
  std::vector<uint8_t> f;
  f.push_back(0x03);
  f.push_back(0x00);
  f.push_back(ch);
  uint32_t ts = nowEpoch();
  f.push_back(ts & 0xFF); f.push_back((ts >> 8) & 0xFF);
  f.push_back((ts >> 16) & 0xFF); f.push_back((ts >> 24) & 0xFF);
  for (size_t i = 0; i < text.length(); i++) f.push_back((uint8_t)text[i]);
  if (!writeFrame(f.data(), f.size())) { err = "write failed"; return false; }

  MeshMessage m;
  m.kind = "channel"; m.outgoing = true; m.channel = ch;
  m.from = "me"; m.text = text; m.timestamp = ts;
  pushMessage(m);
  return true;
}

void meshBridgeSetEpoch(uint32_t epochSeconds) {
  s_epochBase = epochSeconds;
  s_epochAt = millis();
  s_epochSet = true;
}

void meshBridgeSetPin(uint32_t pin) {
  s_pin = pin;
  s_needPin = false;
  s_status = s_target.length() ? "connecting" : "scanning";
  s_prefs.putUInt("pin", pin);   // survive our own reboots
  if (s_client && s_client->isConnected()) s_client->disconnect();
}

// --- discovery / node picker ----------------------------------------------
size_t meshBridgeNodeCount() { return s_found.size(); }

bool meshBridgeNodeAt(size_t i, MeshNodeInfo& out) {
  if (i >= s_found.size()) return false;
  out = s_found[i];
  return true;
}

String meshBridgeTarget() { return s_target; }

void meshBridgeSetTarget(const String& address) {
  s_target = address;
  if (s_target.length()) s_prefs.putString("target", s_target);
  else                   s_prefs.remove("target");
  s_needPin = false;
  if (s_client && s_client->isConnected() && !s_peerAddr.equalsIgnoreCase(address))
    s_client->disconnect();
  s_scanRequest = true;
}

void meshBridgeClearSelection() {
  s_target   = "";
  s_needPin  = false;
  s_prefs.remove("target");
  s_prefs.remove("pin");
  if (s_client && s_client->isConnected()) s_client->disconnect();
  s_scanRequest = true;
}

void meshBridgeRequestScan() {
  if (!s_connected) s_scanRequest = true;
}
