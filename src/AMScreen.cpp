/**
 * @file AMScreen.cpp
 * @brief AM rádió vezérlő képernyő implementáció
 * @details Event-driven gombállapot kezeléssel és optimalizált teljesítménnyel
 *
 * **ARCHITEKTÚRA - Event-driven button state management:**
 * Ez az implementáció teljes mértékben Event-driven architektúrát használ:
 * - NINCS gombállapot polling a loop ciklusban
 * - Gombállapotok CSAK aktiváláskor szinkronizálódnak
 * - Jelentős teljesítményjavulás a korábbi polling megközelítéshez képest
 *
 * **PROJEKTÖSSZETEVŐK:**
 * - Közös függőleges gombsor az FMScreen-nel (8 funkcionális gomb)
 * - AM/MW/LW/SW frekvencia hangolás és megjelenítés
 * - S-Meter (jelerősség) valós idejű frissítés AM módban
 * - Vízszintes gombsor FM gombbal az FMScreen-re navigáláshoz
 *
 * @author Rádió projekt
 * @version 2.0 - Event-driven architecture
 */

#include "AMScreen.h"
#include "CommonRadioButtonHandlers.h"
#include "defines.h"
#include "rtVars.h"
#include "utils.h"
#include <algorithm>

// ===================================================================
// Gomb azonosítók - Event-driven architektúrához (FMScreen mintájára)
// ===================================================================

/**
 * @brief Függőleges gombsor gomb azonosítók
 * @details Jobb oldali gombsor - 8 funkcionális gomb
 */
namespace AMScreenButtonIDs {
static constexpr uint8_t MUTE = 30;    ///< Némítás gomb (toggleable)
static constexpr uint8_t VOLUME = 31;  ///< Hangerő beállítás gomb (pushable)
static constexpr uint8_t AGC = 32;     ///< Automatikus erősítés szabályozás (toggleable)
static constexpr uint8_t ATT = 33;     ///< Csillapító (toggleable)
static constexpr uint8_t SQUELCH = 34; ///< Zajzár beállítás (pushable)
static constexpr uint8_t FREQ = 35;    ///< Frekvencia input (pushable)
static constexpr uint8_t SETUP = 36;   ///< Beállítások képernyő (pushable)
static constexpr uint8_t MEMO = 37;    ///< Memória funkciók (pushable)
} // namespace AMScreenButtonIDs

/**
 * @brief Vízszintes gombsor gomb azonosítók
 * @details Alsó gombsor - navigációs gombok
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
 */
bool AMScreen::handleRotary(const RotaryEvent &event) {
    // Csak forgatás eseményt dolgozzuk fel
    if (event.direction == RotaryEvent::Direction::None) {
        return false;
    }

    // TODO: AM frekvencia hangolás implementálása
    // Ez az FMScreen rotary kezeléshez hasonló lesz
    return true;
}

/**
 * @brief Folyamatos loop hívás - Event-driven optimalizált implementáció
 */
void AMScreen::handleOwnLoop() {
    // *** NINCS GOMBÁLLAPOT POLLING! ***
    // Ez az Event-driven architektúra lényege

    // S-Meter (jelerősség) frissítése AM módban
    static unsigned long lastSMeterUpdate = 0;
    if (millis() - lastSMeterUpdate > 200) { // 200ms frissítési gyakorisággal
        // TODO: S-Meter megjelenítés frissítése AM módban
        lastSMeterUpdate = millis();
    }
}

/**
 * @brief Statikus képernyő tartalom kirajzolása
 */
void AMScreen::drawContent() {
    // TODO: S-Meter skála kirajzolása AM módban
    // TODO: Band információ terület
    // TODO: Statikus címkék és szövegek
}

/**
 * @brief Képernyő aktiválása - Event-driven gombállapot szinkronizálás
 */
void AMScreen::activate() {
    // Alaposztály aktiválás
    UIScreen::activate();

    // *** EGYETLEN GOMBÁLLAPOT SZINKRONIZÁLÁSI PONT ***
    updateVerticalButtonStates();   // Funkcionális gombok szinkronizálása
    updateHorizontalButtonStates(); // Navigációs gombok szinkronizálása
}

// =====================================================================
// UI komponensek layout és management
// =====================================================================

/**
 * @brief UI komponensek létrehozása és képernyőn való elhelyezése
 */
void AMScreen::layoutComponents() {
    // UI komponensek létrehozása
    createVerticalButtonBar();   // Jobb oldali funkcionális gombok
    createHorizontalButtonBar(); // Alsó navigációs gombok
}

/**
 * @brief Függőleges gombsor létrehozása - az FMScreen mintájára
 */
