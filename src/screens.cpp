#include "screen.h"
#include "globals.h"
#include "display.h"
#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════
// Screen 0 — Protocol Monitor
// ═══════════════════════════════════════════════════════════════════
static void drawProtoLabels() {
    for (int i = 0; i < P_COUNT; i++) {
        int y = BAR_Y0 + i * (BAR_H + BAR_GAP);
        tft.setTextDatum(MR_DATUM); tft.setTextFont(2); tft.setTextSize(1);
        tft.setTextColor(pColor[i], BG);
        tft.drawString(pName[i], LABEL_W - 4, y + BAR_H / 2);
    }
}

static void drawProtoBars() {
    uint32_t mx = 1;
    for (int i = 0; i < P_COUNT; i++) if (dCnt[i] > mx) mx = dCnt[i];
    for (int i = 0; i < P_COUNT; i++) {
        int y = BAR_Y0 + i * (BAR_H + BAR_GAP), bY = y + 3, bH = BAR_H - 6;
        int w = mx > 0 ? (int)((uint64_t)dCnt[i] * BAR_MAXW / mx) : 0;
        if (w > BAR_MAXW) w = BAR_MAXW;
        if (w > prevBarW[i])      tft.fillRect(BAR_X + prevBarW[i], bY, w - prevBarW[i], bH, pColor[i]);
        else if (w < prevBarW[i]) tft.fillRect(BAR_X + w, bY, prevBarW[i] - w, bH, BG);
        prevBarW[i] = w;
        float pct = dTotal > 0 ? dCnt[i] * 100.0f / dTotal : 0;
        char b[24]; snprintf(b, sizeof(b), "%lu %d%%", (unsigned long)dCnt[i], (int)(pct + .5f));
        tft.setTextDatum(ML_DATUM); tft.setTextFont(1);
        tft.setTextColor(TFT_WHITE, BG); tft.setTextPadding(COUNT_W);
        tft.drawString(b, BAR_X + BAR_MAXW + 4, y + BAR_H / 2);
    }
    tft.setTextPadding(0);
}

static void drawProtoSparkline() {
    static const uint16_t SBG = 0x0841;
    int ly = SPARK_Y - 14;
    tft.setTextFont(1); tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM); tft.setTextPadding(SPARK_W / 2);
    tft.setTextColor(0x7BEF, BG); tft.drawString("Traffic (60s)", SPARK_X, ly);
    char ps[16]; snprintf(ps, sizeof(ps), "%lu/s", (unsigned long)curPPS);
    tft.setTextDatum(TR_DATUM); tft.setTextColor(TFT_GREEN, BG);
    tft.drawString(ps, SPARK_X + SPARK_W, ly);
    tft.setTextPadding(0);
    uint32_t maxP = 10;
    for (int i = 0; i < HIST_LEN; i++) if (hist[i] > maxP) maxP = hist[i];
    float cW = (float)SPARK_W / HIST_LEN;
    for (int t = 0; t < HIST_LEN; t++) {
        int idx = (hIdx + t) % HIST_LEN;
        int px = SPARK_X + (int)(t * cW), cw = max(1, (int)((t + 1) * cW) - (int)(t * cW));
        tft.fillRect(px, SPARK_Y + 1, cw, SPARK_H - 2, SBG);
        int cumH = 0;
        for (int pr = 0; pr < P_COUNT; pr++) {
            uint32_t v = pHist[pr][idx]; if (!v) continue;
            int bh = (int)((uint64_t)v * (SPARK_H - 2) / maxP); if (bh < 1) bh = 1;
            int by = SPARK_Y + SPARK_H - 1 - cumH - bh;
            if (by < SPARK_Y + 1) by = SPARK_Y + 1;
            tft.fillRect(px, by, cw, bh, pColor[pr]); cumH += bh;
        }
    }
    tft.drawRect(SPARK_X, SPARK_Y, SPARK_W, SPARK_H, 0x4208);
}

