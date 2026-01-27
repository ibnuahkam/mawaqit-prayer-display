# Entwicklungsumgebung einrichten

Diese Anleitung beschreibt die Einrichtung der Entwicklungsumgebung fur das Mawaqit Prayer Times Display Projekt.

## Voraussetzungen

- Windows 10/11, macOS oder Linux
- USB-Anschluss
- Internetverbindung

---

## 1. Visual Studio Code installieren

### Download

1. Offne https://code.visualstudio.com/
2. Lade die Version fur dein Betriebssystem herunter
3. Fuhre das Installationsprogramm aus
4. Starte VSCode nach der Installation

### Empfohlene Einstellungen

Offne die Einstellungen (Ctrl+,) und setze:

```json
{
  "files.autoSave": "afterDelay",
  "editor.formatOnSave": true,
  "editor.tabSize": 4
}
```

---

## 2. PlatformIO Extension installieren

### Installation

1. Offne VSCode
2. Klicke auf das Extensions-Symbol in der linken Seitenleiste (oder Ctrl+Shift+X)
3. Suche nach "PlatformIO IDE"
4. Klicke auf "Install"
5. Warte bis die Installation abgeschlossen ist (kann einige Minuten dauern)
6. Starte VSCode neu wenn aufgefordert

### Uberprufung

Nach der Installation sollte ein Ameisen-Symbol in der linken Seitenleiste erscheinen. Dies ist die PlatformIO-Sidebar.

---

## 3. Projekt offnen

### Methode 1: Ordner offnen

1. File > Open Folder
2. Navigiere zum Projektordner (z.B. `C:\Users\[Name]\Documents\PlatformIO\Projects\mawaqit`)
3. Klicke auf "Ordner auswahlen"

### Methode 2: PlatformIO Home

1. Klicke auf das PlatformIO-Symbol (Ameise)
2. Klicke auf "Open"
3. Wahle "Open Project"
4. Navigiere zum Projektordner

### Erste Initialisierung

Beim ersten Offnen ladt PlatformIO automatisch:
- Die ESP32-S3 Plattform
- Alle Bibliotheken aus `lib_deps` in `platformio.ini`
- Die Toolchain fur das Kompilieren

Dies kann beim ersten Mal 5-10 Minuten dauern.

---

## 4. USB-Treiber installieren

### Windows

Das JC4827W543 Board verwendet einen CH340 oder CP2102 USB-Serial-Chip.

**CH340 Treiber:**
1. Offne https://www.wch.cn/downloads/CH341SER_EXE.html
2. Lade CH341SER.EXE herunter
3. Fuhre das Installationsprogramm aus

**CP2102 Treiber:**
1. Offne https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
2. Lade den Treiber fur dein System herunter
3. Installiere den Treiber

### macOS

Treiber werden normalerweise automatisch installiert. Falls nicht:

```bash
brew install --cask silicon-labs-vcp-driver
```

### Linux

Keine zusatzlichen Treiber erforderlich. Fuege deinen Benutzer zur dialout-Gruppe hinzu:

```bash
sudo usermod -a -G dialout $USER
```

Danach abmelden und wieder anmelden.

---

## 5. Board verbinden und Port ermitteln

### Board anschliessen

1. Verbinde das JC4827W543 Board uber USB-C mit dem Computer
2. Warte einige Sekunden bis der Treiber geladen ist

### Port ermitteln

**Windows:**
1. Offne den Gerate-Manager (Win+X > Gerate-Manager)
2. Unter "Anschlusse (COM & LPT)" findest du den Port (z.B. COM3, COM5)

**macOS:**
```bash
ls /dev/cu.*
```
Der Port heisst z.B. `/dev/cu.usbserial-0001`

**Linux:**
```bash
ls /dev/ttyUSB*
```
Der Port heisst z.B. `/dev/ttyUSB0`

### Port in PlatformIO einstellen (optional)

