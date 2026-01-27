/**
 * Prayer Scheduler Header
 */

#ifndef PRAYER_SCHEDULER_H
#define PRAYER_SCHEDULER_H

#include <Arduino.h>
#include "display_manager.h"

class PrayerScheduler {
public:
    PrayerScheduler();
    
    void update(const PrayerTimes& times);
    int getCurrentPrayerIndex();
    int getNextPrayerIndex();
    String getNextPrayerName();
    String getNextPrayerTime();
    String getRemainingTime();
    bool isPrayerTime(int prayerIndex);
    
private:
    PrayerTimes _times;
    int timeToMinutes(const String& time);
    int getCurrentMinutes();
};

#endif
