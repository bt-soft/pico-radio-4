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

// ===================================================================
// RDS Support - Implementáció
// ===================================================================

/**
 * @brief Lekérdezi az RDS állomásnevet (Program Service)
 * @return String Az RDS állomásnév, vagy üres string ha nem elérhető
 */
String Si4735Manager::getRdsStationName() {
    if (!isRdsAvailable()) {
        return "";
    }
    char *rdsStationName = si4735.getRdsText0A();
    if (rdsStationName != nullptr && strlen(rdsStationName) > 0) {
        char tempName[32];
        strncpy(tempName, rdsStationName, sizeof(tempName) - 1);
        tempName[sizeof(tempName) - 1] = '\0';
        Utils::trimSpaces(tempName);
        String result = String(tempName);
        return result;
    }

    return "";
}

/**
 * @brief Lekérdezi az RDS program típus kódot (PTY)
 * @return uint8_t Az RDS program típus kódja (0-31), vagy 255 ha nincs RDS
 */
uint8_t Si4735Manager::getRdsProgramTypeCode() {
    if (!isRdsAvailable()) {
        return 255; // Nincs RDS
    }

    return si4735.getRdsProgramType();
}

/**
 * @brief Lekérdezi az RDS radio text üzenetet
 * @return String Az RDS radio text, vagy üres string ha nem elérhető
 */
String Si4735Manager::getRdsRadioText() {
    if (!isRdsAvailable()) {
        return "";
    }

    char *rdsText = si4735.getRdsText2A();
    if (rdsText != nullptr && strlen(rdsText) > 0) {
        char tempText[128];
        strncpy(tempText, rdsText, sizeof(tempText) - 1);
        tempText[sizeof(tempText) - 1] = '\0';
        Utils::trimSpaces(tempText);
        String result = String(tempText);
        return result;
    }

    return "";
}

/**
 * @brief Lekérdezi az RDS dátum és idő információt
 * @param year Referencia a év tárolásához
 * @param month Referencia a hónap tárolásához
 * @param day Referencia a nap tárolásához
 * @param hour Referencia az óra tárolásához
 * @param minute Referencia a perc tárolásához
 * @return true ha sikerült lekérdezni a dátum/idő adatokat
 */
bool Si4735Manager::getRdsDateTime(uint16_t &year, uint16_t &month, uint16_t &day, uint16_t &hour, uint16_t &minute) {
    if (!isRdsAvailable()) {
        return false;
    }

    return si4735.getRdsDateTime(&year, &month, &day, &hour, &minute);
}

/**
 * @brief Ellenőrzi, hogy elérhető-e RDS adat
 * @return true ha van érvényes RDS vétel
 */
bool Si4735Manager::isRdsAvailable() {
    // Ellenőrizzük, hogy FM módban vagyunk-e
    BandTable &band = getCurrentBand();
    if (band.bandType != FM_BAND_TYPE) {
        return false;
    }

    // RDS státusz lekérdezése    si4735.getRdsStatus();

    // Kevésbé szigorú ellenőrzés - elég ha RDS fogadás történik
    // Az eredeti túl szigorú volt: si4735.getRdsReceived() && si4735.getRdsSync() && si4735.getRdsSyncFound()
    bool rdsReceived = si4735.getRdsReceived();

    // Ha van RDS vétel, akkor elérhető (sync nélkül is)
    return rdsReceived;
}

/**
 * @brief Lekérdezi az RDS jel minőségét
 * @return uint8_t Az RDS jel minősége (SNR érték)
 */
uint8_t Si4735Manager::getRdsSignalQuality() {
    // A cached signal quality-t használjuk, ami optimalizált
    SignalQualityData signalData = getSignalQuality();
    return signalData.isValid ? signalData.snr : 0;
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

    // Signal quality cache frissítése, ha szükséges    updateSignalCacheIfNeeded();
}