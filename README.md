# Mawaqit Prayer Times Display

[Deutsch](README.de.md) | **English**

A standalone Islamic prayer times display based on ESP32-S3 with capacitive touchscreen. The system fetches prayer times from the Mawaqit.net API and can play the Adhan at configured times.

## Features

- Display of five daily prayer times plus sunrise
- Three display modes: list view, analog clock with time list, countdown to next prayer
- Adhan playback (MP3) with individual activation per prayer
- Five languages: German, English, French, Turkish, Arabic
- Four color themes: Green, Blue, Purple, Black (True Dark)
- Web-based configuration interface
- Touch control for all functions

## Hardware

This project is optimized for the JC4827W543 board:

| Component | Specification |
|-----------|---------------|
| Microcontroller | ESP32-S3-WROOM-1, Dual-Core 240MHz |
| Display | 4.3 inch IPS, 480x272 pixels, NV3041A controller |
| Touch | GT911 capacitive touch controller |
| Flash | 4 MB |
| PSRAM | 8 MB |
| Audio | I2S DAC (internal amplifier) |

## Software Architecture

### Directory Structure

```
src/
  main.cpp              Main program, WiFi, touch logic, web API
  display_manager.cpp   Display rendering, theme system
  touch_manager.cpp     Touch processing (GT911)
  audio_player.cpp      MP3 playback via I2S

include/
  config.h              Hardware configuration, pin definitions
  display_manager.h     Display class
  touch_manager.h       Touch class
  audio_player.h        Audio class

data/
  index.html            Web configuration interface
  adhan.mp3             Adhan audio file (optional)
```

### Technologies Used

| Library | Version | Purpose |
|---------|---------|---------|
| Arduino_GFX | 1.4.4 | QSPI display driver for NV3041A |
| TouchLib | main | GT911 touch controller driver |
| ArduinoJson | 7.2.1 | JSON parsing of API responses |
| ESP8266Audio | 1.9.7 | MP3 decoding and I2S output |
| ESPAsyncWebServer | latest | Asynchronous HTTP server |
| AsyncTCP | latest | TCP stack for web server |
| LittleFS | internal | File system for MP3 storage |

### Memory Layout

| Partition | Size | Content |
|-----------|------|---------|
| app0 | 1.375 MB | Firmware (approx. 1.1 MB used) |
| spiffs | 2.56 MB | LittleFS for audio files |
| nvs | 20 KB | Settings (WiFi, mosque, etc.) |

## Installation

For detailed setup instructions, see [docs/SETUP.md](docs/SETUP.md).

### Quick Start

Requirements:
- Visual Studio Code with PlatformIO extension
- USB cable for ESP32-S3

Upload firmware:
```bash
pio run --target upload
```

Upload filesystem (for Adhan MP3):
```bash
pio run --target uploadfs
```

## Usage

### Touch Gestures

| Gesture | Duration | Action |
|---------|----------|--------|
| Short tap | < 1.5s | Switch between display modes |
| Long press | > 1.5s | Open/close settings |
| Tap during Adhan | - | Stop Adhan |

### Settings

The settings are divided into two areas:

**Left side (X < 235):**
Adhan activation for each prayer (Fajr, Shuruk, Dhuhr, Asr, Maghrib, Isha)

**Right side (X >= 235):**
- Language selection (tap arrow)
- Color theme (tap color box)
- Automatic night mode (toggle)

## Web Configuration

After startup, the configuration interface is accessible at the device's IP address:

```
http://[IP-ADDRESS]/
```

Functions:
- Change WiFi credentials
- Search and select mosque
- Upload Adhan MP3 (maximum 2 MB)
- Toggle display rotation

## API Endpoints

| Endpoint | Method | Function |
|----------|--------|----------|
| /api/times | GET | Get current prayer times |
| /api/settings | GET/POST | Read/write settings |
| /api/rotate | GET | Rotate display by 180 degrees |
| /upload | POST | Upload Adhan MP3 |

## Configuration

Hardware pins are defined in `platformio.ini`:

```ini
build_flags =
  -DDISPLAY_WIDTH=480
  -DDISPLAY_HEIGHT=272
  -DGFX_QSPI_CS=45
  -DGFX_QSPI_SCK=47
  -DGFX_BL=1
  -DTOUCH_SCL=4
  -DTOUCH_SDA=8
  -DTOUCH_INT=3
```

## Documentation

| Document | Description |
|----------|-------------|
| [README.md](README.md) | Project overview (English) |
| [README.de.md](README.de.md) | Project overview (German) |
| [docs/SETUP.md](docs/SETUP.md) | Development environment setup |
| [docs/API.md](docs/API.md) | API documentation |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Technical details and troubleshooting |

## Contributing

Contributions are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md) first.

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) for details.

## Acknowledgments

- [Mawaqit.net](https://mawaqit.net) for the prayer times API
- [moononournation](https://github.com/moononournation) for Arduino_GFX library
