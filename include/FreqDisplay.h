// include/FreqDisplay.h
#ifndef __FREQDISPLAY_H
#define __FREQDISPLAY_H

#include "Band.h"
#include "Config.h"
#include "UIComponent.h"
#include "rtVars.h"
#include <TFT_eSPI.h>

// Színstruktúra a szegmensekhez
struct FreqSegmentColors {
    uint16_t active;
    uint16_t inactive;
    uint16_t indicator;
};

class FreqDisplay : public UIComponent {
  private:
    Band &band;
    Config &config;

    TFT_eSprite spr; // Sprite a szegmensek rajzolásához

    // Színek
    FreqSegmentColors normalColors;
    FreqSegmentColors bfoColors;

    uint16_t currentDisplayFrequency; // Az aktuálisan kijelzendő frekvencia
    bool bfoModeActiveLastDraw;       // Segédváltozó a BFO mód változásának detektálásához
    bool redrawOnlyFrequencyDigits;   // Optimalizált rajzoláshoz: csak a számjegyek

    // Belső helper metódusok
    void drawFrequencyInternal(const String &freq, const __FlashStringHelper *mask, const FreqSegmentColors &colors, const __FlashStringHelper *unit = nullptr);
    const FreqSegmentColors &getSegmentColors() const;
    void displaySsbCwFrequency(uint16_t currentFrequencyValue, const FreqSegmentColors &colors);
    void displayFmAmFrequency(uint16_t currentFrequencyValue, const FreqSegmentColors &colors);
    void drawFrequencySpriteOnly(const String &freq_str, const __FlashStringHelper *mask, const FreqSegmentColors &colors);
    bool determineFreqStrAndMaskForOptimizedDraw(uint16_t frequency, String &outFreqStr, const __FlashStringHelper *&outMask);
    void drawStepUnderline(const FreqSegmentColors &colors);
    uint32_t calcFreqSpriteXPosition() const;

  public:
    FreqDisplay(TFT_eSPI &tft, const Rect &bounds, Band &band_ref, Config &config_ref);
    virtual ~FreqDisplay() = default;

    void setFrequency(uint16_t freq);

    // UIComponent felülírt metódusok
    virtual void draw() override;
    virtual bool handleTouch(const TouchEvent &event) override;
    // A handleRotary-t a szülő képernyő (FMScreen) kezeli, és az hívja meg a setFrequency-t.
};

#endif // __FREQDISPLAY_H
