#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>

#include "config.h"
#include "oled_status.h"
#include "web_ui.h"
#include "mesh_bridge.h"

#if defined(TP_ENABLE_STA)
  #if __has_include("secrets.h")
    #include "secrets.h"
  #endif
#endif

WebServer      server(80);
DNSServer      dnsServer;
TastyPieceState st;

static uint32_t g_bootMs     = 0;
static String   g_log[TP_LOG_LINES];
static uint8_t  g_logHead    = 0;
static uint32_t g_lastOled   = 0;
static uint32_t g_lastSerial = 0;
static uint32_t g_lastLed    = 0;
static bool     g_ledState   = false;
static size_t   g_lastMsgCount = 0;

static void addEvent(const String& s) {
  g_log[g_logHead] = s;
  g_logHead = (g_logHead + 1) % TP_LOG_LINES;
  Serial.printf("[TP] %s\n", s.c_str());
}

static String jsonEscape(const String& in) {
  String o;
  o.reserve(in.length() + 4);
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if (c >= ' ') o += c;
  }
  return o;
}

static void sendIndex() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", TP_HTML);
}

static void handleStatus() {
  JsonDocument doc;
  doc["fw"]            = TP_FW_NAME;
  doc["version"]       = TP_FW_VERSION;
  doc["uptime_s"]      = (millis() - g_bootMs) / 1000;
  doc["ap_ssid"]       = TP_AP_SSID;
  doc["ap_ip"]         = WiFi.softAPIP().toString();
  doc["clients"]       = WiFi.softAPgetStationNum();
  doc["sta_connected"] = false;
  doc["sta_ip"]        = "";
  doc["sta_rssi"]      = 0;
  doc["tx"]            = st.txCount;
  doc["rx"]            = st.rxCount;
  doc["bridge"]        = st.bridgeState;
  doc["peer"]          = st.bridgePeer;
  doc["oled"]          = oledAvailable() ? oledBusInfo() : String("none");
  String out;
  serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", out);
}

static void handleMesh() {
  JsonDocument doc;
  doc["connected"]  = meshBridgeConnected();
  doc["status"]     = meshBridgeStatus();
  doc["peer"]       = meshBridgePeer();
  doc["name"]       = meshBridgeDeviceName();
  doc["model"]      = meshBridgeModel();
  doc["version"]    = meshBridgeVersion();
  doc["battery_mv"] = meshBridgeBatteryMv();

  JsonArray chans = doc["channels"].to<JsonArray>();
  uint8_t nc = meshBridgeChannelCount();
  for (uint8_t i = 0; i < nc && i < 8; i++) {
    JsonObject c = chans.add<JsonObject>();
    c["index"] = i;
    c["name"]  = meshBridgeChannelName(i);
  }

  JsonArray msgs = doc["messages"].to<JsonArray>();
  auto& v = meshBridgeMessages();
  size_t start = v.size() > 20 ? v.size() - 20 : 0;
  for (size_t i = start; i < v.size(); i++) {
    JsonObject m = msgs.add<JsonObject>();
    m["kind"]     = v[i].kind;
    m["channel"]  = v[i].channel;
    m["from"]     = v[i].from;
    m["text"]     = v[i].text;
    m["ts"]       = v[i].timestamp;
    m["snr"]      = v[i].snr;
    m["out"]      = v[i].outgoing;
  }

  String out;
  serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", out);
}

static void handleLog() {
  JsonDocument doc;
  JsonArray arr = doc["log"].to<JsonArray>();
  for (uint8_t i = 0; i < TP_LOG_LINES; i++) {
    uint8_t idx = (g_logHead + i) % TP_LOG_LINES;
    if (g_log[idx].length()) arr.add(jsonEscape(g_log[idx]));
  }
  String out;
  serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", out);
}

static void handleSend() {
  String body = server.arg("plain");
  JsonDocument doc;
  DeserializationError e = deserializeJson(doc, body);
  if (e) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad json\"}");
    return;
  }
  uint8_t ch = doc["channel"] | 0;
  String text = doc["text"].as<String>();
  String err;
  bool ok = meshBridgeSendChannel(ch, text, err);
  JsonDocument out;
  out["ok"] = ok;
  if (!ok) out["error"] = err;
  String s;
  serializeJson(out, s);
  server.send(ok ? 200 : 503, "application/json", s);
}

