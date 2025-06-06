#ifndef __STATIONDATA_H
#define __STATIONDATA_H
#include "ConfigData.h"      // Szükséges a Config_t miatt
#include "StoreEepromBase.h" // Szükséges a StoreEepromBase<T>::getRequiredSize() miatt
#include <Arduino.h>

// Maximális állomásszámok
#define MAX_FM_STATIONS 20
#define MAX_AM_STATIONS 50 // AM/LW/SW/SSB/CW

// Maximális név hossz + null terminátor
#define MAX_STATION_NAME_LEN 15
#define STATION_NAME_BUFFER_SIZE (MAX_STATION_NAME_LEN + 1)

// Egyetlen állomás adatai
struct StationData {
    char name[STATION_NAME_BUFFER_SIZE]; // Állomás neve
    uint16_t frequency;                  // Frekvencia (kHz vagy 10kHz, a bandType alapján)
    int16_t bfoOffset;                   // BFO eltolás Hz-ben SSB/CW esetén (0 AM/FM esetén)
    uint8_t bandIndex;                   // A BandTable indexe, hogy tudjuk a sáv részleteit
    uint8_t modulation;                  // Aktuális moduláció (FM, AM, LSB, USB, CW)
    uint8_t bandwidthIndex;              // Index a Band::bandWidthFM/AM/SSB tömbökben
    // uint16_t antCap;     // Opcionális: Ha az antenna kapacitást is menteni akarod
    // int16_t bfoOffset;   // Opcionális: Ha a BFO eltolást is menteni akarod
};

// FM állomások listája
struct FmStationList_t {
    StationData stations[MAX_FM_STATIONS];
    uint8_t count = 0; // Tárolt állomások száma
};

// AM (és egyéb) állomások listája
struct AmStationList_t {
    StationData stations[MAX_AM_STATIONS];
    uint8_t count = 0; // Tárolt állomások száma
};

// EEPROM címek dinamikus meghatározása
// A Config osztály a 0. címet használja a StoreBase<Config_t> alapértelmezett implementációja szerint.
constexpr uint16_t EEPROM_CONFIG_START_ADDR = 0;
constexpr size_t CONFIG_REQUIRED_SIZE = StoreEepromBase<Config_t>::getRequiredSize();

constexpr uint16_t EEPROM_FM_STATIONS_ADDR = EEPROM_CONFIG_START_ADDR + CONFIG_REQUIRED_SIZE;
constexpr size_t FM_STATIONS_REQUIRED_SIZE = StoreEepromBase<FmStationList_t>::getRequiredSize();

constexpr uint16_t EEPROM_AM_STATIONS_ADDR = EEPROM_FM_STATIONS_ADDR + FM_STATIONS_REQUIRED_SIZE;
constexpr size_t AM_STATIONS_REQUIRED_SIZE = StoreEepromBase<AmStationList_t>::getRequiredSize();

// Fordítási idejű ellenőrzés, hogy a kiosztás nem lépi-e túl az EEPROM méretét.
// Az EEPROM_SIZE makró a StoreEepromBase.h-ban van definiálva (alapértelmezetten 2048).
static_assert(EEPROM_AM_STATIONS_ADDR + AM_STATIONS_REQUIRED_SIZE <= EEPROM_SIZE,
              "EEPROM layout exceeds EEPROM_SIZE. Check Config_t, FmStationList_t, AmStationList_t sizes or EEPROM_SIZE.");
#endif // __STATIONDATA_H
