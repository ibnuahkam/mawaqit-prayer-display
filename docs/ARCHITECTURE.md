# Projektarchitektur und Erkenntnisse

## Verzeichnisstruktur im Source Control

### Problem: Mehrere Repositories in VSCode Source Control

VSCode zeigt mehrere "Repositories" an:
- GFX Library for Arduino
- TouchLib
- ESPAsyncWebServer
- AsyncTCP

### Ursache

Dies sind **keine Submodule** des Projekts, sondern **externe Bibliotheken**, die von PlatformIO automatisch heruntergeladen werden. Sie befinden sich im Ordner:

```
.pio/libdeps/esp32s3/
```

Jede Bibliothek hat ihren eigenen `.git` Ordner, weshalb VSCode sie als separate Repositories erkennt.

### Lösung

Diese Bibliotheken sollten **nicht** committed werden. Sie werden durch `platformio.ini` definiert und automatisch heruntergeladen.

Die `.gitignore` Datei sollte enthalten:
```
.pio/
```

Falls ein Git-Repository für das Projekt initialisiert wird, werden diese Ordner ignoriert.

---

## Abhangigkeiten (lib_deps)

| Bibliothek | Version | Zweck |
|------------|---------|-------|
| Arduino_GFX | v1.4.4 | Display-Treiber (NV3041A QSPI) |
| TouchLib | main | Touch-Controller (GT911) |
| ArduinoJson | 7.2.1 | JSON-Parsing fur API |
| ESP8266Audio | 1.9.7 | MP3-Wiedergabe |
| ESPAsyncWebServer | latest | Asynchroner Webserver |
| AsyncTCP | latest | TCP fur ESPAsyncWebServer |

---

## Haufige Fehler und Losungen

### 1. API Fehler -1

**Ursache:** WiFi-Verbindung noch nicht stabil beim API-Aufruf.

**Losung:** Nach WiFi-Verbindung 2-3 Sekunden warten bevor API aufgerufen wird.

```cpp
// In connectWiFi()
delay(2000);  // Warten auf stabile Verbindung
fetchPrayerTimes();
```

### 2. Touch-Koordinaten falsch

**Ursache:** Bei Display-Rotation mussen Touch-Koordinaten transformiert werden.

**Losung:** Fur Rotation 2 (180 Grad):
```cpp
_x = 480 - raw_x;
_y = 272 - raw_y;
```

### 3. Display friert ein

**Mogliche Ursachen:**
- Komplexe Gradient-Berechnungen
- Memory Leak durch haufige String-Operationen
- Zu schnelle Display-Updates

**Losungen:**
- Einfache fillScreen() statt Gradienten
- Clock-Update alle 5 Sekunden statt jede Sekunde
- Heap-Monitoring einbauen

### 4. ESP32 reagiert nicht mehr

**Losung:** Board in Boot-Modus versetzen:
1. USB trennen
2. BOOT-Taste gedruckt halten
3. USB einstecken
4. BOOT loslassen nach 2 Sekunden

---

## Build-Konfiguration

### Partition Table (partitions_custom.csv)

```csv
nvs,      data, nvs,     0x9000,  0x5000
otadata,  data, ota,     0xe000,  0x2000
app0,     app,  ota_0,   0x10000, 0x160000
spiffs,   data, spiffs,  0x170000,0x290000
```

- **app0:** 1.375 MB fur Firmware
- **spiffs:** 2.56 MB fur LittleFS (Adhan MP3)

### Build Flags

Wichtige Flags in `platformio.ini`:
- `BOARD_HAS_PSRAM` - PSRAM aktivieren
- `CONFIG_ASYNC_TCP_USE_WDT=0` - Watchdog fur AsyncTCP deaktivieren
- `DISPLAY_WIDTH/HEIGHT` - Display-Grosse
- `TOUCH_MODULES_GT911=1` - GT911 Touch-Controller

---

## Versionierung

Bei Anderungen an der Firmware sollte die Version in `config.h` aktualisiert werden:

```cpp
#define FIRMWARE_VERSION "1.0.0"
```
