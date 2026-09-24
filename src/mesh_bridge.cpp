#include "mesh_bridge.h"
#include "config.h"

#include <NimBLEDevice.h>
#include <Preferences.h>
#include <vector>
#include <cstring>
#include <ctime>
#include <cmath>

// ---------------------------------------------------------------------------
// Protocols
//   MeshCore   : Nordic UART Service (NUS), framed companion protocol.
//   Meshtastic : its own service UUID; raw protobuf ToRadio / FromRadio over
//                TORADIO (write) / FROMRADIO (read), FROMNUM notifies "data".
// ---------------------------------------------------------------------------
#define SVC_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"   // MeshCore service
#define RX_UUID  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"   // app -> fw
#define TX_UUID  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"   // fw -> app

#define MT_SVC_UUID      "6ba1b218-15a8-461f-9fa8-5dcae273eafd"
#define MT_TORADIO_UUID  "f75c76d2-129e-4dad-a1dd-7866124401e7"
#define MT_FROMRADIO_UUID "2c55e69e-4993-11ed-b878-0242ac120002"
#define MT_FROMNUM_UUID  "ed9da18c-a800-4f66-a670-aa7547e34453"

// Shared constant: Meshtastic broadcast destination.
static const uint32_t MT_BROADCAST = 0xFFFFFFFFu;
// Meshtastic port numbers we care about.
static const uint32_t PN_TEXT_MESSAGE = 1;

// MeshCore companion packet types
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

// ---------------------------------------------------------------------------
// BLE objects
// ---------------------------------------------------------------------------
static NimBLEScan*   s_scan      = nullptr;
static NimBLEClient* s_client    = nullptr;
static NimBLERemoteCharacteristic* s_rx = nullptr;   // MeshCore write
static NimBLERemoteCharacteristic* s_tx = nullptr;   // MeshCore notify

// Meshtastic characteristics
static NimBLERemoteCharacteristic* s_mtTor     = nullptr;  // write
static NimBLERemoteCharacteristic* s_mtFrom    = nullptr;  // read
static NimBLERemoteCharacteristic* s_mtFromnum = nullptr;  // notify

static volatile bool s_connected  = false;
static volatile bool s_connecting = false;
static volatile bool s_secure     = false;
static volatile bool s_needPin    = false;
static uint32_t s_pin = 123456;
static String s_status = "idle";
static String s_peer;
static String s_proto = "";        // "meshcore" | "meshtastic"

static uint32_t s_t0        = 0;     // millis at (re)connect
static int      s_initStage = 0;     // MeshCore init sequence
static uint32_t s_chNext    = 0;
static uint8_t  s_chIdx     = 0;
static uint32_t s_lastPoll  = 0;
static bool     s_wantPoll  = false;

// Meshtastic runtime
static bool     s_mtConfigSent = false;
static bool     s_mtDone       = false;   // config_complete seen
static uint32_t s_mtMyNum      = 0;
static uint32_t s_mtLastRead   = 0;
static volatile bool s_mtRead  = false;   // FROMNUM fired
static int      s_mtReadFail   = 0;

static std::vector<MeshNodeInfo> s_found;
static String   s_target;
static String   s_peerAddr;
static bool     s_scanRequest = false;

static bool     s_epochSet = false;
static uint32_t s_epochBase = 0, s_epochAt = 0;
static Preferences s_prefs;

// Decoded device state (shared across protocols)
static String   s_devName, s_model, s_version;
static uint16_t s_batteryMv = 0;
static uint8_t  s_maxChannels = 0;
static String   s_channels[8];
static std::vector<MeshMessage> s_msgs;
static std::vector<MeshPeerInfo> s_peers;

