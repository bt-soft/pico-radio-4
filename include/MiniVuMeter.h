#ifndef MINI_VU_METER_H
#define MINI_VU_METER_H

#include "MiniAudioDisplay.h"

/**
 * @brief Mini VU meter komponens
 */
class MiniVuMeter : public MiniAudioDisplay {
  public:
    /**
     * @brief VU meter stílus
     */
    enum class Style {
        HORIZONTAL_BAR, // Vízszintes sáv
        VERTICAL_BAR,   // Függőleges sáv
        NEEDLE,         // Tű mutató
        LED_STRIP       // LED csík
    };

    /**
     * @brief Konstruktor
     * @param tft TFT display referencia
     * @param bounds Display határai
     * @param style VU meter stílus
     * @param colors Színséma (opcionális)
     */
    MiniVuMeter(TFT_eSPI &tft, const Rect &bounds, Style style = Style::HORIZONTAL_BAR, const ColorScheme &colors = ColorScheme::defaultScheme());

    /**
     * @brief Típus lekérdezése
     */
    MiniAudioDisplayType getType() const override { return MiniAudioDisplayType::VU_METER; }

    /**
     * @brief Stílus beállítása
     */
    void setStyle(Style style);
    Style getStyle() const { return style_; }

    /**
     * @brief Peak hold idő beállítása (ms)
     */
    void setPeakHoldTime(uint32_t timeMs);

  protected:
    /**
     * @brief Tartalom kirajzolása
     */
    void drawContent() override;

  private:
    Style style_;
    float currentLevel_;
    float peakLevel_;
    uint32_t peakHoldTime_;
    uint32_t lastPeakTime_;

    // Stílus specifikus rajzoló függvények
    void drawHorizontalBar();
    void drawVerticalBar();
    void drawNeedle();
    void drawLedStrip();

    // Segédfüggvények
    float calculateRMSLevel();
    uint16_t levelToColor(float level);
    static constexpr float LEVEL_DECAY = 0.8f;
    static constexpr uint32_t DEFAULT_PEAK_HOLD_TIME = 1000; // 1 second
};

#endif // MINI_VU_METER_H
