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
// RDS Support - PTY (Program Type) tábla
// ===================================================================

/**
 * @brief RDS Program Type (PTY) nevek táblája
 * @details Az RDS standard 32 különböző program típust definiál (0-31).
 * Minden PTY kódhoz tartozik egy szöveges leírás.
 */
const char *RDS_PTY_NAMES[] = {
    "No defined",            // 0
    "News",                  // 1
    "Current affairs",       // 2
    "Information",           // 3
    "Sport",                 // 4
    "Education",             // 5
    "Drama",                 // 6
    "Culture",               // 7
    "Science",               // 8
    "Varied",                // 9
    "Pop Music",             // 10
    "Rock Music",            // 11
    "Easy Listening",        // 12
    "Light Classical",       // 13
    "Serious Classical",     // 14
    "Other Music",           // 15
    "Weather",               // 16
    "Finance",               // 17
    "Children's Programmes", // 18
    "Social Affairs",        // 19
    "Religion",              // 20
    "Phone-in",              // 21
    "Travel",                // 22
    "Leisure",               // 23
    "Jazz Music",            // 24
    "Country Music",         // 25
    "National Music",        // 26
    "Oldies Music",          // 27
    "Folk Music",            // 28
    "Documentary",           // 29
    "Alarm Test",            // 30
    "Alarm"                  // 31
};

#define RDS_PTY_COUNT (sizeof(RDS_PTY_NAMES) / sizeof(RDS_PTY_NAMES[0]))

// ===================================================================
// RDS Support - Implementáció
// ===================================================================

/**
 * @brief Lekérdezi az RDS állomásnevet (Program Service)
 * @return String Az RDS állomásnév, vagy üres string ha nem elérhető
 */
String Si4735Manager::getRdsStationName() {
    if (!isRdsAvailable()) {
        DEBUG("Si4735Manager::getRdsStationName() - RDS nem elérhető\n");
        return "";
    }

    char *rdsStationName = si4735.getRdsText0A();
    if (rdsStationName != nullptr && strlen(rdsStationName) > 0) {
        char tempName[32];
        strncpy(tempName, rdsStationName, sizeof(tempName) - 1);
        tempName[sizeof(tempName) - 1] = '\0';
        Utils::trimSpaces(tempName);
        String result = String(tempName);
        DEBUG("Si4735Manager::getRdsStationName() - Állomásnév: %s\n", result.c_str());
        return result;
    }

    DEBUG("Si4735Manager::getRdsStationName() - Üres vagy null állomásnév\n");
    return "";
}

/**
 * @brief Lekérdezi az RDS program típust (PTY)
 * @return String Az RDS program típus szöveges leírása
 */
String Si4735Manager::getRdsProgramType() {
    if (!isRdsAvailable()) {
        return "";
    }

    uint8_t ptyCode = si4735.getRdsProgramType();
    if (ptyCode < RDS_PTY_COUNT) {
        return String(RDS_PTY_NAMES[ptyCode]);
    }

    return "Unknown PTY";
}

/**
 * @brief Lekérdezi az RDS radio text üzenetet
 * @return String Az RDS radio text, vagy üres string ha nem elérhető
 */
String Si4735Manager::getRdsRadioText() {
    if (!isRdsAvailable()) {
        DEBUG("Si4735Manager::getRdsRadioText() - RDS nem elérhető\n");
        return "";
    }

    char *rdsText = si4735.getRdsText2A();
    if (rdsText != nullptr && strlen(rdsText) > 0) {
        char tempText[128];
        strncpy(tempText, rdsText, sizeof(tempText) - 1);
        tempText[sizeof(tempText) - 1] = '\0';
        Utils::trimSpaces(tempText);
        String result = String(tempText);
        DEBUG("Si4735Manager::getRdsRadioText() - Radio text: %s\n", result.c_str());
        return result;
    }

    DEBUG("Si4735Manager::getRdsRadioText() - Üres vagy null radio text\n");
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

    // RDS státusz lekérdezése
    si4735.getRdsStatus();

    // Kevésbé szigorú ellenőrzés - elég ha RDS fogadás történik
    // Az eredeti túl szigorú volt: si4735.getRdsReceived() && si4735.getRdsSync() && si4735.getRdsSyncFound()
    bool rdsReceived = si4735.getRdsReceived();
    bool rdsSync = si4735.getRdsSync();

    // Debug logolás csak ritkán, hogy ne spammelje a konzolt
    static unsigned long lastDebugLog = 0;
    if (millis() - lastDebugLog > 10000) { // 10 másodpercenként
        DEBUG("Si4735Manager::isRdsAvailable() - RdsReceived: %s, RdsSync: %s\n", rdsReceived ? "true" : "false", rdsSync ? "true" : "false");
        lastDebugLog = millis();
    }

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

    // Signal quality cache frissítése, ha szükséges
    updateSignalCacheIfNeeded(); // RDS status debug - csak FM módban és 5 másodpercenként
    static unsigned long lastRdsDebug = 0;
    if (millis() - lastRdsDebug > 5000) {
        BandTable &band = getCurrentBand();
        if (band.bandType == FM_BAND_TYPE) {
            si4735.getRdsStatus();
            bool rdsReceived = si4735.getRdsReceived();
            bool rdsSync = si4735.getRdsSync();
            bool rdsSyncFound = si4735.getRdsSyncFound();

            // Signal quality információ
            SignalQualityData signalData = getSignalQuality();
            uint16_t currentFreq = band.currFreq;

            DEBUG("Si4735Manager::loop() - Freq: %d kHz (%.2f MHz), Signal: RSSI=%d SNR=%d\n", currentFreq, currentFreq / 100.0, signalData.rssi, signalData.snr);
            DEBUG("Si4735Manager::loop() - RDS Status: Received=%s, Sync=%s, SyncFound=%s\n", rdsReceived ? "true" : "false", rdsSync ? "true" : "false",
                  rdsSyncFound ? "true" : "false");
        }
        lastRdsDebug = millis();
    }
}