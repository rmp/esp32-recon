#pragma once

// Channel hopping
#define CH_MIN   1
#define CH_MAX   13
#define HOP_MS   150
#define DISP_MS  500
#define STAT_MS  1000
#define HIST_LEN 60

// Buffer sizes
#define MAX_APS        50
#define MAX_DEVS       200
#define MAX_DEAUTH     48
#define MAX_PROBES     128
#define MAX_HANDSHAKES 16
#define MAX_PMKIDS     32
#define MAX_BLE_DEVS   64

// Screen geometry — 320x480 portrait
#define SW         320
#define SH         480
#define HDR_H      34
#define STAT_H     18
#define CONTENT_Y  (HDR_H + STAT_H)
#define NAV_H      20
#define CONTENT_H  (SH - CONTENT_Y - NAV_H)

// Protocol bar layout (screen 0)
#define BAR_Y0    (CONTENT_Y + 6)
#define BAR_H     30
#define BAR_GAP   4
#define LABEL_W   66
#define COUNT_W   68
#define BAR_X     (LABEL_W + 2)
#define BAR_MAXW  (SW - BAR_X - COUNT_W - 6)

// Sparkline
#define SPARK_Y   (BAR_Y0 + 6 * (BAR_H + BAR_GAP) + 20)
#define SPARK_X   8
#define SPARK_W   (SW - 16)
#define SPARK_H   (SH - NAV_H - SPARK_Y - 4)

// SD card (VSPI bus — display uses HSPI)
#define SD_CS_PIN 5
