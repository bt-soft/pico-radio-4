#ifndef __SI4735_BAND_H
#define __SI4735_BAND_H

//-------------------- Band
#include "Band.h"

#include "Si4735Runtime.h"

class Si4735Band : public Si4735Runtime, public Band {

  private:
    // SSB betöltve?
    bool ssbLoaded = false;
    /**
     * SSB patch betöltése
     */
    void loadSSB();

  public:
    /**
     * @brief Si4735Band osztály konstruktora
     */
    Si4735Band() : Si4735Runtime(), Band() {};

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
     * @brief Band beállítása
     */
    void useBand();

    /**
     * Sávszélesség beállítása
     */
    void setBandWidth();

    /**
     * @brief A hangolás a memória állomásra
     * @param frequency A hangolási frekvencia
     * @param bfoOffset A BFO eltolás (ha van)
     * @param bandIndex A band indexe (FM, MW, SW, LW)
     * @param demodModIndex A demodulációs mód indexe (FM, AM, LSB, USB, CW)
     * @param bandwidthIndex A sávszélesség indexe
     */
    void tuneMemoryStation(uint16_t frequency, int16_t bfoOffset, uint8_t bandIndex, uint8_t demodModIndex, uint8_t bandwidthIndex);
};

#endif // __SI4735_BAND_H