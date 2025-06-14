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

    bool isInMemory = checkCurrentFrequencyInMemory();

    // StatusLine frissítése ha létezik
    if (statusLineComp) {
        // Frissítjük a StatusLine komponenst az aktuális állomás memóriában lévő státuszával
        statusLineComp->updateStationInMemory(isInMemory);
    }

    return isInMemory;
}

// ===================================================================
// Közös vízszintes gombsor implementáció
// ===================================================================

/**
 * @brief Közös vízszintes gombsor létrehozása és inicializálása
 * @details Létrehozza a közös gombokat, amiket minden RadioScreen használ
 */
void RadioScreen::createCommonHorizontalButtons() {

    constexpr uint16_t H_BUTTON_DEFAULT_HEIGHT = 40; // Alapértelmezett gomb magassága a vízszintes gombsorhoz
    constexpr uint16_t H_BUTTON_DEFAULT_WIDTH = 70;  // Egyedi gomb szélessége
    constexpr uint16_t H_BUTTON_DEFAULT_GAP = 3;     // Gombok közötti távolság

    // ===================================================================
    // Gombsor pozicionálás - Bal alsó sarok
    // ===================================================================
    constexpr uint16_t buttonBarHeight = H_BUTTON_DEFAULT_HEIGHT + 10;   // Vízszintes gombsor konténer magassága (alapértelmezett + padding)
    constexpr uint16_t buttonBarX = 0;                                   // Bal szélhez igazítva
    const uint16_t buttonBarY = UIComponent::SCREEN_H - buttonBarHeight; // Alsó szélhez igazítva

    // ===================================================================
    // Közös gomb konfigurációk
    // ===================================================================
    std::vector<UIHorizontalButtonBar::ButtonConfig> buttonConfigs = //
        {
            // 1. HAM - Ham rádió funkcionalitás
            {CommonHorizontalButtonIDs::HAM_BUTTON, "Ham", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
             [this](const UIButton::ButtonEvent &event) { handleHamButton(event); }},

            // 2. BAND - Sáv (Band) kezelés
            {CommonHorizontalButtonIDs::BAND_BUTTON, "Band", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
             [this](const UIButton::ButtonEvent &event) { handleBandButton(event); }},

            // 3. SCAN - Folyamatos keresés
            {CommonHorizontalButtonIDs::SCAN_BUTTON, "Scan", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
             [this](const UIButton::ButtonEvent &event) { handleScanButton(event); }} //
        };

    // ===================================================================
    // Leszármazott osztályok specifikus gombjai
    // ===================================================================
    addSpecificHorizontalButtons(buttonConfigs);

    // ===================================================================
    // Gombsor szélessége dinamikus számítás
    // ===================================================================
    const uint16_t buttonBarWidth = buttonConfigs.size() * H_BUTTON_DEFAULT_WIDTH + (buttonConfigs.size() - 1) * H_BUTTON_DEFAULT_GAP; // Dinamikus szélesség

    // ===================================================================
    // UIHorizontalButtonBar objektum létrehozása
    // ===================================================================
    horizontalButtonBar = std::make_shared<UIHorizontalButtonBar>(     //
        tft,                                                           // TFT display referencia
        Rect(buttonBarX, buttonBarY, buttonBarWidth, buttonBarHeight), // Gombsor pozíció és méret
        buttonConfigs,                                                 // Gomb konfigurációk
        H_BUTTON_DEFAULT_WIDTH,                                        // Egyedi gomb szélessége
        H_BUTTON_DEFAULT_HEIGHT,                                       // Egyedi gomb magassága
        H_BUTTON_DEFAULT_GAP                                           // Gombok közötti távolság
    );

    // Komponens hozzáadása a képernyőhöz
    addChild(horizontalButtonBar);
}

/**
 * @brief Közös vízszintes gombsor állapotainak szinkronizálása
 * @details Csak aktiváláskor hívódik meg! Event-driven architektúra.
 */
void RadioScreen::updateCommonHorizontalButtonStates() {
    if (!horizontalButtonBar)
        return;

    // Alapértelmezett állapotok - a leszármazott osztályok felülírhatják
    // Ham gomb: alapértelmezetten kikapcsolva
    horizontalButtonBar->setButtonState(CommonHorizontalButtonIDs::HAM_BUTTON, UIButton::ButtonState::Off);

    // Band gomb: alapértelmezetten kikapcsolva
    horizontalButtonBar->setButtonState(CommonHorizontalButtonIDs::BAND_BUTTON, UIButton::ButtonState::Off);

    // Scan gomb: alapértelmezetten kikapcsolva
    horizontalButtonBar->setButtonState(CommonHorizontalButtonIDs::SCAN_BUTTON, UIButton::ButtonState::Off);
}

// ===================================================================
// Közös gomb eseménykezelők - Alapértelmezett implementáció
// ===================================================================

/**
 * @brief HAM gomb eseménykezelő - Ham rádió funkcionalitás
 * @param event Gomb esemény (Clicked)
 * @details Alapértelmezett implementáció - leszármazott osztályok felülírhatják
 */
void RadioScreen::handleHamButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        // TODO: Ham rádió funkcionalitás implementálása
        // Jelenleg placeholder - később implementáljuk
        Serial.println("RadioScreen::handleHamButton - Ham funkció (TODO)");
    }
}

/**
 * @brief BAND gomb eseménykezelő - Sáv (Band) kezelés
 * @param event Gomb esemény (Clicked)
 * @details Alapértelmezett implementáció - leszármazott osztályok felülírhatják
 */
void RadioScreen::handleBandButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        // TODO: Band váltás funkcionalitás implementálása
        // Jelenleg placeholder - később implementáljuk
        Serial.println("RadioScreen::handleBandButton - Band váltás (TODO)");
    }
}

/**
 * @brief SCAN gomb eseménykezelő - Folyamatos keresés
 * @param event Gomb esemény (Clicked)
 * @details Alapértelmezett implementáció - leszármazott osztályok felülírhatják
 */
void RadioScreen::handleScanButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        // TODO: Scan (folyamatos keresés) funkcionalitás implementálása
        // Jelenleg placeholder - később implementáljuk
        Serial.println("RadioScreen::handleScanButton - Scan funkció (TODO)");
    }
}
