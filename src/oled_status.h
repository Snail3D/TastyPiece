#pragma once

#include <Arduino.h>

// Shared runtime state surfaced to the OLED and the web API.
struct TastyPieceState {
  String    apSsid;
  IPAddress apIp;
  IPAddress staIp;
  bool      staConnected = false;
  int32_t   staRssi      = 0;
  uint32_t  txCount      = 0;
  uint32_t  rxCount      = 0;
  uint8_t   clients      = 0;
  String    lastEvent;
  String    bridgeState  = "idle";   // idle | scanning | connected | error
  String    bridgePeer   = "";
};

bool   oledInit();
bool   oledAvailable();
String oledBusInfo();
void   oledRender(const TastyPieceState& st);
