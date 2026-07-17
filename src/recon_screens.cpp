#include "screen.h"
#include "globals.h"
#include "display.h"
#include "sd_utils.h"
#include "ble_scanner.h"
#include "sniffer.h"
#include "oui_db.h"
#include "parser.h"
#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════
// Screen 9 — Probe Request Logger
// ═══════════════════════════════════════════════════════════════════
void drawProbeLog(bool full) {
    int rowH = 16, hdrY = CONTENT_Y + 2;
    int maxRows = (CONTENT_H - 20) / rowH;
    int n = (int)probeCount;

    if (full) {
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_CYAN, BG);
        tft.drawString("MAC", 4, hdrY);
        tft.drawString("Sought SSID", 100, hdrY);
        tft.drawString("dBm", 268, hdrY);
        tft.drawFastHLine(2, hdrY + 10, SW - 4, 0x4208);
    }

    int y = hdrY + 14;
    for (int r = 0; r < maxRows && r < n; r++) {
        int pi = (probeHead - 1 - r + MAX_PROBES) % MAX_PROBES;
        ProbeReq& pr = probes[pi];
        int ry = y + r * rowH;
        tft.fillRect(0, ry, SW, rowH, BG);
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM);
        char mc[12]; fmtMACShort(mc, pr.mac);
        tft.setTextColor(pr.randomMAC ? TFT_YELLOW : TFT_WHITE, BG);
        tft.drawString(mc, 4, ry + 3);
        char ss[22]; strncpy(ss, pr.ssid[0] ? pr.ssid : "(broadcast)", 21); ss[21] = '\0';
        tft.setTextColor(pr.ssid[0] ? TFT_GREEN : 0x7BEF, BG);
        tft.drawString(ss, 100, ry + 3);
        char rs[6]; snprintf(rs, sizeof(rs), "%d", pr.rssi);
        tft.setTextColor(rssiColor(pr.rssi), BG);
        tft.drawString(rs, 268, ry + 3);
    }
    int usedH = y + min(maxRows, n) * rowH;
    if (usedH < CONTENT_Y + CONTENT_H) tft.fillRect(0, usedH, SW, CONTENT_Y + CONTENT_H - usedH, BG);

    tft.setTextDatum(BR_DATUM); tft.setTextFont(1);
    tft.setTextColor(0x4208, BG); tft.setTextPadding(SW);
    char info[32]; snprintf(info, sizeof(info), "%d probes captured", n);
    tft.drawString(info, SW - 4, SH - NAV_H - 2);
    tft.setTextPadding(0);
}

// ═══════════════════════════════════════════════════════════════════
// Screen 10 — Hidden SSID Revealer
// ═══════════════════════════════════════════════════════════════════
void drawHiddenSSID(bool full) {
    int rowH = 24, hdrY = CONTENT_Y + 2;
    int maxRows = (CONTENT_H - 20) / rowH;

    if (full) {
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_CYAN, BG);
        tft.drawString("BSSID", 4, hdrY);
        tft.drawString("Hidden SSID", 110, hdrY);
        tft.drawString("CH", 280, hdrY);
        tft.drawFastHLine(2, hdrY + 10, SW - 4, 0x4208);
    }

    int y = hdrY + 14;
    int drawn = 0;
    for (int i = 0; i < (int)nAps && drawn < maxRows; i++) {
        if (!aps[i].hidden) continue;
        int ry = y + drawn * rowH;
        tft.fillRect(0, ry, SW, rowH, BG);
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM);
        char mc[12]; fmtMACShort(mc, aps[i].bssid);
        tft.setTextColor(TFT_WHITE, BG);
        tft.drawString(mc, 4, ry + 4);

        if (aps[i].revealedSSID[0]) {
            tft.setTextColor(TFT_GREEN, BG);
            tft.drawString(aps[i].revealedSSID, 110, ry + 4);
            tft.setTextColor(TFT_GREEN, BG);
            tft.drawString("FOUND", 110, ry + 14);
        } else {
            tft.setTextColor(TFT_RED, BG);
            tft.drawString("???", 110, ry + 4);
            tft.setTextColor(0x7BEF, BG);
            tft.drawString("waiting...", 110, ry + 14);
        }
        char ch[4]; snprintf(ch, sizeof(ch), "%d", aps[i].channel);
        tft.setTextColor(TFT_WHITE, BG);
        tft.drawString(ch, 284, ry + 4);
        drawn++;
    }

    int usedH = y + drawn * rowH;
    if (usedH < CONTENT_Y + CONTENT_H) tft.fillRect(0, usedH, SW, CONTENT_Y + CONTENT_H - usedH, BG);

    if (drawn == 0) {
        tft.setTextDatum(MC_DATUM); tft.setTextFont(2);
        tft.setTextColor(0x7BEF, BG);
        tft.drawString("No hidden APs detected", SW / 2, CONTENT_Y + CONTENT_H / 2);
    }
}

