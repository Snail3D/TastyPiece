#pragma once

// ---------------------------------------------------------------------------
// TastyPiece build configuration
// ---------------------------------------------------------------------------

#define TP_FW_NAME    "TastyPiece"
#define TP_FW_VERSION "0.1.0"

// SoftAP that phones join. Keep it short and friendly.
#define TP_AP_SSID    "TastyPiece"
#define TP_AP_PASS    "tastypiece"   // >= 8 chars; set "" for open AP
#define TP_AP_CHANNEL 6
#define TP_AP_IP      IPAddress(192, 168, 4, 1)
#define TP_AP_GW      IPAddress(192, 168, 4, 1)
#define TP_AP_MASK    IPAddress(255, 255, 255, 0)

// Captive-portal DNS TTL
#define TP_DNS_TTL    60

// How often the OLED refreshes (ms)
#define TP_OLED_INTERVAL_MS 1000

// Number of lines kept in the on-device event log
#define TP_LOG_LINES 8

// STA (development) timeout when joining an upstream network
#define TP_STA_TIMEOUT_MS 15000

// Onboard status LED (ESP32-C3 SuperMini: GPIO8, active low)
#if defined(BOARD_ESP32C3_SUPERMINI)
  #define TP_STATUS_LED 8
#endif