// ---------------------------------------------------------------------------
// tiny protobuf codec (Meshtastic)
// ---------------------------------------------------------------------------
static void pv(std::vector<uint8_t>& o, uint64_t v) {
  while (v >= 0x80) { o.push_back((uint8_t)(v | 0x80)); v >>= 7; }
  o.push_back((uint8_t)v);
}
static void pt(std::vector<uint8_t>& o, uint32_t f, uint32_t w) { pv(o, ((uint64_t)f << 3) | w); }
static void pU(std::vector<uint8_t>& o, uint32_t f, uint32_t v) { pt(o, f, 0); pv(o, v); }
static void pB(std::vector<uint8_t>& o, uint32_t f, const uint8_t* d, size_t n) {
  pt(o, f, 2); pv(o, n); o.insert(o.end(), d, d + n);
}
static void pB(std::vector<uint8_t>& o, uint32_t f, const std::vector<uint8_t>& d) {
  pB(o, f, d.data(), d.size());
}
static void pF(std::vector<uint8_t>& o, uint32_t f, uint32_t v) {
  pt(o, f, 5);
  o.push_back(v & 0xFF); o.push_back((v >> 8) & 0xFF);
  o.push_back((v >> 16) & 0xFF); o.push_back((v >> 24) & 0xFF);
}

struct PbR {
  const uint8_t* p; size_t n; size_t i = 0;
  PbR(const uint8_t* d, size_t len) : p(d), n(len) {}
  bool varint(uint64_t& v) {
    v = 0; int shift = 0;
    while (i < n) {
      uint8_t b = p[i++];
      v |= (uint64_t)(b & 0x7F) << shift;
      if (!(b & 0x80)) return true;
      shift += 7;
      if (shift > 63) return false;
    }
    return false;
  }
  bool next(uint32_t& field, uint32_t& wire) {
    if (i >= n) return false;
    uint64_t tag;
    if (!varint(tag)) return false;
    field = (uint32_t)(tag >> 3);
    wire  = (uint32_t)(tag & 7);
    return true;
  }
  bool bytes(const uint8_t*& d, size_t& len) {
    uint64_t l;
    if (!varint(l)) return false;
    if (i + l > n) return false;
    d = p + i; len = (size_t)l; i += (size_t)l;
    return true;
  }
  bool fixed32(uint32_t& v) {
    if (i + 4 > n) return false;
    v = (uint32_t)p[i] | ((uint32_t)p[i+1] << 8) | ((uint32_t)p[i+2] << 16) | ((uint32_t)p[i+3] << 24);
    i += 4; return true;
  }
  bool skip(uint32_t wire) {
    if (wire == 0) { uint64_t d; return varint(d); }
    if (wire == 1) { i += 8; return i <= n; }
    if (wire == 5) { i += 4; return i <= n; }
    if (wire == 2) { uint64_t l; if (!varint(l)) return false; i += (size_t)l; return i <= n; }
    return false;
  }
};

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

static uint32_t rndId() {
  static uint32_t x = 0x2545F491;
  x ^= x << 13; x ^= x >> 17; x ^= x << 5;
  return x ? x : 1;
}

static void pushMessage(const MeshMessage& m) {
  s_msgs.push_back(m);
  if (s_msgs.size() > 60) s_msgs.erase(s_msgs.begin(), s_msgs.begin() + (s_msgs.size() - 60));
}

static String channelName(uint8_t idx) {
  if (idx >= 8 || s_channels[idx].isEmpty()) return String("CH") + (idx + 1);
  return s_channels[idx];
}

static String peerName(uint32_t num) {
  for (auto& p : s_peers)
    if (p.num == num) return p.longName.length() ? p.longName : (p.shortName.length() ? p.shortName : String(num));
  char b[12];
  snprintf(b, sizeof(b), "%08x", (unsigned)num);
  return String("!") + b;
}

static void upsertPeer(const MeshPeerInfo& in) {
  for (auto& p : s_peers)
    if (p.num == in.num) {
      if (in.longName.length())  p.longName  = in.longName;
      if (in.shortName.length()) p.shortName = in.shortName;
      if (in.lastHeard) p.lastHeard = in.lastHeard;
      if (in.hops)      p.hops      = in.hops;
      if (in.snr != 0)  p.snr       = in.snr;
      p.viaMqtt = in.viaMqtt;
      return;
    }
  s_peers.push_back(in);
  if (s_peers.size() > 80) s_peers.erase(s_peers.begin(), s_peers.begin() + (s_peers.size() - 80));
}

