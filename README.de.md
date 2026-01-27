# Mawaqit Gebetszeiten-Anzeige

**Deutsch** | [English](README.md)

Eigenstaendige islamische Gebetszeiten-Anzeige basierend auf ESP32-S3 mit kapazitivem Touchscreen. Das System bezieht Gebetszeiten ueber die Mawaqit.net API und kann den Adhan zu den konfigurierten Zeiten abspielen.

## Funktionen

- Anzeige der fuenf taeglichen Gebetszeiten plus Sonnenaufgang
- Drei Anzeigemodi: Listenansicht, Analoguhr mit Zeitliste, Countdown zum naechsten Gebet
- Adhan-Wiedergabe (MP3) mit individueller Aktivierung pro Gebet
- Fuenf Sprachen: Deutsch, Englisch, Franzoesisch, Tuerkisch, Arabisch
- Vier Farbthemen: Gruen, Blau, Violett, Schwarz (True Dark)
- Web-basierte Konfigurationsoberflaeche
- Touch-Steuerung fuer alle Funktionen

## Hardware

Dieses Projekt ist fuer das JC4827W543 Board optimiert:

| Komponente | Spezifikation |
|------------|---------------|
| Mikrocontroller | ESP32-S3-WROOM-1, Dual-Core 240MHz |
| Display | 4.3 Zoll IPS, 480x272 Pixel, NV3041A Controller |
| Touch | GT911 kapazitiver Touch-Controller |
| Flash | 4 MB |
| PSRAM | 8 MB |
| Audio | I2S DAC (interner Verstaerker) |

## Software-Architektur

### Verzeichnisstruktur

```
src/
  main.cpp              Hauptprogramm, WiFi, Touch-Logik, Web-API
  display_manager.cpp   Display-Rendering, Theme-System
  touch_manager.cpp     Touch-Verarbeitung (GT911)
  audio_player.cpp      MP3-Wiedergabe ueber I2S

include/
  config.h              Hardware-Konfiguration, Pin-Definitionen
  display_manager.h     Display-Klasse
  touch_manager.h       Touch-Klasse
  audio_player.h        Audio-Klasse

data/
  index.html            Web-Konfigurationsoberflaeche
  adhan.mp3             Adhan-Audiodatei (optional)
```

### Verwendete Technologien

| Bibliothek | Version | Verwendungszweck |
|------------|---------|------------------|
| Arduino_GFX | 1.4.4 | QSPI Display-Treiber fuer NV3041A |
| TouchLib | main | GT911 Touch-Controller Treiber |
| ArduinoJson | 7.2.1 | JSON-Parsing der API-Antworten |
| ESP8266Audio | 1.9.7 | MP3-Dekodierung und I2S-Ausgabe |
| ESPAsyncWebServer | latest | Asynchroner HTTP-Server |
| AsyncTCP | latest | TCP-Stack fuer Webserver |
| LittleFS | intern | Dateisystem fuer MP3-Speicherung |

### Speicher-Aufteilung

| Partition | Groesse | Inhalt |
|-----------|---------|--------|
| app0 | 1.375 MB | Firmware (ca. 1.1 MB belegt) |
| spiffs | 2.56 MB | LittleFS fuer Audiodateien |
| nvs | 20 KB | Einstellungen (WiFi, Moschee, etc.) |

## Installation

Eine ausfuehrliche Anleitung zur Einrichtung der Entwicklungsumgebung findest du in [docs/SETUP.md](docs/SETUP.md).

### Kurzanleitung

Voraussetzungen:
- Visual Studio Code mit PlatformIO Extension
- USB-Kabel fuer ESP32-S3

Firmware hochladen:
```bash
pio run --target upload
```

Dateisystem hochladen (fuer Adhan-MP3):
```bash
pio run --target uploadfs
```

## Bedienung

### Touch-Gesten

| Geste | Dauer | Aktion |
|-------|-------|--------|
| Kurzes Tippen | < 1.5s | Wechsel zwischen Anzeigemodi |
| Langes Druecken | > 1.5s | Einstellungen oeffnen/schliessen |
| Tippen bei Adhan | - | Adhan stoppen |

### Einstellungen

Die Einstellungen sind in zwei Bereiche aufgeteilt:

**Linke Seite (X < 235):**
Adhan-Aktivierung fuer jedes Gebet (Fajr, Shuruk, Dhuhr, Asr, Maghrib, Isha)

**Rechte Seite (X >= 235):**
- Sprachauswahl (Pfeil antippen)
- Farbthema (Farbbox antippen)
- Automatischer Nachtmodus (Toggle)

## Web-Konfiguration

Nach dem Start ist die Konfigurationsoberflaeche unter der IP-Adresse des Geraets erreichbar:

```
http://[IP-ADRESSE]/
```

Funktionen:
- WiFi-Zugangsdaten aendern
- Moschee suchen und auswaehlen
- Adhan-MP3 hochladen (maximal 2 MB)
- Display-Rotation umschalten

## API-Endpunkte

| Endpunkt | Methode | Funktion |
|----------|---------|----------|
| /api/times | GET | Aktuelle Gebetszeiten abrufen |
| /api/settings | GET/POST | Einstellungen lesen/schreiben |
| /api/rotate | GET | Display um 180 Grad drehen |
| /upload | POST | Adhan-MP3 hochladen |

## Konfiguration

Die Hardware-Pins sind in `platformio.ini` definiert:

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

## Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [README.md](README.md) | Projektuebersicht (Englisch) |
| [README.de.md](README.de.md) | Projektuebersicht (Deutsch) |
| [docs/SETUP.md](docs/SETUP.md) | Einrichtung der Entwicklungsumgebung |
| [docs/API.md](docs/API.md) | API-Dokumentation |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Technische Details und Fehlerlosung |

## Mitwirken

Beitraege sind willkommen. Bitte lies zuerst [CONTRIBUTING.md](CONTRIBUTING.md).

## Lizenz

Dieses Projekt steht unter der MIT-Lizenz - siehe [LICENSE](LICENSE) fuer Details.

## Danksagungen

- [Mawaqit.net](https://mawaqit.net) fuer die Gebetszeiten-API
- [moononournation](https://github.com/moononournation) fuer die Arduino_GFX Bibliothek