void drawProtocol(bool full) {
    if (full) drawProtoLabels();
    drawProtoBars();
    drawProtoSparkline();
}

// ═══════════════════════════════════════════════════════════════════
// Screen 1 — Channel Heatmap
// ═══════════════════════════════════════════════════════════════════
void drawChannels(bool full) {
    const int bW = 18, gap = 3, nCh = 13;
    const int totalW = nCh * bW + (nCh - 1) * gap;
    const int x0 = (SW - totalW) / 2;
    const int barTop = CONTENT_Y + 8, barBot = SH - NAV_H - 28;
    const int maxH = barBot - barTop;
    const int labelY = barBot + 4;

    uint32_t mx = 1;
    for (int c = 1; c <= 13; c++) if (cstat[c].total > mx) mx = cstat[c].total;

    for (int c = 1; c <= 13; c++) {
        int bx = x0 + (c - 1) * (bW + gap);
        int h = mx > 0 ? (int)((uint64_t)cstat[c].total * maxH / mx) : 0;
        if (h > maxH) h = maxH;
        float intens = mx > 0 ? (float)cstat[c].total / mx : 0;
        uint16_t col = heatColor(intens);
        if (h > prevChBarH[c])      tft.fillRect(bx, barBot - h, bW, h - prevChBarH[c], col);
        else if (h < prevChBarH[c]) tft.fillRect(bx, barBot - prevChBarH[c], bW, prevChBarH[c] - h, BG);
        if (h > 0 && full)          tft.fillRect(bx, barBot - h, bW, h, col);
        prevChBarH[c] = h;
        char lb[4]; snprintf(lb, sizeof(lb), "%d", c);
        tft.setTextDatum(TC_DATUM); tft.setTextFont(1); tft.setTextSize(1);
        tft.setTextColor(c == curCh ? TFT_CYAN : TFT_WHITE, BG);
        tft.setTextPadding(bW + gap); tft.drawString(lb, bx + bW / 2, labelY);
        if (h > 12 || full) {
            char ct[10]; snprintf(ct, sizeof(ct), "%lu", (unsigned long)cstat[c].total);
            tft.setTextDatum(BC_DATUM);
            tft.setTextColor(TFT_WHITE, h > 12 ? col : BG);
            tft.setTextPadding(bW + gap); tft.drawString(ct, bx + bW / 2, barBot - h - 2);
        }
    }
    tft.setTextPadding(0);
    int ry = labelY + 12;
    tft.setTextDatum(TC_DATUM); tft.setTextFont(1);
    for (int c = 1; c <= 13; c++) {
        int bx = x0 + (c - 1) * (bW + gap);
        if (cstat[c].rssiN == 0) continue;
        int8_t avg = (int8_t)(cstat[c].rssiSum / cstat[c].rssiN);
        char rb[6]; snprintf(rb, sizeof(rb), "%d", avg);
        tft.setTextColor(rssiColor(avg), BG);
        tft.setTextPadding(bW + gap); tft.drawString(rb, bx + bW / 2, ry);
    }
    tft.setTextPadding(0);
}

