#pragma once
#include <stdint.h>

enum Proto : uint8_t { P_BEACON=0, P_DATA, P_PROBE, P_CTRL, P_AUTH, P_OTHER, P_COUNT };

struct FrameInfo {
    Proto   proto;
    uint8_t type;
    uint8_t subtype;
    bool    retry;
    bool    isDeauth;
    bool    isProbeReq;
    bool    isProbeResp;
};

struct BeaconData {
    char    ssid[33];
    uint8_t channel;
    uint8_t encryption;   // 0=open 1=WEP 2=WPA 3=WPA2
    uint16_t interval;
    bool    hasSSID;
};

struct ProbeData {
    char ssid[33];
    bool hasSSID;
    bool isRequest;
};

struct EAPOLData {
    uint8_t msgNum;       // 1-4 for 4-way handshake
    bool    hasPMKID;
    uint8_t pmkid[16];
};

bool classifyFrame(const uint8_t* frame, uint16_t len, FrameInfo& out);
bool parseBeaconTags(const uint8_t* frame, uint16_t len, BeaconData& out);
bool parseProbe(const uint8_t* frame, uint16_t len, ProbeData& out);
bool parseEAPOL(const uint8_t* frame, uint16_t len, EAPOLData& out);
bool isLocallyAdminMAC(const uint8_t* mac);

const uint8_t* frameAddr1(const uint8_t* frame);
const uint8_t* frameAddr2(const uint8_t* frame);
const uint8_t* frameAddr3(const uint8_t* frame);