// ---------------------------------------------------------------------------
// MeshCore parsing
// ---------------------------------------------------------------------------
static bool writeFrame(const uint8_t* d, size_t n) {
  if (!s_connected || !s_rx) return false;
  bool ok = s_rx->writeValue(d, n, true);
  Serial.printf("[BRIDGE] tx 0x%02X len=%u ok=%d\n", d[0], (unsigned)n, (int)ok);
  return ok;
}

static void handleMeshCoreFrame(const uint8_t* d, size_t n) {
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
      break;
    case PKT_CHANNEL_INFO: {
      if (n < 34) break;
      uint8_t idx = d[1];
      if (idx < 8) s_channels[idx] = rdStr(d + 2, 32);
      break;
    }
    case PKT_CHANNEL_MSG:
    case PKT_CHANNEL_MSG_V3: {
      size_t o = (t == PKT_CHANNEL_MSG_V3) ? 4 : 1;
      MeshMessage m; m.kind = "channel"; m.outgoing = false;
      if (t == PKT_CHANNEL_MSG_V3 && n >= 2) m.snr = (float)(int8_t)d[1] / 4.0f;
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
      MeshMessage m; m.kind = "direct"; m.outgoing = false;
      if (t == PKT_CONTACT_MSG_V3 && n >= 2) m.snr = (float)(int8_t)d[1] / 4.0f;
      if (n < o + 11) break;
      String pk; char b[3];
      for (int i = 0; i < 6; i++) { snprintf(b, sizeof(b), "%02x", d[o + i]); pk += b; }
      m.from = pk;
      uint8_t txtType = d[o + 7];
      m.timestamp = rdU32(d + o + 8);
      size_t textOff = o + 12 + (txtType == 2 ? 4 : 0);
      if (textOff < n) m.text = rdStr(d + textOff, n - textOff);
      pushMessage(m);
      break;
    }
    case PKT_MSGS_WAITING:
      s_wantPoll = true;
      break;
    default: break;
  }
}

// ---------------------------------------------------------------------------
// Meshtastic parsing / sending
// ---------------------------------------------------------------------------
static void mtParseUser(const uint8_t* d, size_t n, MeshPeerInfo& peer) {
  PbR r(d, n);
  uint32_t f, w;
  while (r.next(f, w)) {
    if (w == 2) {
      const uint8_t* b; size_t l;
      if (!r.bytes(b, l)) break;
      if (f == 2) peer.longName = rdStr(b, l);
      else if (f == 3) peer.shortName = rdStr(b, l);
      else if (f == 1) { /* id string */ }
    } else r.skip(w);
  }
}

static void mtParseNodeInfo(const uint8_t* d, size_t n) {
  PbR r(d, n);
  MeshPeerInfo peer; uint32_t f, w;
  while (r.next(f, w)) {
    if (f == 1 && w == 0) { uint64_t v; r.varint(v); peer.num = (uint32_t)v; }
    else if (f == 2 && w == 2) { const uint8_t* b; size_t l; if (r.bytes(b, l)) mtParseUser(b, l, peer); }
    else if (f == 4 && w == 5) { uint32_t v; if (r.fixed32(v)) peer.snr = *(float*)&v; }
    else if (f == 5 && w == 5) { r.fixed32(peer.lastHeard); }
    else if (f == 8 && w == 0) { uint64_t v; r.varint(v); peer.viaMqtt = v != 0; }
    else if (f == 9 && w == 0) { uint64_t v; r.varint(v); peer.hops = (uint32_t)v; }
    else r.skip(w);
  }
  if (peer.num) {
    upsertPeer(peer);
    if (!s_devName.length() && peer.num == s_mtMyNum)
      s_devName = peer.longName.length() ? peer.longName : peer.shortName;
  }
}

static void mtParseChannel(const uint8_t* d, size_t n) {
  PbR r(d, n);
  int idx = -1; uint32_t f, w; String name;
  while (r.next(f, w)) {
    if (f == 1 && w == 0) { uint64_t v; r.varint(v); idx = (int)v; }
    else if (f == 2 && w == 2) {
      const uint8_t* b; size_t l;
      if (r.bytes(b, l)) {
        PbR s(b, l); uint32_t sf, sw;
        while (s.next(sf, sw)) {
          if (sf == 3 && sw == 2) { const uint8_t* nb; size_t nl; if (s.bytes(nb, nl)) name = rdStr(nb, nl); }
          else s.skip(sw);
        }
      }
    } else r.skip(w);
  }
  if (idx >= 0 && idx < 8) {
    s_channels[idx] = name.length() ? name : (String("Channel ") + (idx + 1));
    if (idx + 1 > s_maxChannels) s_maxChannels = idx + 1;
  }
}

