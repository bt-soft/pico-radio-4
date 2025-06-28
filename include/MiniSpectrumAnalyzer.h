#ifndef MINI_SPECTRUM_ANALYZER_H
#define MINI_SPECTRUM_ANALYZER_H

#include "MiniAudioDisplay.h"

/**
 * @brief Mini spektrum analizátor komponens
 */
class MiniSpectrumAnalyzer : public MiniAudioDisplay {
  public:
    /**
     * @brief Spektrum megjelenítési mód
     */
    enum class DisplayMode {
        BARS,       // Oszlopok
        LINE,       // Vonal
        FILLED_LINE // Kitöltött vonal
    };

    /**
     * @brief Konstruktor
     * @param tft TFT display referencia
     * @param bounds Display határai
     * @param mode Megjelenítési mód
     * @param colors Színséma (opcionális)
     */
    MiniSpectrumAnalyzer(TFT_eSPI &tft, const Rect &bounds, DisplayMode mode = DisplayMode::BARS, const ColorScheme &colors = ColorScheme::defaultScheme());

    /**
     * @brief Destruktor
     */
    virtual ~MiniSpectrumAnalyzer();

    /**
     * @brief Típus lekérdezése
     */
    MiniAudioDisplayType getType() const override { return MiniAudioDisplayType::SPECTRUM_BARS; }

    /**
     * @brief Megjelenítési mód beállítása
     */
    void setDisplayMode(DisplayMode mode);
    DisplayMode getDisplayMode() const { return displayMode_; }

    /**
     * @brief Frekvencia tartomány beállítása
     */
    void setFrequencyRange(float minFreq, float maxFreq);

    /**
     * @brief Sávok számának beállítása
     */
    void setBandCount(uint16_t bandCount);
    uint16_t getBandCount() const { return bandCount_; }

  protected:
    /**
     * @brief Tartalom kirajzolása
     */
    void drawContent() override;

  private:
    DisplayMode displayMode_;
    uint16_t bandCount_;
    float minFrequency_;
    float maxFrequency_;

    // Adatbufferek
    float *fftData_;
    float *bandData_;
    float *peakHold_;

    // Megjelenítési paraméterek
    uint16_t fftDataSize_;
    static constexpr uint16_t DEFAULT_BAND_COUNT = 20;
    static constexpr float DEFAULT_MIN_FREQ = 300.0f;
    static constexpr float DEFAULT_MAX_FREQ_AM = 6000.0f;
    static constexpr float DEFAULT_MAX_FREQ_FM = 15000.0f;
    static constexpr float PEAK_HOLD_DECAY = 0.95f;

    // Privát segédfüggvények
    void allocateBuffers();
    void deallocateBuffers();
    void updateBandData();
    void drawBars();
    void drawLine();
    void drawFilledLine();
    uint16_t amplitudeToHeight(float amplitude);
    uint16_t getBarColor(float amplitude, float maxAmplitude);
};

#endif // MINI_SPECTRUM_ANALYZER_H
