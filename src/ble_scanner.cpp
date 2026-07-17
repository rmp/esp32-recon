#include "ble_scanner.h"
#include "globals.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

static BLEScan* pScan = nullptr;
static bool scanning = false;

class ScanCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice dev) override {
        const uint8_t* addr = *dev.getAddress().getNative();

        // Check for existing entry
        for (int i = 0; i < nBLEDevs; i++) {
            if (memcmp(bleDevs[i].mac, addr, 6) == 0) {
                bleDevs[i].rssi = dev.getRSSI();
                bleDevs[i].lastSeen = millis();
                if (dev.haveName()) {
                    strncpy(bleDevs[i].name, dev.getName().c_str(), 31);
                    bleDevs[i].name[31] = '\0';
                }
                return;
            }
        }
        if (nBLEDevs >= MAX_BLE_DEVS) return;

        BLEDevEntry& d = bleDevs[nBLEDevs];
        memcpy(d.mac, addr, 6);
        d.rssi = dev.getRSSI();
        d.addrType = dev.getAddressType();
        d.lastSeen = millis();
        d.isTracker = false;

        if (dev.haveName()) {
            strncpy(d.name, dev.getName().c_str(), 31);
            d.name[31] = '\0';
        } else {
            d.name[0] = '\0';
        }

        // Heuristic tracker detection: known tracker manufacturer data
        if (dev.haveManufacturerData()) {
            std::string mfr = dev.getManufacturerData();
            if (mfr.size() >= 2) {
                uint16_t cid = (uint8_t)mfr[0] | ((uint8_t)mfr[1] << 8);
                // Apple (0x004C) with specific subtypes = AirTag/FindMy
                if (cid == 0x004C && mfr.size() >= 3 && (uint8_t)mfr[2] == 0x12)
                    d.isTracker = true;
                // Tile (0x000D)
                if (cid == 0x000D) d.isTracker = true;
            }
        }
        nBLEDevs++;
    }
};

static ScanCallbacks scanCB;

void bleInit() {
    if (bleActive) return;
    BLEDevice::init("");
    pScan = BLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(&scanCB, false);
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(99);
    bleActive = true;
}

void bleDeinit() {
    if (!bleActive) return;
    if (pScan && scanning) pScan->stop();
    scanning = false;
    BLEDevice::deinit(false);
    pScan = nullptr;
    bleActive = false;
}

bool bleIsActive() { return bleActive; }

void bleStartScan(int dur) {
    if (!bleActive || !pScan) return;
    scanning = true;
    pScan->start(dur, false);
    scanning = false;
}

bool bleScanComplete() {
    return !scanning;
}