static void mtParseMyInfo(const uint8_t* d, size_t n) {
  PbR r(d, n);
  uint32_t f, w;
  while (r.next(f, w)) {
    if (f == 1 && w == 0) { uint64_t v; r.varint(v); s_mtMyNum = (uint32_t)v; }
    else r.skip(w);
  }
}

static void mtParseEntry(const uint8_t* d, size_t n) {   // a single Data message
  PbR r(d, n);
  uint32_t f, w;
  while (r.next(f, w)) {
    if (f == 1 && w == 0) { uint64_t v; r.varint(v); s_maxChannels = s_maxChannels; (void)v; }
    else r.skip(w);
  }
}

static void mtParsePacket(const uint8_t* d, size_t n) {
  uint32_t from = 0, to = 0, chan = 0, rxTime = 0, pktId = 0, rxRssi = 0, hopStart = 0;
  float snr = 0; uint32_t portnum = 0; const uint8_t* payload = nullptr; size_t payLen = 0;
  bool haveData = false;

  PbR r(d, n);
  uint32_t f, w;
  while (r.next(f, w)) {
    if (f == 1 && w == 5) r.fixed32(from);
    else if (f == 2 && w == 5) r.fixed32(to);
    else if (f == 3 && w == 0) { uint64_t v; r.varint(v); chan = (uint32_t)v; }
    else if (f == 4 && w == 2) {
      const uint8_t* b; size_t l;
      if (r.bytes(b, l)) {
        PbR q(b, l); uint32_t qf, qw;
        while (q.next(qf, qw)) {
          if (qf == 1 && qw == 0) { uint64_t v; q.varint(v); portnum = (uint32_t)v; haveData = true; }
          else if (qf == 2 && qw == 2) { const uint8_t* pb; size_t pl; if (q.bytes(pb, pl)) { payload = pb; payLen = pl; } }
          else q.skip(qw);
        }
      }
    }
    else if (f == 6 && w == 5) r.fixed32(pktId);
    else if (f == 7 && w == 5) r.fixed32(rxTime);
    else if (f == 8 && w == 5) { uint32_t v; if (r.fixed32(v)) snr = *(float*)&v; }
    else if (f == 12 && w == 0) { uint64_t v; r.varint(v); rxRssi = (uint32_t)(int32_t)v; }
    else if (f == 15 && w == 0) { uint64_t v; r.varint(v); hopStart = (uint32_t)v; }
    else r.skip(w);
  }
  (void)pktId; (void)hopStart; (void)rxRssi;

  if (!haveData || portnum != PN_TEXT_MESSAGE || !payload) return;

  MeshMessage m;
  bool outbound = (from == s_mtMyNum);
  m.outgoing = outbound;
  m.timestamp = rxTime ? rxTime : nowEpoch();
  m.snr = snr;
  if (to == MT_BROADCAST || to == 0) {
    m.kind = "channel";
    m.channel = (uint8_t)chan;
    m.from = outbound ? "me" : channelName(m.channel);
  } else {
    m.kind = "direct";
    m.channel = 0;
    m.from = outbound ? "me" : peerName(from);
  }
  m.text = rdStr(payload, payLen);
  if (m.text.length()) pushMessage(m);
}

