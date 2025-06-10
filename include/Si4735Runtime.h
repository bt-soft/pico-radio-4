#ifndef __SI4735_RUNTIME_H
#define __SI4735_RUNTIME_H

#include "Si4735Base.h"

class Si4735Runtime : public Si4735Base {
  public:
    // AGC beállítási lehetőségek
    enum class AgcGainMode : uint8_t {
        Off = 0,       // AGC kikapcsolva (de technikailag aktív marad, csak a csillapítás 0)
        Automatic = 1, // AGC engedélyezve (teljesen automatikus működés)
        Manual = 2     // AGC manuális beállítással (a config.data.currentAGCgain értékével)
    };

  private:
    uint32_t hardwareAudioMuteElapsed; // SI4735 hardware audio mute állapot start ideje
    bool isSquelchMuted = false;       // Kezdetben nincs némítva a squelch miatt
    bool hardwareAudioMuteState;       // SI4735 hardware audio mute állapot

  protected:
    void manageHardwareAudioMute();
    void manageSquelch();

  public:
    Si4735Runtime() : Si4735Base(), hardwareAudioMuteState(false), hardwareAudioMuteElapsed(millis()) {};

    void hardwareAudioMuteOn();
    void checkAGC();
};

#endif // __SI4735_RUNTIME_H