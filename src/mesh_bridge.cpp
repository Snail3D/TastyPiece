#include "mesh_bridge.h"
#include "config.h"
#include "mt_settings_gen.h"

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
  PKT_CUSTOM_VARS = 0x15, PKT_TUNING_PARAMS = 0x17, PKT_AUTOADD_CONFIG = 0x19,
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
static bool          s_paused     = false;   // user disconnected: don't auto-reconnect
static MeshNodeInfo  s_lastPeer;             // last successfully connected node (for the locked UI)
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

// MeshCore settings (parsed from SELF_INFO + GET_* responses)
struct McCfg {
  bool     ready       = false;
  String   name;        bool haveName = false;
  int      txPower     = 0;   bool haveTx = false;
  uint32_t freqHz      = 0, bwHz = 0;
  uint8_t  sf = 0, cr = 0;    bool haveRadio = false;
  int32_t  lat = 0, lon = 0;  bool havePos = false;
  uint8_t  multiAcks = 0, advertLoc = 0, telemBase = 0, telemLoc = 0, telemEnv = 0, manualAdd = 0;
  bool     haveOther   = false;
  float    rxDelay     = 0, airtime = 0;  bool haveTuning = false;
  uint8_t  autoCfg = 0, autoHops = 0;     bool haveAuto = false;
  String   chName[8];  bool chHave[8] = {false};
  uint8_t  chSecret[8][16]; bool chSecretHave[8] = {false};
  String   varName[16], varValue[16]; int varCount = 0;
};
static McCfg s_mc;
static bool  s_mcReqTuning = false, s_mcReqVars = false, s_mcReqAuto = false;
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
// Meshtastic settings: Config / ModuleConfig / Channel / Owner over AdminMessage
// ---------------------------------------------------------------------------
static bool mtWriteToRadio(const std::vector<uint8_t>& frame);

#define MT_CFG_SLOTS 10   // ConfigType 0..9
#define MT_MOD_SLOTS 17   // ModuleConfigType 0..16
#define MT_BLOB_MAX  160

static uint8_t  s_cfgBuf[MT_CFG_SLOTS][MT_BLOB_MAX];
static uint16_t s_cfgLen[MT_CFG_SLOTS] = {0};
static bool     s_cfgHave[MT_CFG_SLOTS] = {false};
static uint8_t  s_modBuf[MT_MOD_SLOTS][MT_BLOB_MAX];
static uint16_t s_modLen[MT_MOD_SLOTS] = {0};
static bool     s_modHave[MT_MOD_SLOTS] = {false};
static uint8_t  s_chBuf[8][MT_BLOB_MAX];
static uint16_t s_chLen[8] = {0};
static bool     s_chHave[8] = {false};
static uint8_t  s_ownerBuf[MT_BLOB_MAX];
static uint16_t s_ownerLen = 0;
static bool     s_ownerHave = false;
static uint8_t  s_reqStage = 0;      // 0..35 request sweep, 200 = done
static uint32_t s_lastOwnerReq = 0;
static bool     s_settingsReady = false;

static const MtFieldDef* mtFindField(uint8_t scope, uint8_t type, uint8_t field) {
  for (uint16_t i = 0; i < MT_FIELD_COUNT; i++) {
    const MtFieldDef& f = MT_FIELDS[i];
    if (f.scope == scope && f.type == type && f.field == field) return &f;
  }
  return nullptr;
}

static void mtStoreBlob(uint8_t* dst, uint16_t& len, bool& have, const uint8_t* d, size_t n) {
  if (n > MT_BLOB_MAX) n = MT_BLOB_MAX;
  memcpy(dst, d, n);
  len = (uint16_t)n;
  have = true;
}

static void mtStoreChannelMsg(const uint8_t* d, size_t n);   // fwd

static void mtStoreConfigMsg(const uint8_t* d, size_t n) {   // Config message body
  PbR r(d, n); uint32_t f, w;
  while (r.next(f, w)) {
    if (w == 2 && f >= 1 && f <= 10) {
      const uint8_t* b; size_t l;
      if (r.bytes(b, l)) mtStoreBlob(s_cfgBuf[f - 1], s_cfgLen[f - 1], s_cfgHave[f - 1], b, l);
    } else r.skip(w);
  }
  s_settingsReady = true;
}