// ═══════════════════════════════════════════════════════════════════
// Screen 2 — AP Scanner
// ═══════════════════════════════════════════════════════════════════
void drawAPScan(bool full) {
    int idx[MAX_APS]; int n = (int)nAps;
    for (int i = 0; i < n; i++) idx[i] = i;
    for (int i = 0; i < n - 1; i++) for (int j = 0; j < n - 1 - i; j++)
        if (aps[idx[j]].rssi < aps[idx[j + 1]].rssi) { int t = idx[j]; idx[j] = idx[j + 1]; idx[j + 1] = t; }

    int rowH = 20, hdrY = CONTENT_Y + 2;
    int maxRows = (CONTENT_H - 22) / rowH;
    int start = scrollOfs;
    if (start > n - maxRows) start = max(0, n - maxRows);

    tft.setTextFont(1); tft.setTextSize(1); tft.setTextDatum(TL_DATUM);
    if (full) {
        tft.setTextColor(TFT_CYAN, BG);
        tft.fillRect(0, hdrY, SW, 14, BG);
        tft.drawString("SSID", 4, hdrY);
        tft.drawString("dBm", 180, hdrY);
        tft.drawString("CH", 220, hdrY);
        tft.drawString("Enc", 252, hdrY);
        tft.drawFastHLine(2, hdrY + 12, SW - 4, 0x4208);
    }

    int y = hdrY + 16;
    tft.setTextColor(TFT_WHITE, BG);
    for (int r = 0; r < maxRows && start + r < n; r++) {
        APInfo& ap = aps[idx[start + r]];
        int ry = y + r * rowH;
        tft.fillRect(0, ry, SW, rowH, BG);
        tft.setTextDatum(TL_DATUM);
        char ss[21]; strncpy(ss, ap.ssid[0] ? ap.ssid : "(hidden)", 20); ss[20] = '\0';
        tft.setTextColor(start + r == trackedAP ? TFT_CYAN : TFT_WHITE, BG);
        tft.drawString(ss, 4, ry + 4);
        char rs[8]; snprintf(rs, sizeof(rs), "%d", ap.rssi);
        tft.setTextColor(rssiColor(ap.rssi), BG); tft.drawString(rs, 180, ry + 4);
        char ch[4]; snprintf(ch, sizeof(ch), "%d", ap.channel);
        tft.setTextColor(TFT_WHITE, BG); tft.drawString(ch, 224, ry + 4);
        tft.setTextColor(ap.encryption == 0 ? TFT_RED : TFT_GREEN, BG);
        tft.drawString(encStr(ap.encryption), 252, ry + 4);
    }
    int usedH = y + min(maxRows, max(0, n - start)) * rowH;
    if (usedH < CONTENT_Y + CONTENT_H) tft.fillRect(0, usedH, SW, CONTENT_Y + CONTENT_H - usedH, BG);
    tft.setTextDatum(TR_DATUM); tft.setTextColor(0x7BEF, BG);
    if (start > 0) tft.drawString("^ more", SW - 4, hdrY);
    tft.setTextDatum(BR_DATUM);
    if (start + maxRows < n) tft.drawString("v more", SW - 4, SH - NAV_H - 2);
}