// ═══════════════════════════════════════════════════════════════════
// Screen 11 — WPA Handshake Capture
// ═══════════════════════════════════════════════════════════════════
void drawHandshake(bool full) {
    int rowH = 28, hdrY = CONTENT_Y + 2;
    int maxRows = (CONTENT_H - 20) / rowH;
    int n = (int)nHandshakes;

    if (full) {
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_CYAN, BG);
        tft.drawString("AP SSID", 4, hdrY);
        tft.drawString("Client", 140, hdrY);
        tft.drawString("Msgs", 250, hdrY);
        tft.drawFastHLine(2, hdrY + 10, SW - 4, 0x4208);
    }

    int y = hdrY + 14;
    for (int r = 0; r < maxRows && r < n; r++) {
        HandshakeState& hs = handshakes[r];
        int ry = y + r * rowH;
        tft.fillRect(0, ry, SW, rowH, BG);
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM);

        char ss[16]; strncpy(ss, hs.ssid[0] ? hs.ssid : "(unknown)", 15); ss[15] = '\0';
        bool complete = (hs.captured & 0x0F) == 0x0F;
        tft.setTextColor(complete ? TFT_GREEN : TFT_WHITE, BG);
        tft.drawString(ss, 4, ry + 3);

        char cl[12]; fmtMACShort(cl, hs.client);
        tft.setTextColor(TFT_WHITE, BG);
        tft.drawString(cl, 140, ry + 3);

        // Show which messages captured: [1][2][3][4]
        char msgs[20];
        snprintf(msgs, sizeof(msgs), "%c%c%c%c",
                 hs.captured & 1 ? '1' : '-',
                 hs.captured & 2 ? '2' : '-',
                 hs.captured & 4 ? '3' : '-',
                 hs.captured & 8 ? '4' : '-');
        tft.setTextColor(complete ? TFT_GREEN : TFT_YELLOW, BG);
        tft.drawString(msgs, 250, ry + 3);

        // BSSID on second line
        char bss[12]; fmtMACShort(bss, hs.bssid);
        tft.setTextColor(0x7BEF, BG);
        tft.drawString(bss, 4, ry + 15);

        if (complete) {
            tft.setTextColor(TFT_GREEN, BG);
            tft.drawString("COMPLETE", 140, ry + 15);
        }
    }

    int usedH = y + min(maxRows, n) * rowH;
    if (usedH < CONTENT_Y + CONTENT_H) tft.fillRect(0, usedH, SW, CONTENT_Y + CONTENT_H - usedH, BG);

    if (n == 0) {
        tft.setTextDatum(MC_DATUM); tft.setTextFont(2);
        tft.setTextColor(0x7BEF, BG);
        tft.drawString("Listening for EAPOL...", SW / 2, CONTENT_Y + CONTENT_H / 2);
    }
}

