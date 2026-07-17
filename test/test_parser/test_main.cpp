#include <unity.h>
#include "parser.h"
#include <string.h>

// ═══════════════════════════════════════════════════════════════════
// Test frame data
// ═══════════════════════════════════════════════════════════════════

// Beacon: type=0 subtype=8, SSID="Test", channel=6, WPA2 (RSN IE present)
static const uint8_t beacon_frame[] = {
    0x80, 0x00,                                     // FC: mgmt/beacon
    0x00, 0x00,                                     // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,             // Addr1 (broadcast)
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,             // Addr2 (BSSID)
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,             // Addr3 (BSSID)
    0x00, 0x00,                                     // Seq
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,         // Timestamp
    0x64, 0x00,                                     // Beacon interval (100 TU)
    0x11, 0x00,                                     // Capability (ESS + Privacy)
    // Tagged params:
    0x00, 0x04, 'T','e','s','t',                    // SSID (id=0, len=4)
    0x03, 0x01, 0x06,                               // DS Param Set (channel 6)
    0x30, 0x06, 0x01,0x00, 0x00,0x0F,0xAC,0x04,     // RSN IE (WPA2)
};

// Beacon with empty SSID (hidden network), no RSN, privacy bit set → WEP
static const uint8_t hidden_wep_beacon[] = {
    0x80, 0x00, 0x00, 0x00,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x11,0x22,0x33,0x44,0x55,0x66,
    0x11,0x22,0x33,0x44,0x55,0x66,
    0x00, 0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xC8, 0x00,                                     // interval 200 TU
    0x10, 0x00,                                     // Capability: Privacy only
    // Tagged params:
    0x00, 0x00,                                     // SSID (empty)
    0x03, 0x01, 0x0B,                               // DS Param: channel 11
};

// Probe request: type=0 subtype=4, SSID="MyWiFi"
static const uint8_t probe_req[] = {
    0x40, 0x00,                                     // FC: mgmt/probe-req
    0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00,
    // Tagged params:
    0x00, 0x06, 'M','y','W','i','F','i',            // SSID
};

// Probe request with empty SSID (broadcast)
static const uint8_t probe_req_broadcast[] = {
    0x40, 0x00, 0x00, 0x00,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xAB,0xCD,0xEF,0x01,0x02,0x03,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x00, 0x00,
    0x00, 0x00,                                     // SSID tag, length 0
};

// Probe response: type=0 subtype=5
static const uint8_t probe_resp[] = {
    0x50, 0x00,                                     // FC: mgmt/probe-resp
    0x00, 0x00,
    0xDE,0xAD,0xBE,0xEF,0x01,0x02,
    0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,
    0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,
    0x00, 0x00,
    0x00, 0x06, 'S','e','c','r','e','t',
};

// Data frame: type=2 subtype=0
static const uint8_t data_frame[] = {
    0x08, 0x00,                                     // FC: data
    0x00, 0x00,
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
    0x00, 0x00,
};

// Data frame with retry flag set
static const uint8_t data_retry[] = {
    0x08, 0x08,                                     // FC: data + retry
    0x00, 0x00,
    0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,
    0x11,0x22,0x33,0x44,0x55,0x66,
    0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,
    0x00, 0x00,
};

// Deauth frame: type=0 subtype=12
static const uint8_t deauth_frame[] = {
    0xC0, 0x00,                                     // FC: mgmt/deauth
    0x00, 0x00,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66,             // Target
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,             // Source
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,             // BSSID
    0x00, 0x00,
    0x03, 0x00,                                     // Reason
};

// Control frame (ACK): type=1 subtype=13
static const uint8_t ack_frame[] = {
    0xD4, 0x00,                                     // FC: ctrl/ack
    0x00, 0x00,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66,             // Receiver
};

