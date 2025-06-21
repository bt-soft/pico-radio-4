/**
 * @file RadioScreen.cpp
 * @brief Rádió vezérlő képernyő alaposztály implementáció
 * @details Seek callback infrastruktúra és rádió-specifikus funkcionalitás
 */
#include <memory>

#include "Config.h"
#include "MultiButtonDialog.h"
#include "RadioScreen.h"
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
// UIScreen interface override
// ===================================================================

/**
 * @brief RadioScreen aktiválása - Signal quality cache invalidálás
 * @details Minden RadioScreen aktiváláskor invalidálja a signal quality cache-t,
 * hogy az S-meter azonnal frissüljön az új frekvencián.
 */
void RadioScreen::activate() {
    // Szülő osztály aktiválása (UIScreen)
    UIScreen::activate();

    // Signal quality cache invalidálása képernyő aktiváláskor
    // Ez biztosítja, hogy az S-meter azonnal frissüljön, amikor visszatérünk
    // a memória képernyőről vagy másik képernyőről
    if (pSi4735Manager) {
        pSi4735Manager->invalidateSignalCache();

        // Frekvencia kijelző inicializálása az aktuális frekvenciával
        // Ez azért szükséges, mert a FreqDisplay konstruktor 0-ra inicializál
        if (freqDisplayComp) {
            uint16_t currentFreq = pSi4735Manager->getSi4735().getCurrentFrequency();
            DEBUG("RadioScreen::activate - Current frequency: %d\n", currentFreq);
            freqDisplayComp->setFrequencyWithFullDraw(currentFreq, !pSi4735Manager->isCurrentDemodSSBorCW());
        }
    }
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
 * @brief Frekvencia mentése a band táblába
 * @details Szinkronizálja az aktuális frekvenciát minden szükséges helyre
 */
void RadioScreen::saveCurrentFrequency() {
    if (pSi4735Manager) {
        // Az SI4735 osztály cache-ból olvassuk az aktuális frekvenciát, nem használunk chip olvasást
        uint32_t currentFreq = pSi4735Manager->getSi4735().getCurrentFrequency();

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

    // Az SI4735 osztály cache-ból olvassuk az aktuális frekvenciát, nem használunk chip olvasást
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

    constexpr uint16_t H_BUTTON_DEFAULT_HEIGHT = UIButton::DEFAULT_BUTTON_HEIGHT; // Alapértelmezett gomb magassága a vízszintes gombsorhoz
    constexpr uint16_t H_BUTTON_DEFAULT_WIDTH = 70;                               // Egyedi gomb szélessége
    constexpr uint16_t H_BUTTON_DEFAULT_GAP = 3;                                  // Gombok közötti távolság

    // ===================================================================
    // Gombsor pozicionálás - Bal alsó sarok
    // ===================================================================
    constexpr uint16_t buttonBarHeight = H_BUTTON_DEFAULT_HEIGHT + 10;               // Vízszintes gombsor konténer magassága (alapértelmezett + padding)
    constexpr uint16_t buttonBarX = 0;                                               // Bal szélhez igazítva
    const uint16_t buttonBarY = UIComponent::SCREEN_H - H_BUTTON_DEFAULT_HEIGHT - 5; // Gombok alsó széle a képernyő alján (5px alsó margin)

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
    if (event.state != UIButton::EventButtonState::Clicked) {
        return;
    }

    processBandButton(true); // Ham sáv kezelése
}

/**
 * @brief BAND gomb eseménykezelő - Sáv (Band) kezelés
 * @param event Gomb esemény (Clicked)
 * @details Alapértelmezett implementáció - leszármazott osztályok felülírhatják
 */
void RadioScreen::handleBandButton(const UIButton::ButtonEvent &event) {
    if (event.state != UIButton::EventButtonState::Clicked) {
        return;
    }

    processBandButton(false); // Nem Ham sávok kezelése (pl. FM, AM, LW, SW stb.)
}

/**
 * @brief Közös BAND gomb eseménykezelő - Sáv (Band) kezelés
 * @param isHamBand Igaz, ha a Ham sávot kell kezelni, hamis, ha más sáv
 */
void RadioScreen::processBandButton(bool isHamBand) {

    uint16_t dialogHeight = isHamBand ? 180 : 250; // Alapértelmezett dialógus magasság
    uint8_t _bandCount;

    // Először lekérdezzük, hogy max hány elemet kell tudnunk tárolni
    uint8_t maxBandCount = pSi4735Manager->getFilteredBandCount(isHamBand);

    // Allokáljuk a megfelelő méretű tömböt okos pointerrel
    std::unique_ptr<const char *[]> _bandNames(new const char *[maxBandCount]);

    // Betöltjük a sáv neveket a tömbünkbe
    pSi4735Manager->getBandNames(_bandNames.get(), _bandCount, isHamBand);

    // Megkeressük az aktuális sáv indexét a szűrt sávok tömbben
    int _currentBandIndex = -1; // -1 = nincs találat (ha nem található az a sáv, amiben éppen vagyunk a szűrt sávok között)
    const char *_currentBandName = pSi4735Manager->getCurrentBandName();
    for (int i = 0; i < _bandCount; i++) {
        if (strcmp(_bandNames[i], _currentBandName) == 0) {
            _currentBandIndex = i;
            break;
        }
    }
    auto bandDialog = std::make_shared<MultiButtonDialog>(
        this, this->tft,                                                              // Képernyő referencia
        "All Radio Bands", "",                                                        // Dialógus címe és üzenete
        _bandNames.get(), _bandCount,                                                 // Gombok feliratai és számuk
        [this](int buttonIndex, const char *buttonLabel, MultiButtonDialog *dialog) { // Gomb kattintás kezelése
            // Átállítjuk a használni kívánt BAND indexét
            config.data.currentBandIdx = pSi4735Manager->getBandIdxByBandName(buttonLabel);

            // Átállítjuk a rádiót a kiválasztott sávra
            pSi4735Manager->init();

            // Jelezzük, hogy ez band dialógus volt - az onDialogClosed fogja kezelni
            lastDialogWasBandDialog = true;
        },
        true,                           // Automatikusan bezárja-e a dialógust gomb kattintáskor
        _currentBandIndex,              // Az alapértelmezett (jelenlegi) gomb indexe a szűrt sávok tömbjében (-1 = ha nem található a sávok nevei között)
        true,                           // Ha true, az alapértelmezett gomb le van tiltva; ha false, csak vizuálisan kiemelve
        Rect(-1, -1, 400, dialogHeight) // Dialógus mérete (ha -1, akkor automatikusan a képernyő közepére igazítja)
    );
    this->showDialog(bandDialog);

    // Az okos pointer automatikusan felszabadítja a memóriát a scope végén
}

/**
 * @brief SCAN gomb eseménykezelő - Folyamatos keresés
 * @param event Gomb esemény (Clicked)
 * @details Átkapcsol a ScanScreen-re spektrum analizátor funkcionalitással
 */
void RadioScreen::handleScanButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("RadioScreen::handleScanButton - Switching to ScanScreen\n");
        getScreenManager()->switchToScreen(SCREEN_NAME_SCAN);
    }
}