// ═══════════════════════════════════════════════════════════════════
// Screen 3 — Signal Tracker
// ═══════════════════════════════════════════════════════════════════
void drawSignal(bool full) {
    if (trackedAP < 0 || trackedAP >= (int)nAps) {
        int best = -1; int8_t bestR = -127;
        for (int i = 0; i < (int)nAps; i++) if (aps[i].rssi > bestR) { bestR = aps[i].rssi; best = i; }
        trackedAP = best;
    }
    int y = CONTENT_Y + 4;
    if (trackedAP >= 0 && trackedAP < (int)nAps) {
        APInfo& ap = aps[trackedAP];
        tft.setTextDatum(TC_DATUM); tft.setTextFont(2);
        tft.setTextColor(TFT_CYAN, BG); tft.setTextPadding(SW);
        char hdr[48]; snprintf(hdr, sizeof(hdr), "%s  CH %d", ap.ssid[0] ? ap.ssid : "(hidden)", ap.channel);
        tft.drawString(hdr, SW / 2, y);
        y += 24;
        tft.setTextFont(4); tft.setTextSize(2);
        tft.setTextColor(rssiColor(trackedRSSI), BG); tft.setTextPadding(200);
        char big[16]; snprintf(big, sizeof(big), "%d", trackedRSSI);
        tft.drawString(big, SW / 2, y);
        y += 56;
        tft.setTextFont(2); tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, BG); tft.setTextPadding(200);
        const char* q = trackedRSSI >= -50 ? "Excellent" : trackedRSSI >= -60 ? "Good" : trackedRSSI >= -70 ? "Fair" : "Weak";
        char ql[32]; snprintf(ql, sizeof(ql), "dBm  -  %s", q);
        tft.drawString(ql, SW / 2, y); tft.setTextPadding(0);
    } else {
        tft.setTextDatum(MC_DATUM); tft.setTextFont(2);
        tft.setTextColor(0x7BEF, BG); tft.drawString("No AP detected", SW / 2, CONTENT_Y + 60);
        return;
    }
    y += 28;
    int cH = SH - NAV_H - y - 30, cY = y, cX = SPARK_X, cW = SPARK_W;
    static const uint16_t SBG = 0x0841;
    int8_t minR = -90, maxR = -30;
    float colW = (float)cW / HIST_LEN;
    for (int t = 0; t < HIST_LEN; t++) {
        int idx = (rssiIdx + t) % HIST_LEN;
        int px = cX + (int)(t * colW), cww = max(1, (int)((t + 1) * colW) - (int)(t * colW));
        tft.fillRect(px, cY, cww, cH, SBG);
        int8_t r = rssiHist[idx];
        if (r < -95) continue;
        float norm = (float)(r - minR) / (maxR - minR);
        if (norm < 0) norm = 0; if (norm > 1) norm = 1;
        int py = cY + cH - 1 - (int)(norm * (cH - 2));
        tft.fillRect(px, py, cww, max(2, cY + cH - 1 - py), rssiColor(r));
    }
    tft.drawRect(cX, cY, cW, cH, 0x4208);
    int scY = cY + cH + 4;
    tft.setTextFont(1); tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM); tft.setTextColor(0x7BEF, BG); tft.setTextPadding(40);
    tft.drawString("-90", cX, scY);
    tft.setTextDatum(TR_DATUM); tft.drawString("-30", cX + cW, scY);
    tft.setTextDatum(TC_DATUM); tft.setTextColor(0x4208, BG);
    tft.drawString("tap to cycle AP", SW / 2, scY + 10); tft.setTextPadding(0);
}

// ═══════════════════════════════════════════════════════════════════
// Screen 4 — Device Activity
// ═══════════════════════════════════════════════════════════════════
void drawDevices(bool full) {
    int idx[MAX_DEVS]; int n = (int)nDevs;
    for (int i = 0; i < n; i++) idx[i] = i;
    unsigned long now = millis();
    for (int i = 0; i < n - 1; i++) for (int j = 0; j < n - 1 - i; j++)
        if (devs[idx[j]].lastSeen < devs[idx[j + 1]].lastSeen) { int t = idx[j]; idx[j] = idx[j + 1]; idx[j + 1] = t; }

    int rowH = 18, hdrY = CONTENT_Y + 2;
    int maxRows = (CONTENT_H - 20) / rowH;
    int start = scrollOfs;
    if (start > n - maxRows) start = max(0, n - maxRows);

    if (full) {
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM); tft.setTextColor(TFT_CYAN, BG);
        tft.drawString("MAC", 4, hdrY); tft.drawString("dBm", 130, hdrY);
        tft.drawString("CH", 170, hdrY); tft.drawString("Pkts", 200, hdrY);
        tft.drawString("Ago", 260, hdrY);
        tft.drawFastHLine(2, hdrY + 10, SW - 4, 0x4208);
    }
    int y = hdrY + 14;
    for (int r = 0; r < maxRows && start + r < n; r++) {
        DevInfo& d = devs[idx[start + r]];
        int ry = y + r * rowH;
        tft.fillRect(0, ry, SW, rowH, BG);
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM);
        char mc[18]; fmtMACShort(mc, d.mac);
        unsigned long ago = (now - d.lastSeen) / 1000;
        bool recent = ago < 5;
        tft.setTextColor(recent ? TFT_GREEN : TFT_WHITE, BG); tft.drawString(mc, 4, ry + 4);
        char rs[6]; snprintf(rs, sizeof(rs), "%d", d.rssi);
        tft.setTextColor(rssiColor(d.rssi), BG); tft.drawString(rs, 130, ry + 4);
        char ch[4]; snprintf(ch, sizeof(ch), "%d", d.channel);
        tft.setTextColor(TFT_WHITE, BG); tft.drawString(ch, 174, ry + 4);
        char pk[10]; snprintf(pk, sizeof(pk), "%lu", (unsigned long)d.pktCount);
        tft.drawString(pk, 200, ry + 4);
        char ag[10];
        if (ago < 60)        snprintf(ag, sizeof(ag), "%lus", ago);
        else if (ago < 3600) snprintf(ag, sizeof(ag), "%lum", ago / 60);
        else                 snprintf(ag, sizeof(ag), "%luh", ago / 3600);
        tft.setTextColor(recent ? TFT_GREEN : 0x7BEF, BG); tft.drawString(ag, 260, ry + 4);
    }
    int usedH = y + min(maxRows, max(0, n - start)) * rowH;
    if (usedH < CONTENT_Y + CONTENT_H) tft.fillRect(0, usedH, SW, CONTENT_Y + CONTENT_H - usedH, BG);
}

