/**
 * @file AMScreen.cpp
 * @brief AM rádió vezérlő képernyő implementáció
 * @details Event-driven gombállapot kezeléssel, optimalizált teljesítménnyel és
 *          univerzális gomb ID rendszer használatával
 *
 * **ARCHITEKTÚRA - Event-driven button state management**:
 * Ez az implementáció teljes mértékben Event-driven architektúrát használ:
 * - NINCS gombállapot polling a loop ciklusban (teljesítmény optimalizálás)
 * - Gombállapotok CSAK aktiváláskor szinkronizálódnak
 * - Jelentős teljesítményjavulás a korábbi polling megközelítéshez képest
 * - Univerzális gomb ID rendszer (CommonVerticalButtons) használata
 *
 * **PROJEKTÖSSZETEVŐK**:
 * - Közös függőleges gombsor az FMScreen-nel (8 univerzális funkcionális gomb)
 * - AM/MW/LW/SW frekvencia hangolás és megjelenítés
 * - S-Meter (jelerősség és SNR) valós idejű frissítés AM módban
 * - Vízszintes navigációs gombsor FM gombbal az FMScreen-re váltáshoz
 *
 * **VÁLTOZÁSOK (v3.0 - 2025.06.11)**:
 * - Univerzális gomb ID rendszer bevezetése
 * - Duplikált gombkezelési kód eliminálása (~25 sor eltávolítva)
 * - Template komplexitás megszüntetése (AMScreenButtonIDStruct eltávolítva)
 * - Közös factory pattern az FMScreen-nel
 * - Egyszerűsített metódus hívások
 *
 * @author Rádió projekt
 * @version 3.0 - Univerzális gomb ID rendszer (2025.06.11)
 */

#include "AMScreen.h"
#include "CommonVerticalButtons.h"
#include "defines.h"
#include "rtVars.h"
#include "utils.h"
#include <algorithm>

// ===================================================================
// Vízszintes gombsor azonosítók - Képernyő-specifikus navigáció
// ===================================================================

/**
 * @brief Vízszintes gombsor gomb azonosítók (AM képernyő specifikus)
 * @details Alsó navigációs gombsor - képernyőváltáshoz használt gombok
 *
 * **ID tartomány**: 40-42 (nem ütközik a univerzális 10-17 és FM 20-22 tartománnyal)
 * **Funkció**: Képernyők közötti navigáció (FM, Test, Setup)
 * **Gomb típus**: Pushable (egyszeri nyomás → képernyőváltás)
 */
namespace AMScreenHorizontalButtonIDs {
static constexpr uint8_t FM_BUTTON = 40;    ///< FM képernyőre váltás (pushable)
static constexpr uint8_t TEST_BUTTON = 41;  ///< Test képernyőre váltás (pushable)
static constexpr uint8_t SETUP_BUTTON = 42; ///< Setup képernyőre váltás (pushable)
} // namespace AMScreenHorizontalButtonIDs

// =====================================================================
// Konstruktor és inicializálás
// =====================================================================

/**
 * @brief AMScreen konstruktor implementáció - FMScreen mintájára
 * @param tft TFT display referencia
 * @param si4735Manager Si4735 rádió chip kezelő referencia
 */
AMScreen::AMScreen(TFT_eSPI &tft, Si4735Manager &si4735Manager) : UIScreen(tft, SCREEN_NAME_AM, &si4735Manager) {

    // UI komponensek létrehozása és elhelyezése
    layoutComponents();
}

// =====================================================================
// UIScreen interface megvalósítás
// =====================================================================

