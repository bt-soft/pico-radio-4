#ifndef __Si4735_MANAGER_H
#define __Si4735_MANAGER_H

#include "Config.h"
#include "Si4735Band.h"

/**
 * @brief Struktúra a rádió jel minőségi adatainak tárolására
 */
struct SignalQualityData {
    uint8_t rssi;       // RSSI érték (0-127)
    uint8_t snr;        // SNR érték (0-127)
    uint32_t timestamp; // Utolsó frissítés időbélyege (millis())
    bool isValid;       // Adatok érvényességi flag

    SignalQualityData() : rssi(0), snr(0), timestamp(0), isValid(false) {}
};

class Si4735Manager : public Si4735Band {

  private:
    int8_t currentBandIdx = -1; // Kezdetben az 1-es band index van beállítva

    // Signal quality cache
    SignalQualityData signalCache;
    static const uint32_t CACHE_TIMEOUT_MS = 1000; // 1 másodperc cache timeout

    /**
     * @brief Frissíti a signal quality cache-t
     */
    void updateSignalCache();

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

    /**
     * @brief Lekérdezi az aktuális RDS Program Service (PS) nevet.
     * @note Csak a MemmoryDisplay.cpp fájlban használjuk.
     * @return String Az állomásnév, vagy üres String, ha nem elérhető.
     */
    String getCurrentRdsProgramService();

    /**
     * Loop függvény a squelchez és a hardver némításhoz.
     * Ez a függvény folyamatosan figyeli a squelch állapotát és kezeli a hardver némítást.
     */
    void loop();

    /**
     * @brief Lekéri a signal quality adatokat cache-elt módon (max 1mp késleltetés)
     * @return SignalQualityData A cache-elt signal quality adatok
     */
    SignalQualityData getSignalQuality();

    /**
     * @brief Lekéri a signal quality adatokat valós időben (közvetlen chip lekérdezés)
     * @return SignalQualityData A friss signal quality adatok
     */
    SignalQualityData getSignalQualityRealtime();

    /**
     * @brief Lekéri csak az RSSI értéket cache-elt módon
     * @return uint8_t RSSI érték
     */
    uint8_t getRSSI();

    /**
     * @brief Lekéri csak az SNR értéket cache-elt módon
     * @return uint8_t SNR érték
     */
    uint8_t getSNR();
};

#endif // __Si4735_MANAGER_H