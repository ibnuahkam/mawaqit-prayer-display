/**
 * Display Manager Implementation for JC4827W543
 * 
 * WICHTIG: 
 * - Arduino_Canvas MUSS verwendet werden für QSPI Display
 * - flush() MUSS nach dem Zeichnen aufgerufen werden
 */

#include "display_manager.h"
#include <Preferences.h>

// Display modes
static const int MODE_SPLASH = 0;
static const int MODE_SETUP = 1;
static const int MODE_LIST = 2;
static const int MODE_NEXT_PRAYER = 3;
static const int MODE_ADHAN = 4;

// External settings (from main.cpp)
extern int currentLanguage;
extern int currentTheme;
extern bool autoNightMode;

// Prayer names in different languages
// [language][prayer_index] - 0=Fajr, 1=Sunrise, 2=Dhuhr, 3=Asr, 4=Maghrib, 5=Isha
static const char* PRAYER_NAMES[5][6] = {
    {"Fajr", "Shuruk", "Dhuhr", "Asr", "Maghrib", "Isha"},       // DE
    {"Fajr", "Sunrise", "Dhuhr", "Asr", "Maghrib", "Isha"},      // EN  
    {"Fajr", "Chourouk", "Dhuhr", "Asr", "Maghrib", "Isha"},     // FR
    {"Sabah", "Gunes", "Ogle", "Ikindi", "Aksam", "Yatsi"},      // TR
    {"Fajr", "Shuruk", "Dhuhr", "Asr", "Maghrib", "Isha"}        // AR (same names)
};

// UI strings in different languages
static const char* STR_NEXT[5] = {"Naechstes", "Next", "Prochain", "Sonraki", "Next"};
static const char* STR_REMAINING[5] = {"verbleibend", "remaining", "restant", "kalan", "remaining"};
static const char* STR_SETTINGS[5] = {"Einstellungen", "Settings", "Parametres", "Ayarlar", "Settings"};

// Helper to get theme colors
void getThemeColors(uint16_t* bg, uint16_t* header, uint16_t* accent, uint16_t* text) {
    switch (currentTheme) {
        case 1: // Blue
            *bg = 0x0010;     // Very dark blue (almost black)
            *header = 0x1930; // Dark navy
            *accent = 0x5D7F; // Light blue
            *text = 0xFFFF;   // White
            break;
        case 2: // Purple
            *bg = 0x2004;     // Very dark purple
            *header = 0x4010; // Dark purple
            *accent = 0xB4DF; // Light purple
            *text = 0xFFFF;   // White
            break;
        case 3: // Dark - TRUE BLACK
            *bg = 0x0000;     // Pure black
            *header = 0x18C3; // Very dark grey
            *accent = 0x4A69; // Medium grey
            *text = 0xC618;   // Light grey text
            break;
        default: // Green (0)
            *bg = 0x0200;     // Very dark green
            *header = 0x0420; // Dark green
            *accent = 0x07E0; // Bright green
            *text = 0xFFFF;   // White
            break;
    }
}

DisplayManager::DisplayManager() 
    : bus(nullptr), gfx(nullptr), _screenW(480), _screenH(272),
      _headerH(35), _rowH(38), _currentMode(MODE_SPLASH), 
      _currentPrayer(-1), _adhanActive(false) {
}

void DisplayManager::begin() {
    Serial.println("[DISPLAY] Initializing with Canvas...");
    
    /**
     * DISPLAY ROTATION FIX - Dokumentation
     * ====================================
     * Problem: NV3041A mit Arduino_Canvas zeigt Pixelfehler wenn Rotation
     *          im Konstruktor gesetzt wird.
     * 
     * Lösung:  
     * 1. Display mit Rotation 0 initialisieren
     * 2. Canvas mit korrekter Größe erstellen (480x272 für Landscape)
     * 3. Nach begin() die Rotation via setRotation() setzen
     * 
     * Rotation-Werte:
     * - 0 = Normal Landscape (USB links)
     * - 2 = 180° gedreht (USB rechts)
     */
    
    // Load rotation from preferences
    Preferences prefs;
    prefs.begin("mawaqit", true);
    int rotation = prefs.getInt("rotation", 0);  // Default 0
    prefs.end();
    Serial.printf("[DISPLAY] Rotation from prefs: %d\n", rotation);
    
    // Backlight ON first
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, HIGH);
    
    // Create QSPI bus
    bus = new Arduino_ESP32QSPI(GFX_QSPI_CS, GFX_QSPI_SCK, GFX_QSPI_D0, 
                                 GFX_QSPI_D1, GFX_QSPI_D2, GFX_QSPI_D3);
    
    // Create display with rotation 0, we handle rotation via Canvas
    Arduino_GFX* g = new Arduino_NV3041A(bus, GFX_NOT_DEFINED, 0, true);
    
    // Canvas size depends on rotation
    int canvasW = (rotation == 1 || rotation == 3) ? DISPLAY_HEIGHT : DISPLAY_WIDTH;
    int canvasH = (rotation == 1 || rotation == 3) ? DISPLAY_WIDTH : DISPLAY_HEIGHT;
    
    gfx = new Arduino_Canvas(canvasW, canvasH, g);
    
    if (!gfx->begin()) {
        Serial.println("[DISPLAY] ERROR: begin() failed!");
        return;
    }
    
    // Apply rotation after begin
    gfx->setRotation(rotation);
    
    _screenW = gfx->width();
    _screenH = gfx->height();
    
    gfx->fillScreen(COL_BLACK);
    flush();
    
    Serial.printf("[DISPLAY] Initialized: %dx%d (rot=%d)\n", _screenW, _screenH, rotation);
}

