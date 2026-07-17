#include "display.h"
#include "globals.h"
#include "screen.h"
#include <Arduino.h>

void initDisplay() {
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(BG);
#ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
#endif
    uint16_t cal[5] = {300, 3600, 300, 3600, 1};
    tft.setTouch(cal);

    pColor[P_BEACON] = tft.color565(100, 149, 237);
    pColor[P_DATA]   = tft.color565(50, 205, 50);
    pColor[P_PROBE]  = tft.color565(255, 215, 0);
    pColor[P_CTRL]   = tft.color565(0, 206, 209);
    pColor[P_AUTH]   = tft.color565(255, 99, 71);
    pColor[P_OTHER]  = tft.color565(169, 169, 169);

    chCol[1]  = tft.color565(255, 60, 60);   chCol[2]  = tft.color565(255, 130, 40);
    chCol[3]  = tft.color565(255, 200, 40);  chCol[4]  = tft.color565(200, 255, 40);
    chCol[5]  = tft.color565(80, 230, 80);   chCol[6]  = tft.color565(40, 220, 180);
    chCol[7]  = tft.color565(40, 200, 255);  chCol[8]  = tft.color565(80, 140, 255);
    chCol[9]  = tft.color565(120, 100, 255); chCol[10] = tft.color565(170, 80, 255);
    chCol[11] = tft.color565(220, 60, 220);  chCol[12] = tft.color565(255, 60, 180);
    chCol[13] = tft.color565(255, 100, 130);
}

void drawHeader() {
    tft.fillRect(0, 0, SW, HDR_H, BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(4); tft.setTextSize(1);
    tft.setTextColor(TFT_CYAN, BG);
    tft.drawString(screens[curScreen].name, SW / 2, HDR_H / 2 + 1);
    tft.setTextFont(1);
    tft.setTextColor(0x4208, BG);
    tft.setTextDatum(ML_DATUM); tft.drawString("<", 6, HDR_H / 2);
    tft.setTextDatum(MR_DATUM); tft.drawString(">", SW - 6, HDR_H / 2);
}

void drawStatBar() {
    tft.setTextDatum(TL_DATUM); tft.setTextFont(1); tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, STAT_BG);
    tft.setTextPadding(SW);
    char b[64];
    snprintf(b, sizeof(b), " CH:%-2d  %lu pkts  %lu/s  %d dev  %d AP",
             curCh, (unsigned long)dTotal, (unsigned long)curPPS, (int)nDevs, (int)nAps);
    tft.drawString(b, 0, HDR_H + 4);
    tft.setTextPadding(0);
}

void drawNav() {
    int y = SH - NAV_H / 2;
    int sp = 14;
    int x0 = (SW - NUM_SCREENS * sp) / 2;
    tft.fillRect(0, SH - NAV_H, SW, NAV_H, BG);
    for (int i = 0; i < NUM_SCREENS; i++) {
        int x = x0 + i * sp + sp / 2;
        if (i == curScreen) tft.fillCircle(x, y, 3, TFT_CYAN);
        else                tft.drawCircle(x, y, 2, 0x4208);
    }
}

void clearContent() {
    tft.fillRect(0, CONTENT_Y, SW, CONTENT_H, BG);
    scrollOfs = 0;
    memset(prevBarW, 0, sizeof(prevBarW));
    memset(prevChBarH, 0, sizeof(prevChBarH));
    memset(prevChHBarW, 0, sizeof(prevChHBarW));
}

void fmtMAC(char* b, const uint8_t* m) {
    snprintf(b, 18, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
}

void fmtMACShort(char* b, const uint8_t* m) {
    snprintf(b, 12, "%02X:%02X..%02X:%02X", m[0], m[1], m[4], m[5]);
}

uint16_t rssiColor(int8_t r) {
    if (r >= -50) return TFT_GREEN;
    if (r >= -60) return TFT_YELLOW;
    if (r >= -70) return tft.color565(255, 165, 0);
    return TFT_RED;
}

uint16_t heatColor(float t) {
    if (t < 0) t = 0; if (t > 1) t = 1;
    int r = t < 0.5f ? (int)(t * 2 * 255) : 255;
    int g = t < 0.5f ? 255 : (int)((1 - t) * 2 * 255);
    return tft.color565(r, g, 0);
}

const char* encStr(uint8_t e) {
    static const char* s[] = {"OPEN", "WEP", "WPA", "WPA2"};
    return e < 4 ? s[e] : "?";
}
