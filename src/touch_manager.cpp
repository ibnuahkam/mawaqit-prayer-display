/**
 * Touch Manager Implementation using TouchLib
 * 
 * WICHTIG: GT911 read() gibt true wenn Buffer-Daten vorhanden, 
 * getPointNum() gibt die tatsächliche Anzahl der Touch-Punkte!
 */

#include "touch_manager.h"
#include "display_manager.h"

TouchManager::TouchManager() 
    : touch(nullptr), _display(nullptr), _x(0), _y(0), 
      _currentlyTouched(false), _lastTouched(false),
      _justPressed(false), _justReleased(false),
      _initialized(false), _touchStartTime(0), _touchDuration(0) {
}

void TouchManager::begin(DisplayManager* display) {
    _display = display;
    
    Serial.println("[TOUCH] Initializing GT911 with TouchLib...");
    
    // Reset touchscreen
    pinMode(TOUCH_RES, OUTPUT);
    digitalWrite(TOUCH_RES, LOW);
    delay(200);
    digitalWrite(TOUCH_RES, HIGH);
    delay(200);
    
    // Initialize I2C
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    
    // Create TouchLib instance
    touch = new TouchLib(Wire, TOUCH_SDA, TOUCH_SCL, GT911_SLAVE_ADDRESS1);
    touch->init();
    
    _initialized = true;
    Serial.println("[TOUCH] Initialized!");
}

bool TouchManager::update() {
    if (!_initialized || !touch) {
        return false;
    }
    
    // Save last state
    _lastTouched = _currentlyTouched;
    _justPressed = false;
    _justReleased = false;
    
    // Read touch data - WICHTIG: getPointNum() prüfen!
    touch->read();
    uint8_t numPoints = touch->getPointNum();
    
    if (numPoints > 0) {
        // Touch detected
        TP_Point t = touch->getPoint(0);
        // Fix for rotation 2: invert BOTH X and Y coordinates
        // Display is 480x272, touch panel reports raw coordinates
        _x = 480 - t.x;
        _y = 272 - t.y;
        _currentlyTouched = true;
        
        // Detect NEW press
        if (!_lastTouched) {
            _justPressed = true;
            _touchStartTime = millis();
            Serial.printf("[TOUCH] PRESSED at x=%d, y=%d\n", _x, _y);
        }
        
        // Update duration
        _touchDuration = millis() - _touchStartTime;
    } else {
        // No touch
        _currentlyTouched = false;
        
        // Detect release
        if (_lastTouched) {
            _justReleased = true;
            _touchDuration = millis() - _touchStartTime;
            Serial.printf("[TOUCH] RELEASED after %lu ms\n", _touchDuration);
        }
    }
    
    return _currentlyTouched;
}