void DisplayManager::flush() {
    if (gfx) gfx->flush();
}

void DisplayManager::drawCenteredText(const String& text, int y, uint16_t color, int size) {
    gfx->setTextSize(size);
    gfx->setTextColor(color);
    
    int16_t x1, y1;
    uint16_t w, h;
    gfx->getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
    gfx->setCursor((_screenW - w) / 2, y);
    gfx->print(text);
}

void DisplayManager::showSplashScreen() {
    _currentMode = MODE_SPLASH;
    gfx->fillScreen(COL_BLACK);
    
    drawCenteredText("MAWAQIT", 60, COL_GREEN, 4);
    drawCenteredText("Gebetszeiten", 120, COL_WHITE, 2);
    
    // Crescent moon
    int cx = _screenW / 2;
    gfx->drawCircle(cx, 180, 25, COL_YELLOW);
    gfx->fillCircle(cx + 10, 175, 20, COL_BLACK);
    
    drawCenteredText("v1.0", _screenH - 30, COL_DARKGREY, 1);
    
    flush();
    Serial.println("[DISPLAY] Splash screen shown");
}

void DisplayManager::showConnecting(const String& status) {
    gfx->fillScreen(COL_BLACK);
    
    drawCenteredText("Verbinde...", 100, COL_YELLOW, 2);
    drawCenteredText(status, 150, COL_WHITE, 1);
    
    flush();
}

void DisplayManager::showError(const String& message) {
    gfx->fillScreen(COL_RED);
    
    drawCenteredText("FEHLER", 80, COL_WHITE, 3);
    drawCenteredText(message, 140, COL_WHITE, 1);
    
    flush();
}

void DisplayManager::showSetupScreen(const String& ssid, const String& password, const String& ip) {
    _currentMode = MODE_SETUP;
    gfx->fillScreen(COL_NAVY);
    
    drawCenteredText("WLAN Setup", 15, COL_YELLOW, 2);
    
    // Left side - instructions
    int leftX = 20;
    gfx->setTextSize(1);
    gfx->setTextColor(COL_WHITE);
    
    gfx->setCursor(leftX, 50);
    gfx->print("1. Mit WLAN verbinden:");
    
    gfx->setTextColor(COL_GREEN);
    gfx->setCursor(leftX + 10, 70);
    gfx->printf("SSID: %s", ssid.c_str());
    gfx->setCursor(leftX + 10, 90);
    gfx->printf("Pass: %s", password.c_str());
    
    gfx->setTextColor(COL_WHITE);
    gfx->setCursor(leftX, 120);
    gfx->print("2. Browser oeffnen:");
    
    gfx->setTextColor(COL_ORANGE);
    gfx->setCursor(leftX + 10, 140);
    gfx->printf("http://%s", ip.c_str());
    
    gfx->setTextColor(COL_WHITE);
    gfx->setCursor(leftX, 170);
    gfx->print("3. Moschee auswaehlen");
    
    // Right side - WiFi Info Box (no QR - saves memory)
    int boxX = 270;
    int boxY = 50;
    int boxW = 180;
    int boxH = 160;
    
    gfx->fillRoundRect(boxX, boxY, boxW, boxH, 10, COL_WHITE);
    gfx->drawRoundRect(boxX, boxY, boxW, boxH, 10, gfx->color565(100, 100, 100));
    
    // WiFi icon
    gfx->setTextSize(3);
    gfx->setTextColor(COL_DARKGREEN);
    gfx->setCursor(boxX + 65, boxY + 15);
    gfx->print("@");
    
    // SSID
    gfx->setTextSize(1);
    gfx->setTextColor(gfx->color565(80, 80, 80));
    gfx->setCursor(boxX + 30, boxY + 55);
    gfx->print("WLAN Name:");
    
    gfx->setTextSize(2);
    gfx->setTextColor(COL_DARKGREEN);
    gfx->setCursor(boxX + 10, boxY + 75);
    gfx->print(ssid.substring(0, 12));
    
    // Password
    gfx->setTextSize(1);
    gfx->setTextColor(gfx->color565(80, 80, 80));
    gfx->setCursor(boxX + 30, boxY + 105);
    gfx->print("Passwort:");
    
    gfx->setTextSize(2);
    gfx->setTextColor(gfx->color565(200, 50, 50));
    gfx->setCursor(boxX + 25, boxY + 125);
    gfx->print(password);
    
    flush();
    Serial.println("[DISPLAY] Setup screen shown");
}

