#pragma once
#include <stdint.h>

void   sdInit();
bool   sdIsAvailable();
bool   pcapStart();
void   pcapStop();
void   pcapWritePacket(const uint8_t* data, uint16_t len, uint32_t tsSec, uint32_t tsUsec);