// ═══════════════════════════════════════════════════════════════════
// Screen 5 — Traffic per Channel
// ═══════════════════════════════════════════════════════════════════
void drawTraffic(bool full) {
    const int rowH = 26, gap = 3, labW = 36, cntW = 60;
    const int barX = labW + 2, barMW = SW - barX - cntW - 8;
    int y0 = CONTENT_Y + 4;
    uint32_t mx = 1;
    for (int c = 1; c <= 13; c++) if (cstat[c].total > mx) mx = cstat[c].total;
    for (int c = 1; c <= 13; c++) {
        int y = y0 + (c - 1) * (rowH + gap), bY = y + 3, bH = rowH - 6;
        int w = mx > 0 ? (int)((uint64_t)cstat[c].total * barMW / mx) : 0;
        if (w > barMW) w = barMW;
        if (w > prevChHBarW[c])      tft.fillRect(barX + prevChHBarW[c], bY, w - prevChHBarW[c], bH, chCol[c]);
        else if (w < prevChHBarW[c]) tft.fillRect(barX + w, bY, prevChHBarW[c] - w, bH, BG);
        prevChHBarW[c] = w;
        if (full) {
            char lb[6]; snprintf(lb, sizeof(lb), "CH%d", c);
            tft.setTextDatum(MR_DATUM); tft.setTextFont(1); tft.setTextSize(1);
            tft.setTextColor(chCol[c], BG); tft.drawString(lb, labW - 2, y + rowH / 2);
        }
        char ct[16]; snprintf(ct, sizeof(ct), "%lu", (unsigned long)cstat[c].total);
        tft.setTextDatum(ML_DATUM); tft.setTextFont(1);
        tft.setTextColor(TFT_WHITE, BG); tft.setTextPadding(cntW);
        tft.drawString(ct, barX + barMW + 4, y + rowH / 2);
    }
    tft.setTextPadding(0);
}

