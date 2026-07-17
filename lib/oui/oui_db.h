#pragma once
#include <stdint.h>

struct OUIEntry {
    uint8_t    prefix[3];
    const char* vendor;
};

const char* ouiLookup(const uint8_t* mac);
int         ouiCount();