static void mtStoreModuleMsg(const uint8_t* d, size_t n) {   // ModuleConfig message body
  PbR r(d, n); uint32_t f, w;
  while (r.next(f, w)) {
    if (w == 2 && f >= 1 && f <= MT_MOD_SLOTS) {
      const uint8_t* b; size_t l;
      if (r.bytes(b, l)) mtStoreBlob(s_modBuf[f - 1], s_modLen[f - 1], s_modHave[f - 1], b, l);
    } else r.skip(w);
  }
  s_settingsReady = true;
}

static void mtStoreChannelMsg(const uint8_t* d, size_t n) {  // Channel message body
  PbR r(d, n); int idx = 0; uint32_t f, w;
  while (r.next(f, w)) {
    if (f == 1 && w == 0) { uint64_t v; r.varint(v); idx = (int)v; }
    else r.skip(w);
  }
  if (idx < 0 || idx > 7) return;
  mtStoreBlob(s_chBuf[idx], s_chLen[idx], s_chHave[idx], d, n);
}

static void mtParseAdmin(const uint8_t* d, size_t n) {       // AdminMessage
  PbR r(d, n); uint32_t f, w;
  while (r.next(f, w)) {
    const uint8_t* b; size_t l;
    if (w == 2 && f == 2) { if (r.bytes(b, l)) mtStoreChannelMsg(b, l); }
    else if (w == 2 && f == 4) { if (r.bytes(b, l)) mtStoreBlob(s_ownerBuf, s_ownerLen, s_ownerHave, b, l); }
    else if (w == 2 && f == 6) { if (r.bytes(b, l)) mtStoreConfigMsg(b, l); }
    else if (w == 2 && f == 8) { if (r.bytes(b, l)) mtStoreModuleMsg(b, l); }
    else r.skip(w);
  }
}

static void mtChannelParts(uint8_t idx, uint32_t& role, const uint8_t*& settings, size_t& slen) {
  role = 0; settings = nullptr; slen = 0;
  if (idx > 7 || !s_chHave[idx]) return;
  PbR r(s_chBuf[idx], s_chLen[idx]); uint32_t f, w;
  while (r.next(f, w)) {
    if (f == 3 && w == 0) { uint64_t v; r.varint(v); role = (uint32_t)v; }
    else if (f == 2 && w == 2) { const uint8_t* b; size_t l; if (r.bytes(b, l)) { settings = b; slen = l; } }
    else r.skip(w);
  }
}

static void mtJsonEsc(String& o, const String& s) {
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c < 0x20) { char b[10]; snprintf(b, sizeof(b), "\\u%04x", (unsigned)(uint8_t)c); o += b; }
    else o += c;
  }
}

static bool mtDecodeIntoJson(String& o, uint8_t scope, uint8_t type, const uint8_t* d,
                             size_t n, bool& first) {
  PbR r(d, n); uint32_t f, w; bool any = false;
  while (r.next(f, w)) {
    const MtFieldDef* fd = mtFindField(scope, type, f);
    if (!fd || fd->rep) { r.skip(w); continue; }
    if (fd->kind == 4 && w != 2) { r.skip(w); continue; }
    if (fd->kind != 4 && w != 0 && w != 5) { r.skip(w); continue; }
    if (!first) o += ',';
    first = false; any = true;
    o += '"'; o += fd->key; o += "\":";
    if (fd->kind == 4) {
      const uint8_t* b; size_t l;
      if (!r.bytes(b, l)) return any;
      o += '"'; mtJsonEsc(o, rdStr(b, l)); o += '"';
    } else if (fd->kind == 3) {
      uint32_t bits = 0;
      if (!r.fixed32(bits)) return any;
      float fv; memcpy(&fv, &bits, 4);
      char buf[24]; snprintf(buf, sizeof(buf), "%.3f", fv); o += buf;
    } else {
      uint64_t v = 0;
      if (!r.varint(v)) return any;
      char buf[20];
      if (fd->kind == 2) snprintf(buf, sizeof(buf), "%d", (int32_t)v);
      else if (fd->kind == 5 || fd->kind == 1) snprintf(buf, sizeof(buf), "%u", (uint32_t)v);
      else { o += v ? "1" : "0"; continue; }
      o += buf;
    }
  }
  return any;
}