// EAPOL Message 1 (from AP, with PMKID)
static const uint8_t eapol_m1[] = {
    // MAC header (24 bytes) — data frame, from-DS
    0x08, 0x02, 0x00, 0x00,
    0x11,0x22,0x33,0x44,0x55,0x66,                   // Addr1 (client)
    0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,                   // Addr2 (AP/BSSID)
    0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,                   // Addr3 (BSSID)
    0x00, 0x00,
    // LLC/SNAP (8 bytes)
    0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8E,
    // EAPOL header (4 bytes)
    0x02, 0x03, 0x00, 0x75,                           // v2, EAPOL-Key, len=117
    // Key descriptor (starts at MAC+LLC+EAPOL = 24+8+4 = offset 36)
    0x02,                                             // RSN key descriptor
    0x00, 0x8A,                                       // Key Info: ACK + Pairwise + Version2 (Msg 1)
    0x00, 0x10,                                       // Key Length
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,           // Replay Counter
    // Key Nonce (32 bytes)
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
    0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,
    0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,
    // Key IV (16 bytes)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    // Key RSC (8 bytes)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    // Key ID (8 bytes)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    // Key MIC (16 bytes — all zeros for Msg 1)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    // Key Data Length (2 bytes)
    0x00, 0x16,                                       // 22 bytes
    // Key Data (PMKID KDE)
    0xDD, 0x14,                                       // type=vendor, len=20
    0x00, 0x0F, 0xAC, 0x04,                           // OUI 00:0F:AC, type 4 (PMKID)
    0xDE,0xAD,0xBE,0xEF,0xCA,0xFE,0xBA,0xBE,         // PMKID (16 bytes)
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
};

// ═══════════════════════════════════════════════════════════════════
// Frame classification tests
// ═══════════════════════════════════════════════════════════════════

void test_classify_beacon() {
    FrameInfo fi;
    TEST_ASSERT_TRUE(classifyFrame(beacon_frame, sizeof(beacon_frame), fi));
    TEST_ASSERT_EQUAL(P_BEACON, fi.proto);
    TEST_ASSERT_EQUAL(0, fi.type);
    TEST_ASSERT_EQUAL(8, fi.subtype);
    TEST_ASSERT_FALSE(fi.retry);
    TEST_ASSERT_FALSE(fi.isDeauth);
}

void test_classify_probe_req() {
    FrameInfo fi;
    TEST_ASSERT_TRUE(classifyFrame(probe_req, sizeof(probe_req), fi));
    TEST_ASSERT_EQUAL(P_PROBE, fi.proto);
    TEST_ASSERT_TRUE(fi.isProbeReq);
    TEST_ASSERT_FALSE(fi.isProbeResp);
}

void test_classify_probe_resp() {
    FrameInfo fi;
    TEST_ASSERT_TRUE(classifyFrame(probe_resp, sizeof(probe_resp), fi));
    TEST_ASSERT_EQUAL(P_PROBE, fi.proto);
    TEST_ASSERT_TRUE(fi.isProbeResp);
    TEST_ASSERT_FALSE(fi.isProbeReq);
}

void test_classify_data() {
    FrameInfo fi;
    TEST_ASSERT_TRUE(classifyFrame(data_frame, sizeof(data_frame), fi));
    TEST_ASSERT_EQUAL(P_DATA, fi.proto);
    TEST_ASSERT_EQUAL(2, fi.type);
    TEST_ASSERT_FALSE(fi.retry);
}

void test_classify_data_retry() {
    FrameInfo fi;
    TEST_ASSERT_TRUE(classifyFrame(data_retry, sizeof(data_retry), fi));
    TEST_ASSERT_EQUAL(P_DATA, fi.proto);
    TEST_ASSERT_TRUE(fi.retry);
}

void test_classify_deauth() {
    FrameInfo fi;
    TEST_ASSERT_TRUE(classifyFrame(deauth_frame, sizeof(deauth_frame), fi));
    TEST_ASSERT_EQUAL(P_AUTH, fi.proto);
    TEST_ASSERT_TRUE(fi.isDeauth);
}

