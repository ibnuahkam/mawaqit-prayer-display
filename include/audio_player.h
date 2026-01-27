/**
 * Audio Player Header (Adhan)
 */

#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h>

class AdhanPlayer {
public:
    AdhanPlayer();
    
    bool begin();
    void play(const String& filename = "");
    void stop();
    void setVolume(int vol);
    bool isPlaying();
    void loop();
    
private:
    bool _initialized;
    bool _playing;
    int _volume;
};

#endif
