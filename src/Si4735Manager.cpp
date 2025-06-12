#include "Si4735Manager.h"

/**
 * @brief Konstruktor, amely inicializálja a Si4735 eszközt.
 * @param config A konfigurációs objektum, amely tartalmazza a beállításokat.
 * @param band A Band objektum, amely kezeli a rádió sávokat.
 */
Si4735Manager::Si4735Manager() : Si4735Band(), currentBandIdx(-1) {
    setAudioMuteMcuPin(PIN_AUDIO_MUTE); // Audio Mute pin
    // Audio unmute
    si4735.setAudioMute(false);
}

/**
 * @brief Inicializáljuk az osztályt, beállítjuk a rádió sávot és hangerőt.
 */
void Si4735Manager::init() { // Band init, ha változott az épp használt band
    if (currentBandIdx != config.data.currentBandIdx) {

        // A Band  visszaállítása a konfigból
        bandInit(currentBandIdx == -1); // Rendszer induláskor -1 a currentBandIdx változást figyelő flag

        // A sávra preferált demodulációs mód betöltése
        bandSet(true); // Hangerő beállítása
        si4735.setVolume(config.data.currVolume);

        currentBandIdx = config.data.currentBandIdx;
    }

    // Rögtön be is állítjuk az AGC-t
    checkAGC();
}

/**
 * A BFO lépésközöket állítja be, csak SSB módban működik
 * A BFO lépésközök a következő értékek lehetnek: 1, 10, 25 Hz.
 */
void Si4735Manager::setStep() {
    BandTable &currentBand = getCurrentBand();
    uint8_t currMod = currentBand.currMod;

    if (rtv::bfoOn && (currMod == LSB or currMod == USB or currMod == CW)) {
        if (config.data.currentBFOStep == 1)
            config.data.currentBFOStep = 10;
        else if (config.data.currentBFOStep == 10)
            config.data.currentBFOStep = 25;
        else
            config.data.currentBFOStep = 1;
    }

    if (!rtv::SCANbut) {
        useBand();
        checkAGC();
    }
}

/**
 * @brief Lekérdezi az aktuális RDS Program Service (PS) nevet.
 * @note Csak a MemmoryDisplay.cpp fájlban használjuk.
 * @return String Az állomásnév, vagy üres String, ha nem elérhető.
 */
String Si4735Manager::getCurrentRdsProgramService() {
    // Csak FM módban van értelme RDS-t keresni
    if (getCurrentBandType() != FM_BAND_TYPE) {
        return "";
    }

    si4735.getRdsStatus();                                // Frissítsük az RDS állapotát
    if (si4735.getRdsReceived() && si4735.getRdsSync()) { // Csak ha van érvényes RDS jel
        char *rdsPsName = si4735.getRdsText0A();          // Program Service Name (állomásnév)
        if (rdsPsName != nullptr && strlen(rdsPsName) > 0) {
            char tempRdsName[STATION_NAME_BUFFER_SIZE]; // STATION_NAME_BUFFER_SIZE a StationData.h-ból
            strncpy(tempRdsName, rdsPsName, STATION_NAME_BUFFER_SIZE - 1);
            tempRdsName[STATION_NAME_BUFFER_SIZE - 1] = '\0'; // Biztos null-terminálás
            Utils::trimSpaces(tempRdsName);                   // Esetleges felesleges szóközök eltávolítása mindkét oldalról
            return String(tempRdsName);
        }
    }
    return ""; // Nincs érvényes RDS PS név
}

/**
 * Loop függvény a squelchez és a hardver némításhoz.
 * Ez a függvény folyamatosan figyeli a squelch állapotát és kezeli a hardver némítást.
 */
void Si4735Manager::loop() {

    // Squelch kezelése
    manageSquelch();

    // Hardver némítás kezelése
    manageHardwareAudioMute();
    // Signal quality cache frissítése, ha szükséges
    updateSignalCacheIfNeeded();
}