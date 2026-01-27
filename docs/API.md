# API-Dokumentation

## Mawaqit.net API

Das System nutzt die offentliche Mawaqit.net API zum Abrufen der Gebetszeiten.

### Endpunkt

```
GET https://mawaqit.net/api/2.0/mosque/search?word={suchbegriff}
```

### Antwortformat

```json
[
  {
    "uuid": "xxxxx-xxxxx-xxxxx",
    "name": "Moschee Name",
    "times": ["05:30", "07:15", "12:30", "15:45", "18:20", "20:00"],
    "city": "Stadt",
    "country": "Land"
  }
]
```

Die `times`-Array enthalt in Reihenfolge:
1. Fajr
2. Sunrise (Shuruk)
3. Dhuhr
4. Asr
5. Maghrib
6. Isha

### Fehlerbehandlung

| HTTP-Code | Bedeutung | Aktion |
|-----------|-----------|--------|
| 200 | Erfolg | Daten verarbeiten |
| -1 | Verbindungsfehler | WiFi prufen, erneut versuchen |
| 404 | Nicht gefunden | Suchbegriff anpassen |
| 500 | Server-Fehler | Spater erneut versuchen |

---

## Interne REST-API

Das Gerat stellt folgende Endpunkte bereit:

### GET /api/times

Gibt die aktuellen Gebetszeiten zuruck.

**Antwort:**
```json
{
  "fajr": "05:30",
  "sunrise": "07:15",
  "dhuhr": "12:30",
  "asr": "15:45",
  "maghrib": "18:20",
  "isha": "20:00",
  "mosque": "Moschee Name"
}
```

### GET /api/settings

Gibt die aktuellen Einstellungen zuruck.

**Antwort:**
```json
{
  "adhan": {
    "fajr": true,
    "shuruk": false,
    "dhuhr": true,
    "asr": true,
    "maghrib": true,
    "isha": true
  },
  "language": 0,
  "theme": 0,
  "nightMode": true
}
```

### POST /api/wifi

Speichert neue WiFi-Zugangsdaten.

**Parameter:**
- `ssid` - WLAN-Name
- `password` - WLAN-Passwort

### POST /api/mosque

Speichert die ausgewahlte Moschee.

**Parameter:**
- `id` - Moschee-UUID
- `name` - Moschee-Name

### GET /api/rotate

Dreht das Display um 180 Grad und startet das Gerat neu.

### POST /upload

Ladt eine MP3-Datei als Adhan hoch.

**Content-Type:** multipart/form-data

**Parameter:**
- `file` - MP3-Datei (maximal 2 MB)

---

## Datenstruktur

### PrayerTimes (intern)

```cpp
struct PrayerTimes {
    String fajr;
    String sunrise;
    String dhuhr;
    String asr;
    String maghrib;
    String isha;
    String mosqueName;
    bool valid;
};
```

### AdhanSettings (intern)

```cpp
struct AdhanSettings {
    bool fajr;
    bool shuruk;
    bool dhuhr;
    bool asr;
    bool maghrib;
    bool isha;
};
```

---

## NVS-Speicher

Einstellungen werden im Non-Volatile Storage (NVS) gespeichert:

| Schlussel | Typ | Beschreibung |
|-----------|-----|--------------|
| ssid | String | WiFi SSID |
| password | String | WiFi Passwort |
| mosqueId | String | Moschee UUID |
| mosqueName | String | Moschee Name |
| rotation | int | Display-Rotation (0 oder 2) |
| adhanFajr | bool | Adhan fur Fajr |
| adhanShuruk | bool | Adhan fur Shuruk |
| adhanDhuhr | bool | Adhan fur Dhuhr |
| adhanAsr | bool | Adhan fur Asr |
| adhanMaghrib | bool | Adhan fur Maghrib |
| adhanIsha | bool | Adhan fur Isha |
| language | int | Sprache (0-4) |
| theme | int | Farbthema (0-3) |
| nightMode | bool | Auto-Nachtmodus |
