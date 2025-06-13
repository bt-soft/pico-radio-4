#ifndef __Si4735_MANAGER_H
#define __Si4735_MANAGER_H

#include "Config.h"
#include "Si4735Band.h"

class Si4735Manager : public Si4735Band {

  private:
    int8_t currentBandIdx = -1; // Kezdetben az 1-es band index van beállítva

  public:
    /**
     * @brief Konstruktor, amely inicializálja a Si4735 eszközt.
     * @param band A Band objektum, amely kezeli a rádió sávokat.
     */
    Si4735Manager();

    /**
     * @brief Inicializáljuk az osztályt, beállítjuk a rádió sávot és hangerőt.
     */
    void init();

    /**
     * A BFO lépésközöket állítja be, csak SSB módban működik
     * A BFO lépésközök a következő értékek lehetnek: 1, 10, 25 Hz.
     */
    void setStep();

    // ===================================================================
    // RDS Support - Kiterjesztett RDS funkcionalitás
    // ===================================================================

    /**
     * @brief Lekérdezi az aktuális RDS Program Service (PS) nevet.
     * @note Csak a MemmoryDisplay.cpp fájlban használjuk.
     * @return String Az állomásnév, vagy üres String, ha nem elérhető.
     */
    String getCurrentRdsProgramService();

    /**
     * @brief Lekérdezi az RDS állomásnevet (Program Service)
     * @return String Az RDS állomásnév, vagy üres string ha nem elérhető
     */
    String getRdsStationName();

    /**
     * @brief Lekérdezi az RDS program típust (PTY)
     * @return String Az RDS program típus szöveges leírása
     */
    String getRdsProgramType();

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

    /**
     * Loop függvény a squelchez és a hardver némításhoz.
     * Ez a függvény folyamatosan figyeli a squelch állapotát és kezeli a hardver némítást.
     */
    void loop();
};

#endif // __Si4735_MANAGER_H