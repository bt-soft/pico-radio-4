#ifndef AUDIO_DATA_PROVIDER_H
#define AUDIO_DATA_PROVIDER_H

#include <cstdint>

/**
 * @brief Audio data provider interface
 * @details Ez az interface biztosítja az egységes hozzáférést az audio adatokhoz
 * a különböző megjelenítési komponensek számára.
 */
class IAudioDataProvider {
  public:
    virtual ~IAudioDataProvider() = default;

    /**
     * @brief FFT magnitude adatok lekérése
     * @return Double tömb pointer a magnitude adatokhoz
     */
    virtual const double *getMagnitudeData() const = 0;

    /**
     * @brief Oszcilloszkóp adatok lekérése
     * @return Int16 tömb pointer az oszcilloszkóp mintákhoz
     */
    virtual const int16_t *getOscilloscopeData() const = 0;

    /**
     * @brief Envelope (burkológörbe) adatok lekérése
     * @return Uint8 tömb pointer az envelope adatokhoz
     */
    virtual const uint8_t *getEnvelopeData() const = 0;

    /**
     * @brief Waterfall adatok lekérése
     * @return Uint8 tömb pointer a waterfall adatokhoz
     */
    virtual const uint8_t *getWaterfallData() const = 0;

    /**
     * @brief FFT bin szélesség lekérése Hz-ben
     * @return Bin szélesség Hz-ben
     */
    virtual float getBinWidthHz() const = 0;

    /**
     * @brief Aktuális FFT méret lekérése
     * @return FFT méret (samples)
     */
    virtual uint16_t getFftSize() const = 0;

    /**
     * @brief Mintavételi frekvencia lekérése
     * @return Sample rate Hz-ben
     */
    virtual uint32_t getSampleRate() const = 0;

    /**
     * @brief Ellenőrzi, hogy vannak-e új audio adatok
     * @return True, ha új adatok állnak rendelkezésre
     */
    virtual bool isDataReady() const = 0;

    /**
     * @brief Adatok készre jelölése (internal use)
     */
    virtual void markDataConsumed() = 0;

    /**
     * @brief Audio processing aktív állapot
     * @return True, ha az audio feldolgozás aktív
     */
    virtual bool isProcessingActive() const = 0;

    /**
     * @brief Audio processing elindítása/leállítása
     * @param active True = indítás, False = leállítás
     */
    virtual void setProcessingActive(bool active) = 0;
};

/**
 * @brief Audio komponens típusok
 */
enum class AudioComponentType : uint8_t {
    SPECTRUM_LOW_RES = 0,  ///< Alacsony felbontású spektrum (24 sáv)
    SPECTRUM_HIGH_RES = 1, ///< Magas felbontású spektrum (teljes szélesség)
    OSCILLOSCOPE = 2,      ///< Oszcilloszkóp
    ENVELOPE = 3,          ///< Burkológörbe
    WATERFALL = 4,         ///< Vízesés diagram
    CW_TUNING = 5,         ///< CW hangolási segéd
    RTTY_TUNING = 6,       ///< RTTY hangolási segéd
    OFF = 7                ///< Kikapcsolva
};

/**
 * @brief Audio adatok státusz info
 */
struct AudioDataStatus {
    uint32_t timestamp;        ///< Utolsó frissítés időbélyege
    uint16_t processedSamples; ///< Feldolgozott minták száma
    float cpuUsagePercent;     ///< CPU használat százalék
    bool dmaOverrun;           ///< DMA overrun történt
    bool fftOverrun;           ///< FFT processing túl lassú
};

#endif // AUDIO_DATA_PROVIDER_H
