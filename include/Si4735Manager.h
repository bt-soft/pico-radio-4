#ifndef __Si4735_MANAGER_H
#define __Si4735_MANAGER_H

#include "Config.h"
#include "Si4735Rds.h"

class Si4735Manager : public Si4735Rds {

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

    /**
     * Loop függvény a squelchez és a hardver némításhoz.
     * Ez a függvény folyamatosan figyeli a squelch állapotát és kezeli a hardver némítást.
     */
    void loop();
};

#endif // __Si4735_MANAGER_H