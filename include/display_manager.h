/**
 * Display Manager Header for JC4827W543 (NV3041A + QSPI)
 * 
 * WICHTIG: Verwendet Arduino_Canvas für korrekte Darstellung!
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "config.h"

// Prayer times structure
struct PrayerTimes {
    String fajr;
    String sunrise;
    String dhuhr;
    String asr;
    String maghrib;
    String isha;
    String date;
    String hijriDate;
    String mosqueName;
    bool valid;
    
    PrayerTimes() : valid(false) {}
};

// Adhan settings structure
struct AdhanSettings {
    bool fajr;
    bool shuruk;  // Sunrise alarm
    bool dhuhr;
    bool asr;
    bool maghrib;
    bool isha;
    
    AdhanSettings() : fajr(true), shuruk(false), dhuhr(true), asr(true), maghrib(true), isha(true) {}
};

// Theme colors
struct ThemeColors {
    uint16_t background;
    uint16_t headerBg;
    uint16_t text;
    uint16_t accent;
    uint16_t highlight;
};

// Language strings indices
enum Language { LANG_DE = 0, LANG_EN, LANG_FR, LANG_TR, LANG_AR };

// Theme indices
enum Theme { THEME_GREEN = 0, THEME_BLUE, THEME_PURPLE, THEME_DARK };

class DisplayManager {
public:
    DisplayManager();
    
    void begin();
    void flush();
    
    // Screens
    void showSplashScreen();
    void showConnecting(const String& status);
    void showError(const String& message);
    void showSetupScreen(const String& ssid, const String& password, const String& ip);
    void showMosqueSetupScreen(const String& ip);
    void showPrayerList(const PrayerTimes& data, int currentPrayerIndex);
    void showNextPrayer(const String& nextPrayerName, const String& nextPrayerTime, const String& remainingTime);
    void showAdhanScreen(const String& prayerName, bool showLyrics = false);
    void hideAdhanScreen();
    
    // New display modes
    void showClockScreen(const PrayerTimes& data);
    void showNextPrayer(const PrayerTimes& data);
    void showSettingsScreen(const AdhanSettings& settings, int selectedItem = 0);
    void setRotation(int rotation);
    
    // Updates
    void updateNextPrayerCountdown(const String& remainingTime);
    void showWiFiStatus(bool connected);
    
    // Display info
    int width() const { return _screenW; }
    int height() const { return _screenH; }
    
private:
    Arduino_DataBus* bus;
    Arduino_GFX* gfx;
    
    int _screenW;
    int _screenH;
    int _headerH;
    int _rowH;
    int _currentMode;
    int _currentPrayer;
    bool _adhanActive;
    
    // Colors
    static const uint16_t COL_BLACK = 0x0000;
    static const uint16_t COL_WHITE = 0xFFFF;
    static const uint16_t COL_GREEN = 0x07E0;
    static const uint16_t COL_DARKGREEN = 0x03E0;
    static const uint16_t COL_RED = 0xF800;
    static const uint16_t COL_BLUE = 0x001F;
    static const uint16_t COL_YELLOW = 0xFFE0;
    static const uint16_t COL_ORANGE = 0xFD20;
    static const uint16_t COL_NAVY = 0x000F;
    static const uint16_t COL_DARKGREY = 0x7BEF;
    static const uint16_t COL_LIGHTGREY = 0xC618;
    
    void drawCenteredText(const String& text, int y, uint16_t color, int size);
    void drawPrayerRow(int y, const String& name, const String& time, bool active, bool isSunrise);
};

#endif