static void handlePair() {
  String body = server.arg("plain");
  JsonDocument doc;
  if (deserializeJson(doc, body)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad json\"}");
    return;
  }
  String pinStr = doc["pin"].as<String>();
  uint32_t pin = (uint32_t)strtoul(pinStr.c_str(), nullptr, 10);
  if (pin == 0) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad pin\"}");
    return;
  }
  meshBridgeSetPin(pin);
  addEvent(String("BLE pin ") + pin);
  server.send(200, "application/json", "{\"ok\":true}");
}

static void handleSerialCmd(const String& line) {
  String cmd = line;
  cmd.trim();
  int sp = cmd.indexOf(' ');
  String verb = sp < 0 ? cmd : cmd.substring(0, sp);
  String rest = sp < 0 ? "" : cmd.substring(sp + 1);
  verb.toLowerCase();

  if (verb == "pin") {
    uint32_t pin = (uint32_t)strtoul(rest.c_str(), nullptr, 10);
    if (pin) { meshBridgeSetPin(pin); addEvent(String("BLE pin ") + pin); }
  } else if (verb == "send") {
    int sp2 = rest.indexOf(' ');
    int ch = sp2 < 0 ? 0 : rest.substring(0, sp2).toInt();
    String text = sp2 < 0 ? rest : rest.substring(sp2 + 1);
    String err;
    bool ok = meshBridgeSendChannel((uint8_t)ch, text, err);
    Serial.printf("[TP] send=%d %s\n", (int)ok, ok ? "" : err.c_str());
  } else if (verb == "status") {
    Serial.printf("[TP] bridge=%s peer=%s name=%s model=%s ver=%s batt=%umV chans=%u msgs=%u\n",
                  meshBridgeStatus().c_str(), meshBridgePeer().c_str(),
                  meshBridgeDeviceName().c_str(), meshBridgeModel().c_str(),
                  meshBridgeVersion().c_str(), meshBridgeBatteryMv(),
                  meshBridgeChannelCount(), (unsigned)meshBridgeMessages().size());
    for (uint8_t i = 0; i < meshBridgeChannelCount() && i < 8; i++)
      Serial.printf("[TP]   ch%u = %s\n", i, meshBridgeChannelName(i).c_str());
    for (auto& m : meshBridgeMessages())
      Serial.printf("[TP]   %s%s ch%u %s: %s\n", m.outgoing ? "TX" : "RX",
                    m.kind == "direct" ? "(dm)" : "", m.channel, m.from.c_str(), m.text.c_str());
  } else if (verb == "selftest") {
    WiFiClient c;
    if (c.connect(WiFi.softAPIP(), 80)) {
      c.printf("GET /api/mesh HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
               WiFi.softAPIP().toString().c_str());
      uint32_t t = millis();
      while (c.connected() && millis() - t < 4000 && !c.available()) {
        server.handleClient();
        dnsServer.processNextRequest();
        delay(5);
      }
      String status = c.readStringUntil('\n');
      String body;
      while (c.available()) body += (char)c.read();
      c.stop();
      Serial.printf("[TP] selftest: %s body=%u bytes\n", status.c_str(), (unsigned)body.length());
      Serial.println("[TP] selftest body: " + body.substring(0, 220));
    } else {
      Serial.println("[TP] selftest: connect to AP failed");
    }
  } else {
    Serial.println("[TP] cmds: status | selftest | pin <nnnnnn> | send <channel> <text>");
  }
}

static void handleTime() {
  String body = server.arg("plain");
  JsonDocument doc;
  if (deserializeJson(doc, body)) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }
  uint32_t epoch = doc["epoch"] | 0;
  if (epoch > 1000000000) meshBridgeSetEpoch(epoch);
  server.send(200, "application/json", "{\"ok\":true}");
}

static void handlePortal() {
  String loc = String("http://") + WiFi.softAPIP().toString() + "/";
  server.sendHeader("Location", loc, true);
  server.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);
  delay(400);
  g_bootMs = millis();

  Serial.printf("\n=== %s v%s ===\n", TP_FW_NAME, TP_FW_VERSION);

