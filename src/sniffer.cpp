#include "sniffer.h"
#include "globals.h"
#include "parser.h"
#include "oui_db.h"
#include <WiFi.h>
#include "esp_wifi.h"

static int findAP(const uint8_t* bssid) {
    for (int i = 0; i < nAps; i++)
        if (memcmp(aps[i].bssid, bssid, 6) == 0) return i;
    return -1;
}

static void handleBeacon(const uint8_t* f, uint16_t len, int8_t rssi) {
    if (len < 36) return;
    const uint8_t* bssid = f + 16;
    int idx = findAP(bssid);
    bool isNew = idx < 0;
    if (isNew) {
        if (nAps >= MAX_APS) return;
        idx = nAps++;
        memset(&aps[idx], 0, sizeof(APInfo));
        memcpy(aps[idx].bssid, bssid, 6);
    }
    APInfo& ap = aps[idx];
    ap.rssi = rssi;
    ap.channel = curCh;
    ap.beaconIntvl = f[32] | (f[33] << 8);
    ap.lastSeen = millis();
    ap.beaconCount++;

    if (isNew) {
        BeaconData bd;
        if (parseBeaconTags(f, len, bd)) {
            if (bd.hasSSID) {
                memcpy(ap.ssid, bd.ssid, 33);
            } else {
                ap.hidden = true;
            }
            if (bd.channel) ap.channel = bd.channel;
            ap.encryption = bd.encryption;
        }
    }
    if (trackedAP >= 0 && idx == trackedAP) trackedRSSI = rssi;
}

static void recordDeauth(const uint8_t* f, uint16_t len) {
    if (len < 24) return;
    DeauthEvt& e = deauthBuf[deauthHead];
    memcpy(e.dst, f + 4, 6);
    memcpy(e.src, f + 10, 6);
    e.channel = curCh;
    e.ts = millis();
    deauthHead = (deauthHead + 1) % MAX_DEAUTH;
    deauthTotal++;
}

static void trackDevice(const uint8_t* mac, int8_t rssi) {
    for (int i = 0; i < nDevs; i++) {
        if (memcmp(devs[i].mac, mac, 6) == 0) {
            devs[i].rssi = rssi;
            devs[i].channel = curCh;
            devs[i].lastSeen = millis();
            devs[i].pktCount++;
            return;
        }
    }
    if (nDevs >= MAX_DEVS) return;
    DevInfo& d = devs[nDevs++];
    memcpy(d.mac, mac, 6);
    d.rssi = rssi;
    d.channel = curCh;
    d.firstSeen = d.lastSeen = millis();
    d.pktCount = 1;
}

static void handleProbeReq(const uint8_t* f, uint16_t len, int8_t rssi) {
    ProbeData pd;
    if (!parseProbe(f, len, pd)) return;
    const uint8_t* mac = f + 10;

    ProbeReq& pr = probes[probeHead];
    memcpy(pr.mac, mac, 6);
    if (pd.hasSSID) memcpy(pr.ssid, pd.ssid, 33);
    else pr.ssid[0] = '\0';
    pr.rssi = rssi;
    pr.channel = curCh;
    pr.timestamp = millis();
    pr.randomMAC = isLocallyAdminMAC(mac);
    probeHead = (probeHead + 1) % MAX_PROBES;
    if (probeCount < MAX_PROBES) probeCount++;
}

static void handleProbeResp(const uint8_t* f, uint16_t len) {
    ProbeData pd;
    if (!parseProbe(f, len, pd) || !pd.hasSSID) return;
    const uint8_t* bssid = f + 10;
    int idx = findAP(bssid);
    if (idx >= 0 && aps[idx].hidden && aps[idx].revealedSSID[0] == '\0') {
        memcpy(aps[idx].revealedSSID, pd.ssid, 33);
    }
}

