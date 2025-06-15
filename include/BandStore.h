#ifndef __BAND_STORE_H
#define __BAND_STORE_H

#include "EepromLayout.h" // EEPROM_BAND_DATA_ADDR konstanshoz
#include "StoreBase.h"
#include "defines.h"

// Forward deklaráció a Band osztályhoz
class Band;

// BandTable változó adatainak struktúrája (mentéshez)
struct BandTableData_t {
    uint16_t currFreq; // Aktuális frekvencia
    uint8_t currStep;  // Aktuális lépésköz
    uint8_t currMod;   // Aktuális moduláció
    uint16_t antCap;   // Antenna capacitor
};

// Teljes Band adatok struktúrája (az összes band számára)
struct BandStoreData_t {
    BandTableData_t bands[BANDTABLE_SIZE]; // Band tábla mérete (defines.h-ból)
};

/**
 * Band adatok mentését és betöltését kezelő osztály
 */
class BandStore : public StoreBase<BandStoreData_t> {
  public:
    // A band adatok, alapértelmezett értékek (0) lesznek a konstruktorban
    BandStoreData_t data;

  protected:
    const char *getClassName() const override { return "BandStore"; }

    /**
     * Referencia az adattagra, csak az ős használja
     */
    BandStoreData_t &getData() override { return data; };

    /**
     * Const referencia az adattagra, CRC számításhoz
     */
    const BandStoreData_t &getData() const override { return data; };

    // Felülírjuk a mentést/betöltést a megfelelő EEPROM címmel
    uint16_t performSave() override { return StoreEepromBase<BandStoreData_t>::save(getData(), EEPROM_BAND_DATA_ADDR, getClassName()); }
    uint16_t performLoad() override { return StoreEepromBase<BandStoreData_t>::load(getData(), EEPROM_BAND_DATA_ADDR, getClassName()); }

    /**
     * Alapértelmezett értékek betöltése - minden adat nullázása
     */
    void loadDefaults() override {
        for (uint8_t i = 0; i < BANDTABLE_SIZE; i++) {
            data.bands[i].currFreq = 0;
            data.bands[i].currStep = 0;
            data.bands[i].currMod = 0;
            data.bands[i].antCap = 0;
        }
    }

  public:
    /**
     * Konstruktor - inicializálja az adatokat nullákkal
     */
    BandStore() {
        // Minden band adatát nullázzuk inicializáláskor
        for (uint8_t i = 0; i < BANDTABLE_SIZE; i++) {
            data.bands[i].currFreq = 0;
            data.bands[i].currStep = 0;
            data.bands[i].currMod = 0;
            data.bands[i].antCap = 0;
        }
    } /**
       * BandTable változó adatainak betöltése a tárolt adatokból
       * @param bandTable A BandTable tömb referenciája
       */
    void loadToBandTable(BandTable *bandTable) {
        for (uint8_t i = 0; i < BANDTABLE_SIZE; i++) {
            if (data.bands[i].currFreq != 0) {
                bandTable[i].currFreq = data.bands[i].currFreq;
                bandTable[i].currStep = data.bands[i].currStep;
                bandTable[i].currMod = data.bands[i].currMod;
                bandTable[i].antCap = data.bands[i].antCap;
            }
        }
    }

    /**
     * BandTable változó adatainak mentése a store-ba
     * @param bandTable A BandTable tömb referenciája
     */
    void saveFromBandTable(const BandTable *bandTable) {
        for (uint8_t i = 0; i < BANDTABLE_SIZE; i++) {
            data.bands[i].currFreq = bandTable[i].currFreq;
            data.bands[i].currStep = bandTable[i].currStep;
            data.bands[i].currMod = bandTable[i].currMod;
            data.bands[i].antCap = bandTable[i].antCap;
        }
        // Az adatok megváltoztak, a checkSave() fogja észlelni
    }
};

#endif // __BAND_STORE_H
