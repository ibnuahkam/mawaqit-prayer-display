/**
 * Mawaqit Prayer Times Display - Configuration
 */

#ifndef CONFIG_H
#define CONFIG_H

// WiFi AP Settings
#define WIFI_AP_NAME "Mawaqit-Setup"
#define WIFI_AP_PASSWORD "12345678"

// Display Settings (JC4827W543)
#define DISPLAY_WIDTH 480
#define DISPLAY_HEIGHT 272
#define DISPLAY_ROTATION 2  // 0=normal, 2=180 Grad gedreht

// Display Pins (QSPI)
#define GFX_QSPI_CS 45
#define GFX_QSPI_SCK 47
#define GFX_QSPI_D0 21
#define GFX_QSPI_D1 48
#define GFX_QSPI_D2 40
#define GFX_QSPI_D3 39
#define GFX_BL 1

// Touch Pins (GT911)
#define TOUCH_SDA 8
#define TOUCH_SCL 4
#define TOUCH_RES 38
#define TOUCH_INT 3

// Audio Settings
#define DAC_PIN 25

// API Settings
#define MAWAQIT_API_URL "https://mawaqit.net/api/2.0"

// Timing
#define PRAYER_CHECK_INTERVAL 1000      // 1 second
#define DISPLAY_UPDATE_INTERVAL 1000    // 1 second  
#define PRAYER_TIMES_REFRESH 3600000    // 1 hour

#endif // CONFIG_H