void DisplayManager::showMosqueSetupScreen(const String& ip) {
    _currentMode = MODE_SETUP;
    gfx->fillScreen(COL_DARKGREEN);
    
    drawCenteredText("WLAN verbunden!", 50, COL_WHITE, 2);
    drawCenteredText("OK", 100, COL_YELLOW, 3);
    
    gfx->setTextColor(COL_WHITE);
    gfx->setTextSize(1);
    int cx = _screenW / 2;
    
    drawCenteredText("Jetzt Moschee konfigurieren:", 150, COL_WHITE, 1);
    drawCenteredText("http://" + ip, 180, COL_YELLOW, 1);
    
    flush();
    Serial.println("[DISPLAY] Mosque setup screen shown");
}

void DisplayManager::showPrayerList(const PrayerTimes& data, int currentPrayerIndex) {
    _currentMode = MODE_LIST;
    _currentPrayer = currentPrayerIndex;
    
    // Get theme colors with safety
    uint16_t bgColor, headerColor, accentColor, textColor;
    getThemeColors(&bgColor, &headerColor, &accentColor, &textColor);
    
    // Simple solid background with theme color
    gfx->fillScreen(bgColor);
    
    // Header with theme
    gfx->fillRect(0, 0, _screenW, 40, headerColor);
    gfx->drawFastHLine(0, 40, _screenW, accentColor);
    
    // Zeit links im Header
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char timeStr[10];
        sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        gfx->setTextSize(2);
        gfx->setTextColor(COL_YELLOW);
        gfx->setCursor(10, 12);
        gfx->print(timeStr);
    }
    
    // Moschee-Name rechts im Header (gekürzt)
    String title = data.mosqueName.length() > 0 ? data.mosqueName : "Gebetszeiten";
    if (title.length() > 22) title = title.substring(0, 20) + "..";
    gfx->setTextSize(2);
    gfx->setTextColor(textColor);
    // Rechtsbündig
    int16_t x1, y1;
    uint16_t tw, th;
    gfx->getTextBounds(title.c_str(), 0, 0, &x1, &y1, &tw, &th);
    gfx->setCursor(_screenW - tw - 10, 12);
    gfx->print(title);
    
    // Prayer rows - 2 columns layout with localized names
    String times[] = {data.fajr, data.sunrise, data.dhuhr, data.asr, data.maghrib, data.isha};
    // Safe bounds check for language
    int lang = (currentLanguage >= 0 && currentLanguage < 5) ? currentLanguage : 0;
    const char** names = PRAYER_NAMES[lang];
    const char* icons[] = {"*", "O", "-", "-", "-", "*"};
    
    int colW = _screenW / 2 - 10;
    int rowH = 35;
    int startY = 50;
    
    // Find current and next prayer
    int nowMins = 0;
    if (getLocalTime(&timeinfo)) {
        nowMins = timeinfo.tm_hour * 60 + timeinfo.tm_min;
    }
    
    int currentIdx = -1;
    int nextIdx = -1;
    for (int i = 5; i >= 0; i--) {
        if (i == 1) continue; // Skip sunrise
        int h = times[i].substring(0, 2).toInt();
        int m = times[i].substring(3, 5).toInt();
        int pMins = h * 60 + m;
        if (pMins <= nowMins) {
            currentIdx = i;
            break;
        }
    }
    for (int i = 0; i < 6; i++) {
        if (i == 1) continue;
        int h = times[i].substring(0, 2).toInt();
        int m = times[i].substring(3, 5).toInt();
        int pMins = h * 60 + m;
        if (pMins > nowMins) {
            nextIdx = i;
            break;
        }
    }
    
    for (int i = 0; i < 6; i++) {
        int col = i < 3 ? 0 : 1;
        int row = i % 3;
        int x = 10 + col * (colW + 10);
        int y = startY + row * (rowH + 8);
        
        bool isNext = (i == nextIdx);
        bool isCurrent = (i == currentIdx);
        bool isSunrise = (i == 1);
        bool isPassed = false;
        
        int h = times[i].substring(0, 2).toInt();
        int m = times[i].substring(3, 5).toInt();
        if (h * 60 + m < nowMins && !isSunrise) isPassed = true;
        
        // Background with theme - darker version of header
        uint16_t bgCol = headerColor;
        if (isNext) bgCol = accentColor;
        if (isSunrise) bgCol = gfx->color565(60, 40, 10);
        if (isPassed) bgCol = gfx->color565(30, 30, 30);
        
        gfx->fillRoundRect(x, y, colW, rowH, 6, bgCol);
        
        // Left accent bar for next prayer
        if (isNext) {
            gfx->fillRect(x, y, 4, rowH, textColor);
        }
        
        // Prayer name
        gfx->setTextSize(2);
        uint16_t nameCol = isPassed ? COL_DARKGREY : (isNext ? COL_BLACK : textColor);
        if (isSunrise) nameCol = COL_ORANGE;
        gfx->setTextColor(nameCol);
        gfx->setCursor(x + 12, y + 9);
        gfx->print(names[i]);
        
        // Prayer time
        gfx->setTextColor(isPassed ? COL_DARKGREY : (isNext ? COL_BLACK : textColor));
        gfx->setCursor(x + colW - 65, y + 9);
        gfx->print(times[i]);
    }
    
    // Bottom info bar with theme
    gfx->fillRect(0, _screenH - 28, _screenW, 28, headerColor);
    gfx->drawFastHLine(0, _screenH - 28, _screenW, accentColor);
    
    if (nextIdx >= 0) {
        int h = times[nextIdx].substring(0, 2).toInt();
        int m = times[nextIdx].substring(3, 5).toInt();
        int diff = (h * 60 + m) - nowMins;
        if (diff > 0) {
            char buf[50];
            // Use localized "Next" text
            int lang = (currentLanguage >= 0 && currentLanguage < 5) ? currentLanguage : 0;
            const char* nextStr[] = {"Naechstes", "Next", "Prochain", "Sonraki", "Next"};
            sprintf(buf, "%s: %s in %d:%02d", nextStr[lang], names[nextIdx], diff / 60, diff % 60);
            gfx->setTextSize(1);
            gfx->setTextColor(textColor);
            gfx->setCursor(10, _screenH - 18);
            gfx->print(buf);
        }
    }
    
    // Touch hint with theme
    gfx->setTextColor(accentColor);
    gfx->setCursor(_screenW - 100, _screenH - 18);
    const char* tapStr[] = {"Tippen: Modi", "Tap: Modes", "Appuyer", "Dokun", "Tap"};
    int lang2 = (currentLanguage >= 0 && currentLanguage < 5) ? currentLanguage : 0;
    gfx->print(tapStr[lang2]);
    
    flush();
    Serial.println("[DISPLAY] Prayer list shown");
}

