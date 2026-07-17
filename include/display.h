#pragma once
#include <stdint.h>

void initDisplay();
void drawHeader();
void drawStatBar();
void drawNav();
void clearContent();

void fmtMAC(char* buf, const uint8_t* mac);
void fmtMACShort(char* buf, const uint8_t* mac);
uint16_t rssiColor(int8_t rssi);
uint16_t heatColor(float t);
const char* encStr(uint8_t enc);