static bool mtEncodeValue(std::vector<uint8_t>& o, uint32_t f, uint8_t kind, const String& in) {
  String t = in; t.trim();
  switch (kind) {
    case 0:  pU(o, f, (t == "1" || t.equalsIgnoreCase("true")) ? 1 : 0); return true;
    case 1:  pU(o, f, (uint32_t)strtoul(t.c_str(), nullptr, 10)); return true;
    case 5:  pU(o, f, (uint32_t)strtoul(t.c_str(), nullptr, 10)); return true;
    case 2:  { int32_t x = (int32_t)strtol(t.c_str(), nullptr, 10); pt(o, f, 0); pv(o, (uint64_t)(int64_t)x); return true; }
    case 3:  { float x = strtof(t.c_str(), nullptr); uint32_t bits; memcpy(&bits, &x, 4); pF(o, f, bits); return true; }
    case 4:  pB(o, f, (const uint8_t*)t.c_str(), t.length()); return true;
  }
  return false;
}

// copy d, replacing one field's value (or appending it)
static bool mtEncodeBlob(uint8_t scope, uint8_t type, const uint8_t* d, size_t n,
                         uint8_t field, const String& value, std::vector<uint8_t>& out) {
  const MtFieldDef* fd = mtFindField(scope, type, field);
  if (!fd || fd->rep) return false;
  PbR r(d, n); uint32_t f, w; bool done = false;
  while (r.i < n) {
    size_t start = r.i;
    if (!r.next(f, w)) break;
    PbR tmp(r.p, r.n); tmp.i = r.i;
    if (!tmp.skip(w)) break;
    size_t end = tmp.i;
    if (f == field && !done) {
      if (!mtEncodeValue(out, f, fd->kind, value)) return false;
      done = true;
    } else {
      out.insert(out.end(), d + start, d + end);
    }
    r.i = end;
  }
  if (!done && !mtEncodeValue(out, field, fd->kind, value)) return false;
  return true;
}

static void mtAdminFrame(const std::vector<uint8_t>& admin) {
  std::vector<uint8_t> data;
  pU(data, 1, 6);                 // PortNum ADMIN_APP
  pU(data, 3, 1);                 // Data.want_response -> handlers reply
  pB(data, 2, admin);
  std::vector<uint8_t> pkt;
  pF(pkt, 1, 0);                  // from = 0 -> treated as local admin request
  pF(pkt, 2, s_mtMyNum);          // to (the node itself)
  pB(pkt, 4, data);
  pF(pkt, 6, rndId());
  pU(pkt, 10, 1);                 // want_ack
  std::vector<uint8_t> frame;
  pB(frame, 1, pkt);
  mtWriteToRadio(frame);
}

bool meshBridgeSettingsReady() { return s_settingsReady; }

void meshBridgeRequestSettings() { s_lastOwnerReq = 0; s_reqStage = 0; }

void meshBridgeRequestOwner() {
  std::vector<uint8_t> a;
  pU(a, 3, 1);      // get_owner_request
  mtAdminFrame(a);
}

String meshBridgeConfigJson() {
  String o; o.reserve(6144);
  o += "{\"ready\":"; o += s_settingsReady ? "true" : "false";

  o += ",\"config\":{";
  bool g = false;
  for (uint8_t t = 0; t < MT_CFG_SLOTS; t++) {
    if (!s_cfgHave[t]) continue;
    if (g) o += ','; g = true;
    char kb[6]; snprintf(kb, sizeof(kb), "%u", (unsigned)t);
    o += '"'; o += kb; o += "\":{";
    bool first = true;
    mtDecodeIntoJson(o, 0, t, s_cfgBuf[t], s_cfgLen[t], first);
    o += '}';
  }

  o += "},\"module\":{";
  g = false;
  for (uint8_t t = 0; t < MT_MOD_SLOTS; t++) {
    if (!s_modHave[t]) continue;
    if (g) o += ','; g = true;
    char kb[6]; snprintf(kb, sizeof(kb), "%u", (unsigned)t);
    o += '"'; o += kb; o += "\":{";
    bool first = true;
    mtDecodeIntoJson(o, 1, t, s_modBuf[t], s_modLen[t], first);
    o += '}';
  }

  o += "},\"owner\":{";
  if (s_ownerHave) { bool first = true; mtDecodeIntoJson(o, 3, 0, s_ownerBuf, s_ownerLen, first); }

  o += "},\"channel\":{";
  g = false;
  for (uint8_t i = 0; i < 8; i++) {
    if (!s_chHave[i]) continue;
    if (g) o += ','; g = true;
    uint32_t role; const uint8_t* settings; size_t slen;
    mtChannelParts(i, role, settings, slen);
    char kb[6]; snprintf(kb, sizeof(kb), "%u", (unsigned)i);
    o += '"'; o += kb; o += "\":{\"index\":"; o += kb;
    o += ",\"role\":";
    char rb[8]; snprintf(rb, sizeof(rb), "%u", (unsigned)role); o += rb;
    if (settings) { bool first = false; mtDecodeIntoJson(o, 2, 0, settings, slen, first); }
    o += '}';
  }
  o += "}}";
  return o;
}