void DisplayManager::drawPrayerRow(int y, const String& name, const String& time, bool active, bool isSunrise) {
    // Background
    uint16_t bgColor = active ? COL_DARKGREEN : COL_BLACK;
    if (isSunrise) bgColor = COL_NAVY;
    gfx->fillRect(0, y, _screenW, _rowH - 2, bgColor);
    
    // Prayer name
    gfx->setTextSize(2);
    gfx->setTextColor(isSunrise ? COL_ORANGE : (active ? COL_YELLOW : COL_WHITE));
    gfx->setCursor(20, y + 10);
    gfx->print(name);
    
    // Prayer time
    gfx->setTextColor(active ? COL_WHITE : COL_LIGHTGREY);
    gfx->setCursor(_screenW - 100, y + 10);
    gfx->print(time);
    
    // Active indicator
    if (active) {
        gfx->fillCircle(8, y + _rowH/2, 4, COL_GREEN);
    }
}

void DisplayManager::showNextPrayer(const String& nextPrayerName, const String& nextPrayerTime, const String& remainingTime) {
    _currentMode = MODE_NEXT_PRAYER;
    gfx->fillScreen(COL_BLACK);
    
    drawCenteredText("Naechstes Gebet", 30, COL_YELLOW, 2);
    drawCenteredText(nextPrayerName, 80, COL_GREEN, 3);
    drawCenteredText(nextPrayerTime, 130, COL_WHITE, 2);
    
    // Countdown box
    gfx->fillRoundRect(30, 165, _screenW - 60, 60, 8, COL_DARKGREEN);
    drawCenteredText("Verbleibend", 175, COL_WHITE, 1);
    drawCenteredText(remainingTime, 200, COL_WHITE, 2);
    
    flush();
}

void DisplayManager::updateNextPrayerCountdown(const String& remainingTime) {
    if (_currentMode != MODE_NEXT_PRAYER) return;
    
    gfx->fillRect(35, 195, _screenW - 70, 30, COL_DARKGREEN);
    drawCenteredText(remainingTime, 200, COL_WHITE, 2);
    
    flush();
}

void DisplayManager::showAdhanScreen(const String& prayerName, bool showLyrics) {
    _currentMode = MODE_ADHAN;
    _adhanActive = true;
    
    // Background gradient
    for (int i = 0; i < _screenH; i++) {
        uint16_t col = gfx->color565(0, 50 - i/8, 25 - i/15);
        gfx->drawFastHLine(0, i, _screenW, col);
    }
    
    // Header
    gfx->fillRect(0, 0, _screenW, 45, gfx->color565(0, 70, 35));
    drawCenteredText("ADHAN", 12, COL_YELLOW, 2);
    
    // Prayer name - big
    drawCenteredText(prayerName, 70, COL_WHITE, 4);
    
    // Crescent moon animation area
    int cx = _screenW / 2;
    gfx->fillCircle(cx, 140, 35, COL_YELLOW);
    gfx->fillCircle(cx + 15, 135, 30, gfx->color565(0, 50, 25));
    
    // Lyrics / Arabic text (simplified)
    if (showLyrics) {
        gfx->setTextSize(1);
        gfx->setTextColor(COL_LIGHTGREY);
        drawCenteredText("Allahu Akbar, Allahu Akbar", 190, COL_WHITE, 1);
        drawCenteredText("Ash-hadu an la ilaha illa Allah", 210, COL_LIGHTGREY, 1);
    }
    
    // Touch hint at bottom
    gfx->fillRect(0, _screenH - 35, _screenW, 35, gfx->color565(30, 30, 30));
    drawCenteredText("Tippen zum Stoppen", _screenH - 22, COL_YELLOW, 1);
    
    flush();
}

