/**
 * Web Server Header
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <functional>
#include "audio_player.h"

typedef std::function<void(const String& mosqueId, const String& mosqueName)> MosqueSelectedCallback;
typedef std::function<void()> SettingsChangedCallback;

class MawaqitWebServer {
public:
    MawaqitWebServer();
    
    void begin();
    void setOnMosqueSelected(MosqueSelectedCallback cb) { _onMosqueSelected = cb; }
    void setOnSettingsChanged(SettingsChangedCallback cb) { _onSettingsChanged = cb; }
    void setAudioPlayer(AdhanPlayer* player) { _audioPlayer = player; }
    
private:
    AsyncWebServer* _server;
    AdhanPlayer* _audioPlayer;
    MosqueSelectedCallback _onMosqueSelected;
    SettingsChangedCallback _onSettingsChanged;
    
    void setupRoutes();
    String getContentType(const String& filename);
};

#endif
