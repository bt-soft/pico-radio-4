#ifndef CONFIG_DATA_H
#define CONFIG_DATA_H

#include <stdint.h> // uint8_t, uint16_t, stb.

#include "defines.h"

// Konfig struktúra típusdefiníció
struct Config_t {
    uint8_t currentBandIdx;    // Aktuális sáv indexe
    uint32_t currentFrequency; // Aktuális frekvencia (Hz-ben)

    // BandWidht
    uint8_t bwIdxAM;
    uint8_t bwIdxFM;
    uint8_t bwIdxMW; // Hozzáadva a konzisztencia érdekében
    uint8_t bwIdxSSB;

    // Step
    uint8_t ssIdxMW;
    uint8_t ssIdxAM;
    uint8_t ssIdxFM;

    // BFO
    int currentBFO;
    uint8_t currentBFOStep; // BFO lépésköz (pl. 1, 10, 25 Hz)
    int currentBFOmanu;     // Manuális BFO eltolás (pl. -999 ... +999 Hz)

    // Squelch
    uint8_t currentSquelch;
    bool squelchUsesRSSI; // A squlech RSSI alapú legyen?

    // FM RDS
    bool rdsEnabled;

    // Hangerő
    uint8_t currVolume;

    // AGC
    uint8_t agcGain;
    uint8_t currentAGCgain;

    //--- TFT
    uint16_t tftCalibrateData[5];    // TFT touch kalibrációs adatok
    uint8_t tftBackgroundBrightness; // TFT Háttérvilágítás
    bool tftDigitLigth;              // Inaktív szegmens látszódjon?

    //--- System
    uint8_t screenSaverTimeoutMinutes; // Képernyővédő ideje percekben (1-30)
    bool beeperEnabled;                // Hangjelzés engedélyezése
    bool rotaryAcceleratonEnabled;     // Rotary gyorsítás engedélyezése

    // MiniAudioFft módok
    uint8_t miniAudioFftModeAm; // MiniAudioFft módja AM képernyőn
    uint8_t miniAudioFftModeFm; // MiniAudioFft módja FM képernyőn

    // MiniAudioFft erősítés
    float miniAudioFftConfigAm; // -1.0f: Disabled, 0.0f: Auto, >0.0f: Manual Gain Factor
    float miniAudioFftConfigFm; // -1.0f: Disabled, 0.0f: Auto, >0.0f: Manual Gain Factor

    float miniAudioFftConfigAnalyzer; // MiniAudioFft erősítés konfigurációja az Analyzerhez
    float miniAudioFftConfigRtty;     // MiniAudioFft erősítés konfigurációja az RTTY-hez

    uint16_t cwReceiverOffsetHz; // CW vételi eltolás Hz-ben

    // RTTY frekvenciák
    float rttyMarkFrequencyHz; // RTTY Mark frekvencia Hz-ben
    float rttyShiftHz;
};

#endif // CONFIG_DATA_H
