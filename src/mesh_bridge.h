#pragma once

#include <Arduino.h>
#include <vector>

// A single decoded mesh message (channel or direct).
struct MeshMessage {
  String   kind;        // "channel" | "direct"
  uint8_t  channel = 0;
  String   from;        // channel name / pubkey prefix
  String   text;
  uint32_t timestamp = 0;
  float    snr = 0.0f;
  bool     outgoing = false;
};

// --- lifecycle -------------------------------------------------------------
void   meshBridgeBegin();
void   meshBridgeLoop();
bool   meshBridgeConnected();

// --- state -----------------------------------------------------------------
String   meshBridgeStatus();     // idle|scanning|connecting|connected|error
String   meshBridgePeer();       // device name being bridged
String   meshBridgeDeviceName();
String   meshBridgeModel();
String   meshBridgeVersion();
uint16_t meshBridgeBatteryMv();
uint8_t  meshBridgeChannelCount();
String   meshBridgeChannelName(uint8_t idx);
std::vector<MeshMessage>& meshBridgeMessages();

// --- actions ---------------------------------------------------------------
bool meshBridgeSendChannel(uint8_t ch, const String& text, String& err);
void meshBridgeSetEpoch(uint32_t epochSeconds);
void meshBridgeSetPin(uint32_t pin);