void DisplayManager::hideAdhanScreen() {
    _adhanActive = false;
}

// Language strings
static const char* langNames[] = {"DE", "EN", "FR", "TR", "AR"};
static const char* themeNames[] = {"Gruen", "Blau", "Lila", "Dunkel"};

// External settings (defined in main.cpp)
extern int currentLanguage;
extern int currentTheme;
extern bool autoNightMode;

void DisplayManager::showSettingsScreen(const AdhanSettings& settings, int selectedItem) {
    _currentMode = MODE_SETUP;
    
    // Theme colors
    uint16_t bgColor = gfx->color565(10, 25, 20);
    uint16_t headerColor = gfx->color565(0, 50, 35);
    uint16_t accentColor = COL_GREEN;
    
    if (currentTheme == 1) { // Blue
        bgColor = gfx->color565(10, 20, 35);
        headerColor = gfx->color565(20, 40, 80);
        accentColor = gfx->color565(80, 150, 255);
    } else if (currentTheme == 2) { // Purple
        bgColor = gfx->color565(25, 15, 35);
        headerColor = gfx->color565(60, 30, 80);
        accentColor = gfx->color565(180, 100, 255);
    } else if (currentTheme == 3) { // Dark
        bgColor = gfx->color565(15, 15, 15);
        headerColor = gfx->color565(30, 30, 30);
        accentColor = gfx->color565(100, 100, 100);
    }
    
    gfx->fillScreen(bgColor);
    
    // Header
    gfx->fillRect(0, 0, _screenW, 32, headerColor);
    gfx->drawFastHLine(0, 32, _screenW, accentColor);
    gfx->setTextSize(2);
    gfx->setTextColor(COL_WHITE);
    gfx->setCursor(15, 7);
    gfx->print("Einstellungen");
    
    // === LEFT COLUMN: Adhan Toggles ===
    int colW = 235;
    int startY = 40;
    int rowH = 28;
    
    gfx->setTextSize(1);
    gfx->setTextColor(COL_YELLOW);
    gfx->setCursor(10, startY);
    gfx->print("Adhan/Alarm:");
    startY += 15;
    
    const char* names[] = {"Fajr", "Shuruk", "Dhuhr", "Asr", "Maghrib", "Isha"};
    bool states[] = {settings.fajr, settings.shuruk, settings.dhuhr, settings.asr, settings.maghrib, settings.isha};
    
    for (int i = 0; i < 6; i++) {
        int y = startY + i * rowH;
        bool enabled = states[i];
        bool isSunrise = (i == 1);
        
        gfx->fillRoundRect(5, y, colW - 10, rowH - 3, 4, gfx->color565(25, 40, 32));
        
        gfx->setTextSize(2);
        gfx->setTextColor(isSunrise ? COL_ORANGE : COL_WHITE);
        gfx->setCursor(12, y + 5);
        gfx->print(names[i]);
        
        // Toggle
        int tx = 145, ty = y + 3, tw = 50, th = 18;
        gfx->fillRoundRect(tx, ty, tw, th, 9, enabled ? accentColor : gfx->color565(50, 50, 50));
        gfx->fillCircle(enabled ? tx + tw - 10 : tx + 10, ty + 9, 7, COL_WHITE);
    }
    
    // === RIGHT COLUMN: Settings ===
    int rx = 240;
    int ry = 40;
    
    // Language selector
    gfx->setTextSize(1);
    gfx->setTextColor(COL_YELLOW);
    gfx->setCursor(rx, ry);
    gfx->print("Sprache:");
    ry += 14;
    
    gfx->fillRoundRect(rx, ry, 110, 24, 4, gfx->color565(30, 45, 38));
    gfx->setTextSize(2);
    gfx->setTextColor(COL_WHITE);
    gfx->setCursor(rx + 8, ry + 4);
    const char* langFull[] = {"Deutsch", "English", "Francais", "Turkce", "Arabic"};
    gfx->print(langFull[currentLanguage]);
    
    // < > arrows
    gfx->fillTriangle(rx + 115, ry + 12, rx + 125, ry + 5, rx + 125, ry + 19, accentColor);
    ry += 32;
    
    // Theme selector  
    gfx->setTextSize(1);
    gfx->setTextColor(COL_YELLOW);
    gfx->setCursor(rx, ry);
    gfx->print("Farbthema:");
    ry += 14;
    
    // Theme color boxes
    uint16_t themeColors[] = {
        gfx->color565(0, 150, 80),   // Green
        gfx->color565(80, 150, 255), // Blue
        gfx->color565(180, 100, 255),// Purple
        gfx->color565(60, 60, 60)    // Dark
    };
    for (int t = 0; t < 4; t++) {
        int bx = rx + t * 30;
        gfx->fillRoundRect(bx, ry, 26, 22, 4, themeColors[t]);
        if (t == currentTheme) {
            gfx->drawRoundRect(bx - 1, ry - 1, 28, 24, 4, COL_WHITE);
            gfx->drawRoundRect(bx - 2, ry - 2, 30, 26, 4, COL_WHITE);
        }
    }
    ry += 32;
    
    // Auto Night Mode toggle
    gfx->setTextSize(1);
    gfx->setTextColor(COL_YELLOW);
    gfx->setCursor(rx, ry);
    gfx->print("Auto Nachtmodus:");
    ry += 14;
    
    gfx->fillRoundRect(rx, ry, 50, 20, 10, autoNightMode ? accentColor : gfx->color565(50, 50, 50));
    gfx->fillCircle(autoNightMode ? rx + 40 : rx + 10, ry + 10, 8, COL_WHITE);
    
    gfx->setTextSize(1);
    gfx->setTextColor(autoNightMode ? accentColor : COL_DARKGREY);
    gfx->setCursor(rx + 58, ry + 6);
    gfx->print(autoNightMode ? "AN" : "AUS");
    ry += 30;
    
    // Web Config info
    gfx->drawFastHLine(rx, ry, 120, gfx->color565(60, 60, 60));
    ry += 8;
    gfx->setTextSize(1);
    gfx->setTextColor(COL_LIGHTGREY);
    gfx->setCursor(rx, ry);
    gfx->print("Web Config:");
    ry += 14;
    gfx->setTextSize(2);
    gfx->setTextColor(accentColor);
    gfx->setCursor(rx, ry);
    gfx->print("192.168.");
    ry += 18;
    gfx->setCursor(rx + 20, ry);
    gfx->print("178.147");
    
    // Footer
    gfx->fillRect(0, _screenH - 22, _screenW, 22, headerColor);
    gfx->setTextSize(1);
    gfx->setTextColor(COL_LIGHTGREY);
    drawCenteredText("Links=Adhan | Rechts=Optionen | Lang=Zurueck", _screenH - 13, COL_LIGHTGREY, 1);
    
    flush();
}

