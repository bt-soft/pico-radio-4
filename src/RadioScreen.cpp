/**
 * @file RadioScreen.cpp
 * @brief Rádió vezérlő képernyő alaposztály implementáció
 * @details Seek callback infrastruktúra és rádió-specifikus funkcionalitás
 */

#include "RadioScreen.h"
#include "Config.h"
#include "SI4735.h"
#include "StationStore.h"

// ===================================================================
// Konstruktor és inicializálás
// ===================================================================

/**
 * @brief RadioScreen konstruktor - Rádió képernyő alaposztály inicializálás
 * @param tft TFT display referencia
 * @param name Képernyő egyedi neve
 * @param si4735Manager Si4735 rádió chip kezelő referencia
 */
RadioScreen::RadioScreen(TFT_eSPI &tft, const char *name, Si4735Manager *si4735Manager) : UIScreen(tft, name, si4735Manager) {
    // A leszármazott osztályok fogják létrehozni a specifikus komponenseket
}

// ===================================================================
// Seek callback infrastruktúra
// ===================================================================

/// Static pointer az aktuális RadioScreen instance-ra (seek callback-hez)
static RadioScreen *g_currentSeekingRadioScreen = nullptr;

/**
 * @brief Callback függvény a SI4735::seekStationProgress számára
 * @param frequency Aktuális frekvencia a seek során
 *
 * @details C-style callback, ami frissíti a frekvencia kijelzőt seek közben.
 * A g_currentSeekingRadioScreen static pointer-en keresztül éri el az instance-t.
 */
void radioSeekProgressCallback(uint16_t frequency) {
    if (g_currentSeekingRadioScreen && g_currentSeekingRadioScreen->freqDisplayComp) {
        g_currentSeekingRadioScreen->freqDisplayComp->setFrequency(frequency);
        g_currentSeekingRadioScreen->freqDisplayComp->draw(); // Azonnali frissítés a kijelzőn
    }
}

// ===================================================================
// Seek (automatikus állomáskeresés) implementáció
// ===================================================================

/**
 * @brief Seek keresés indítása lefelé valós idejű frekvencia frissítéssel
 * @details Beállítja a callback infrastruktúrát és indítja a seek-et
 */
void RadioScreen::seekStationDown() {
    if (pSi4735Manager) {
        // Static pointer beállítása a callback számára
        g_currentSeekingRadioScreen = this;

        // Seek lefelé valós idejű frekvencia frissítéssel
        pSi4735Manager->getSi4735().seekStationProgress(radioSeekProgressCallback, SEEK_DOWN);

        // Static pointer nullázása
        g_currentSeekingRadioScreen = nullptr;

        // Seek befejezése után: konfiguráció és band tábla frissítése
        saveCurrentFrequency();
    }
}

/**
 * @brief Seek keresés indítása felfelé valós idejű frekvencia frissítéssel
 * @details Beállítja a callback infrastruktúrát és indítja a seek-et
 */
void RadioScreen::seekStationUp() {
    if (pSi4735Manager) {
        // Static pointer beállítása a callback számára
        g_currentSeekingRadioScreen = this;

        // Seek felfelé valós idejű frekvencia frissítéssel
        pSi4735Manager->getSi4735().seekStationProgress(radioSeekProgressCallback, SEEK_UP);

        // Static pointer nullázása
        g_currentSeekingRadioScreen = nullptr;

        // Seek befejezése után: konfiguráció és band tábla frissítése
        saveCurrentFrequency();
    }
}

// ===================================================================
// Rádió-specifikus utility metódusok
// ===================================================================

/**
 * @brief Frekvencia mentése a konfigurációba és band táblába
 * @details Szinkronizálja az aktuális frekvenciát minden szükséges helyre
 */
void RadioScreen::saveCurrentFrequency() {
    if (pSi4735Manager) {
        // Aktuális frekvencia lekérése a SI4735-ből
        uint32_t currentFreq = pSi4735Manager->getSi4735().getCurrentFrequency();

        // Konfiguráció frissítése
        config.data.currentFrequency = currentFreq;

        // Band tábla frissítése
        pSi4735Manager->getCurrentBand().currFreq = currentFreq;
    }
}

/**
 * @brief Ellenőrzi, hogy az aktuális frekvencia benne van-e a memóriában
 * @return true ha a frekvencia elmentett állomás, false egyébként
 */
bool RadioScreen::checkCurrentFrequencyInMemory() const {
    if (!pSi4735Manager) {
        return false;
    }

    // Aktuális frekvencia és band index lekérése
    uint32_t currentFreq = pSi4735Manager->getSi4735().getCurrentFrequency();
    uint8_t currentBandIdx = config.data.currentBandIdx;

    bool isInMemory = false;

    // Ellenőrizzük a megfelelő station store-ban a band típus alapján
    if (currentBandIdx == 0) {
        // FM band - FM StationStore-ban keresünk
        int foundIndex = fmStationStore.findStation(currentFreq, currentBandIdx);
        isInMemory = (foundIndex >= 0);
    } else {
        // AM/MW/LW/SW bandek - AM StationStore-ban keresünk
        int foundIndex = amStationStore.findStation(currentFreq, currentBandIdx);
        isInMemory = (foundIndex >= 0);
    }

    return isInMemory;
}

/**
 * @brief Ellenőrzi, hogy az aktuális frekvencia benne van-e a memóriában
 * @return true ha a frekvencia elmentett állomás, false egyébként
 *
 * @details Ellenőrzi az aktuális frekvenciát a StationStore memóriában.
 * Ha talál egyezést, frissíti a StatusLine státuszát is.
 */
bool RadioScreen::checkAndUpdateMemoryStatus() {
    DEBUG("RadioScreen::checkAndUpdateMemoryStatus() called\n");

    bool isInMemory = checkCurrentFrequencyInMemory();

    DEBUG("Station in memory: %s\n", isInMemory ? "YES" : "NO");

    // StatusLine frissítése ha létezik
    if (statusLineComp) {
        statusLineComp->updateStationInMemory(isInMemory);
        DEBUG("StatusLine updated with memory status\n");
    } else {
        DEBUG("No StatusLine component to update\n");
    }

    return isInMemory;
}
