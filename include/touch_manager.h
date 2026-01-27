/**
 * Touch Manager using TouchLib for GT911
 */

#ifndef TOUCH_MANAGER_H
#define TOUCH_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <TouchLib.h>
#include "config.h"

class DisplayManager;

class TouchManager {
public:
    TouchManager();
    
    void begin(DisplayManager* display);
    bool update();  // Call every loop - updates internal state
    
    int getX() const { return _x; }
    int getY() const { return _y; }
    bool isTouched() const { return _currentlyTouched; }
    bool justPressed() const { return _justPressed; }
    bool justReleased() const { return _justReleased; }
    unsigned long touchDuration() const { return _touchDuration; }
    
private:
    TouchLib* touch;
    DisplayManager* _display;
    int _x, _y;
    bool _currentlyTouched;
    bool _lastTouched;
    bool _justPressed;
    bool _justReleased;
    bool _initialized;
    unsigned long _touchStartTime;
    unsigned long _touchDuration;
};

#endif