// ═══════════════════════════════════════════════════════════════════
// Screen 12 — PMKID Harvester
// ═══════════════════════════════════════════════════════════════════
void drawPMKID(bool full) {
    int rowH = 28, hdrY = CONTENT_Y + 2;
    int maxRows = (CONTENT_H - 20) / rowH;
    int n = (int)nPMKIDs;

    if (full) {
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_CYAN, BG);
        tft.drawString("AP SSID", 4, hdrY);
        tft.drawString("PMKID (first 8 bytes)", 120, hdrY);
        tft.drawFastHLine(2, hdrY + 10, SW - 4, 0x4208);
    }

    int y = hdrY + 14;
    for (int r = 0; r < maxRows && r < n; r++) {
        PMKIDEntry& pe = pmkids[r];
        int ry = y + r * rowH;
        tft.fillRect(0, ry, SW, rowH, BG);
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM);

        char ss[16]; strncpy(ss, pe.ssid[0] ? pe.ssid : "(unknown)", 15); ss[15] = '\0';
        tft.setTextColor(TFT_GREEN, BG);
        tft.drawString(ss, 4, ry + 3);

        char pm[25];
        snprintf(pm, sizeof(pm), "%02X%02X%02X%02X%02X%02X%02X%02X",
                 pe.pmkid[0], pe.pmkid[1], pe.pmkid[2], pe.pmkid[3],
                 pe.pmkid[4], pe.pmkid[5], pe.pmkid[6], pe.pmkid[7]);
        tft.setTextColor(TFT_YELLOW, BG);
        tft.drawString(pm, 120, ry + 3);

        char bss[12]; fmtMACShort(bss, pe.bssid);
        tft.setTextColor(0x7BEF, BG);
        tft.drawString(bss, 4, ry + 15);

        char cl[12]; fmtMACShort(cl, pe.client);
        tft.drawString(cl, 120, ry + 15);
    }

    int usedH = y + min(maxRows, n) * rowH;
    if (usedH < CONTENT_Y + CONTENT_H) tft.fillRect(0, usedH, SW, CONTENT_Y + CONTENT_H - usedH, BG);

    if (n == 0) {
        tft.setTextDatum(MC_DATUM); tft.setTextFont(2);
        tft.setTextColor(0x7BEF, BG);
        tft.drawString("Listening for PMKIDs...", SW / 2, CONTENT_Y + CONTENT_H / 2);
    }
}

// ═══════════════════════════════════════════════════════════════════
// Screen 13 — MAC Randomization Detector
// ═══════════════════════════════════════════════════════════════════
void drawMACRandom(bool full) {
    int y = CONTENT_Y + 4;

    // Count random vs real MACs
    int nRandom = 0, nReal = 0;
    for (int i = 0; i < (int)nDevs; i++) {
        if (isLocallyAdminMAC(devs[i].mac)) nRandom++; else nReal++;
    }
    int total = nRandom + nReal;
    float pct = total > 0 ? nRandom * 100.0f / total : 0;

    // Summary
    tft.setTextDatum(TC_DATUM); tft.setTextFont(4); tft.setTextSize(1);
    tft.setTextColor(pct > 50 ? TFT_YELLOW : TFT_GREEN, BG);
    tft.setTextPadding(200);
    char big[16]; snprintf(big, sizeof(big), "%.0f%%", pct);
    tft.drawString(big, SW / 2, y);
    y += 32;
    tft.setTextFont(2); tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, BG); tft.setTextPadding(SW);
    char sub[40]; snprintf(sub, sizeof(sub), "%d random / %d real / %d total", nRandom, nReal, total);
    tft.drawString(sub, SW / 2, y);
    tft.setTextPadding(0);

    // Device list showing random MACs
    y += 30;
    if (full) {
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_CYAN, BG);
        tft.drawString("Randomized MACs:", 4, y);
        tft.drawFastHLine(2, y + 10, SW - 4, 0x4208);
    }
    y += 14;

    int rowH = 16, maxRows = (SH - NAV_H - y) / rowH;
    int drawn = 0;
    unsigned long now = millis();
    for (int i = 0; i < (int)nDevs && drawn < maxRows; i++) {
        if (!isLocallyAdminMAC(devs[i].mac)) continue;
        int ry = y + drawn * rowH;
        tft.fillRect(0, ry, SW, rowH, BG);
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM);
        char mc[18]; fmtMAC(mc, devs[i].mac);
        tft.setTextColor(TFT_YELLOW, BG);
        tft.drawString(mc, 4, ry + 3);
        char rs[6]; snprintf(rs, sizeof(rs), "%d", devs[i].rssi);
        tft.setTextColor(rssiColor(devs[i].rssi), BG);
        tft.drawString(rs, 220, ry + 3);
        unsigned long ago = (now - devs[i].lastSeen) / 1000;
        char ag[10];
        if (ago < 60)        snprintf(ag, sizeof(ag), "%lus", ago);
        else if (ago < 3600) snprintf(ag, sizeof(ag), "%lum", ago / 60);
        else                 snprintf(ag, sizeof(ag), "%luh", ago / 3600);
        tft.setTextColor(0x7BEF, BG);
        tft.drawString(ag, 270, ry + 3);
        drawn++;
    }
    int usedH = y + drawn * rowH;
    if (usedH < CONTENT_Y + CONTENT_H) tft.fillRect(0, usedH, SW, CONTENT_Y + CONTENT_H - usedH, BG);
}