/**
 * @brief Rotary encoder eseménykezelés - AM frekvencia hangolás implementáció
 * @param event Rotary encoder esemény (forgatás irány, érték, gombnyomás)
 * @return true ha sikeresen kezelte az eseményt, false egyébként
 *
 * @details AM frekvencia hangolás logika (TODO: implementálás szükséges):
 * - Csak akkor reagál, ha nincs aktív dialógus
 * - Rotary klikket figyelmen kívül hagyja (más funkciókhoz)
 * - AM/MW/LW/SW frekvencia léptetés és mentés a band táblába
 * - Frekvencia kijelző azonnali frissítése
 * - Hasonló az FMScreen rotary kezeléshez, de AM-specifikus tartományokkal
 */
bool AMScreen::handleRotary(const RotaryEvent &event) {
    // Biztonsági ellenőrzés: csak forgatás eseményt dolgozzuk fel
    if (event.direction == RotaryEvent::Direction::None) {
        return false;
    }

    // TODO: AM frekvencia hangolás implementálása
    // Ez az FMScreen rotary kezeléshez hasonló lesz, de AM tartományokkal
    // pSi4735Manager->stepFrequency(event.value);  // AM-specifikus léptetés
    // if (freqDisplayComp) {
    //     uint16_t currentRadioFreq = pSi4735Manager->getCurrentBand().currFreq;
    //     freqDisplayComp->setFrequency(currentRadioFreq);
    // }

    return true;
}

/**
 * @brief Folyamatos loop hívás - Event-driven optimalizált implementáció
 * @details Csak valóban szükséges frissítések - NINCS folyamatos gombállapot pollozás!
 *
 * Csak az alábbi komponenseket frissíti minden ciklusban:
 * - S-Meter (jelerősség) - valós idejű adat AM módban
 *
 * Gombállapotok frissítése CSAK:
 * - Képernyő aktiválásakor (activate() metódus)
 * - Specifikus eseményekkor (eseménykezelőkben)
 *
 * **Event-driven előnyök**:
 * - Jelentős teljesítményjavulás a korábbi polling-hoz képest
 * - CPU terhelés csökkentése
 * - Univerzális gombkezelés (CommonVerticalButtons)
 */
void AMScreen::handleOwnLoop() {
    // ===================================================================
    // *** OPTIMALIZÁLT ARCHITEKTÚRA - NINCS GOMBÁLLAPOT POLLING! ***
    // ===================================================================

    // S-Meter (jelerősség és SNR) valós idejű frissítése AM módban
    static unsigned long lastSMeterUpdate = 0;
    if (millis() - lastSMeterUpdate > 200) { // 200ms frissítési gyakorisággal
        // TODO: S-Meter megjelenítés frissítése AM módban
        // if (smeterComp) {
        //     SignalQualityData signalCache = pSi4735Manager->getSignalQuality();
        //     if (signalCache.isValid) {
        //         smeterComp->showRSSI(signalCache.rssi, signalCache.snr, false /* AM mód */);
        //     }
        // }
        lastSMeterUpdate = millis();
    }
}

/**
 * @brief Statikus képernyő tartalom kirajzolása - AM képernyő specifikus elemek
 * @details Csak a statikus UI elemeket rajzolja ki (nem változó tartalom):
 * - S-Meter skála vonalak és számok (AM módhoz optimalizálva)
 * - Band információs terület (AM/MW/LW/SW jelzők)
 * - Statikus címkék és szövegek
 *
 * A dinamikus tartalom (pl. S-Meter érték, frekvencia) a loop()-ban frissül.
 *
 * **TODO implementációk**:
 * - S-Meter skála: RSSI alapú AM skála (0-60 dB tartomány)
 * - Band indikátor: Aktuális band típus megjelenítése
 * - Frekvencia egység: kHz/MHz megfelelő formátumban
 */
void AMScreen::drawContent() {
    // TODO: S-Meter statikus skála kirajzolása AM módban
    // if (smeterComp) {
    //     smeterComp->drawAmeterScale(); // AM-specifikus skála
    // }

    // TODO: Band információs terület kirajzolása
    // drawBandInfoArea();

    // TODO: Statikus címkék és UI elemek
    // drawStaticLabels();
}

