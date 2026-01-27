/**
 * Prayer Scheduler Implementation
 */

#include "prayer_scheduler.h"
#include <time.h>

static const char* PRAYER_NAMES[] = {"Fajr", "Sunrise", "Dhuhr", "Asr", "Maghrib", "Isha"};

PrayerScheduler::PrayerScheduler() {
}

void PrayerScheduler::update(const PrayerTimes& times) {
    _times = times;
}

int PrayerScheduler::timeToMinutes(const String& time) {
    if (time.length() < 5) return -1;
    
    int hour = time.substring(0, 2).toInt();
    int minute = time.substring(3, 5).toInt();
    return hour * 60 + minute;
}

int PrayerScheduler::getCurrentMinutes() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return -1;
    }
    return timeinfo.tm_hour * 60 + timeinfo.tm_min;
}

int PrayerScheduler::getCurrentPrayerIndex() {
    if (!_times.valid) return -1;
    
    int now = getCurrentMinutes();
    if (now < 0) return -1;
    
    String times[] = {_times.fajr, _times.sunrise, _times.dhuhr, 
                      _times.asr, _times.maghrib, _times.isha};
    
    int currentPrayer = 5;  // Default to Isha (last prayer of previous day)
    
    for (int i = 5; i >= 0; i--) {
        if (now >= timeToMinutes(times[i])) {
            currentPrayer = i;
            break;
        }
    }
    
    return currentPrayer;
}

int PrayerScheduler::getNextPrayerIndex() {
    if (!_times.valid) return 0;
    
    int current = getCurrentPrayerIndex();
    if (current < 0) return 0;
    
    // Skip sunrise for next prayer (it's not a prayer)
    int next = (current + 1) % 6;
    if (next == 1) next = 2;  // Skip sunrise
    
    return next;
}

String PrayerScheduler::getNextPrayerName() {
    int next = getNextPrayerIndex();
    return PRAYER_NAMES[next];
}

String PrayerScheduler::getNextPrayerTime() {
    if (!_times.valid) return "--:--";
    
    int next = getNextPrayerIndex();
    String times[] = {_times.fajr, _times.sunrise, _times.dhuhr, 
                      _times.asr, _times.maghrib, _times.isha};
    return times[next];
}

String PrayerScheduler::getRemainingTime() {
    if (!_times.valid) return "--:--";
    
    int now = getCurrentMinutes();
    if (now < 0) return "--:--";
    
    int nextTime = timeToMinutes(getNextPrayerTime());
    if (nextTime < 0) return "--:--";
    
    int diff = nextTime - now;
    if (diff < 0) diff += 24 * 60;  // Next day
    
    int hours = diff / 60;
    int minutes = diff % 60;
    
    char buf[10];
    sprintf(buf, "%02d:%02d", hours, minutes);
    return String(buf);
}

bool PrayerScheduler::isPrayerTime(int prayerIndex) {
    if (!_times.valid || prayerIndex < 0 || prayerIndex > 5) return false;
    if (prayerIndex == 1) return false;  // Sunrise is not a prayer
    
    int now = getCurrentMinutes();
    String times[] = {_times.fajr, _times.sunrise, _times.dhuhr, 
                      _times.asr, _times.maghrib, _times.isha};
    
    int prayerTime = timeToMinutes(times[prayerIndex]);
    
    // Check if within 1 minute of prayer time
    return (now >= prayerTime && now < prayerTime + 1);
}
