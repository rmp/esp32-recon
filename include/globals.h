#pragma once
#include "types.h"
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

// Protocol names + colours
extern const char* pName[P_COUNT];
extern uint16_t pColor[P_COUNT];
extern uint16_t chCol[14];
extern const uint16_t BG;
extern const uint16_t STAT_BG;

// Protocol stats
extern volatile uint32_t cnt[P_COUNT];
extern volatile uint32_t totalPkt;
extern uint32_t dCnt[P_COUNT], dTotal;
extern uint32_t hist[HIST_LEN], pHist[P_COUNT][HIST_LEN];
extern uint8_t  hIdx;
extern uint32_t prevTotal, prevCnt[P_COUNT], curPPS;

// Channel state
extern uint8_t curCh;
extern ChanStat cstat[14];

// AP list
extern APInfo aps[MAX_APS];
extern volatile int nAps;

// Device list
extern DevInfo devs[MAX_DEVS];
extern volatile int nDevs;

// Deauth ring buffer
extern DeauthEvt deauthBuf[MAX_DEAUTH];
extern int deauthHead;
extern volatile uint32_t deauthTotal;
extern uint32_t deauthHist[HIST_LEN], prevDeauth;

// Retry stats
extern volatile uint32_t totalRetry;
extern uint32_t retryHistGlobal[HIST_LEN], prevRetryGlobal;

// Signal tracker
extern int trackedAP;
extern volatile int8_t trackedRSSI;
extern int8_t rssiHist[HIST_LEN];
extern uint8_t rssiIdx;

// Screen state
extern int  curScreen;
extern bool screenChanged;
extern int  scrollOfs;

// Differential rendering state
extern int prevBarW[P_COUNT];
extern int prevChBarH[14];
extern int prevChHBarW[14];

// Timing
extern unsigned long tHop, tDisp, tStat, lastTouch;

// Probe request log
extern ProbeReq probes[MAX_PROBES];
extern int probeHead;
extern volatile int probeCount;

// Handshake capture
extern HandshakeState handshakes[MAX_HANDSHAKES];
extern volatile int nHandshakes;

// PMKID capture
extern PMKIDEntry pmkids[MAX_PMKIDS];
extern volatile int nPMKIDs;

// BLE devices
extern BLEDevEntry bleDevs[MAX_BLE_DEVS];
extern volatile int nBLEDevs;
extern bool bleActive;

// SD card
extern bool sdAvailable;
extern bool pcapRecording;
extern uint32_t pcapPackets;
extern uint32_t pcapBytes;
