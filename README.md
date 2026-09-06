<div align="center">

# Pakura

<!-- placeholder: hero image/gif of Pakura's display in action -->
![Pakura hero placeholder](.png)

A pocket-sized ESP32 anime companion that passively scans, fingerprints, and logs every WiFi network she detects, reporting her findings through her UI and her own access-point-hosted dashboard.

`C++` `Arduino Framework` `ESP32` `PlatformIO` `TFT_eSPI` `SPI` `LittleFS` `SD Card` `REST API` `JSON` `HTML` `CSS` `JavaScript`

</div>

---

## Contents

- [About Pakura](#about-pakura)
- [Networking & Wireless Reconnaissance](#networking--wireless-reconnaissance)
- [Development Story](#development-story)
- [Features](#features)
  - [Hardware](#hardware)
  - [Software Architecture](#software-architecture)
  - [Engineering Highlights](#engineering-highlights)
  - [Skills Demonstrated](#skills-demonstrated)
  - [Project Structure](#project-structure)
- [Building & Installation](#building--installation)
- [Development](#development)
- [Roadmap](#roadmap)
- [Licensing](#licensing)
- [Credits](#credits)

---

## About Pakura

Pakura is a virtual pet that lives on an ESP32 with a touchscreen display. She passively scans for nearby WiFi access points, "eats" (logs) every unique network she discovers, and reacts with expressions and dialogue on screen. Her stats — happiness, energy, experience, level, and lifetime totals — persist across reboots on an SD card.

A built-in web server lets you switch Pakura into access point mode and browse everything she's collected (SSIDs, BSSIDs, signal strength, channel, security type) from any phone or laptop, through a small custom-built dashboard.

## Networking & Wireless Reconnaissance

Pakura's core is a passive 802.11 reconnaissance engine built directly on the ESP32 WiFi stack — the networking is the project, not an add-on.

- **Passive scanning, no association** — every 60 seconds Pakura runs a full-channel WiFi scan (`WiFi.scanNetworks`) in station mode, including hidden SSIDs, without ever authenticating or connecting to any discovered network.
- **Per-network fingerprinting** — for every access point found, Pakura captures:
  - SSID
  - BSSID (MAC address)
  - RSSI (signal strength)
  - Channel
  - Security/authentication mode
- **Encryption/auth classification** — raw `wifi_auth_mode_t` values are translated into human-readable security types (Open, WEP, WPA-PSK, WPA2-PSK, WPA/WPA2-PSK, WPA2-Enterprise, WPA3-PSK, WPA2/WPA3-PSK, WAPI-PSK), so open or weakly-secured networks are immediately visible.
- **BSSID-based de-duplication** — every new scan result is checked against the existing on-SD CSV log by MAC address before being appended, keeping a running, unique inventory instead of re-logging the same access point every scan cycle.
- **Crash-safe persistence** — the network log and stats file are updated using a temp-file-then-replace pattern, so a power loss mid-write can't corrupt the dataset.
- **Self-hosted operator dashboard** — Pakura can stand up her own password-protected WiFi access point (`WiFi.softAP`) and serve a purpose-built HTML/CSS/JS dashboard from an embedded HTTP server, so collected data can be reviewed from any phone or laptop without pulling the SD card.
- **Minimal-disclosure REST API** — the dashboard talks to a small JSON API (`/api/ssids`, `/api/ssid`) designed to limit what's exposed at a glance:
  - The paginated list endpoint returns only a 3-character SSID prefix alongside BSSID, RSSI, channel, and security type.
  - The full SSID is only ever returned from a separate detail endpoint, requested explicitly by record ID.
- **Secrets isolated from version control** — the access point password lives in a local, untracked [include/secrets.h](include/secrets.h), kept separate from the rest of the versioned firmware configuration.

## Development Story

<!-- leave blank for now -->

## Features

### Hardware

- ESP32 development board
- ILI9342-driven TFT display (240x320, SPI) via `TFT_eSPI`
- Resistive touchscreen with raw ADC calibration for touch input
- microSD card storage over SPI for logs, stats, dialogue, and character art
- Onboard flash (LittleFS) for serving the web dashboard assets

### Software Architecture

- **Main firmware ([src/main.cpp](src/main.cpp))** — a screen/state machine (home, log, stats, SSID/BSSID browser, interact, debug) driving all UI rendering and touch handling
- **Display pipeline** — PNG decoding (`PNGdec`) streamed directly from SD to draw Pakura's expressions and icons
- **WiFi scanning & web server** — see [Networking & Wireless Reconnaissance](#networking--wireless-reconnaissance) for the full scanning, fingerprinting, and dashboard/API design
- **Stats engine** — happiness/energy/XP/level tracked in memory and persisted to a JSON file on SD with safe temp-file writes
- **Web dashboard ([data/web](data/web))** — vanilla HTML/CSS/JS front end that paginates and displays Pakura's logged networks and their details

### Engineering Highlights

- Dual-storage design splitting fast-changing logs/stats (SD) from static web assets (LittleFS)
- Hand-rolled CSV/JSON parsing and serialization to avoid heavyweight dependencies on constrained hardware
- Crash-safe file writes using temp files before replacing the primary log/stats files
- Touch-driven UI state machine supporting nested menus, pagination, and a hidden touch "easter egg"
- Debug-only build path (`PAKURA_DEBUG` flag) for tuning stats without shipping debug tools in release builds
- Idle/sleep mode to pause scanning and rendering to save power

### Skills Demonstrated

- 802.11 WiFi scanning, enumeration, and encryption/auth-mode classification (Open/WEP/WPA/WPA2/WPA3/Enterprise)
- MAC address (BSSID) based fingerprinting and de-duplication
- WiFi access point management (`softAP`) and standing up an isolated, device-hosted network
- Designing minimal-disclosure REST APIs (partial data in list views, explicit detail lookups by ID)
- Keeping credentials/secrets out of version control (local, untracked config header)
- Building a JSON API on embedded hardware without heavyweight frameworks
- Embedded C++ development for microcontrollers (Arduino framework on ESP32)
- Project structuring and dependency management with PlatformIO
- SPI peripheral integration: TFT displays, resistive touch panels, SD cards
- On-device image decoding and custom rendering pipelines
- Filesystem management across two storage backends (SD + LittleFS)
- Front-end web development (vanilla HTML/CSS/JS) consuming a device-hosted API
- State machine design for touchscreen UI/UX on limited hardware
- Data persistence and serialization (CSV + JSON) with corruption-safe writes
- Git/GitHub project management and open-source licensing (dual licensing for code vs. character art)

### Project Structure

```
pakura v1/
├── platformio.ini             # PlatformIO project & build configuration
├── data/web/                  # Web dashboard assets (served from LittleFS)
│   ├── index.html
│   ├── script.js
│   └── style.css
├── include/
│   ├── secrets.h               # Local secrets (not committed) — AP password, etc.
│   └── User_Setup.h            # TFT_eSPI display/touch pin configuration
├── src/
│   ├── main.cpp                # Firmware entry point, UI, WiFi scanning, stats
│   ├── web_server.cpp          # Embedded web server & JSON API
│   └── web_server.h
├── LICENSE                    # Code license (PolyForm Noncommercial 1.0.0)
└── LICENSE-CHARACTER           # Character/artwork license (CC BY-NC-SA 4.0)
```

## Building & Installation

1. Install [PlatformIO](https://platformio.org/) (VS Code extension or CLI).
2. Clone this repository.
3. Create [include/secrets.h](include/secrets.h) with your own access point password:
   ```cpp
   #define PAKURA_AP_PASSWORD "your-password-here"
   ```
4. Build and upload the firmware:
   ```
   pio run --target upload
   ```
5. Upload the filesystem image (web dashboard assets):
   ```
   pio run --target uploadfs --environment esp32dev
   ```
6. Populate the SD card with required dialogue/character assets, insert it, and power on Pakura.
[add link to sd card files]

## Development

- Target environment: `esp32dev` (see [platformio.ini](platformio.ini))
- Enable `PAKURA_DEBUG` in `platformio.ini` build flags for the on-device debug/stats-editing screen
- Web dashboard files live in [data/web](data/web) and are flashed to LittleFS separately from the firmware — re-run `uploadfs` after changing them

## Roadmap

✅ Basic placeholder UI  
✅ SD card reading  
✅ Display Pakura png  
✅ Configure touch  
✅ Dialogue  
✅ LOG, MORE menus  
✅ Network scanning  
✅ Result storing  
✅ SSID viewing menu  
✅ Stat mechanics  
✅ New expressions  
✅ Detailed stats menu  
✅ Chat feature  
✅ Sleep mode  
✅ Debug mode  
✅ Wireless access point mode  
✅ HTTP server skeleton  
✅ Better scanning  
✅ BSSID viewing menu  
✅ HTTP server for detailed network info  

## Licensing

Pakura uses a dual license:

- **Code** — licensed under the [PolyForm Noncommercial License 1.0.0](LICENSE). Free to use, modify, and share for non-commercial purposes.
- **Character & Artwork** — licensed separately under [CC BY-NC-SA 4.0](LICENSE-CHARACTER). This covers Pakura's character design, expressions, sprites, and other original visual assets. Commercial use is not permitted.

See the [LICENSE](LICENSE) and [LICENSE-CHARACTER](LICENSE-CHARACTER) files for full terms.

## Credits

Pakura is designed, built, and maintained by [41bie](https://github.com/41bie).

Some character artwork was created with AI assistance and subsequently edited and refined by the author (see [LICENSE-CHARACTER](LICENSE-CHARACTER) for details).

## Support Pakura

[link]

Donations will go towards commissioning an artist / hardware / 3D printing capabilities. Thank you!