static void mtParseFromRadio(const uint8_t* d, size_t n) {
  PbR r(d, n);
  uint32_t f, w;
  while (r.next(f, w)) {
    if (f == 2 && w == 2) { const uint8_t* b; size_t l; if (r.bytes(b, l)) mtParsePacket(b, l); }
    else if (f == 3 && w == 2) { const uint8_t* b; size_t l; if (r.bytes(b, l)) mtParseMyInfo(b, l); }
    else if (f == 4 && w == 2) { const uint8_t* b; size_t l; if (r.bytes(b, l)) mtParseNodeInfo(b, l); }
    else if (f == 10 && w == 2) { const uint8_t* b; size_t l; if (r.bytes(b, l)) mtParseChannel(b, l); }
    else if (f == 7 && w == 0) { uint64_t v; r.varint(v); s_mtDone = true; Serial.printf("[BRIDGE] config complete id=%u\n", (unsigned)v); }
    else if (f == 13 && w == 2) {
      const uint8_t* b; size_t l;
      if (r.bytes(b, l)) {
        PbR q(b, l); uint32_t qf, qw;
        while (q.next(qf, qw)) {
          if (qf == 1 && qw == 2) { const uint8_t* fb; size_t fl; if (q.bytes(fb, fl)) s_version = rdStr(fb, fl); }
          else if (qf == 4 && qw == 2) { const uint8_t* hb; size_t hl; if (q.bytes(hb, hl)) s_model = rdStr(hb, hl); }
          else q.skip(qw);
        }
      }
    }
    else r.skip(w);
  }
}

static bool mtWriteToRadio(const std::vector<uint8_t>& frame) {
  if (!s_connected || !s_mtTor) return false;
  bool ok = s_mtTor->writeValue(frame.data(), frame.size(), true);
  Serial.printf("[BRIDGE] toRadio len=%u ok=%d\n", (unsigned)frame.size(), (int)ok);
  return ok;
}

static void mtSendWantConfig() {
  std::vector<uint8_t> f;
  pU(f, 3, rndId());        // want_config_id
  mtWriteToRadio(f);
}

static void mtReadFromRadio() {
  if (!s_mtFrom || !s_client || !s_client->isConnected()) return;
  for (int i = 0; i < 32; i++) {
    NimBLEAttValue v = s_mtFrom->readValue();
    if (v.length() == 0) break;
    mtParseFromRadio(v.data(), v.length());
  }
  s_mtReadFail = 0;
}

static void mtSendText(uint32_t dest, uint8_t chan, const String& text) {
  std::vector<uint8_t> data;
  pU(data, 1, PN_TEXT_MESSAGE);
  pB(data, 2, (const uint8_t*)text.c_str(), text.length());

  std::vector<uint8_t> pkt;
  pF(pkt, 1, s_mtMyNum ? s_mtMyNum : 0);   // from
  pF(pkt, 2, dest);                        // to
  pU(pkt, 3, chan);                        // channel
  pB(pkt, 4, data);                        // decoded
  pF(pkt, 6, rndId());                     // id
  pU(pkt, 9, 3);                           // hop_limit
  pU(pkt, 10, 1);                          // want_ack

  std::vector<uint8_t> frame;
  pB(frame, 1, pkt);                       // ToRadio.packet
  mtWriteToRadio(frame);
}

// ---------------------------------------------------------------------------
// BLE plumbing
// ---------------------------------------------------------------------------
static void mcNotifyCB(NimBLERemoteCharacteristic*, uint8_t* data, size_t len, bool) {
  if (len == 0) return;
  handleMeshCoreFrame(data, len);
}

static void mtFromnumCB(NimBLERemoteCharacteristic*, uint8_t* data, size_t len, bool) {
  (void)data; (void)len;
  s_mtRead = true;
}