// ═══════════════════════════════════════════════════════════════════
// Screen 6 — Deauth Detector
// ═══════════════════════════════════════════════════════════════════
void drawDeauth(bool full) {
    uint32_t dpm = 0;
    for (int i = 0; i < HIST_LEN; i++) dpm += deauthHist[i];
    int y = CONTENT_Y + 8;
    uint16_t bannerCol = dpm == 0 ? TFT_DARKGREEN : dpm < 10 ? TFT_YELLOW : TFT_RED;
    const char* status = dpm == 0 ? "SAFE" : dpm < 10 ? "WARNING" : "ALERT";
    tft.fillRoundRect(40, y, SW - 80, 50, 8, bannerCol);
    tft.setTextDatum(MC_DATUM); tft.setTextFont(4); tft.setTextSize(1);
    tft.setTextColor(dpm >= 10 || dpm == 0 ? TFT_WHITE : TFT_BLACK, bannerCol);
    tft.drawString(status, SW / 2, y + 25);
    y += 60;
    tft.setTextFont(2); tft.setTextSize(1);
    tft.setTextDatum(TC_DATUM); tft.setTextPadding(SW - 20);
    tft.setTextColor(TFT_WHITE, BG);
    char st[48]; snprintf(st, sizeof(st), "Total: %lu    Rate: %lu/min", (unsigned long)deauthTotal, (unsigned long)dpm);
    tft.drawString(st, SW / 2, y); tft.setTextPadding(0);
    y += 28;
    tft.setTextFont(1); tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_CYAN, BG);
    if (full) {
        tft.drawString("Time     Source        Target       CH", 4, y);
        tft.drawFastHLine(2, y + 10, SW - 4, 0x4208);
    }
    y += 14;
    int maxRows = (SH - NAV_H - y) / 14;
    unsigned long now = millis();
    for (int r = 0; r < maxRows && r < MAX_DEAUTH; r++) {
        int ei = (deauthHead - 1 - r + MAX_DEAUTH) % MAX_DEAUTH;
        DeauthEvt& e = deauthBuf[ei];
        if (e.ts == 0) break;
        int ry = y + r * 14;
        tft.fillRect(0, ry, SW, 14, BG);
        unsigned long ago = (now - e.ts) / 1000;
        char src[12], dst[12];
        fmtMACShort(src, e.src); fmtMACShort(dst, e.dst);
        char line[64]; snprintf(line, sizeof(line), "-%lus  %s > %s  %d", ago, src, dst, e.channel);
        tft.setTextColor(ago < 5 ? TFT_RED : TFT_WHITE, BG);
        tft.drawString(line, 4, ry + 2);
    }
}

// ═══════════════════════════════════════════════════════════════════
// Screen 7 — Beacon Monitor
// ═══════════════════════════════════════════════════════════════════
static bool hasDuplicateSSID(int i) {
    if (aps[i].ssid[0] == '\0') return false;
    for (int j = 0; j < (int)nAps; j++) {
        if (j == i) continue;
        if (strcmp(aps[j].ssid, aps[i].ssid) == 0 && memcmp(aps[j].bssid, aps[i].bssid, 6) != 0)
            return true;
    }
    return false;
}

void drawBeacons(bool full) {
    int rowH = 20, hdrY = CONTENT_Y + 2;
    int maxRows = (CONTENT_H - 20) / rowH;
    int n = (int)nAps;
    if (full) {
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM); tft.setTextColor(TFT_CYAN, BG);
        tft.drawString("SSID", 4, hdrY); tft.drawString("Intvl", 150, hdrY);
        tft.drawString("Cnt", 200, hdrY); tft.drawString("Flag", 250, hdrY);
        tft.drawFastHLine(2, hdrY + 10, SW - 4, 0x4208);
    }
    int y = hdrY + 14;
    for (int r = 0; r < maxRows && r < n; r++) {
        APInfo& ap = aps[r];
        int ry = y + r * rowH;
        tft.fillRect(0, ry, SW, rowH, BG);
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM);
        char ss[18]; strncpy(ss, ap.ssid[0] ? ap.ssid : "(hidden)", 17); ss[17] = '\0';
        bool dup = hasDuplicateSSID(r);
        bool oddInterval = ap.beaconIntvl != 100 && ap.beaconIntvl != 0;
        tft.setTextColor(dup ? TFT_RED : TFT_WHITE, BG); tft.drawString(ss, 4, ry + 4);
        char iv[8]; snprintf(iv, sizeof(iv), "%u", ap.beaconIntvl);
        tft.setTextColor(oddInterval ? TFT_YELLOW : TFT_WHITE, BG); tft.drawString(iv, 150, ry + 4);
        char ct[8]; snprintf(ct, sizeof(ct), "%u", ap.beaconCount);
        tft.setTextColor(TFT_WHITE, BG); tft.drawString(ct, 200, ry + 4);
        if (dup) {
            tft.setTextColor(TFT_RED, BG); tft.drawString("DUPE!", 250, ry + 4);
        } else if (oddInterval) {
            tft.setTextColor(TFT_YELLOW, BG); tft.drawString("odd BI", 250, ry + 4);
        } else {
            tft.setTextColor(TFT_GREEN, BG); tft.drawString("OK", 250, ry + 4);
        }
    }
    int usedH = y + min(maxRows, n) * rowH;
    if (usedH < CONTENT_Y + CONTENT_H) tft.fillRect(0, usedH, SW, CONTENT_Y + CONTENT_H - usedH, BG);
}