void AMScreen::createVerticalButtonBar() {
    // ===================================================================
    // Gombsor pozicionálás - Jobb felső sarok, teljes magasság (dinamikus)
    // ===================================================================
    const uint16_t buttonBarWidth = 65;                       // Optimális gombméret + margók  (FMScreen-hez igazítva)
    const uint16_t buttonBarX = tft.width() - buttonBarWidth; // Pontosan a jobb szélhez igazítva (dinamikus)
    const uint16_t buttonBarY = 0;                            // Legfelső pixeltől kezdve
    const uint16_t buttonBarHeight = tft.height();            // Teljes képernyő magasság kihasználása (dinamikus)

    // ===================================================================
    // Gomb konfigurációk - Event-driven eseménykezelőkkel (REFACTORED)
    // ===================================================================
    std::vector<UIVerticalButtonBar::ButtonConfig> configs = {{AMScreenButtonIDs::MUTE, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonRadioButtonHandlers::handleMuteButton(e, pSi4735Manager); }},

                                                              {AMScreenButtonIDs::VOLUME, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonRadioButtonHandlers::handleVolumeButton(e, pSi4735Manager); }},

                                                              {AMScreenButtonIDs::AGC, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonRadioButtonHandlers::handleAGCButton(e, pSi4735Manager); }},

                                                              {AMScreenButtonIDs::ATT, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonRadioButtonHandlers::handleAttenuatorButton(e, pSi4735Manager); }},

                                                              {AMScreenButtonIDs::SQUELCH, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonRadioButtonHandlers::handleSquelchButton(e, pSi4735Manager); }},

                                                              {AMScreenButtonIDs::FREQ, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonRadioButtonHandlers::handleFrequencyButton(e, pSi4735Manager); }},

                                                              {AMScreenButtonIDs::SETUP, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonRadioButtonHandlers::handleSetupButton(e, getManager()); }},

                                                              {AMScreenButtonIDs::MEMO, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonRadioButtonHandlers::handleMemoryButton(e, pSi4735Manager); }}};

    // ===================================================================
    // UIVerticalButtonBar objektum létrehozása és konfiguráció
    // ===================================================================
    verticalButtonBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(buttonBarX, buttonBarY, buttonBarWidth, buttonBarHeight), configs,
                                                              60, // Egyedi gomb szélessége (pixel)
                                                              32, // Egyedi gomb magassága (pixel)
                                                              4   // Gombok közötti távolság (pixel)
    );

    // Komponens hozzáadása a képernyőhöz
    addChild(verticalButtonBar);
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
// Event-driven gombállapot szinkronizálás
// =====================================================================

/**
 * @brief Függőleges gombsor állapotainak szinkronizálása (REFACTORED)
 */
void AMScreen::updateVerticalButtonStates() {
    if (!verticalButtonBar)
        return;

    // ✅ KÖZÖS KEZELŐ HASZNÁLATA - Nincs kód duplikáció!
    CommonRadioButtonHandlers::updateMuteButtonState(verticalButtonBar.get(), AMScreenButtonIDs::MUTE);

    // TODO: További közös állapot szinkronizálók hozzáadása
    // CommonRadioButtonHandlers::updateAGCButtonState(verticalButtonBar.get(), AMScreenButtonIDs::AGC, pSi4735Manager);
    // CommonRadioButtonHandlers::updateAttenuatorButtonState(verticalButtonBar.get(), AMScreenButtonIDs::ATT, pSi4735Manager);
}

/**
 * @brief Vízszintes gombsor állapotainak szinkronizálása
 */
void AMScreen::updateHorizontalButtonStates() {
    if (!horizontalButtonBar)
        return;

    // FM gomb állapot szinkronizálása
    auto fmButton = horizontalButtonBar->getButton(AMScreenHorizontalButtonIDs::FM_BUTTON);
    if (fmButton) {
        // FM gomb mindig Off állapotban (mi AM képernyőn vagyunk)
        fmButton->setButtonState(UIButton::ButtonState::Off);
    }
}

// =====================================================================
// Függőleges gomb eseménykezelők - az FMScreen mintájára
// =====================================================================

// ===================================================================
// REFAKTORÁLÁS: A függőleges gomb handlereket eltávolítottuk!
// Most a CommonRadioButtonHandlers osztály statikus metódusait használjuk
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
        getManager()->switchToScreen(SCREEN_NAME_FM);
    }
}

/**
 * @brief TEST gomb eseménykezelő - Teszt képernyőre váltás
 */
void AMScreen::handleTestButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        // Test képernyőre váltás
        getManager()->switchToScreen(SCREEN_NAME_TEST);
    }
}

/**
 * @brief SETUP gomb eseménykezelő (vízszintes) - Beállítások képernyőre váltás
 */
void AMScreen::handleSetupButtonHorizontal(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        // Setup képernyőre váltás (ugyanaz, mint a függőleges gomb)
        getManager()->switchToScreen(SCREEN_NAME_SETUP);
    }
}
