#pragma once
#include <stdint.h>
#include "parser.h"
#include "config.h"

struct APInfo {
    uint8_t  bssid[6];
    char     ssid[33];
    int8_t   rssi;
    uint8_t  channel;
    uint8_t  encryption;   // 0=open 1=WEP 2=WPA 3=WPA2
    uint16_t beaconIntvl;
    uint16_t beaconCount;
    unsigned long lastSeen;
    bool     hidden;
    char     revealedSSID[33];
};

struct DevInfo {
    uint8_t  mac[6];
    int8_t   rssi;
    uint8_t  channel;
    uint32_t pktCount;
    unsigned long firstSeen;
    unsigned long lastSeen;
};

struct DeauthEvt {
    uint8_t  src[6];
    uint8_t  dst[6];
    uint8_t  channel;
    unsigned long ts;
};

struct ChanStat {
    volatile uint32_t total;
    uint32_t prevTotal;
    uint32_t hist[HIST_LEN];
    int32_t  rssiSum;
    uint32_t rssiN;
    volatile uint32_t retries;
    uint32_t prevRetries;
    uint32_t retryHist[HIST_LEN];
};

struct ProbeReq {
    uint8_t  mac[6];
    char     ssid[33];
    int8_t   rssi;
    uint8_t  channel;
    unsigned long timestamp;
    bool     randomMAC;
};

struct HandshakeState {
    uint8_t  bssid[6];
    uint8_t  client[6];
    char     ssid[33];
    uint8_t  captured;    // bitmask: bit 0=msg1, bit 1=msg2, etc.
    unsigned long lastUpdate;
};

struct PMKIDEntry {
    uint8_t  bssid[6];
    uint8_t  client[6];
    uint8_t  pmkid[16];
    char     ssid[33];
    unsigned long timestamp;
};

struct BLEDevEntry {
    uint8_t  mac[6];
    char     name[32];
    int8_t   rssi;
    uint8_t  addrType;
    unsigned long lastSeen;
    bool     isTracker;
};
