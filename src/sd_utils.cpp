#include "sd_utils.h"
#include "globals.h"
#include <SPI.h>
#include <SD.h>

static SPIClass vspi(VSPI);
static File pcapFile;

// PCAP global header: magic, v2.4, GMT, snaplen=2500, linktype=105 (802.11)
static const uint8_t PCAP_HDR[] = {
    0xD4,0xC3,0xB2,0xA1,  // magic (little-endian)
    0x02,0x00,0x04,0x00,  // version 2.4
    0x00,0x00,0x00,0x00,  // GMT offset
    0x00,0x00,0x00,0x00,  // sigfigs
    0xC4,0x09,0x00,0x00,  // snaplen 2500
    0x69,0x00,0x00,0x00,  // linktype 105 = IEEE 802.11
};

void sdInit() {
    vspi.begin(18, 19, 23, SD_CS_PIN);
    sdAvailable = SD.begin(SD_CS_PIN, vspi);
    if (sdAvailable) {
        Serial.println("SD card detected");
    } else {
        Serial.println("SD card not detected");
    }
}

bool sdIsAvailable() {
    return sdAvailable;
}

bool pcapStart() {
    if (!sdAvailable) return false;
    char fname[32];
    int n = 0;
    do {
        snprintf(fname, sizeof(fname), "/capture_%03d.pcap", n++);
    } while (SD.exists(fname) && n < 999);

    pcapFile = SD.open(fname, FILE_WRITE);
    if (!pcapFile) return false;

    pcapFile.write(PCAP_HDR, sizeof(PCAP_HDR));
    pcapRecording = true;
    pcapPackets = 0;
    pcapBytes = sizeof(PCAP_HDR);
    Serial.printf("PCAP recording to %s\n", fname);
    return true;
}

void pcapStop() {
    if (pcapFile) pcapFile.close();
    pcapRecording = false;
    Serial.printf("PCAP stopped: %lu packets, %lu bytes\n",
                  (unsigned long)pcapPackets, (unsigned long)pcapBytes);
}

void pcapWritePacket(const uint8_t* data, uint16_t len, uint32_t tsSec, uint32_t tsUsec) {
    if (!pcapRecording || !pcapFile) return;
    uint16_t capLen = len > 2500 ? 2500 : len;

    uint8_t hdr[16];
    hdr[0]  = tsSec & 0xFF;        hdr[1]  = (tsSec >> 8) & 0xFF;
    hdr[2]  = (tsSec >> 16) & 0xFF; hdr[3]  = (tsSec >> 24) & 0xFF;
    hdr[4]  = tsUsec & 0xFF;       hdr[5]  = (tsUsec >> 8) & 0xFF;
    hdr[6]  = (tsUsec >> 16) & 0xFF; hdr[7]  = (tsUsec >> 24) & 0xFF;
    hdr[8]  = capLen & 0xFF;       hdr[9]  = (capLen >> 8) & 0xFF;
    hdr[10] = 0; hdr[11] = 0;
    hdr[12] = len & 0xFF;          hdr[13] = (len >> 8) & 0xFF;
    hdr[14] = 0; hdr[15] = 0;

    pcapFile.write(hdr, 16);
    pcapFile.write(data, capLen);
    pcapFile.flush();
    pcapPackets++;
    pcapBytes += 16 + capLen;
}
