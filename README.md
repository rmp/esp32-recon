# WiFi Network Diagnostics Tool

Portable 17-screen wireless network analyser and passive reconnaissance suite running on an ESP32E with a 320x480 TFT touchscreen. Captures all 802.11 frames via promiscuous mode, channel-hops across 2.4 GHz channels 1-13, and presents real-time visualisations on the display.

## Hardware

- **MCU**: ESP32-D0WD-V3 (dual-core 240 MHz, 320 KB SRAM, 4 MB flash)
- **Display**: 4" 320x480 TFT (ILI9488 via HSPI)
- **Touch**: XPT2046 resistive (CS=33)
- **USB**: CH340 serial (`/dev/tty.wchusbserial1132440`)
- **Optional**: SD card module on VSPI (MOSI=23, MISO=19, CLK=18, CS=5) for PCAP capture

## Build

Requires [PlatformIO](https://platformio.org/).

```
make              # Build firmware
make upload       # Build and flash
make monitor      # Serial console (115200 baud)
make deploy       # Build, flash, and open monitor
make test         # Run native unit tests (no hardware needed)
make size         # Show RAM/flash usage
make erase        # Erase entire flash
make clean        # Remove build artifacts
```

## Navigation

- **Touch**: Tap left edge (<50 px) for previous screen, right edge (>270 px) for next
- **Serial**: `n` / `]` next, `p` / `[` previous, `1`-`9` jump to screen, `0` = screen 10

## Screens

### Diagnostics (1-9)

1. **Protocol Monitor** — Frame-type breakdown as horizontal bars (Beacon, Data, Probe, Control, Auth, Other) with live packet counts and percentages. Stacked-column sparkline shows protocol composition over the last 60 seconds.
2. **Channel Heatmap** — Vertical bars for each channel (1-13) coloured by traffic intensity (green to red). Shows packet count per channel and average RSSI.
3. **AP Scanner** — Sorted list of discovered access points by signal strength. Shows SSID, RSSI, channel, and encryption type (OPEN/WEP/WPA/WPA2). Scrollable via touch.
4. **Signal Tracker** — Tracks a selected AP with a large RSSI readout, signal quality indicator (Excellent/Good/Fair/Weak), and a 60-second RSSI line chart. Tap to cycle through APs.
5. **Device Activity** — List of all unique MAC addresses sorted by last-seen time. Shows RSSI, channel, packet count, and time since last activity.
6. **Traffic per Channel** — Horizontal bar chart showing cumulative packet volume per channel. Uses rainbow colour coding for 13 channels.
7. **Deauth Detector** — Wireless intrusion detection. Displays SAFE/WARNING/ALERT banner based on deauth frame rate, with event log showing source/target MAC and channel.
8. **Beacon Monitor** — Beacon interval analysis. Flags DUPE! (multiple BSSIDs sharing SSID — potential evil twin), odd BI (non-standard interval), or OK.
9. **Retry/Error Rate** — RF quality indicator. Shows overall retry percentage and per-channel retry rate as colour-coded bars.

### Passive Reconnaissance (10-17)

10. **Probe Request Logger** — Captures probe request frames showing which SSIDs nearby devices are seeking. Highlights randomised MACs in yellow.
11. **Hidden SSID Revealer** — Identifies hidden networks from beacons with empty SSIDs, then correlates probe responses to reveal the actual network name.
12. **WPA Handshake Capture** — Monitors for EAPOL 4-way handshake messages. Shows capture progress (1-2-3-4) per AP/client pair. Complete handshakes highlighted green.
13. **PMKID Harvester** — Extracts PMKID from the first EAPOL message key data. Only needs a single frame — no client interaction required.
14. **MAC Randomization Detector** — Identifies devices using locally-administered (randomised) MAC addresses. Shows percentage breakdown and lists all random MACs.
15. **OUI Vendor Lookup** — Identifies device manufacturers from MAC address prefixes using an embedded 200+ entry OUI database. Scrollable device list with vendor names.
16. **PCAP Capture** — Writes raw 802.11 frames to SD card in PCAP format for offline Wireshark analysis. **Requires SD card module** — shows wiring instructions if unavailable. Tap to start/stop recording.
17. **BLE Scanner / Tracker Detector** — Scans for Bluetooth Low Energy devices. Detects AirTags and Tile trackers via manufacturer data. WiFi sniffing pauses while BLE scanning is active.

## Architecture

```
lib/
  parser/           Pure frame parsing (testable natively)
    parser.h/cpp    Frame classification, beacon/probe/EAPOL parsing,
                    PMKID extraction, MAC randomisation detection
  oui/              OUI vendor database
    oui_db.h/cpp    200+ entries, binary search lookup

include/
  config.h          Constants (timing, buffer sizes, screen geometry)
  types.h           Data structures (APInfo, DevInfo, ProbeReq, etc.)
  globals.h         Shared state declarations
  screen.h          Screen struct and draw function declarations
  sniffer.h         WiFi sniffer API
  display.h         Common display functions
  sd_utils.h        SD card / PCAP API
  ble_scanner.h     BLE scanning API

src/
  main.cpp          Setup/loop, screen dispatch, navigation
  globals.cpp       Global state definitions
  sniffer.cpp       WiFi promiscuous mode, packet routing
  display.cpp       Common drawing (header, nav, stat bar, colours)
  screens.cpp       Original 9 diagnostic screens
  recon_screens.cpp 8 passive reconnaissance screens
  sd_utils.cpp      SD card detection, PCAP file writing
  ble_scanner.cpp   BLE device scanning

test/
  test_parser/      26 tests: frame classification, beacon/probe/EAPOL
                    parsing, PMKID extraction, MAC detection
  test_oui/         12 tests: vendor lookup, edge cases
```

- **Sniffer**: ESP-IDF promiscuous mode callback classifies 802.11 frames via the parser library, then routes to appropriate handlers (beacon, probe, EAPOL, deauth, device tracking).
- **Parser library**: Pure functions with zero ESP32 dependencies — fully testable on any platform via `make test`.
- **Channel hopping**: 150 ms dwell per channel, cycling 1-13 continuously.
- **Rendering**: Direct TFT_eSPI drawing with differential updates (only changed pixels are written) for flicker-free display.
- **Screen dispatch**: Function pointer array — adding a new screen means writing a draw function and adding one entry to the `screens[]` array.
- **Thread model**: Sniffer callback runs on the WiFi task (core 0), display updates on the Arduino loop (core 1). Shared counters use `volatile`.
- **BLE coexistence**: WiFi promiscuous mode is paused while BLE scanning is active (shared radio).

## Pin Configuration

| Signal   | HSPI (default) | VSPI (alt)  |
|----------|----------------|-------------|
| MOSI     | 13             | 23          |
| MISO     | 12             | 19          |
| SCLK     | 14             | 18          |
| TFT_CS   | 15             | 5           |
| TFT_DC   | 2              | 27          |
| TFT_RST  | -1             | 33          |
| TFT_BL   | 27             | 32          |
| TOUCH_CS | 33             | 21          |

SD card module uses VSPI (pins 18/19/23, CS=5).

## Resource Usage

- **RAM**: 27% (89 KB / 328 KB)
- **Flash**: 51% (1.6 MB / 3.1 MB) — uses `huge_app` partition scheme
- **Tests**: 38 native tests (parser + OUI), run on host with `make test`

---

## TODO — Additional Tools

### Active Testing (requires authorisation)

- [ ] Deauth Tester
- [ ] Rogue AP / Evil Twin
- [ ] WiFi Karma Attack
- [ ] Beacon Flood
- [ ] Captive Portal
- [ ] DNS Spoofing

### Network Tools (connected mode)

- [ ] ARP Scanner
- [ ] TCP Port Scanner
- [ ] mDNS / SSDP Discovery

### Limitations

- 2.4 GHz only — no 5 GHz scanning
- No WPA3-SAE cracking (capture only)
- CPU-bound when driving display and sniffing simultaneously
- No deep packet inspection of TLS traffic
