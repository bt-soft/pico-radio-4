#ifndef __SI4735_BAND_H
#define __SI4735_BAND_H

#include "Band.h"
#include "Si4735Runtime.h"

class Si4735Band : public Si4735Runtime, public Band {

  private:
    // SSB betöltve?
    bool ssbLoaded;

    /**
     * SSB patch betöltése
     */
    void loadSSB();

  protected:
    // BandTable *currentBand; // Pointer helyett referencia, hogy újra lehessen állítani

    /**
     * @brief Band beállítása
     */
    void useBand();

  public:
    /**
     * @brief Si4735Band osztály konstruktora
     */
    Si4735Band() : Si4735Runtime(), Band(), ssbLoaded(false) {}

    /**
     * @brief BandStore beállítása (örökölt a Band osztályból)
     */
    using Band::setBandStore;

    /**
     * @brief band inicializálása
     * @details A band inicializálása, beállítja az alapértelmezett értékeket és a sávszélességet.
     */
    void bandInit(bool sysStart = false);

    /**
     * @brief Band beállítása
     * @param useDefaults default adatok betültése?
     */
    void bandSet(bool useDefaults = false);

    /**
     * HF Sávszélesség beállítása
     */
    void setBandWidth();

    /**
     * @brief A jelenlegi band típusának lekérdezése
     */
    inline boolean checkBandBounds(uint16_t newFreq) {

        BandTable &currentBand = getCurrentBand();

        // Ellenőrizzük, hogy a frekvencia a band határain belül van-e
        if (newFreq < currentBand.minimumFreq || newFreq > currentBand.maximumFreq) {
            return false; // A frekvencia kívül esik a band határain
        }
        return true; // A frekvencia a band határain belül van
    }

    /**
     * @brief A frekvencia léptetése a rotary encoder értéke alapján
     * @param rotaryValue A rotary encoder értéke (növelés/csökkentés)
     */
    inline uint16_t stepFrequency(int16_t rotaryValue) {

        BandTable &currentBand = getCurrentBand();

        // Kiszámítjuk a frekvencia lépés nagyságát
        int16_t step = rotaryValue * currentBand.currStep; // A lépés nagysága
        uint16_t targetFreq = currentBand.currFreq + step;

        // Korlátozás a sáv határaira
        if (targetFreq < currentBand.minimumFreq) {
            targetFreq = currentBand.minimumFreq;
        } else if (targetFreq > currentBand.maximumFreq) {
            targetFreq = currentBand.maximumFreq;
        } 
        
        // Csak akkor változtatunk, ha tényleg más a cél frekvencia
        if (targetFreq != currentBand.currFreq) {
            // Beállítjuk a frekvenciát
            si4735.setFrequency(targetFreq);

            // El is mentjük a band táblába
            currentBand.currFreq = si4735.getCurrentFrequency();

            // Band adatok mentését megjelöljük
            saveBandData();

            // Ez biztosítja, hogy az S-meter azonnal frissüljön az új frekvencián
            invalidateSignalCache();
        }

        return currentBand.currFreq;
    };

    /**
     * @brief A hangolás a memória állomásra
     * @param bandIndex A band indexe (FM, MW, SW, LW)
     * @param frequency A hangolási frekvencia
     * @param demodModIndex A demodulációs mód indexe (FM, AM, LSB, USB, CW)
     * @param bandwidthIndex A sávszélesség indexe
     */
    void tuneMemoryStation(uint8_t bandIndex, uint16_t frequency, uint8_t demodModIndex, uint8_t bandwidthIndex);
};

#endif // __SI4735_BAND_H