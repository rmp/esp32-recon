#include "globals.h"

TFT_eSPI tft;

const char* pName[P_COUNT] = {"Beacon","Data","Probe","Control","Auth","Other"};
uint16_t pColor[P_COUNT] = {};
uint16_t chCol[14] = {};
const uint16_t BG = TFT_BLACK;
const uint16_t STAT_BG = 0x1082;

volatile uint32_t cnt[P_COUNT] = {};
volatile uint32_t totalPkt = 0;
uint32_t dCnt[P_COUNT] = {}, dTotal = 0;
uint32_t hist[HIST_LEN] = {}, pHist[P_COUNT][HIST_LEN] = {};
uint8_t  hIdx = 0;
uint32_t prevTotal = 0, prevCnt[P_COUNT] = {}, curPPS = 0;

uint8_t curCh = CH_MIN;
ChanStat cstat[14] = {};

APInfo aps[MAX_APS];
volatile int nAps = 0;

DevInfo devs[MAX_DEVS];
volatile int nDevs = 0;

DeauthEvt deauthBuf[MAX_DEAUTH];
int deauthHead = 0;
volatile uint32_t deauthTotal = 0;
uint32_t deauthHist[HIST_LEN] = {}, prevDeauth = 0;

volatile uint32_t totalRetry = 0;
uint32_t retryHistGlobal[HIST_LEN] = {}, prevRetryGlobal = 0;

int trackedAP = -1;
volatile int8_t trackedRSSI = -100;
int8_t rssiHist[HIST_LEN] = {};
uint8_t rssiIdx = 0;

int  curScreen = 0;
bool screenChanged = true;
int  scrollOfs = 0;

int prevBarW[P_COUNT] = {};
int prevChBarH[14] = {};
int prevChHBarW[14] = {};

unsigned long tHop = 0, tDisp = 0, tStat = 0, lastTouch = 0;

ProbeReq probes[MAX_PROBES];
int probeHead = 0;
volatile int probeCount = 0;

HandshakeState handshakes[MAX_HANDSHAKES];
volatile int nHandshakes = 0;

PMKIDEntry pmkids[MAX_PMKIDS];
volatile int nPMKIDs = 0;

BLEDevEntry bleDevs[MAX_BLE_DEVS];
volatile int nBLEDevs = 0;
bool bleActive = false;

bool sdAvailable = false;
bool pcapRecording = false;
uint32_t pcapPackets = 0;
uint32_t pcapBytes = 0;