Falls der Port nicht automatisch erkannt wird, fuege in `platformio.ini` hinzu:

```ini
upload_port = COM5  ; Windows
; upload_port = /dev/cu.usbserial-0001  ; macOS
; upload_port = /dev/ttyUSB0  ; Linux
```

---

## 6. Firmware kompilieren und hochladen

### Kompilieren (Build)

**Methode 1: Tastenkombination**
- Drucke Ctrl+Alt+B

**Methode 2: PlatformIO Toolbar**
- Klicke auf das Hakchen-Symbol unten in der Statusleiste

**Methode 3: Terminal**
```bash
pio run
```

### Hochladen (Upload)

**Methode 1: Tastenkombination**
- Drucke Ctrl+Alt+U

**Methode 2: PlatformIO Toolbar**
- Klicke auf den Pfeil-nach-rechts-Symbol unten in der Statusleiste

**Methode 3: Terminal**
```bash
pio run --target upload
```

### Dateisystem hochladen (fur Adhan-MP3)

Das Dateisystem (LittleFS) enthalt die Adhan-Audiodatei:

**PlatformIO Sidebar:**
1. Klicke auf das PlatformIO-Symbol
2. Unter "Project Tasks" > "esp32s3" > "Platform"
3. Klicke auf "Upload Filesystem Image"

**Terminal:**
```bash
pio run --target uploadfs
```

---

## 7. Serial Monitor

Der Serial Monitor zeigt Debug-Ausgaben des ESP32:

**Offnen:**
- Klicke auf das Stecker-Symbol unten in der Statusleiste
- Oder: Ctrl+Alt+S
- Oder: `pio device monitor`

**Einstellungen:**
- Baudrate: 115200 (in `platformio.ini` als `monitor_speed` definiert)

---

## 8. Problemlosung

### Upload fehlgeschlagen: "No serial data received"

Das Board antwortet nicht. Versetze es in den Boot-Modus:

1. Trenne das USB-Kabel
2. Halte die BOOT-Taste auf dem Board gedruckt
3. Stecke das USB-Kabel ein (BOOT weiter gedruckt halten)
4. Warte 2 Sekunden, dann BOOT loslassen
5. Fuhre den Upload erneut aus

### Port wird nicht erkannt

1. Prufe ob das USB-Kabel Daten ubertragen kann (nicht nur Ladekabel)
2. Versuche einen anderen USB-Port
3. Installiere die Treiber erneut
4. Starte den Computer neu

### Kompilierungsfehler: Library nicht gefunden

PlatformIO ladt Libraries automatisch. Falls Fehler auftreten:

1. Losche den Ordner `.pio`
2. Offne das Projekt erneut
3. Warte bis alle Libraries heruntergeladen sind

### VSCode zeigt mehrere Repositories an

Dies sind die Libraries unter `.pio/libdeps/`. Das ist normal und kein Fehler. Diese Ordner werden durch `.gitignore` ignoriert.

---

## 9. Nutzliche VSCode-Erweiterungen

| Erweiterung | Zweck |
|-------------|-------|
| PlatformIO IDE | Hauptentwicklungsumgebung |
| C/C++ | Syntax-Highlighting, IntelliSense |
| GitLens | Git-Integration (optional) |

---

## 10. Projektstruktur verstehen

Nach dem Offnen des Projekts:

```
mawaqit/
  .pio/                 PlatformIO Build-Ordner (ignoriert)
  .vscode/              VSCode-Einstellungen
  data/                 Dateien fur LittleFS
  docs/                 Dokumentation
  include/              Header-Dateien (.h)
  lib/                  Lokale Libraries (leer)
  src/                  Quellcode (.cpp)
  test/                 Unit-Tests (leer)
  platformio.ini        Projektkonfiguration
  partitions_custom.csv Speicher-Partitionierung
  README.md             Projektbeschreibung
```

Die wichtigsten Dateien zum Bearbeiten sind in `src/` und `include/`.
