/**
 * Web Server Implementation
 */

#include "web_server.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Preferences.h>

MawaqitWebServer::MawaqitWebServer() 
    : _server(nullptr), _audioPlayer(nullptr) {
}

void MawaqitWebServer::begin() {
    Serial.println("[WEB] Starting web server...");
    
    _server = new AsyncWebServer(80);
    setupRoutes();
    _server->begin();
    
    Serial.println("[WEB] Web server started on port 80");
}

void MawaqitWebServer::setupRoutes() {
    // Root - serve index.html
    _server->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        Serial.println("[WEB] Request for /");
        if (LittleFS.exists("/index.html")) {
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            request->send(200, "text/html", 
                "<html><body><h1>Mawaqit Setup</h1>"
                "<p>index.html nicht gefunden!</p>"
                "<p>Bitte LittleFS flashen.</p></body></html>");
        }
    });
    
    // Serve static files
    _server->serveStatic("/", LittleFS, "/");
    
    // API: Get settings
    _server->on("/api/settings", HTTP_GET, [](AsyncWebServerRequest* request) {
        Serial.println("[WEB] GET /api/settings");
        Preferences prefs;
        prefs.begin("mawaqit", true);
        
        JsonDocument doc;
        doc["mosqueId"] = prefs.getString("mosque_id", "");
        doc["mosqueName"] = prefs.getString("mosque_name", "");
        doc["adhanEnabled"] = prefs.getBool("adhan_enabled", true);
        doc["volume"] = prefs.getInt("volume", 80);
        
        prefs.end();
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // API: Set mosque
    _server->on("/api/mosque", HTTP_POST, [](AsyncWebServerRequest* request) {
        request->send(200);
    }, NULL, [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        Serial.println("[WEB] POST /api/mosque");
        String body = String((char*)data).substring(0, len);
        
        JsonDocument doc;
        if (deserializeJson(doc, body) == DeserializationError::Ok) {
            String mosqueId = doc["id"].as<String>();
            String mosqueName = doc["name"].as<String>();
            
            Serial.printf("[WEB] Mosque: %s (%s)\n", mosqueName.c_str(), mosqueId.c_str());
            
            if (_onMosqueSelected) {
                _onMosqueSelected(mosqueId, mosqueName);
            }
            
            request->send(200, "application/json", "{\"success\":true}");
        } else {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        }
    });
    
    // API: Set WiFi
    _server->on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest* request) {
        request->send(200);
    }, NULL, [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        Serial.println("[WEB] POST /api/wifi");
        String body = String((char*)data).substring(0, len);
        
        JsonDocument doc;
        if (deserializeJson(doc, body) == DeserializationError::Ok) {
            String ssid = doc["ssid"].as<String>();
            String password = doc["password"].as<String>();
            
            Serial.printf("[WEB] WiFi: %s\n", ssid.c_str());
            
            Preferences prefs;
            prefs.begin("mawaqit", false);
            prefs.putString("wifi_ssid", ssid);
            prefs.putString("wifi_pass", password);
            prefs.end();
            
            request->send(200, "application/json", "{\"success\":true}");
            
            delay(1000);
            ESP.restart();
        } else {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        }
    });
    
    // Catch-all for captive portal
    _server->onNotFound([](AsyncWebServerRequest* request) {
        Serial.printf("[WEB] Not found: %s -> redirect\n", request->url().c_str());
        request->redirect("http://192.168.4.1/");
    });
}

String MawaqitWebServer::getContentType(const String& filename) {
    if (filename.endsWith(".html")) return "text/html";
    if (filename.endsWith(".css")) return "text/css";
    if (filename.endsWith(".js")) return "application/javascript";
    if (filename.endsWith(".json")) return "application/json";
    return "text/plain";
}
