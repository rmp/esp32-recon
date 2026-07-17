#pragma once

struct Screen {
    const char* name;
    void (*draw)(bool full);
};

// Existing screens (screens.cpp)
void drawProtocol(bool full);
void drawChannels(bool full);
void drawAPScan(bool full);
void drawSignal(bool full);
void drawDevices(bool full);
void drawTraffic(bool full);
void drawDeauth(bool full);
void drawBeacons(bool full);
void drawRetry(bool full);

// Passive recon screens (recon_screens.cpp)
void drawProbeLog(bool full);
void drawHiddenSSID(bool full);
void drawHandshake(bool full);
void drawPMKID(bool full);
void drawMACRandom(bool full);
void drawOUIScreen(bool full);
void drawPCAPScreen(bool full);
void drawBLEScan(bool full);

#define NUM_SCREENS 17

extern Screen screens[NUM_SCREENS];