bool meshBridgeApplySetting(uint8_t scope, uint8_t type, uint8_t field, const String& value, String& err) {
  if (!s_connected || s_proto != "meshtastic") { err = "not connected to a Meshtastic node"; return false; }

  std::vector<uint8_t> begin, commit;
  pU(begin, 64, 1);       // begin_edit_settings
  pU(commit, 65, 1);      // commit_edit_settings
  std::vector<uint8_t> cluster;

  if (scope == 0) {                       // Config
    if (type >= MT_CFG_SLOTS || !s_cfgHave[type]) { err = "config not loaded"; return false; }
    if (!mtEncodeBlob(0, type, s_cfgBuf[type], s_cfgLen[type], field, value, cluster)) { err = "unsupported setting"; return false; }
    std::vector<uint8_t> cfg; pB(cfg, type + 1, cluster);
    std::vector<uint8_t> admin; pB(admin, 34, cfg);        // set_config
    mtAdminFrame(begin); mtAdminFrame(admin); mtAdminFrame(commit);
    mtStoreBlob(s_cfgBuf[type], s_cfgLen[type], s_cfgHave[type], cluster.data(), cluster.size());
    return true;
  }

  if (scope == 1) {                       // ModuleConfig
    if (type >= MT_MOD_SLOTS || !s_modHave[type]) { err = "module config not loaded"; return false; }
    if (!mtEncodeBlob(1, type, s_modBuf[type], s_modLen[type], field, value, cluster)) { err = "unsupported setting"; return false; }
    std::vector<uint8_t> mc; pB(mc, type + 1, cluster);
    std::vector<uint8_t> admin; pB(admin, 35, mc);         // set_module_config
    mtAdminFrame(begin); mtAdminFrame(admin); mtAdminFrame(commit);
    mtStoreBlob(s_modBuf[type], s_modLen[type], s_modHave[type], cluster.data(), cluster.size());
    return true;
  }

  if (scope == 2) {                       // Channel
    if (type > 7 || !s_chHave[type]) { err = "channel not loaded"; return false; }
    uint32_t role; const uint8_t* settings; size_t slen;
    mtChannelParts(type, role, settings, slen);
    if (!settings) { err = "channel settings missing"; return false; }
    if (!mtEncodeBlob(2, 0, settings, slen, field, value, cluster)) { err = "unsupported setting"; return false; }
    std::vector<uint8_t> ch;
    pU(ch, 1, type);
    pB(ch, 2, cluster);
    pU(ch, 3, role ? role : 1);
    std::vector<uint8_t> admin; pB(admin, 33, ch);         // set_channel
    mtAdminFrame(begin); mtAdminFrame(admin); mtAdminFrame(commit);
    mtStoreBlob(s_chBuf[type], s_chLen[type], s_chHave[type], ch.data(), ch.size());
    return true;
  }

  if (scope == 3) {                       // Owner (User)
    if (!s_ownerHave) { err = "owner not loaded"; return false; }
    if (!mtEncodeBlob(3, 0, s_ownerBuf, s_ownerLen, field, value, cluster)) { err = "unsupported setting"; return false; }
    std::vector<uint8_t> admin; pB(admin, 32, cluster);    // set_owner
    mtAdminFrame(begin); mtAdminFrame(admin); mtAdminFrame(commit);
    mtStoreBlob(s_ownerBuf, s_ownerLen, s_ownerHave, cluster.data(), cluster.size());
    return true;
  }

  err = "bad scope";
  return false;
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
      if (n >= 58) {                       // tx power, position, other params, radio params
        uint32_t freq = 0, bw = 0;
        int32_t lat = 0, lon = 0;
        memcpy(&lat, d + 36, 4);
        memcpy(&lon, d + 40, 4);
        memcpy(&freq, d + 48, 4);
        memcpy(&bw, d + 52, 4);
        s_mc.txPower   = (int8_t)d[2];
        s_mc.lat = lat; s_mc.lon = lon;  s_mc.havePos = true;
        s_mc.multiAcks = d[44]; s_mc.advertLoc = d[45];
        s_mc.telemBase = d[46] & 3; s_mc.telemLoc = (d[46] >> 2) & 3; s_mc.telemEnv = (d[46] >> 4) & 3;
        s_mc.manualAdd   = d[47];
        s_mc.freqHz = freq; s_mc.bwHz = bw; s_mc.sf = d[56]; s_mc.cr = d[57];
        s_mc.haveTx = s_mc.haveRadio = s_mc.haveOther = true;
      }
      size_t o = 58;
      if (o < n) { s_mc.name = rdStr(d + o, n - o); s_mc.haveName = !s_mc.name.isEmpty(); }
      if (s_devName.isEmpty()) s_devName = s_mc.name.length() ? s_mc.name : String("MeshCore node");
      s_mc.ready = true;
      Serial.printf("[BRIDGE] self name='%s'\n", s_devName.c_str());
      break;
    }
    case PKT_TUNING_PARAMS: {
      if (n >= 9) {
        uint32_t rx = 0, af = 0;
        memcpy(&rx, d + 1, 4); memcpy(&af, d + 5, 4);
        s_mc.rxDelay = (float)rx / 1000.0f;
        s_mc.airtime = (float)af / 1000.0f;
        s_mc.haveTuning = true;
      }
      break;
    }
    case PKT_CUSTOM_VARS: {
      s_mc.varCount = 0;
      String all = rdStr(d + 1, n > 1 ? n - 1 : 0);
      int start = 0;
      while (start < (int)all.length() && s_mc.varCount < 16) {
        int comma = all.indexOf(',', start);
        String pair = comma < 0 ? all.substring(start) : all.substring(start, comma);
        int colon = pair.indexOf(':');
        if (colon > 0) {
          s_mc.varName[s_mc.varCount]  = pair.substring(0, colon);
          s_mc.varValue[s_mc.varCount] = pair.substring(colon + 1);
          s_mc.varCount++;
        }
        if (comma < 0) break;
        start = comma + 1;
      }
      break;
    }
    case PKT_AUTOADD_CONFIG: {
      if (n >= 3) { s_mc.autoCfg = d[1]; s_mc.autoHops = d[2]; s_mc.haveAuto = true; }
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
  int idx = 0; uint32_t f, w; String name;
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
  mtStoreChannelMsg(d, n);
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

  if (haveData && portnum == 6 && payload) mtParseAdmin(payload, payLen);
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
    else if (f == 5 && w == 2) { const uint8_t* b; size_t l; if (r.bytes(b, l)) mtStoreConfigMsg(b, l); }
    else if (f == 9 && w == 2) { const uint8_t* b; size_t l; if (r.bytes(b, l)) mtStoreModuleMsg(b, l); }
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
  s_status = s_paused ? "disconnected" : (s_target.length() ? "connecting" : "scanning");
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
    s_mc = McCfg();
    s_mcReqTuning = s_mcReqVars = s_mcReqAuto = false;
    Serial.printf("[BRIDGE] paired + connected to '%s'\n", s_peer.c_str());
    return true;
  }

  Serial.println("[BRIDGE] no known mesh service on device");
  s_client->disconnect();
  s_connecting = false;
  return false;
}