/**
 * @brief Képernyő aktiválása - Event-driven gombállapot szinkronizálás
 * @details Meghívódik, amikor a felhasználó erre a képernyőre vált.
 *
 * Ez az EGYETLEN hely, ahol a gombállapotokat szinkronizáljuk a rendszer állapotával:
 * - Függőleges gombok: Mute, AGC, Attenuator állapotok
 * - Vízszintes gombok: Navigációs gombok állapotai
 *
 * **Event-driven előnyök**:
 * - NINCS folyamatos polling a loop()-ban
 * - Csak aktiváláskor történik szinkronizálás
 * - Jelentős teljesítményjavulás
 * - Univerzális gombkezelés (CommonVerticalButtons)
 *
 * **Szinkronizált állapotok**:
 * - MUTE gomb ↔ rtv::muteStat
 * - AGC gomb ↔ Si4735 AGC állapot (TODO)
 * - ATTENUATOR gomb ↔ Si4735 attenuator állapot (TODO)
 */
void AMScreen::activate() {

    // Alaposztály aktiválás (UI komponens hierarchia)
    UIScreen::activate();

    // ===================================================================
    // *** EGYETLEN GOMBÁLLAPOT SZINKRONIZÁLÁSI PONT - Event-driven ***
    // ===================================================================
    updateAllVerticalButtonStates(pSi4735Manager); // Univerzális funkcionális gombok (mixin method)
    updateHorizontalButtonStates();                // AM-specifikus navigációs gombok
}

/**
 * @brief Dialógus bezárásának kezelése - Gombállapot szinkronizálás
 * @details Az utolsó dialógus bezárásakor frissíti a gombállapotokat
 *
 * Ez a metódus biztosítja, hogy a gombállapotok konzisztensek maradjanak
 * a dialógusok bezárása után. Különösen fontos a ValueChangeDialog-ok
 * (Volume, Attenuator, Squelch, Frequency) után.
 */
void AMScreen::onDialogClosed(UIDialogBase *closedDialog) {
    DEBUG("AMScreen::onDialogClosed - Dialog closed, checking if last dialog\n");

    // Először hívjuk az alap implementációt (stack cleanup, navigation logic)
    UIScreen::onDialogClosed(closedDialog);

    // Ha ez volt az utolsó dialógus, frissítsük a gombállapotokat
    if (!isDialogActive()) {
        DEBUG("AMScreen::onDialogClosed - Last dialog closed, updating button states\n");
        updateAllVerticalButtonStates(pSi4735Manager); // Függőleges gombok szinkronizálása
        updateHorizontalButtonStates();                // Vízszintes gombok szinkronizálása
    }
}

// =====================================================================
// UI komponensek layout és management
// =====================================================================

/**
 * @brief UI komponensek létrehozása és képernyőn való elhelyezése
 */
void AMScreen::layoutComponents() {              // UI komponensek létrehozása
    createCommonVerticalButtons(pSi4735Manager); // ButtonsGroupManager használata
    createHorizontalButtonBar();                 // Alsó navigációs gombok
}

/**
 * @brief Vízszintes gombsor létrehozása - az FMScreen mintájára
 */