// ═══════════════════════════════════════════════════════════════════
// Screen 8 — Retry / Error Rate
// ═══════════════════════════════════════════════════════════════════
void drawRetry(bool full) {
    int y = CONTENT_Y + 4;
    float pct = totalPkt > 0 ? totalRetry * 100.0f / totalPkt : 0;
    tft.setTextDatum(TC_DATUM); tft.setTextFont(4); tft.setTextSize(1);
    tft.setTextColor(pct < 5 ? TFT_GREEN : pct < 15 ? TFT_YELLOW : TFT_RED, BG);
    tft.setTextPadding(200);
    char big[16]; snprintf(big, sizeof(big), "%.1f%%", pct);
    tft.drawString(big, SW / 2, y);
    y += 32;
    tft.setTextFont(2); tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, BG); tft.setTextPadding(200);
    char sub[32]; snprintf(sub, sizeof(sub), "Retry Rate (%lu/%lu)", (unsigned long)totalRetry, (unsigned long)(uint32_t)totalPkt);
    tft.drawString(sub, SW / 2, y); tft.setTextPadding(0);
    y += 30;
    const int rowH = 22, gap = 2, labW = 36, cntW = 50;
    const int barX = labW + 2, barMW = SW - barX - cntW - 8;
    uint32_t mx = 1;
    for (int c = 1; c <= 13; c++) if (cstat[c].retries > mx) mx = cstat[c].retries;
    if (full) {
        tft.setTextDatum(TC_DATUM); tft.setTextFont(1);
        tft.setTextColor(0x7BEF, BG); tft.drawString("Retries per channel", SW / 2, y - 12);
    }
    for (int c = 1; c <= 13; c++) {
        int ry = y + (c - 1) * (rowH + gap), bY = ry + 3, bH = rowH - 6;
        int w = mx > 0 ? (int)((uint64_t)cstat[c].retries * barMW / mx) : 0;
        if (w > barMW) w = barMW;
        float chPct = cstat[c].total > 0 ? cstat[c].retries * 100.0f / cstat[c].total : 0;
        uint16_t col = chPct < 5 ? TFT_GREEN : chPct < 15 ? TFT_YELLOW : TFT_RED;
        if (full && w > 0) tft.fillRect(barX, bY, w, bH, col);
        else if (w > prevChHBarW[c]) tft.fillRect(barX + prevChHBarW[c], bY, w - prevChHBarW[c], bH, col);
        else if (w < prevChHBarW[c]) tft.fillRect(barX + w, bY, prevChHBarW[c] - w, bH, BG);
        if (full) {
            char lb[6]; snprintf(lb, sizeof(lb), "%d", c);
            tft.setTextDatum(MR_DATUM); tft.setTextFont(1);
            tft.setTextColor(chCol[c], BG); tft.drawString(lb, labW - 2, ry + rowH / 2);
        }
        char ct[20]; snprintf(ct, sizeof(ct), "%lu %.0f%%", (unsigned long)cstat[c].retries, chPct);
        tft.setTextDatum(ML_DATUM); tft.setTextFont(1);
        tft.setTextColor(TFT_WHITE, BG); tft.setTextPadding(cntW);
        tft.drawString(ct, barX + barMW + 4, ry + rowH / 2);
    }
    tft.setTextPadding(0);
}
