/**
 * Mawaqit API Header
 */

#ifndef MAWAQIT_API_H
#define MAWAQIT_API_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "display_manager.h"

struct MosqueInfo {
    String id;
    String name;
    String city;
    String country;
};

class MawaqitAPI {
public:
    MawaqitAPI();
    
    bool fetchPrayerTimes(const String& mosqueId, PrayerTimes& times);
    bool searchMosques(const String& query, std::vector<MosqueInfo>& results);
    
private:
    HTTPClient http;
    String buildUrl(const String& endpoint);
};

#endif
