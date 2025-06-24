#ifndef MINI_AUDIO_DISPLAY_H
#define MINI_AUDIO_DISPLAY_H

#include "IAudioDataProvider.h"
#include "UIComponent.h"
#include <TFT_eSPI.h>

/**
 * @brief Mini audio megjelenítő komponens különböző módokkal
 *
 * Ez a komponens beágyazható az FM/AM képernyőkbe és különböző
 * audio vizualizációs módokat támogat.
 */
class MiniAudioDisplay : public UIComponent {
  public:
    // Megjelenítési módok
    enum class DisplayMode : uint8_t {
        Off = 0,
        SpectrumLowRes,  // Alacsony felbontású spektrum (24 sáv)
        SpectrumHighRes, // Magas felbontású spektrum
        Oscilloscope,    // Oszcilloszkóp
        Waterfall,       // Vízesés diagram
        Envelope,        // Burkológörbe
        TuningAid        // CW/RTTY hangolási segéd
    };

    // CW/RTTY hangolási segéd típusok
    enum class TuningAidType : uint8_t { CW_TUNING, RTTY_TUNING, OFF_DECODER }; // Konstruktor
    MiniAudioDisplay(TFT_eSPI &tft, const Rect &bounds, IAudioDataProvider *audioProvider, uint8_t &configModeRef, float maxDisplayFreqHz = 15000.0f);

    virtual ~MiniAudioDisplay(); // UIComponent interface implementáció
    virtual void draw() override;

    // Touch esemény kezelése - mód váltáshoz
    virtual void onClick(const TouchEvent &event) override;

    // Specifikus metódusok
    void update(); // Frissítési metódus
    void setDisplayMode(DisplayMode mode);
    DisplayMode getDisplayMode() const { return currentMode; }

    void setTuningAidType(TuningAidType type);
    TuningAidType getTuningAidType() const { return currentTuningAidType; }

    void setMaxDisplayFreq(float freqHz) { maxDisplayFreqHz = freqHz; }
    void forceRedraw() {
        needsRedraw = true;
        memset(peakBuffer, 0, sizeof(peakBuffer)); // Peak clear is újrarajzolásnál
    }

  private:
    // Rajzoló metódusok
    void drawModeIndicator();
    void drawSpectrumLowRes();
    void drawSpectrumHighRes();
    void drawOscilloscope();
    void drawWaterfall();
    void drawEnvelope();
    void drawTuningAid(); // Segéd metódusok
    void cycleModes();
    uint16_t frequencyToX(float freqHz);
    float xToFrequency(uint16_t x);
    uint16_t getWaterfallColor(float magnitude);
    void clearDisplay();

    // Sprite kezelés
    void initializeSpectrumSprite();
    void cleanupSpectrumSprite();

    // Állapot változók
    DisplayMode currentMode;
    TuningAidType currentTuningAidType;
    uint8_t &configModeFieldRef;
    IAudioDataProvider *audioProvider;
    float maxDisplayFreqHz;
    uint32_t lastModeChangeTime;
    uint32_t lastTouchTime;
    uint32_t lastPeakResetTime; // Peak reset időzítőhöz
    bool needsRedraw;
    bool modeIndicatorVisible;                                  // Buffer-ek a rajzoláshoz
    int peakBuffer[24];                                         // Csúcsértékek alacsony felbontású spektrumhoz
    uint8_t waterfallBuffer[80][128];                           // Vízesés buffer (max 128 széles, 80 magas)
    TFT_eSprite *spectrumSprite;                                // Sprite a villogásmentes spektrum rajzoláshoz
    bool spriteCreated;                                         // Sprite állapot flag// Konstansok
    static constexpr uint32_t MODE_INDICATOR_TIMEOUT_MS = 3000; // 3 sec
    static constexpr uint32_t TOUCH_DEBOUNCE_MS = 300;
    static constexpr uint32_t PEAK_RESET_INTERVAL_MS = 20000; // 20 sec - ritkább peak buffer reset
    static constexpr float LOW_RES_MIN_FREQ_HZ = 300.0f;
    static constexpr float CW_TUNING_SPAN_HZ = 600.0f;

    // Színek
    static constexpr uint16_t COLOR_SPECTRUM = TFT_GREEN;
    static constexpr uint16_t COLOR_PEAK = TFT_YELLOW;
    static constexpr uint16_t COLOR_BACKGROUND = TFT_BLACK;
    static constexpr uint16_t COLOR_GRID = TFT_DARKGREY;
    static constexpr uint16_t COLOR_CW_TARGET = TFT_GREEN;
    static constexpr uint16_t COLOR_RTTY_MARK = TFT_MAGENTA;
    static constexpr uint16_t COLOR_RTTY_SPACE = TFT_CYAN;
};

#endif // MINI_AUDIO_DISPLAY_H