// ===================================================================
// S-Meter (jelerősség mérő) kezelés
// ===================================================================

/**
 * @brief S-Meter frissítése optimalizált időzítéssel
 * @param isFMMode true = FM mód, false = AM mód
 * @details 250ms-es intervallummal frissíti az S-meter-t (4 Hz)
 * Belső változás detektálással - csak szükség esetén rajzol újra
 */
void RadioScreen::updateSMeter(bool isFMMode) {
    static uint32_t lastSmeterUpdate = 0;
    uint32_t currentTime = millis();

    // S-meter frissítés 250ms-enként (4 Hz) - elegendő a vizuális visszajelzéshez
    if (smeterComp && (currentTime - lastSmeterUpdate >= 250)) {
        // Cache-elt jelerősség adatok lekérése a Si4735Manager-től
        SignalQualityData signalCache = pSi4735Manager->getSignalQuality();
        if (signalCache.isValid) {
            // RSSI és SNR megjelenítése a megfelelő módban
            smeterComp->showRSSI(signalCache.rssi, signalCache.snr, isFMMode);
        }
        lastSmeterUpdate = currentTime;
    }
}

/**
 * @brief Band váltás kezelése dialog bezárás után
 * @param dialog A bezárandó dialógus
 * @details Egyszerű metódus, ami elvégzi a dialog cleanup-ot és a szükséges képernyőváltást/refresh-t
 */
void RadioScreen::handleBandSwitchAfterDialog(UIDialogBase *dialog) {
    // Dialog cleanup rajzolás nélkül
    performDialogCleanupWithoutDraw(dialog);

    // Ellenőrizzük, hogy szükséges-e képernyőváltás
    if (pSi4735Manager) {
        const char *targetScreenName = pSi4735Manager->isCurrentBandFM() ? SCREEN_NAME_FM : SCREEN_NAME_AM;

        if (!STREQ(this->getName(), targetScreenName)) {
            // === KÉPERNYŐVÁLTÁS ===
            getScreenManager()->switchToScreen(targetScreenName);
        } else {
            // === UGYANAZON KÉPERNYŐ - GYORS REFRESH ===
            refreshScreenComponents();
        }
    }
}

/**
 * @brief A kijelző explicit frissítése
 * @details Frissíti a kijelzőt komponenseit
 * Hasznos band váltás után, amikor ugyanaz a screen marad aktív
 */
void RadioScreen::refreshScreenComponents() {

    // Frekvenciakijelző frissítése
    if (freqDisplayComp && pSi4735Manager) {
        // Az SI4735 osztály cache-ból olvassuk az aktuális frekvenciát, nem használunk chip olvasást
        uint16_t currentFreq = pSi4735Manager->getSi4735().getCurrentFrequency();

        // Teljes újrarajzolás, finomhangolás csak SSB/CW módban
        freqDisplayComp->setFrequencyWithFullDraw(currentFreq, !pSi4735Manager->isCurrentDemodSSBorCW());
    }
}

// ===================================================================
// UIScreen interface override - Dialog handling
// ===================================================================

/**
 * @brief Dialógus bezárásának kezelése
 * @param closedDialog A bezárt dialógus referencia
 * @details Normál UIScreen cleanup, vagy band switch ha szükséges
 */
void RadioScreen::onDialogClosed(UIDialogBase *closedDialog) {

    if (lastDialogWasBandDialog) {
        // === BAND DIALÓGUS VOLT ===
        // A band konfiguráció már be van állítva a callback-ben, csak a cleanup kell
        lastDialogWasBandDialog = false; // Reset flag
        handleBandSwitchAfterDialog(closedDialog);
    } else {
        // === NORMÁL DIALÓGUS (X gomb, Cancel, stb.) ===
        // Normál UIScreen dialog cleanup
        UIScreen::onDialogClosed(closedDialog);
    }
}
