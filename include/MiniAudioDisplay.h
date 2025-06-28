#ifndef MINI_AUDIO_DISPLAY_H
#define MINI_AUDIO_DISPLAY_H

#include "AudioProcessor.h"
#include "UIComponent.h"
#include <TFT_eSPI.h>

/**
 * @brief Mini audio display típusok
 */
enum class MiniAudioDisplayType {
    NONE = 0,          // Kikapcsolva
    SPECTRUM_BARS = 1, // Spektrum sávok
    SPECTRUM_LINE = 2, // Spektrum vonal
    WATERFALL = 3,     // Vízesés diagram
    OSCILLOSCOPE = 4,  // Oszcilloszkóp
    VU_METER = 5       // VU meter
};

/**
 * @brief Absztrakt alaposztály mini audio display komponensekhez
 */
class MiniAudioDisplay : public UIComponent {
  public:
    /**
     * @brief Konstruktor
     * @param tft TFT display referencia
     * @param bounds Display határai
     * @param colors Színséma (opcionális)
     */
    MiniAudioDisplay(TFT_eSPI &tft, const Rect &bounds, const ColorScheme &colors = ColorScheme::defaultScheme());

    /**
     * @brief Destruktor
     */
    virtual ~MiniAudioDisplay() = default;

    /**
     * @brief Kirajzolás
     */
    void draw() override;

    /**
     * @brief Frissítés (adatok lekérése és kirajzolás)
     */
    virtual void update();

    /**
     * @brief Típus lekérdezése
     */
    virtual MiniAudioDisplayType getType() const = 0;

    /**
     * @brief Engedélyezés/letiltás
     */
    void setEnabled(bool enabled);
    bool isEnabled() const { return !isDisabled(); }

    /**
     * @brief Színséma beállítása
     */
    void setColorScheme(uint16_t primary, uint16_t secondary, uint16_t background);

  protected:
    /**
     * @brief Specifikus kirajzolás - ezt implementálják a leszármazott osztályok
     */
    virtual void drawContent() = 0;

    /**
     * @brief AudioProcessor lekérése
     */
    AudioProcessor *getAudioProcessor();

    // Színek
    uint16_t primaryColor_;
    uint16_t secondaryColor_;
    uint16_t backgroundColor_;

    // Állapot
    uint32_t lastUpdateTime_;

    // Frissítési frekvencia
    static constexpr uint32_t UPDATE_INTERVAL_MS = 50; // 20 FPS
};

#endif // MINI_AUDIO_DISPLAY_H
