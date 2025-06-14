#ifndef __SI4735_RDS_H
#define __SI4735_RDS_H

#include "Band.h"
#include "Si4735Band.h"
#include "utils.h"

/**
 * @brief Si4735Rds osztály - RDS funkcionalitás kezelése
 * @details Ez az osztály tartalmazza az összes RDS-hez kapcsolódó funkcionalitást
 */
class Si4735Rds : public Si4735Band {

  private:
#define MAX_STATION_NAME_LENGTH 8
    // Az SI4735 bufferét használjuk,
    // ez csak ez jelző pointer a képernyő törlésének szükségességének jelzésére
    char *rdsStationName = NULL;

#define MAX_MESSAGE_LENGTH 64
    char rdsInfo[MAX_MESSAGE_LENGTH];
    bool rdsInfoChanged = false; // Flag a szöveg megváltozásának jelzésére

  public:
    /**
     * @brief Si4735Rds osztály konstruktora
     */
    Si4735Rds() : Si4735Band() {}

    // ===================================================================
    // RDS Support - RDS funkcionalitás
    // ===================================================================

    // /**
    //  * @brief Lekérdezi az aktuális RDS Program Service (PS) nevet.
    //  * @note Csak a MemmoryDisplay.cpp fájlban használjuk.
    //  * @return String Az állomásnév, vagy üres String, ha nem elérhető.
    //  */
    // String getCurrentRdsProgramService();

    /**
     * @brief Lekérdezi az RDS állomásnevet (Program Service)
     * @return String Az RDS állomásnév, vagy üres string ha nem elérhető
     */
    String getRdsStationName();

    /**
     * @brief Lekérdezi az RDS program típus kódot (PTY)
     * @return uint8_t Az RDS program típus kódja (0-31), vagy 255 ha nincs RDS
     */
    uint8_t getRdsProgramTypeCode();

    /**
     * @brief Lekérdezi az RDS radio text üzenetet
     * @return String Az RDS radio text, vagy üres string ha nem elérhető
     */
    String getRdsRadioText();

    /**
     * @brief Lekérdezi az RDS dátum és idő információt
     * @param year Referencia a év tárolásához
     * @param month Referencia a hónap tárolásához
     * @param day Referencia a nap tárolásához
     * @param hour Referencia az óra tárolásához
     * @param minute Referencia a perc tárolásához
     * @return true ha sikerült lekérdezni a dátum/idő adatokat
     */
    bool getRdsDateTime(uint16_t &year, uint16_t &month, uint16_t &day, uint16_t &hour, uint16_t &minute);

    /**
     * @brief Ellenőrzi, hogy elérhető-e RDS adat
     * @return true ha van érvényes RDS vétel
     */
    bool isRdsAvailable();

    /**
     * @brief Lekérdezi az RDS jel minőségét
     * @return uint8_t Az RDS jel minősége (SNR érték)
     */
    uint8_t getRdsSignalQuality();
};

#endif // __SI4735_RDS_H