void AMScreen::createHorizontalButtonBar() {
    // ===================================================================
    // Gombsor pozicionálás - Bal alsó sarok (dinamikus)
    // ===================================================================
    const uint16_t buttonBarHeight = 35;                        // Optimális gombmagasság (FMScreen-hez igazítva)
    const uint16_t buttonBarX = 0;                              // Bal szélhez igazítva
    const uint16_t buttonBarY = tft.height() - buttonBarHeight; // Alsó szélhez igazítva (dinamikus)
    const uint16_t buttonBarWidth = 220;                        // 3 gomb + margók optimális szélessége

    // ===================================================================
    // Gomb konfigurációk - Navigációs események
    // ===================================================================
    std::vector<UIHorizontalButtonBar::ButtonConfig> configs = {
        {AMScreenHorizontalButtonIDs::FM_BUTTON, "FM", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &e) { handleFMButton(e); }},
        {AMScreenHorizontalButtonIDs::TEST_BUTTON, "Test", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &e) { handleTestButton(e); }},
        {AMScreenHorizontalButtonIDs::SETUP_BUTTON, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &e) { handleSetupButtonHorizontal(e); }}};

    // ===================================================================
    // UIHorizontalButtonBar objektum létrehozása
    // ===================================================================
    horizontalButtonBar = std::make_shared<UIHorizontalButtonBar>(tft, Rect(buttonBarX, buttonBarY, buttonBarWidth, buttonBarHeight), configs,
                                                                  70, // Egyedi gomb szélessége (pixel)
                                                                  30, // Egyedi gomb magassága (pixel)
                                                                  3   // Gombok közötti távolság (pixel)
    );

    // Komponens hozzáadása a képernyőhöz
    addChild(horizontalButtonBar);
}

// =====================================================================
// EVENT-DRIVEN GOMBÁLLAPOT SZINKRONIZÁLÁS - Univerzális rendszer
// =====================================================================

/**
 * @brief Vízszintes gombsor állapotainak szinkronizálása - AM képernyő specifikus
 * @details Navigációs gombok állapot kezelése:
 *
 * **FM gomb állapot logika**:
 * - Mindig Off állapotban (mivel jelenleg AM képernyőn vagyunk)
 * - Vizuálisan jelzi, hogy nem FM módban vagyunk
 * - Kattintásra átváltás FM képernyőre
 *
 * **Jövőbeli bővítési lehetőségek**:
 * - Test gomb állapot kezelése
 * - Setup gomb állapot kezelése
 * - Band-specifikus vizuális visszajelzések
 */
void AMScreen::updateHorizontalButtonStates() {
    if (!horizontalButtonBar)
        return;

    // FM gomb állapot szinkronizálása (AM képernyőn mindig Off)
    auto fmButton = horizontalButtonBar->getButton(AMScreenHorizontalButtonIDs::FM_BUTTON);
    if (fmButton) {
        // FM gomb Off állapotban: jelzi hogy jelenleg AM módban vagyunk
        fmButton->setButtonState(UIButton::ButtonState::Off);
    }

    // További navigációs gombok állapot kezelése itt...
    // TODO: Test és Setup gombok állapot szinkronizálása szükség szerint
}

// =====================================================================
// Függőleges gomb eseménykezelők - az FMScreen mintájára
// =====================================================================

// ===================================================================
// REFAKTORÁLÁS: A függőleges gomb handlereket eltávolítottuk!
// Most a CommonVerticalButtons osztály statikus metódusait használjuk
// A teljes gombsor létrehozás is áthelyeződött a közös factory-ba
// ===================================================================

// =====================================================================
// Vízszintes gomb eseménykezelők - 3 navigációs gomb
// =====================================================================

/**
 * @brief FM gomb eseménykezelő - FM képernyőre váltás
 * @details **KERESZTNAVIGÁCIÓ:** FMScreen-re navigálás
 */
void AMScreen::handleFMButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        // FM képernyőre váltás - keresztnavigáció
        getScreenManager()->switchToScreen(SCREEN_NAME_FM);
    }
}

/**
 * @brief TEST gomb eseménykezelő - Teszt képernyőre váltás
 */
void AMScreen::handleTestButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        // Test képernyőre váltás
        getScreenManager()->switchToScreen(SCREEN_NAME_TEST);
    }
}

/**
 * @brief SETUP gomb eseménykezelő (vízszintes) - Beállítások képernyőre váltás
 */
void AMScreen::handleSetupButtonHorizontal(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        // Setup képernyőre váltás (ugyanaz, mint a függőleges gomb)
        getScreenManager()->switchToScreen(SCREEN_NAME_SETUP);
    }
}