// ---------------------------------------------------------------------------
// MeshCore settings
// ---------------------------------------------------------------------------
static String hexStr(const uint8_t* d, int n) {
  static const char* H = "0123456789abcdef";
  String s;
  for (int i = 0; i < n; i++) { s += H[(d[i] >> 4) & 0xF]; s += H[d[i] & 0xF]; }
  return s;
}
static int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
static bool hexBytes(const String& s, uint8_t* out, int n) {
  if ((int)s.length() < n * 2) return false;
  for (int i = 0; i < n; i++) {
    int hi = hexNibble(s[i * 2]), lo = hexNibble(s[i * 2 + 1]);
    if (hi < 0 || lo < 0) return false;
    out[i] = (uint8_t)((hi << 4) | lo);
  }
  return true;
}
static void putU32(std::vector<uint8_t>& f, uint32_t v) {
  f.push_back(v & 0xFF); f.push_back((v >> 8) & 0xFF);
  f.push_back((v >> 16) & 0xFF); f.push_back((v >> 24) & 0xFF);
}

String meshBridgeMeshCoreSettingsJson() {
  String o; o.reserve(2048);
  o += "{\"protocol\":\"meshcore\",\"ready\":";
  o += s_mc.ready ? "true" : "false";
  o += ",\"device\":{";
  bool first = true;
  auto key = [&](const char* k) { if (!first) o += ','; first = false; o += '"'; o += k; o += "\":"; };
  if (s_mc.haveName)  { key("name"); o += '"'; mtJsonEsc(o, s_mc.name); o += '"'; }
  if (s_mc.haveTx)    { key("tx_power"); o += String(s_mc.txPower); }
  if (s_mc.haveRadio) {
    key("freq_khz"); o += String(s_mc.freqHz / 1000);
    key("bw_hz"); o += String(s_mc.bwHz);
    key("spreading_factor"); o += String(s_mc.sf);
    key("coding_rate"); o += String(s_mc.cr);
  }
  if (s_mc.havePos) {
    key("latitude");  o += String(s_mc.lat / 1000000.0, 6);
    key("longitude"); o += String(s_mc.lon / 1000000.0, 6);
  }
  if (s_mc.haveOther) {
    key("multi_acks"); o += String(s_mc.multiAcks ? 1 : 0);
    key("advert_location_policy"); o += String(s_mc.advertLoc);
    key("telemetry_base"); o += String(s_mc.telemBase);
    key("telemetry_location"); o += String(s_mc.telemLoc);
    key("telemetry_environment"); o += String(s_mc.telemEnv);
    key("manual_add_contacts"); o += String(s_mc.manualAdd ? 1 : 0);
  }
  if (s_mc.haveTuning) {
    key("rx_delay_base"); o += String(s_mc.rxDelay, 3);
    key("airtime_factor"); o += String(s_mc.airtime, 3);
  }
  if (s_mc.haveAuto) { key("autoadd_config"); o += String(s_mc.autoCfg); key("autoadd_max_hops"); o += String(s_mc.autoHops); }

  o += "},\"channel\":{";
  bool g = false;
  for (int i = 0; i < 8; i++) {
    if (!s_mc.chHave[i]) continue;
    if (g) o += ','; g = true;
    o += '"'; o += String(i); o += "\":{\"index\":"; o += String(i);
    o += ",\"name\":\""; mtJsonEsc(o, s_mc.chName[i]); o += '"';
    if (s_mc.chSecretHave[i]) { o += ",\"secret\":\""; o += hexStr(s_mc.chSecret[i], 16); o += '"'; }
    o += '}';
  }

  o += "},\"vars\":[";
  for (int i = 0; i < s_mc.varCount; i++) {
    if (i) o += ',';
    o += "{\"n\":\""; mtJsonEsc(o, s_mc.varName[i]);
    o += "\",\"v\":\""; mtJsonEsc(o, s_mc.varValue[i]); o += "\"}";
  }
  o += "]}";
  return o;
}