void test_classify_ack() {
    FrameInfo fi;
    TEST_ASSERT_TRUE(classifyFrame(ack_frame, sizeof(ack_frame), fi));
    TEST_ASSERT_EQUAL(P_CTRL, fi.proto);
    TEST_ASSERT_EQUAL(1, fi.type);
}

void test_classify_too_short() {
    uint8_t tiny[] = {0x80, 0x00, 0x00};
    FrameInfo fi;
    TEST_ASSERT_FALSE(classifyFrame(tiny, 3, fi));
}

void test_classify_null() {
    FrameInfo fi;
    TEST_ASSERT_FALSE(classifyFrame(NULL, 0, fi));
}

// ═══════════════════════════════════════════════════════════════════
// Beacon parsing tests
// ═══════════════════════════════════════════════════════════════════

void test_beacon_ssid() {
    BeaconData bd;
    TEST_ASSERT_TRUE(parseBeaconTags(beacon_frame, sizeof(beacon_frame), bd));
    TEST_ASSERT_TRUE(bd.hasSSID);
    TEST_ASSERT_EQUAL_STRING("Test", bd.ssid);
}

void test_beacon_channel() {
    BeaconData bd;
    TEST_ASSERT_TRUE(parseBeaconTags(beacon_frame, sizeof(beacon_frame), bd));
    TEST_ASSERT_EQUAL(6, bd.channel);
}

void test_beacon_interval() {
    BeaconData bd;
    TEST_ASSERT_TRUE(parseBeaconTags(beacon_frame, sizeof(beacon_frame), bd));
    TEST_ASSERT_EQUAL(100, bd.interval);
}

void test_beacon_wpa2() {
    BeaconData bd;
    TEST_ASSERT_TRUE(parseBeaconTags(beacon_frame, sizeof(beacon_frame), bd));
    TEST_ASSERT_EQUAL(3, bd.encryption);
}

void test_beacon_hidden_wep() {
    BeaconData bd;
    TEST_ASSERT_TRUE(parseBeaconTags(hidden_wep_beacon, sizeof(hidden_wep_beacon), bd));
    TEST_ASSERT_FALSE(bd.hasSSID);
    TEST_ASSERT_EQUAL(11, bd.channel);
    TEST_ASSERT_EQUAL(200, bd.interval);
    TEST_ASSERT_EQUAL(1, bd.encryption);  // WEP (privacy bit, no RSN/WPA)
}

void test_beacon_too_short() {
    uint8_t tiny[20] = {};
    BeaconData bd;
    TEST_ASSERT_FALSE(parseBeaconTags(tiny, 20, bd));
}

// ═══════════════════════════════════════════════════════════════════
// Probe parsing tests
// ═══════════════════════════════════════════════════════════════════

void test_probe_req_ssid() {
    ProbeData pd;
    TEST_ASSERT_TRUE(parseProbe(probe_req, sizeof(probe_req), pd));
    TEST_ASSERT_TRUE(pd.isRequest);
    TEST_ASSERT_TRUE(pd.hasSSID);
    TEST_ASSERT_EQUAL_STRING("MyWiFi", pd.ssid);
}

void test_probe_broadcast() {
    ProbeData pd;
    TEST_ASSERT_TRUE(parseProbe(probe_req_broadcast, sizeof(probe_req_broadcast), pd));
    TEST_ASSERT_TRUE(pd.isRequest);
    TEST_ASSERT_FALSE(pd.hasSSID);
}

void test_probe_resp_parse() {
    ProbeData pd;
    TEST_ASSERT_TRUE(parseProbe(probe_resp, sizeof(probe_resp), pd));
    TEST_ASSERT_FALSE(pd.isRequest);
    TEST_ASSERT_TRUE(pd.hasSSID);
    TEST_ASSERT_EQUAL_STRING("Secret", pd.ssid);
}

// ═══════════════════════════════════════════════════════════════════
// EAPOL parsing tests
// ═══════════════════════════════════════════════════════════════════

void test_eapol_msg1_detection() {
    EAPOLData ed;
    TEST_ASSERT_TRUE(parseEAPOL(eapol_m1, sizeof(eapol_m1), ed));
    TEST_ASSERT_EQUAL(1, ed.msgNum);
}