#if defined(TP_STATUS_LED)
  pinMode(TP_STATUS_LED, OUTPUT);
  digitalWrite(TP_STATUS_LED, HIGH);
#endif

  bool oled = oledInit();
  Serial.printf("OLED: %s (%s)\n", oled ? "ok" : "none", oledBusInfo().c_str());
  if (oled) oledRender(st);

  WiFi.persistent(false);
  WiFi.setHostname(TP_FW_NAME);
  WiFi.mode(WIFI_AP);

  WiFi.softAPConfig(TP_AP_IP, TP_AP_GW, TP_AP_MASK);
  bool ap = WiFi.softAP(TP_AP_SSID, TP_AP_PASS, TP_AP_CHANNEL);
  st.apSsid = TP_AP_SSID;
  st.apIp   = WiFi.softAPIP();
  Serial.printf("SoftAP '%s' %s -> %s\n", TP_AP_SSID, ap ? "up" : "FAIL",
                WiFi.softAPIP().toString().c_str());
  addEvent(String("AP ") + TP_AP_SSID + " " + WiFi.softAPIP().toString());

  dnsServer.start(53, "*", WiFi.softAPIP());

  meshBridgeBegin();

  server.on("/", HTTP_GET, []() { sendIndex(); });
  server.on("/api/status", HTTP_GET, []() { handleStatus(); });
  server.on("/api/mesh", HTTP_GET, []() { handleMesh(); });
  server.on("/api/log", HTTP_GET, []() { handleLog(); });
  server.on("/api/send", HTTP_POST, []() { handleSend(); });
  server.on("/api/pair", HTTP_POST, []() { handlePair(); });
  server.on("/api/time", HTTP_POST, []() { handleTime(); });

  static const char* kProbePaths[] = {
    "/generate_204", "/gen_204", "/hotspot-detect.html",
    "/library/test/success.html", "/connecttest.txt", "/ncsi.txt",
    "/redirect", "/success.txt", "/canonical.html"
  };
  for (const char* p : kProbePaths) server.on(p, HTTP_GET, []() { handlePortal(); });

  server.onNotFound([]() { handlePortal(); });
  server.begin();
  addEvent("portal ready");
}

void loop() {
  // tiny serial console
  static String serialBuf;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuf.length()) { handleSerialCmd(serialBuf); serialBuf = ""; }
    } else if (serialBuf.length() < 200) {
      serialBuf += c;
    }
  }

  dnsServer.processNextRequest();
  server.handleClient();
  meshBridgeLoop();

  st.clients     = WiFi.softAPgetStationNum();
  st.bridgeState = meshBridgeStatus();
  st.bridgePeer  = meshBridgePeer();

  auto& v = meshBridgeMessages();
  uint32_t tx = 0, rx = 0;
  for (auto& m : v) { if (m.outgoing) tx++; else rx++; }
  st.txCount = tx;
  st.rxCount = rx;

  if (v.size() != g_lastMsgCount) {
    if (!v.empty()) {
      MeshMessage& m = v.back();
      st.lastEvent = String(m.outgoing ? "TX " : "RX ") + m.from + ": " + m.text;
    }
    g_lastMsgCount = v.size();
  }

  uint32_t now = millis();

  if (now - g_lastOled > TP_OLED_INTERVAL_MS) {
    g_lastOled = now;
    oledRender(st);
  }

  if (now - g_lastSerial > 10000) {
    g_lastSerial = now;
    Serial.printf("[TP] up=%lus ap=%s clients=%u bridge=%s peer=%s tx=%u rx=%u heap=%u\n",
                  now / 1000, st.apIp.toString().c_str(), st.clients,
                  st.bridgeState.c_str(), st.bridgePeer.c_str(),
                  st.txCount, st.rxCount, (unsigned)ESP.getFreeHeap());
  }

#if defined(TP_STATUS_LED)
  if (now - g_lastLed > 1000) {
    g_lastLed = now;
    g_ledState = !g_ledState;
    digitalWrite(TP_STATUS_LED, g_ledState ? LOW : HIGH);
  }
#endif

  delay(2);
}