// ═══════════════════════════════════════════════════════════════════
// Screen 14 — OUI Vendor Lookup
// ═══════════════════════════════════════════════════════════════════
void drawOUIScreen(bool full) {
    int rowH = 18, hdrY = CONTENT_Y + 2;
    int maxRows = (CONTENT_H - 20) / rowH;
    int n = (int)nDevs;
    int start = scrollOfs;
    if (start > n - maxRows) start = max(0, n - maxRows);

    if (full) {
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_CYAN, BG);
        tft.drawString("MAC", 4, hdrY);
        tft.drawString("Vendor", 110, hdrY);
        tft.drawString("dBm", 268, hdrY);
        tft.drawFastHLine(2, hdrY + 10, SW - 4, 0x4208);
    }

    int y = hdrY + 14;
    for (int r = 0; r < maxRows && start + r < n; r++) {
        DevInfo& d = devs[start + r];
        int ry = y + r * rowH;
        tft.fillRect(0, ry, SW, rowH, BG);
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM);

        char mc[12]; fmtMACShort(mc, d.mac);
        bool isRandom = isLocallyAdminMAC(d.mac);
        tft.setTextColor(isRandom ? TFT_YELLOW : TFT_WHITE, BG);
        tft.drawString(mc, 4, ry + 4);

        const char* vendor = ouiLookup(d.mac);
        char vname[20];
        if (isRandom) strncpy(vname, "(random)", 19);
        else if (vendor) strncpy(vname, vendor, 19);
        else strncpy(vname, "(unknown)", 19);
        vname[19] = '\0';
        tft.setTextColor(vendor ? TFT_GREEN : 0x7BEF, BG);
        tft.drawString(vname, 110, ry + 4);

        char rs[6]; snprintf(rs, sizeof(rs), "%d", d.rssi);
        tft.setTextColor(rssiColor(d.rssi), BG);
        tft.drawString(rs, 268, ry + 4);
    }

    int usedH = y + min(maxRows, max(0, n - start)) * rowH;
    if (usedH < CONTENT_Y + CONTENT_H) tft.fillRect(0, usedH, SW, CONTENT_Y + CONTENT_H - usedH, BG);

    tft.setTextDatum(TR_DATUM); tft.setTextColor(0x7BEF, BG);
    if (start > 0) tft.drawString("^ more", SW - 4, hdrY);
    tft.setTextDatum(BR_DATUM);
    if (start + maxRows < n) tft.drawString("v more", SW - 4, SH - NAV_H - 2);
}

// ═══════════════════════════════════════════════════════════════════
// Screen 15 — PCAP Capture (SD Card)
// ═══════════════════════════════════════════════════════════════════
void drawPCAPScreen(bool full) {
    int y = CONTENT_Y + 20;

    if (!sdAvailable) {
        // SD card not available — show reason
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4); tft.setTextSize(1);
        tft.setTextColor(TFT_RED, BG);
        tft.drawString("Unavailable", SW / 2, y + 40);
        tft.setTextFont(2);
        tft.setTextColor(TFT_YELLOW, BG);
        tft.setTextPadding(SW - 20);
        tft.drawString("SD card not detected", SW / 2, y + 80);
        tft.setTextColor(0x7BEF, BG);
        tft.drawString("Wire SD module to VSPI bus:", SW / 2, y + 110);
        tft.drawString("MOSI=23  MISO=19  CLK=18  CS=5", SW / 2, y + 130);
        tft.drawString("Reboot after connecting", SW / 2, y + 160);
        tft.setTextPadding(0);
        return;
    }

    // SD available — show capture controls
    uint16_t statusCol = pcapRecording ? TFT_RED : TFT_DARKGREEN;
    const char* statusTxt = pcapRecording ? "RECORDING" : "STOPPED";
    tft.fillRoundRect(40, y, SW - 80, 50, 8, statusCol);
    tft.setTextDatum(MC_DATUM); tft.setTextFont(4); tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, statusCol);
    tft.drawString(statusTxt, SW / 2, y + 25);

    y += 70;
    tft.setTextFont(2); tft.setTextSize(1);
    tft.setTextDatum(TC_DATUM); tft.setTextPadding(SW - 20);
    tft.setTextColor(TFT_WHITE, BG);
    char st[48]; snprintf(st, sizeof(st), "Packets: %lu", (unsigned long)pcapPackets);
    tft.drawString(st, SW / 2, y);
    y += 24;
    char sz[48];
    if (pcapBytes < 1024) snprintf(sz, sizeof(sz), "Size: %lu B", (unsigned long)pcapBytes);
    else if (pcapBytes < 1048576) snprintf(sz, sizeof(sz), "Size: %.1f KB", pcapBytes / 1024.0f);
    else snprintf(sz, sizeof(sz), "Size: %.1f MB", pcapBytes / 1048576.0f);
    tft.drawString(sz, SW / 2, y);

    y += 40;
    tft.setTextColor(0x4208, BG);
    tft.drawString("Tap center to start/stop", SW / 2, y);
    tft.setTextPadding(0);
}

