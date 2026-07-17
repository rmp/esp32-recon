#include <Arduino.h>
#include "config.h"
#include "globals.h"
#include "sniffer.h"
#include "display.h"
#include "screen.h"
#include "sd_utils.h"
#include "ble_scanner.h"

Screen screens[NUM_SCREENS] = {
    {"Protocol",   drawProtocol},
    {"Channels",   drawChannels},
    {"AP Scan",    drawAPScan},
    {"Signal",     drawSignal},
    {"Devices",    drawDevices},
    {"Traffic/Ch", drawTraffic},
    {"Deauth",     drawDeauth},
    {"Beacons",    drawBeacons},
    {"Retry/Err",  drawRetry},
    {"Probes",     drawProbeLog},
    {"Hidden AP",  drawHiddenSSID},
    {"Handshake",  drawHandshake},
    {"PMKID",      drawPMKID},
    {"MAC Rand",   drawMACRandom},
    {"OUI Lookup", drawOUIScreen},
    {"PCAP",       drawPCAPScreen},
    {"BLE Scan",   drawBLEScan},
};

static int prevScreen = -1;

static void switchScreen(int delta) {
    curScreen = (curScreen + NUM_SCREENS + delta) % NUM_SCREENS;
    screenChanged = true;
}

static void handleTouch() {
    uint16_t tx, ty;
    if (!tft.getTouch(&tx, &ty, 300)) return;
    if (millis() - lastTouch < 350) return;
    lastTouch = millis();

    if (tx < 50) { switchScreen(-1); return; }
    if (tx > SW - 50) { switchScreen(+1); return; }

    // Screen-specific tap handling
    if (curScreen == 3 && trackedAP >= 0) {
        trackedAP = (trackedAP + 1) % max(1, (int)nAps);
        trackedRSSI = aps[trackedAP].rssi;
        screenChanged = true;
    }
    if (curScreen == 2 || curScreen == 4 || curScreen == 14) {
        if (ty < CONTENT_Y + CONTENT_H / 2) scrollOfs = max(0, scrollOfs - 8);
        else scrollOfs += 8;
    }
    // PCAP toggle
    if (curScreen == 15 && sdAvailable) {
        if (pcapRecording) pcapStop();
        else pcapStart();
        screenChanged = true;
    }
}

static void handleSerial() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == 'n' || c == ']') switchScreen(+1);
        else if (c == 'p' || c == '[') switchScreen(-1);
        else if (c >= '1' && c <= '9') { curScreen = (c - '1') % NUM_SCREENS; screenChanged = true; }
        else if (c == '0') { curScreen = 9; screenChanged = true; }
    }
}

void setup() {
    Serial.begin(115200); delay(50);
    Serial.println("\n=== WiFi Network Diagnostics ===");
    Serial.printf("%d screens | navigate: n/p ]/[ 1-9\n", NUM_SCREENS);

    initDisplay();

    // Splash
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(4); tft.setTextColor(TFT_CYAN);
    tft.drawString("WiFi Diagnostics", SW / 2, SH / 2 - 24);
    tft.setTextFont(2); tft.setTextColor(TFT_WHITE);
    char splash[32]; snprintf(splash, sizeof(splash), "%d tools  |  CH 1-13", NUM_SCREENS);
    tft.drawString(splash, SW / 2, SH / 2 + 12);
    tft.setTextFont(1); tft.setTextColor(0x7BEF);
    tft.drawString("Starting sniffer...", SW / 2, SH / 2 + 36);

    sdInit();
    initSniffer();
    Serial.println("Sniffer active");

    memset(rssiHist, -100, sizeof(rssiHist));

    delay(800);
    tft.fillScreen(BG);
    screenChanged = true;
    tHop = tDisp = tStat = millis();
}

void loop() {
    unsigned long now = millis();

    if (now - tHop >= HOP_MS) { hopChannel(); tHop = now; }

    // Per-second stats
    if (now - tStat >= STAT_MS) {
        uint32_t t = totalPkt;
        curPPS = t - prevTotal;
        hist[hIdx] = curPPS;
        for (int i = 0; i < P_COUNT; i++) {
            uint32_t c = cnt[i]; pHist[i][hIdx] = c - prevCnt[i]; prevCnt[i] = c;
        }
        prevTotal = t;
        for (int c = 1; c <= 13; c++) {
            uint32_t ct = cstat[c].total;
            cstat[c].hist[hIdx] = ct - cstat[c].prevTotal; cstat[c].prevTotal = ct;
            uint32_t rt = cstat[c].retries;
            cstat[c].retryHist[hIdx] = rt - cstat[c].prevRetries; cstat[c].prevRetries = rt;
        }
        deauthHist[hIdx] = deauthTotal - prevDeauth; prevDeauth = deauthTotal;
        uint32_t cr = totalRetry;
        retryHistGlobal[hIdx] = cr - prevRetryGlobal; prevRetryGlobal = cr;
        rssiHist[rssiIdx] = trackedRSSI;
        rssiIdx = (rssiIdx + 1) % HIST_LEN;
        hIdx = (hIdx + 1) % HIST_LEN;
        tStat = now;
    }

    handleTouch();
    handleSerial();

    // Pause/resume sniffer when entering/leaving BLE screen
    if (curScreen != prevScreen) {
        if (prevScreen == 16 && bleActive) {
            bleDeinit();
            resumeSniffer();
        }
        prevScreen = curScreen;
    }

    // Display update
    if (now - tDisp >= DISP_MS) {
        dTotal = totalPkt;
        for (int i = 0; i < P_COUNT; i++) dCnt[i] = cnt[i];

        bool full = screenChanged;
        if (screenChanged) {
            clearContent();
            drawHeader();
            drawNav();
            screenChanged = false;
        }

        drawStatBar();
        screens[curScreen].draw(full);

        Serial.printf("[%lus] Scr:%d CH:%d Tot:%lu %lu/s Dev:%d AP:%d\n",
                      (unsigned long)(now / 1000), curScreen, curCh,
                      (unsigned long)dTotal, (unsigned long)curPPS, (int)nDevs, (int)nAps);
        tDisp = now;
    }
}