void test_eapol_pmkid_extraction() {
    EAPOLData ed;
    TEST_ASSERT_TRUE(parseEAPOL(eapol_m1, sizeof(eapol_m1), ed));
    TEST_ASSERT_TRUE(ed.hasPMKID);
    uint8_t expected[] = {0xDE,0xAD,0xBE,0xEF,0xCA,0xFE,0xBA,0xBE,
                          0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
    TEST_ASSERT_EQUAL_MEMORY(expected, ed.pmkid, 16);
}

void test_eapol_not_eapol() {
    EAPOLData ed;
    TEST_ASSERT_FALSE(parseEAPOL(data_frame, sizeof(data_frame), ed));
}

void test_eapol_beacon_rejected() {
    EAPOLData ed;
    TEST_ASSERT_FALSE(parseEAPOL(beacon_frame, sizeof(beacon_frame), ed));
}

// ═══════════════════════════════════════════════════════════════════
// MAC address tests
// ═══════════════════════════════════════════════════════════════════

void test_locally_admin_mac() {
    uint8_t random_mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02};
    TEST_ASSERT_TRUE(isLocallyAdminMAC(random_mac));  // 0xDE & 0x02 = 0x02
}

void test_global_mac() {
    uint8_t global_mac[] = {0xAC, 0xDE, 0x48, 0x00, 0x11, 0x22};
    TEST_ASSERT_FALSE(isLocallyAdminMAC(global_mac)); // 0xAC & 0x02 = 0x00
}

void test_null_mac() {
    TEST_ASSERT_FALSE(isLocallyAdminMAC(NULL));
}

// ═══════════════════════════════════════════════════════════════════
// Address extraction tests
// ═══════════════════════════════════════════════════════════════════

void test_frame_addresses() {
    const uint8_t* a1 = frameAddr1(beacon_frame);
    TEST_ASSERT_EQUAL(0xFF, a1[0]);  // broadcast

    const uint8_t* a2 = frameAddr2(beacon_frame);
    TEST_ASSERT_EQUAL(0xAA, a2[0]);
    TEST_ASSERT_EQUAL(0xBB, a2[1]);

    const uint8_t* a3 = frameAddr3(beacon_frame);
    TEST_ASSERT_EQUAL(0xAA, a3[0]);
}

// ═══════════════════════════════════════════════════════════════════
// Runner
// ═══════════════════════════════════════════════════════════════════

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char** argv) {
    UNITY_BEGIN();

    // Classification
    RUN_TEST(test_classify_beacon);
    RUN_TEST(test_classify_probe_req);
    RUN_TEST(test_classify_probe_resp);
    RUN_TEST(test_classify_data);
    RUN_TEST(test_classify_data_retry);
    RUN_TEST(test_classify_deauth);
    RUN_TEST(test_classify_ack);
    RUN_TEST(test_classify_too_short);
    RUN_TEST(test_classify_null);

    // Beacon parsing
    RUN_TEST(test_beacon_ssid);
    RUN_TEST(test_beacon_channel);
    RUN_TEST(test_beacon_interval);
    RUN_TEST(test_beacon_wpa2);
    RUN_TEST(test_beacon_hidden_wep);
    RUN_TEST(test_beacon_too_short);

    // Probe parsing
    RUN_TEST(test_probe_req_ssid);
    RUN_TEST(test_probe_broadcast);
    RUN_TEST(test_probe_resp_parse);

    // EAPOL
    RUN_TEST(test_eapol_msg1_detection);
    RUN_TEST(test_eapol_pmkid_extraction);
    RUN_TEST(test_eapol_not_eapol);
    RUN_TEST(test_eapol_beacon_rejected);

    // MAC
    RUN_TEST(test_locally_admin_mac);
    RUN_TEST(test_global_mac);
    RUN_TEST(test_null_mac);

    // Addresses
    RUN_TEST(test_frame_addresses);

    return UNITY_END();
}
