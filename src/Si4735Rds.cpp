#include "Si4735Rds.h"
#include "Config.h"
#include "StationData.h"

// /**
//  * @brief Lekérdezi az aktuális RDS Program Service (PS) nevet.
//  * @note Csak a MemmoryDisplay.cpp fájlban használjuk.
//  * @return String Az állomásnév, vagy üres String, ha nem elérhető.
//  */
// String Si4735Rds::getCurrentRdsProgramService() {

//     // Csak FM módban van értelme RDS-t keresni
//     if (!isCurrentBandFM()) {
//         return "";
//     }

//     si4735.getRdsStatus();                                // Frissítsük az RDS állapotát
//     if (si4735.getRdsReceived() && si4735.getRdsSync()) { // Csak ha van érvényes RDS jel
//         char *rdsPsName = si4735.getRdsText0A();          // Program Service Name (állomásnév)
//         if (rdsPsName != nullptr && strlen(rdsPsName) > 0) {
//             char tempRdsName[STATION_NAME_BUFFER_SIZE]; // STATION_NAME_BUFFER_SIZE a StationData.h-ból
//             strncpy(tempRdsName, rdsPsName, STATION_NAME_BUFFER_SIZE - 1);
//             tempRdsName[STATION_NAME_BUFFER_SIZE - 1] = '\0'; // Biztos null-terminálás
//             Utils::trimSpaces(tempRdsName);                   // Esetleges felesleges szóközök eltávolítása mindkét oldalról
//             return String(tempRdsName);
//         }
//     }

//         return ""; // Nincs érvényes RDS PS név
// }

/**
 * @brief Lekérdezi az RDS állomásnevet (Program Service)
 * @return String Az RDS állomásnév, vagy üres string ha nem elérhető
 */
String Si4735Rds::getRdsStationName() {

    // Ellenőrizzük, hogy FM módban vagyunk-e
    if (!isCurrentBandFM()) {
        return "";
    }

    // RDS státusz frissítése
    si4735.getRdsStatus();

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
uint8_t Si4735Rds::getRdsProgramTypeCode() {

    // Ellenőrizzük, hogy FM módban vagyunk-e
    if (!isCurrentBandFM()) {
        return 255; // Nincs RDS
    }

    // RDS státusz frissítése
    si4735.getRdsStatus();

    return si4735.getRdsProgramType();
}

/**
 * @brief Lekérdezi az RDS radio text üzenetet
 * @return String Az RDS radio text, vagy üres string ha nem elérhető
 */
String Si4735Rds::getRdsRadioText() {

    // Ellenőrizzük, hogy FM módban vagyunk-e
    if (!isCurrentBandFM()) {
        return "";
    }

    // RDS státusz frissítése
    si4735.getRdsStatus();

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
bool Si4735Rds::getRdsDateTime(uint16_t &year, uint16_t &month, uint16_t &day, uint16_t &hour, uint16_t &minute) {

    // Ellenőrizzük, hogy FM módban vagyunk-e
    if (!isCurrentBandFM()) {
        return false;
    }

    // RDS státusz frissítése
    si4735.getRdsStatus();

    return si4735.getRdsDateTime(&year, &month, &day, &hour, &minute);
}

/**
 * @brief Ellenőrzi, hogy elérhető-e RDS adat
 * @return true ha van érvényes RDS vétel
 */
bool Si4735Rds::isRdsAvailable() {

    // Ellenőrizzük, hogy FM módban vagyunk-e
    if (!isCurrentBandFM()) {
        return false;
    }

    // RDS státusz lekérdezése
    si4735.getRdsStatus();

    if (!si4735.getRdsReceived() or !si4735.getRdsSync() or !si4735.getRdsSyncFound()) {
        return false;
    }
    return true;
}

/**
 * @brief Lekérdezi az RDS jel minőségét
 * @return uint8_t Az RDS jel minősége (SNR érték)
 */
uint8_t Si4735Rds::getRdsSignalQuality() {
    // A cached signal quality-t használjuk, ami optimalizált
    SignalQualityData signalData = getSignalQuality();
    return signalData.isValid ? signalData.snr : 0;
}