bool meshBridgeMeshCoreApply(uint8_t scope, uint8_t type, uint8_t field, const String& value, String& err) {
  if (!s_connected || s_proto != "meshcore") { err = "not connected to a MeshCore node"; return false; }
  std::vector<uint8_t> f;

  if (scope == 100) {                 // device / radio / params
    switch (field) {
      case 0: {                       // advert name
        f.push_back(8);
        for (size_t i = 0; i < value.length() && i < 31; i++) f.push_back((uint8_t)value[i]);
        s_mc.name = value; s_mc.haveName = true;
        break;
      }
      case 1: {                       // tx power (dBm)
        f.push_back(12); f.push_back((uint8_t)(int8_t)value.toInt());
        s_mc.txPower = value.toInt(); s_mc.haveTx = true;
        break;
      }
      case 2: case 3: case 4: case 5: { // radio params (whole set)
        uint32_t freq = (field == 2) ? (uint32_t)value.toInt() * 1000 : s_mc.freqHz;
        uint32_t bw   = (field == 3) ? (uint32_t)value.toInt() : s_mc.bwHz;
        uint8_t  sf   = (field == 4) ? (uint8_t)value.toInt() : s_mc.sf;
        uint8_t  cr   = (field == 5) ? (uint8_t)value.toInt() : s_mc.cr;
        f.push_back(11); putU32(f, freq); putU32(f, bw); f.push_back(sf); f.push_back(cr); f.push_back(0);
        s_mc.freqHz = freq; s_mc.bwHz = bw; s_mc.sf = sf; s_mc.cr = cr; s_mc.haveRadio = true;
        break;
      }
      case 6: case 7: {               // lat / lon
        int32_t lat = (field == 6) ? (int32_t)(value.toFloat() * 1000000.0f) : s_mc.lat;
        int32_t lon = (field == 7) ? (int32_t)(value.toFloat() * 1000000.0f) : s_mc.lon;
        f.push_back(14); putU32(f, (uint32_t)lat); putU32(f, (uint32_t)lon); putU32(f, 0);
        s_mc.lat = lat; s_mc.lon = lon; s_mc.havePos = true;
        break;
      }
      case 8: case 9: case 10: case 11: case 12: case 13: {  // other params (whole set)
        if (field == 8)  s_mc.multiAcks = value.toInt() ? 1 : 0;
        if (field == 9)  s_mc.advertLoc  = (uint8_t)value.toInt();
        if (field == 10) s_mc.telemBase  = (uint8_t)value.toInt();
        if (field == 11) s_mc.telemLoc   = (uint8_t)value.toInt();
        if (field == 12) s_mc.telemEnv   = (uint8_t)value.toInt();
        if (field == 13) s_mc.manualAdd  = value.toInt() ? 1 : 0;
        f.push_back(38);
        f.push_back(s_mc.manualAdd);
        f.push_back((uint8_t)((s_mc.telemEnv << 4) | (s_mc.telemLoc << 2) | s_mc.telemBase));
        f.push_back(s_mc.advertLoc);
        f.push_back(s_mc.multiAcks);
        s_mc.haveOther = true;
        break;
      }
      case 14: case 15: {             // tuning params
        float rx = (field == 14) ? value.toFloat() : s_mc.rxDelay;
        float af = (field == 15) ? value.toFloat() : s_mc.airtime;
        f.push_back(21); putU32(f, (uint32_t)(rx * 1000.0f)); putU32(f, (uint32_t)(af * 1000.0f));
        s_mc.rxDelay = rx; s_mc.airtime = af; s_mc.haveTuning = true;
        break;
      }
      case 16: case 17: {             // auto-add config
        uint8_t cfg  = (field == 16) ? (uint8_t)value.toInt() : s_mc.autoCfg;
        uint8_t hops = (field == 17) ? (uint8_t)value.toInt() : s_mc.autoHops;
        f.push_back(58); f.push_back(cfg); f.push_back(hops);
        s_mc.autoCfg = cfg; s_mc.autoHops = hops; s_mc.haveAuto = true;
        break;
      }
      case 18: {                      // path hash mode (0..2)
        f.push_back(61); f.push_back(0); f.push_back((uint8_t)value.toInt());
        break;
      }
      case 19: {                      // BLE PIN (0 = random)
        f.push_back(37); putU32(f, (uint32_t)value.toInt());
        break;
      }
      default: err = "unsupported setting"; return false;
    }
    return writeFrame(f.data(), f.size());
  }

  if (scope == 102) {                 // channel(name + secret)
    if (type > 7) { err = "bad channel"; return false; }
    uint8_t name[32]; memset(name, 0, sizeof(name));
    String nm = (field == 0) ? value : s_mc.chName[type];
    for (size_t i = 0; i < nm.length() && i < 31; i++) name[i] = (uint8_t)nm[i];
    uint8_t secret[16];
    if (field == 1) { if (!hexBytes(value, secret, 16)) { err = "secret must be 32 hex chars"; return false; } }
    else memcpy(secret, s_mc.chSecret[type], 16);
    f.push_back(32); f.push_back((uint8_t)type);
    f.insert(f.end(), name, name + 32);
    f.insert(f.end(), secret, secret + 16);
    s_mc.chName[type] = nm; s_mc.chHave[type] = true;
    memcpy(s_mc.chSecret[type], secret, 16); s_mc.chSecretHave[type] = true;
    return writeFrame(f.data(), f.size());
  }

  if (scope == 104) {                 // custom var: type = index, value = new value
    if (type >= s_mc.varCount) { err = "unknown variable"; return false; }
    String pair = s_mc.varName[type] + ":" + value;
    f.push_back(41);
    for (size_t i = 0; i < pair.length(); i++) f.push_back((uint8_t)pair[i]);
    s_mc.varValue[type] = value;
    return writeFrame(f.data(), f.size());
  }

  err = "bad scope";
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
  NimBLEDevice::setMTU(517);
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

static void scanForNodes() {
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

    // user asked to disconnect: stay offline, only scan on demand for the picker
    if (s_paused) {
      s_status = "disconnected";
      if (s_scanRequest) scanForNodes();
      vTaskDelay(pdMS_TO_TICKS(1200));
      return;
    }

    s_status = s_target.length() ? "connecting" : "scanning";
    scanForNodes();

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
      s_lastPeer = *pick;
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
    if (!s_mtConfigSent) { mtSendWantConfig(); s_mtConfigSent = true; s_reqStage = 0; }
    if (s_mtRead || now - s_mtLastRead > 1500) {
      mtReadFromRadio();
      s_mtLastRead = now;
      s_mtRead = false;
    }
    // configs, module configs and channels arrive with want_config;
    // only the owner has to be fetched via AdminMessage (retry until we have it)
    if (s_mtDone && !s_ownerHave && now - s_lastOwnerReq > 2500) {
      meshBridgeRequestOwner();
      s_lastOwnerReq = now;
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
    if (!s_mcReqTuning) { uint8_t f[1] = {43}; writeFrame(f, 1); s_mcReqTuning = true; }
    if (!s_mcReqVars)   { uint8_t f[1] = {40}; writeFrame(f, 1); s_mcReqVars = true; }
    if (!s_mcReqAuto)   { uint8_t f[1] = {59}; writeFrame(f, 1); s_mcReqAuto = true; }
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
  s_target   = address;
  s_paused   = false;                 // picking a node resumes auto-connect
  if (s_target.length()) s_prefs.putString("target", s_target);
  else                   s_prefs.remove("target");
  s_needPin = false;
  if (s_client && s_client->isConnected() && !s_peerAddr.equalsIgnoreCase(address))
    s_client->disconnect();
  s_scanRequest = true;
}

void meshBridgeClearSelection() {
  s_target   = "";
  s_paused   = false;
  s_needPin  = false;
  s_lastPeer = MeshNodeInfo();
  s_prefs.remove("target");
  s_prefs.remove("pin");
  if (s_client && s_client->isConnected()) s_client->disconnect();
  s_scanRequest = true;
}

void meshBridgeRequestScan() {
  if (!s_connected) s_scanRequest = true;
}

bool meshBridgeLocked() {
  return !s_paused && (s_connected || s_connecting || s_needPin);
}

bool meshBridgePeerInfo(MeshNodeInfo& out) {
  if (s_lastPeer.address.length()) { out = s_lastPeer; return true; }
  if (s_peerAddr.length() || s_peer.length()) {
    out.address  = s_peerAddr;
    out.name     = s_peer.length() ? s_peer : s_peerAddr;
    out.proto    = s_proto;
    out.meshcore = (s_proto == "meshcore");
    return true;
  }
  return false;
}

void meshBridgeDisconnect() {
  s_paused = true;                    // stay offline until the user picks again
  s_needPin = false;
  if (s_client) s_client->disconnect();
  s_connected  = false;
  s_connecting = false;
  s_status     = "disconnected";
  s_scanRequest = true;               // refresh the picker list
  Serial.println("[BRIDGE] disconnected by user");
}

void meshBridgeReconnect() {
  s_paused = false;
  s_needPin = false;
  s_status = s_target.length() ? "connecting" : "scanning";
  s_scanRequest = true;
  Serial.println("[BRIDGE] reconnect requested");
}