void DisplayManager::showWiFiStatus(bool connected) {
    uint16_t color = connected ? COL_GREEN : COL_RED;
    gfx->fillCircle(_screenW - 10, 10, 5, color);
    flush();
}

void DisplayManager::setRotation(int rotation) {
    // Canvas doesn't support runtime rotation well
    // Just redraw the screen - rotation change requires reboot
    Serial.printf("[DISPLAY] Rotation request: %d (requires reboot)\n", rotation);
}

void DisplayManager::showClockScreen(const PrayerTimes& data) {
    _currentMode = MODE_NEXT_PRAYER;
    
    // Get theme colors
    uint16_t bgColor, headerColor, accentColor, textColor;
    getThemeColors(&bgColor, &headerColor, &accentColor, &textColor);
    
    // Simple solid background
    gfx->fillScreen(bgColor);
    
    // Get current time
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        drawCenteredText("Zeit nicht verfuegbar", _screenH/2, COL_RED, 2);
        flush();
        return;
    }
    
    int h = timeinfo.tm_hour;
    int m = timeinfo.tm_min;
    int s = timeinfo.tm_sec;
    
    // Draw large analog clock (center-left)
    int cx = 130;
    int cy = _screenH / 2 + 5;
    int r = 105;
    
    // Clock face - outer ring with theme accent
    for (int i = 0; i < 3; i++) {
        gfx->drawCircle(cx, cy, r - i, accentColor);
    }
    
    // Inner circle - header color
    gfx->fillCircle(cx, cy, r - 4, headerColor);
    
    // Hour marks - thicker and with numbers
    const char* hourNums[] = {"12", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"};
    for (int i = 0; i < 12; i++) {
        float angle = i * 30.0 * PI / 180.0 - PI/2;
        
        // Thick marks with theme
        int x1 = cx + (r - 18) * cos(angle);
        int y1 = cy + (r - 18) * sin(angle);
        int x2 = cx + (r - 8) * cos(angle);
        int y2 = cy + (r - 8) * sin(angle);
        gfx->drawLine(x1, y1, x2, y2, textColor);
        gfx->drawLine(x1+1, y1, x2+1, y2, textColor);
        
        // Hour numbers
        int nx = cx + (r - 32) * cos(angle);
        int ny = cy + (r - 32) * sin(angle);
        gfx->setTextSize(1);
        gfx->setTextColor(textColor);
        gfx->setCursor(nx - 5, ny - 4);
        gfx->print(hourNums[i]);
    }
    
    // Minute marks
    for (int i = 0; i < 60; i++) {
        if (i % 5 == 0) continue;
        float angle = i * 6.0 * PI / 180.0 - PI/2;
        int x1 = cx + (r - 12) * cos(angle);
        int y1 = cy + (r - 12) * sin(angle);
        int x2 = cx + (r - 7) * cos(angle);
        int y2 = cy + (r - 7) * sin(angle);
        gfx->drawLine(x1, y1, x2, y2, accentColor);
    }
    
    // Hour hand (thick, themed)
    float hourAngle = ((h % 12) + m / 60.0) * 30.0 * PI / 180.0 - PI/2;
    int hx = cx + 50 * cos(hourAngle);
    int hy = cy + 50 * sin(hourAngle);
    for (int w = -2; w <= 2; w++) {
        gfx->drawLine(cx + w, cy, hx + w, hy, textColor);
        gfx->drawLine(cx, cy + w, hx, hy + w, textColor);
    }
    
    // Minute hand (medium, accent)
    float minAngle = m * 6.0 * PI / 180.0 - PI/2;
    int mx = cx + 75 * cos(minAngle);
    int my = cy + 75 * sin(minAngle);
    for (int w = -1; w <= 1; w++) {
        gfx->drawLine(cx + w, cy, mx + w, my, accentColor);
        gfx->drawLine(cx, cy + w, mx, my + w, accentColor);
    }
    
    // Second hand (thin, yellow)
    float secAngle = s * 6.0 * PI / 180.0 - PI/2;
    int sx = cx + 85 * cos(secAngle);
    int sy = cy + 85 * sin(secAngle);
    gfx->drawLine(cx, cy, sx, sy, COL_YELLOW);
    // Tail of second hand
    int stx = cx - 15 * cos(secAngle);
    int sty = cy - 15 * sin(secAngle);
    gfx->drawLine(cx, cy, stx, sty, COL_YELLOW);
    
    // Center dot with accent
    gfx->fillCircle(cx, cy, 8, accentColor);
    gfx->fillCircle(cx, cy, 4, headerColor);
    
    // Right side - Digital time and prayer times
    int rx = 260;
    
    // Digital time - bigger
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d", h, m);
    gfx->setTextSize(3);
    gfx->setTextColor(textColor);
    gfx->setCursor(rx, 8);
    gfx->print(timeStr);
    
    // Seconds
    sprintf(timeStr, ":%02d", s);
    gfx->setTextSize(2);
    gfx->setTextColor(accentColor);
    gfx->setCursor(rx + 100, 15);
    gfx->print(timeStr);
    
    // Date - localized days
    int lang = (currentLanguage >= 0 && currentLanguage < 5) ? currentLanguage : 0;
    const char* days_de[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
    const char* days_en[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char* days_fr[] = {"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"};
    const char* days_tr[] = {"Paz", "Pzt", "Sal", "Car", "Per", "Cum", "Cmt"};
    const char** days[] = {days_de, days_en, days_fr, days_tr, days_en};
    
    char dateStr[40];
    sprintf(dateStr, "%s, %d.", days[lang][timeinfo.tm_wday], timeinfo.tm_mday);
    gfx->setTextSize(1);
    gfx->setTextColor(textColor);
    gfx->setCursor(rx, 42);
    gfx->print(dateStr);
    
    // ALL Prayer times - LARGER TEXT with localized names
    int nowMins = h * 60 + m;
    String prayers[] = {data.fajr, data.sunrise, data.dhuhr, data.asr, data.maghrib, data.isha};
    // Use lang from above for prayer names
    const char** names = PRAYER_NAMES[lang];
    
    int listY = 58;
    int rowH = 32;  // Bigger rows
    int nextIdx = -1;
    
    // Find next prayer
    for (int i = 0; i < 6; i++) {
        if (i == 1) continue;
        int ph = prayers[i].substring(0, 2).toInt();
        int pm = prayers[i].substring(3, 5).toInt();
        if (ph * 60 + pm > nowMins) {
            nextIdx = i;
            break;
        }
    }
    if (nextIdx == -1) nextIdx = 0; // Fajr tomorrow
    
    // Draw LARGER prayer list
    for (int i = 0; i < 6; i++) {
        int y = listY + i * rowH;
        bool isNext = (i == nextIdx);
        bool isSunrise = (i == 1);
        int ph = prayers[i].substring(0, 2).toInt();
        int pm = prayers[i].substring(3, 5).toInt();
        bool passed = (ph * 60 + pm <= nowMins) && !isNext;
        
        // Background for next prayer with theme accent
        if (isNext) {
            gfx->fillRoundRect(rx - 5, y - 2, 215, rowH - 2, 4, headerColor);
        }
        
        // Name - SIZE 2 now!
        gfx->setTextSize(2);
        gfx->setTextColor(passed ? COL_DARKGREY : (isSunrise ? COL_ORANGE : (isNext ? COL_YELLOW : textColor)));
        gfx->setCursor(rx, y + 5);
        // Shorten names for display
        char shortName[6];
        strncpy(shortName, names[i], 5);
        shortName[5] = '\0';
        gfx->print(shortName);
        
        // Time - SIZE 2 now!
        gfx->setCursor(rx + 75, y + 5);
        gfx->print(prayers[i]);
        
        // Countdown for next
        if (isNext) {
            int diffMins = (ph * 60 + pm) - nowMins;
            if (diffMins < 0) diffMins += 24 * 60; // Tomorrow
            char cd[15];
            sprintf(cd, "%dh%02d", diffMins / 60, diffMins % 60);
            gfx->setTextColor(accentColor);
            gfx->setCursor(rx + 150, y + 5);
            gfx->print(cd);
        }
    }
    
    flush();
}

void DisplayManager::showNextPrayer(const PrayerTimes& data) {
    _currentMode = MODE_NEXT_PRAYER;
    
    // Get theme colors
    uint16_t bgColor, headerColor, accentColor, textColor;
    getThemeColors(&bgColor, &headerColor, &accentColor, &textColor);
    
    // Simple solid background
    gfx->fillScreen(bgColor);
    
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        drawCenteredText("Zeit nicht verfuegbar", _screenH/2, COL_RED, 2);
        flush();
        return;
    }
    
    int h = timeinfo.tm_hour;
    int m = timeinfo.tm_min;
    int s = timeinfo.tm_sec;
    int nowMins = h * 60 + m;
    
    String prayers[] = {data.fajr, data.sunrise, data.dhuhr, data.asr, data.maghrib, data.isha};
    // Safe bounds check
    int lang = (currentLanguage >= 0 && currentLanguage < 5) ? currentLanguage : 0;
    const char** names = PRAYER_NAMES[lang];
    
    String nextName = names[0];  // Fajr
    String nextTime = data.fajr;
    int diffMins = 0;
    bool foundToday = false;
    
    // Find next prayer today
    for (int i = 0; i < 6; i++) {
        if (i == 1) continue;  // Skip sunrise
        int ph = prayers[i].substring(0, 2).toInt();
        int pm = prayers[i].substring(3, 5).toInt();
        int pMins = ph * 60 + pm;
        
        if (pMins > nowMins) {
            nextName = names[i];
            nextTime = prayers[i];
            diffMins = pMins - nowMins;
            foundToday = true;
            break;
        }
    }
    
    // If no prayer left today, calculate time until Fajr tomorrow
    if (!foundToday) {
        nextName = names[0];  // Fajr
        nextTime = data.fajr;
        int fajrH = data.fajr.substring(0, 2).toInt();
        int fajrM = data.fajr.substring(3, 5).toInt();
        int fajrMins = fajrH * 60 + fajrM;
        // Minutes until midnight + minutes from midnight to Fajr
        diffMins = (24 * 60 - nowMins) + fajrMins;
    }
    
    // Big centered display with localized text
    gfx->setTextSize(1);
    gfx->setTextColor(COL_LIGHTGREY);
    drawCenteredText(STR_NEXT[currentLanguage], 20, COL_LIGHTGREY, 2);
    
    // Prayer name - huge
    drawCenteredText(nextName, 60, accentColor, 4);
    
    // Time
    gfx->setTextSize(2);
    gfx->setTextColor(textColor);
    drawCenteredText(nextTime, 115, textColor, 3);
    
    // Countdown box with theme
    gfx->fillRoundRect(80, 150, _screenW - 160, 70, 10, headerColor);
    gfx->drawRoundRect(80, 150, _screenW - 160, 70, 10, accentColor);
    
    // Calculate hours, minutes, seconds for countdown
    int totalSecs = diffMins * 60 - s;
    if (totalSecs < 0) totalSecs += 60;
    int diffH = totalSecs / 3600;
    int diffM = (totalSecs % 3600) / 60;
    int diffS = totalSecs % 60;
    
    char countdown[20];
    sprintf(countdown, "%d:%02d:%02d", diffH, diffM, diffS);
    drawCenteredText(countdown, 170, textColor, 3);
    
    // Remaining label
    gfx->setTextSize(1);
    drawCenteredText(STR_REMAINING[currentLanguage], 205, COL_LIGHTGREY, 1);
    
    // Current time
    char currentTime[10];
    sprintf(currentTime, "%02d:%02d:%02d", h, m, s);
    gfx->setTextSize(2);
    gfx->setTextColor(COL_DARKGREY);
    gfx->setCursor(10, _screenH - 30);
    gfx->print(currentTime);
    
    // Mosque name
    gfx->setTextSize(1);
    gfx->setCursor(_screenW - 150, _screenH - 20);
    gfx->print(data.mosqueName.substring(0, 20));
    
    flush();
}