static void handleEAPOL(const uint8_t* f, uint16_t len, int8_t rssi) {
    EAPOLData ed;
    if (!parseEAPOL(f, len, ed)) return;

    const uint8_t* addr1 = f + 4;
    const uint8_t* addr2 = f + 10;
    const uint8_t* bssid;
    const uint8_t* client;

    uint8_t toDS   = f[1] & 0x01;
    uint8_t fromDS = f[1] & 0x02;
    if (toDS && !fromDS)      { bssid = addr1; client = addr2; }
    else if (!toDS && fromDS) { bssid = addr2; client = addr1; }
    else return;

    // PMKID
    if (ed.hasPMKID && nPMKIDs < MAX_PMKIDS) {
        bool dup = false;
        for (int i = 0; i < nPMKIDs; i++) {
            if (memcmp(pmkids[i].bssid, bssid, 6) == 0 &&
                memcmp(pmkids[i].client, client, 6) == 0) { dup = true; break; }
        }
        if (!dup) {
            PMKIDEntry& pe = pmkids[nPMKIDs++];
            memcpy(pe.bssid, bssid, 6);
            memcpy(pe.client, client, 6);
            memcpy(pe.pmkid, ed.pmkid, 16);
            pe.timestamp = millis();
            int ap = findAP(bssid);
            if (ap >= 0) memcpy(pe.ssid, aps[ap].ssid, 33);
            else pe.ssid[0] = '\0';
        }
    }

    // Handshake tracking
    int hi = -1;
    for (int i = 0; i < nHandshakes; i++) {
        if (memcmp(handshakes[i].bssid, bssid, 6) == 0 &&
            memcmp(handshakes[i].client, client, 6) == 0) { hi = i; break; }
    }
    if (hi < 0) {
        if (nHandshakes >= MAX_HANDSHAKES) return;
        hi = nHandshakes++;
        memset(&handshakes[hi], 0, sizeof(HandshakeState));
        memcpy(handshakes[hi].bssid, bssid, 6);
        memcpy(handshakes[hi].client, client, 6);
        int ap = findAP(bssid);
        if (ap >= 0) memcpy(handshakes[hi].ssid, aps[ap].ssid, 33);
    }
    handshakes[hi].captured |= (1 << (ed.msgNum - 1));
    handshakes[hi].lastUpdate = millis();
}

static void IRAM_ATTR snifferCB(void* buf, wifi_promiscuous_pkt_type_t /*type*/) {
    auto* pkt = (wifi_promiscuous_pkt_t*)buf;
    const uint8_t* p = pkt->payload;
    uint16_t len = pkt->rx_ctrl.sig_len;
    int8_t rssi = pkt->rx_ctrl.rssi;

    FrameInfo fi;
    if (!classifyFrame(p, len, fi)) return;

    if (curCh >= 1 && curCh <= 13) {
        cstat[curCh].total++;
        cstat[curCh].rssiSum += rssi;
        cstat[curCh].rssiN++;
        if (fi.retry) cstat[curCh].retries++;
    }
    if (fi.retry) totalRetry++;

    switch (fi.type) {
    case 0: // Management
        if (fi.proto == P_BEACON) handleBeacon(p, len, rssi);
        else if (fi.isDeauth)    recordDeauth(p, len);
        if (fi.isProbeReq)       handleProbeReq(p, len, rssi);
        else if (fi.isProbeResp) handleProbeResp(p, len);
        if (len >= 16) trackDevice(p + 10, rssi);
        break;
    case 1: // Control
        break;
    case 2: // Data
        if (len >= 16) trackDevice(p + 10, rssi);
        handleEAPOL(p, len, rssi);
        break;
    }

    cnt[fi.proto]++;
    totalPkt++;

    // PCAP capture
    if (pcapRecording && sdAvailable) {
        extern void pcapWritePacket(const uint8_t*, uint16_t, uint32_t, uint32_t);
        unsigned long ms = millis();
        pcapWritePacket(p, len, ms / 1000, (ms % 1000) * 1000);
    }
}

void initSniffer() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    wifi_promiscuous_filter_t f = {};
    f.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT |
                    WIFI_PROMIS_FILTER_MASK_DATA  |
                    WIFI_PROMIS_FILTER_MASK_CTRL;
    esp_wifi_set_promiscuous_filter(&f);
    esp_wifi_set_promiscuous_rx_cb(snifferCB);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(curCh, WIFI_SECOND_CHAN_NONE);
}

void hopChannel() {
    curCh = curCh >= CH_MAX ? CH_MIN : curCh + 1;
    esp_wifi_set_channel(curCh, WIFI_SECOND_CHAN_NONE);
}

void pauseSniffer() {
    esp_wifi_set_promiscuous(false);
}

void resumeSniffer() {
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(curCh, WIFI_SECOND_CHAN_NONE);
}
