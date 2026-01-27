/**
 * Mawaqit API Implementation
 */

#include "mawaqit_api.h"
#include <WiFiClientSecure.h>

MawaqitAPI::MawaqitAPI() {
}

String MawaqitAPI::buildUrl(const String& endpoint) {
    return "https://mawaqit.net/api/2.0" + endpoint;
}

bool MawaqitAPI::fetchPrayerTimes(const String& mosqueId, PrayerTimes& times) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[API] Not connected to WiFi");
        return false;
    }
    
    WiFiClientSecure client;
    client.setInsecure();  // Skip certificate verification
    
    String url = buildUrl("/mosque/" + mosqueId + "/times");
    Serial.printf("[API] Fetching: %s\n", url.c_str());
    
    http.begin(client, url);
    http.addHeader("Accept", "application/json");
    
    int httpCode = http.GET();
    
    if (httpCode != 200) {
        Serial.printf("[API] HTTP error: %d\n", httpCode);
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.printf("[API] JSON parse error: %s\n", error.c_str());
        return false;
    }
    
    // Extract times for today
    JsonArray timesArray = doc["times"];
    if (timesArray.size() == 0) {
        Serial.println("[API] No times in response");
        return false;
    }
    
    // Get first entry (today)
    JsonArray todayTimes = timesArray[0];
    if (todayTimes.size() >= 6) {
        times.fajr = todayTimes[0].as<String>();
        times.sunrise = todayTimes[1].as<String>();
        times.dhuhr = todayTimes[2].as<String>();
        times.asr = todayTimes[3].as<String>();
        times.maghrib = todayTimes[4].as<String>();
        times.isha = todayTimes[5].as<String>();
    }
    
    // Get mosque info
    if (doc.containsKey("name")) {
        times.mosqueName = doc["name"].as<String>();
    }
    
    times.valid = true;
    
    Serial.printf("[API] Prayer times loaded: Fajr=%s, Dhuhr=%s, Maghrib=%s\n", 
                  times.fajr.c_str(), times.dhuhr.c_str(), times.maghrib.c_str());
    
    return true;
}

bool MawaqitAPI::searchMosques(const String& query, std::vector<MosqueInfo>& results) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }
    
    WiFiClientSecure client;
    client.setInsecure();
    
    String url = buildUrl("/mosque/search?word=" + query);
    Serial.printf("[API] Searching: %s\n", url.c_str());
    
    http.begin(client, url);
    http.addHeader("Accept", "application/json");
    
    int httpCode = http.GET();
    
    if (httpCode != 200) {
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    JsonDocument doc;
    if (deserializeJson(doc, payload)) {
        return false;
    }
    
    results.clear();
    JsonArray arr = doc.as<JsonArray>();
    
    for (JsonObject obj : arr) {
        MosqueInfo info;
        info.id = obj["uuid"].as<String>();
        info.name = obj["name"].as<String>();
        info.city = obj["city"].as<String>();
        info.country = obj["countryCode"].as<String>();
        results.push_back(info);
    }
    
    Serial.printf("[API] Found %d mosques\n", results.size());
    return true;
}
