#include "oled_status.h"
#include "config.h"

#include <Wire.h>
#include <U8g2lib.h>

// ---------------------------------------------------------------------------
// Panel selection
// ---------------------------------------------------------------------------
#if defined(TP_OLED_SSD1306_72X40)
  static U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
#elif defined(TP_OLED_SSD1306)
  static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
#else
  #error "No OLED panel defined for this target"
#endif

static bool   s_oledOk = false;
static String s_busInfo = "none";

// Candidate I2C pin pairs to probe (board-dependent defaults first).
static const int8_t kPins[][2] = {
#if defined(BOARD_HELTEC_V3)
  {17, 18}, {18, 17},
#endif
  {5, 6}, {6, 5},          // ESP32-C3 SuperMini OLED
  {8, 9}, {9, 8},
  {0, 1}, {1, 0},
  {4, 5}, {5, 4},
  {2, 3}, {3, 2},
  {6, 7}, {7, 6},
  {20, 21}, {21, 20},
};

static bool i2cProbe(int sda, int scl) {
  Wire.end();
  Wire.begin(sda, scl);
  Wire.beginTransmission(0x3C);
  return Wire.endTransmission() == 0;
}

bool oledInit() {
#if defined(BOARD_ESP32DEV)
  // Classic ESP32 dev boards have no I2C OLED. Probing arbitrary pins is
  // unsafe here: GPIO6-11 are the SPI-flash pins, so an I2C probe on them
  // wedges flash access and watchdog-resets the chip. Skip probing.
  s_oledOk  = false;
  s_busInfo = "none (esp32dev)";
  return false;
#else
  for (auto& p : kPins) {
    if (i2cProbe(p[0], p[1])) {
      // Leave Wire configured on the discovered pins.
      Wire.setClock(400000);
      u8g2.setI2CAddress(0x3C << 1);
      u8g2.begin();
      s_oledOk  = true;
      s_busInfo = String("SDA") + p[0] + " SCL" + p[1];
      return true;
    }
  }
  s_oledOk  = false;
  s_busInfo = "not found";
  return false;
#endif
}

bool   oledAvailable() { return s_oledOk; }
String oledBusInfo()   { return s_busInfo; }

static void drawLine(uint8_t y, const String& text) {
  u8g2.drawStr(0, y, text.c_str());
}

void oledRender(const TastyPieceState& st) {
  if (!s_oledOk) return;

  u8g2.clearBuffer();
#if defined(TP_OLED_SSD1306_72X40)
  u8g2.setFont(u8g2_font_4x6_tf);
#else
  u8g2.setFont(u8g2_font_5x7_tf);
#endif

  // Row 1 — identity
  drawLine(7, "TastyPiece v" TP_FW_VERSION);

  // Row 2 — AP endpoint
  drawLine(14, "AP " + st.apIp.toString());

  // Row 3 — upstream / bridge status
  String mid = st.staConnected ? (String("W ") + st.staIp.toString()) : String("W down");
  drawLine(21, mid);

  // Row 4 — counters
  drawLine(28, "TX " + String(st.txCount) + " RX " + String(st.rxCount) +
               " C" + String(st.clients));

  // Row 5 — live status ticker
  drawLine(35, st.lastEvent.substring(0, 22));

  u8g2.sendBuffer();
}
