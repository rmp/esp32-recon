#pragma once

void bleInit();
void bleDeinit();
bool bleIsActive();
void bleStartScan(int durationSec);
bool bleScanComplete();