// ═══════════════════════════════════════════════════════════════════
// Screen 16 — BLE Scanner / Tracker Detector
// ═══════════════════════════════════════════════════════════════════
static bool bleScanStarted = false;
static unsigned long bleScanTime = 0;

void drawBLEScan(bool full) {
    int rowH = 20, hdrY = CONTENT_Y + 2;
    int maxRows = (CONTENT_H - 20) / rowH;
    int n = (int)nBLEDevs;

    // Auto-start BLE scan when entering this screen
    if (full && !bleActive) {
        pauseSniffer();
        bleInit();
        bleScanStarted = false;
    }

    // Start a scan every 8 seconds
    if (bleActive && (!bleScanStarted || millis() - bleScanTime > 8000)) {
        nBLEDevs = 0;
        bleStartScan(5);
        bleScanStarted = true;
        bleScanTime = millis();
    }

    if (full) {
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_CYAN, BG);
        tft.drawString("Name/MAC", 4, hdrY);
        tft.drawString("dBm", 220, hdrY);
        tft.drawString("Type", 268, hdrY);
        tft.drawFastHLine(2, hdrY + 10, SW - 4, 0x4208);
    }

    int y = hdrY + 14;
    n = (int)nBLEDevs;
    for (int r = 0; r < maxRows && r < n; r++) {
        BLEDevEntry& d = bleDevs[r];
        int ry = y + r * rowH;
        tft.fillRect(0, ry, SW, rowH, BG);
        tft.setTextFont(1); tft.setTextDatum(TL_DATUM);

        if (d.name[0]) {
            char nm[24]; strncpy(nm, d.name, 23); nm[23] = '\0';
            tft.setTextColor(TFT_GREEN, BG);
            tft.drawString(nm, 4, ry + 4);
        } else {
            char mc[18]; fmtMAC(mc, d.mac);
            tft.setTextColor(TFT_WHITE, BG);
            tft.drawString(mc, 4, ry + 4);
        }

        char rs[6]; snprintf(rs, sizeof(rs), "%d", d.rssi);
        tft.setTextColor(rssiColor(d.rssi), BG);
        tft.drawString(rs, 220, ry + 4);

        if (d.isTracker) {
            tft.setTextColor(TFT_RED, BG);
            tft.drawString("TRACK", 268, ry + 4);
        } else if (isLocallyAdminMAC(d.mac)) {
            tft.setTextColor(TFT_YELLOW, BG);
            tft.drawString("rand", 268, ry + 4);
        } else {
            tft.setTextColor(0x7BEF, BG);
            tft.drawString("BLE", 268, ry + 4);
        }
    }

    int usedH = y + min(maxRows, n) * rowH;
    if (usedH < CONTENT_Y + CONTENT_H) tft.fillRect(0, usedH, SW, CONTENT_Y + CONTENT_H - usedH, BG);

    if (n == 0 && bleActive) {
        tft.setTextDatum(MC_DATUM); tft.setTextFont(2);
        tft.setTextColor(0x7BEF, BG);
        tft.drawString("Scanning BLE...", SW / 2, CONTENT_Y + CONTENT_H / 2);
    }

    tft.setTextDatum(BR_DATUM); tft.setTextFont(1);
    tft.setTextColor(0x4208, BG); tft.setTextPadding(SW);
    char info[32]; snprintf(info, sizeof(info), "%d BLE devices  WiFi paused", n);
    tft.drawString(info, SW - 4, SH - NAV_H - 2);
    tft.setTextPadding(0);
}