static void disconnectCB(NimBLEClient*) {
  s_connected = false;
  s_secure    = false;
  s_rx = nullptr; s_tx = nullptr;
  s_mtTor = nullptr; s_mtFrom = nullptr; s_mtFromnum = nullptr;
  s_peer = ""; s_peerAddr = "";
  s_proto = "";
  s_mtConfigSent = false;
  s_mtDone = false;
  s_status = s_target.length() ? "connecting" : "scanning";
}

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
    bool mc = dev->isAdvertisingService(NimBLEUUID(SVC_UUID)) ||
              dev->getName().find("MeshCore") != std::string::npos;
    bool mt = dev->isAdvertisingService(NimBLEUUID(MT_SVC_UUID)) ||
              dev->getName().find("Meshtastic") != std::string::npos;
    // de-dup / refresh RSSI
    for (auto& d : s_found) {
      if (d.address.equalsIgnoreCase(addr)) {
        d.rssi = dev->getRSSI(); d.name = name; d.meshcore = mc;
        d.proto = mc ? "meshcore" : (mt ? "meshtastic" : "");
        return;
      }
    }
    MeshNodeInfo info;
    info.name    = name.isEmpty() ? addr : name;
    info.address = addr;
    info.rssi    = dev->getRSSI();
    info.meshcore = mc;
    info.proto   = mc ? "meshcore" : (mt ? "meshtastic" : "");
    s_found.push_back(info);
    if (s_found.size() > 32) {
      for (size_t i = 0; i < s_found.size(); i++) {
        if (s_found[i].proto.isEmpty()) { s_found.erase(s_found.begin() + i); break; }
      }
      if (s_found.size() > 32) s_found.erase(s_found.begin());
    }
    if (mc || mt)
      Serial.printf("[BRIDGE] found '%s' %s [%s]\n", info.name.c_str(), addr.c_str(),
                    info.proto.c_str());
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
  Serial.printf("[BRIDGE] connecting to '%s' %s ...\n", name.c_str(), addr.toString().c_str());
  if (!s_client->connect(addr, true)) {
    Serial.println("[BRIDGE] connect failed");
    s_connecting = false;
    s_status = s_target.length() ? "connecting" : "scanning";
    if (s_client->isConnected()) s_client->disconnect();
    return false;
  }
  Serial.println("[BRIDGE] link up, resolving GATT");

  NimBLERemoteService* mtSvc = s_client->getService(NimBLEUUID(MT_SVC_UUID));
  NimBLERemoteService* mcSvc = s_client->getService(NimBLEUUID(SVC_UUID));

  if (mtSvc) {
    s_mtTor     = mtSvc->getCharacteristic(NimBLEUUID(MT_TORADIO_UUID));
    s_mtFrom    = mtSvc->getCharacteristic(NimBLEUUID(MT_FROMRADIO_UUID));
    s_mtFromnum = mtSvc->getCharacteristic(NimBLEUUID(MT_FROMNUM_UUID));
    if (!s_mtTor || !s_mtFrom) {
      Serial.println("[BRIDGE] Meshtastic characteristics missing");
      s_client->disconnect(); s_connecting = false; return false;
    }
    if (s_mtFromnum && !s_mtFromnum->subscribe(true, mtFromnumCB, true))
      Serial.println("[BRIDGE] FROMNUM subscribe failed (continuing)");
    s_proto = "meshtastic";
    s_connected = true;
    s_connecting = false;
    s_status = "connected";
    s_peerAddr = addr.toString().c_str();
    s_t0 = millis();
    s_mtConfigSent = false; s_mtDone = false;
    s_mtRead = true;
    Serial.printf("[BRIDGE] Meshtastic link up to '%s'\n", s_peer.c_str());
    return true;
  }

  if (mcSvc) {
    s_rx = mcSvc->getCharacteristic(NimBLEUUID(RX_UUID));
    s_tx = mcSvc->getCharacteristic(NimBLEUUID(TX_UUID));
    if (!s_rx || !s_tx) {
      Serial.println("[BRIDGE] MeshCore characteristics missing");
      s_client->disconnect(); s_connecting = false; return false;
    }
    if (!s_tx->subscribe(true, mcNotifyCB, true)) {
      Serial.println("[BRIDGE] subscribe failed");
      s_client->disconnect(); s_connecting = false;
      s_status = s_target.length() ? "connecting" : "scanning";
      return false;
    }
    s_proto = "meshcore";
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

  Serial.println("[BRIDGE] no known mesh service on device");
  s_client->disconnect();
  s_connecting = false;
  return false;
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
  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_DISPLAY);

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
    if (s_needPin) {
      s_status = "need_pin";
      vTaskDelay(pdMS_TO_TICKS(500));
      return;
    }

    s_status = s_target.length() ? "connecting" : "scanning";
    s_found.clear();
    s_scanRequest = false;
    Serial.println("[BRIDGE] scanning 5s ...");
    s_scan->start(5, false);
    s_scan->stop();
    Serial.printf("[BRIDGE] scan done: %u device(s)\n", (unsigned)s_found.size());
    for (auto& d : s_found)
      if (!d.proto.isEmpty())
        Serial.printf("[BRIDGE]   %-22s %-17s %4d dBm [%s]\n", d.name.c_str(),
                      d.address.c_str(), d.rssi, d.proto.c_str());

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
        if (!d.proto.isEmpty() && (!pick || d.rssi > pick->rssi)) pick = &d;
      if (!pick) { vTaskDelay(pdMS_TO_TICKS(2500)); return; }
    }

    if (connectToFoundDevice(NimBLEAddress(pick->address.c_str()), pick->name)) {
      if (!s_target.length()) {
        s_target = pick->address;
        s_prefs.putString("target", s_target);
      }
    } else {
      vTaskDelay(pdMS_TO_TICKS(1500));
    }
    return;
  }

  if (!s_connected) return;

  if (s_proto == "meshtastic") {
    if (!s_mtConfigSent) { mtSendWantConfig(); s_mtConfigSent = true; }
    if (s_mtRead || now - s_mtLastRead > 1500) {
      mtReadFromRadio();
      s_mtLastRead = now;
      s_mtRead = false;
    }
    if (s_mtDone && s_version.isEmpty()) s_version = "(meshtastic)";
    return;
  }

  // MeshCore init + poll
  uint32_t el = now - s_t0;
  if (s_initStage == 0 && s_secure && el > 400) {
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
String meshBridgeProtocol(){ return s_proto; }

String meshBridgeDeviceName() {
  if (!s_devName.isEmpty()) return s_devName;
  return s_peer.length() ? s_peer : String("—");
}
String   meshBridgeModel()      { return s_model; }
String   meshBridgeVersion()    { return s_version; }
uint16_t meshBridgeBatteryMv()  { return s_batteryMv; }
uint32_t meshBridgeMyNum()      { return s_mtMyNum; }

uint8_t meshBridgeChannelCount() {
  uint8_t c = 0;
  for (uint8_t i = 0; i < 8; i++) if (s_channels[i].length()) c = i + 1;
  return c ? c : (s_maxChannels ? s_maxChannels : 1);
}
String meshBridgeChannelName(uint8_t idx) { return channelName(idx); }
std::vector<MeshMessage>& meshBridgeMessages() { return s_msgs; }

size_t meshBridgePeerCount() { return s_peers.size(); }
bool meshBridgePeerAt(size_t i, MeshPeerInfo& out) {
  if (i >= s_peers.size()) return false;
  out = s_peers[i];
  return true;
}

bool meshBridgeSendChannel(uint8_t ch, const String& text, String& err) {
  if (!s_connected) { err = "not connected"; return false; }
  if (ch > 7) { err = "bad channel"; return false; }

  if (s_proto == "meshtastic") {
    mtSendText(MT_BROADCAST, ch, text);
  } else {
    std::vector<uint8_t> f;
    f.push_back(0x03); f.push_back(0x00); f.push_back(ch);
    uint32_t ts = nowEpoch();
    f.push_back(ts & 0xFF); f.push_back((ts >> 8) & 0xFF);
    f.push_back((ts >> 16) & 0xFF); f.push_back((ts >> 24) & 0xFF);
    for (size_t i = 0; i < text.length(); i++) f.push_back((uint8_t)text[i]);
    if (!writeFrame(f.data(), f.size())) { err = "write failed"; return false; }
  }

  MeshMessage m;
  m.kind = "channel"; m.outgoing = true; m.channel = ch;
  m.from = "me"; m.text = text; m.timestamp = nowEpoch();
  pushMessage(m);
  return true;
}

bool meshBridgeSendDirect(uint32_t dest, const String& text, String& err) {
  if (!s_connected) { err = "not connected"; return false; }
  if (s_proto != "meshtastic") { err = "direct messages not supported for this node yet"; return false; }
  mtSendText(dest, 0, text);
  MeshMessage m;
  m.kind = "direct"; m.outgoing = true; m.channel = 0;
  m.from = "me"; m.text = text; m.timestamp = nowEpoch();
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
  s_prefs.putUInt("pin", pin);
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
