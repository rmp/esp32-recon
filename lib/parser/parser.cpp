#include "parser.h"
#include <string.h>

const uint8_t* frameAddr1(const uint8_t* f) { return f + 4; }
const uint8_t* frameAddr2(const uint8_t* f) { return f + 10; }
const uint8_t* frameAddr3(const uint8_t* f) { return f + 16; }

bool classifyFrame(const uint8_t* f, uint16_t len, FrameInfo& out) {
    if (!f || len < 10) return false;
    memset(&out, 0, sizeof(out));
    out.type    = (f[0] >> 2) & 3;
    out.subtype = (f[0] >> 4) & 0xF;
    out.retry   = (f[1] & 0x08) != 0;

    switch (out.type) {
    case 0: // Management
        switch (out.subtype) {
        case 8:            out.proto = P_BEACON; break;
        case 4:            out.proto = P_PROBE; out.isProbeReq  = true; break;
        case 5:            out.proto = P_PROBE; out.isProbeResp = true; break;
        case 12:           out.proto = P_AUTH;  out.isDeauth    = true; break;
        case 10:           out.proto = P_AUTH;  break; // disassoc
        case 0: case 1:    // assoc req/resp
        case 2: case 3:    // reassoc req/resp
        case 11:           out.proto = P_AUTH;  break;
        default:           out.proto = P_OTHER;
        }
        break;
    case 1: out.proto = P_CTRL; break;
    case 2: out.proto = P_DATA; break;
    default: out.proto = P_OTHER;
    }
    return true;
}

bool parseBeaconTags(const uint8_t* f, uint16_t len, BeaconData& out) {
    if (!f || len < 36) return false;
    memset(&out, 0, sizeof(out));

    out.interval = f[32] | (f[33] << 8);
    uint16_t cap = f[34] | (f[35] << 8);

    bool gotE = false;
    int o = 36;
    while (o + 2 <= (int)len) {
        uint8_t id = f[o], tl = f[o + 1];
        if (o + 2 + tl > (int)len) break;
        if (id == 0 && tl > 0 && tl <= 32) {
            memcpy(out.ssid, f + o + 2, tl);
            out.ssid[tl] = '\0';
            out.hasSSID = true;
        } else if (id == 3 && tl >= 1) {
            out.channel = f[o + 2];
        } else if (id == 48) {
            out.encryption = 3; gotE = true;   // RSN IE → WPA2
        } else if (id == 221 && tl >= 4 &&
                   f[o+2]==0x00 && f[o+3]==0x50 && f[o+4]==0xF2 && f[o+5]==0x01) {
            if (out.encryption < 2) out.encryption = 2; gotE = true; // WPA
        }
        o += 2 + tl;
    }
    if (!gotE && (cap & 0x0010)) out.encryption = 1; // Privacy → WEP
    return true;
}

bool parseProbe(const uint8_t* f, uint16_t len, ProbeData& out) {
    if (!f || len < 24) return false;
    memset(&out, 0, sizeof(out));
    out.isRequest = ((f[0] >> 4) & 0xF) == 4;

    int o = 24;
    while (o + 2 <= (int)len) {
        uint8_t id = f[o], tl = f[o + 1];
        if (o + 2 + tl > (int)len) break;
        if (id == 0) {
            if (tl > 0 && tl <= 32) {
                memcpy(out.ssid, f + o + 2, tl);
                out.ssid[tl] = '\0';
                out.hasSSID = true;
            }
            break;
        }
        o += 2 + tl;
    }
    return true;
}

bool parseEAPOL(const uint8_t* f, uint16_t len, EAPOLData& out) {
    if (!f) return false;
    memset(&out, 0, sizeof(out));

    uint8_t fst = (f[0] >> 4) & 0xF;
    int hdr = (fst >= 8) ? 26 : 24;  // QoS data → 26-byte MAC header

    if ((int)len < hdr + 8 + 4) return false;

    // LLC/SNAP: AA AA 03 00 00 00  +  EtherType 88 8E
    if (f[hdr]   != 0xAA || f[hdr+1] != 0xAA || f[hdr+2] != 0x03) return false;
    if (f[hdr+3] != 0x00 || f[hdr+4] != 0x00 || f[hdr+5] != 0x00) return false;
    if (f[hdr+6] != 0x88 || f[hdr+7] != 0x8E) return false;

    int eo = hdr + 8;  // EAPOL header start
    if ((int)len < eo + 4) return false;
    if (f[eo + 1] != 3) return false;  // type must be EAPOL-Key

    int ko = eo + 4;   // key descriptor start
    if ((int)len < ko + 3) return false;

    uint16_t keyInfo = (f[ko + 1] << 8) | f[ko + 2];
    bool ack     = (keyInfo & 0x0080) != 0;
    bool mic     = (keyInfo & 0x0100) != 0;
    bool secure  = (keyInfo & 0x0200) != 0;
    bool install = (keyInfo & 0x0040) != 0;

    if      ( ack && !mic)                  out.msgNum = 1;
    else if (!ack &&  mic && !secure)       out.msgNum = 2;
    else if ( ack &&  mic &&  install)      out.msgNum = 3;
    else if (!ack &&  mic &&  secure)       out.msgNum = 4;
    else return false;

    // Extract PMKID from Message 1 key data
    if (out.msgNum == 1 && (int)len >= ko + 95 + 2) {
        uint16_t kdLen = (f[ko + 93] << 8) | f[ko + 94];
        int kdOfs = ko + 95;
        if (kdLen > 0 && (int)len >= kdOfs + (int)kdLen) {
            int p = kdOfs;
            while (p + 2 <= kdOfs + (int)kdLen) {
                uint8_t kdeType = f[p];
                uint8_t kdeLen  = f[p + 1];
                if (p + 2 + kdeLen > kdOfs + (int)kdLen) break;
                if (kdeType == 0xDD && kdeLen >= 20 &&
                    f[p+2]==0x00 && f[p+3]==0x0F && f[p+4]==0xAC && f[p+5]==0x04) {
                    memcpy(out.pmkid, f + p + 6, 16);
                    out.hasPMKID = true;
                    break;
                }
                p += 2 + kdeLen;
            }
        }
    }
    return true;
}

bool isLocallyAdminMAC(const uint8_t* mac) {
    return mac && (mac[0] & 0x02) != 0;
}
