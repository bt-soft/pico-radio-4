/**
 * @file FMScreen.cpp
 * @brief FM rádió vezérlő képernyő implementáció
 * @details Event-driven gombállapot kezeléssel, optimalizált teljesítménnyel és
 *          univerzális gomb ID rendszer használatával
 *
 * **Fő funkciók**:
 * - FM frekvencia hangolás rotary encoder-rel (87.5-108.0 MHz tartomány)
 * - S-Meter (jelerősség és SNR) valós idejű megjelenítés FM módban
 * - Univerzális függőleges gombsor: Mute, Volume, AGC, Attenuator, Squelch, Freq, Setup, Memory
 * - Vízszintes navigációs gombsor: AM, Test, Setup gombok (képernyőváltáshoz)
 * - Event-driven gombállapot szinkronizálás (csak aktiváláskor és eseményekkor)
 *
 * **Architektúra változások (v3.0)**:
 * - Univerzális gomb ID rendszer használata (CommonVerticalButtons)
 * - Duplikált gombkezelési kód eliminálása (~25 sor eltávolítva)
 * - Egyszerűsített factory pattern (template nélkül)
 * - Közös gombkezelési logika az AMScreen-nel
 *
 * **Teljesítmény optimalizációk**:
 * - NINCS folyamatos gombállapot polling a loop()-ban
 * - S-Meter 200ms frissítési gyakorisággal
 * - Frekvencia kijelző azonnali frissítés rotary eseményekkor
 * - Gombállapotok csak aktiváláskor szinkronizálódnak
 *
 * @author Rádió projekt
 * @version 3.0 - Univerzális gomb ID rendszer (2025.06.11)
 */

#include "FMScreen.h"
#include "Band.h"
#include "CommonVerticalButtons.h"
#include "FreqDisplay.h"
#include "SMeter.h"
#include "StatusLine.h"
#include "UIColorPalette.h"
#include "UIHorizontalButtonBar.h"
#include "UIVerticalButtonBar.h"
#include "rtVars.h"

// ===================================================================
// UNIVERZÁLIS GOMB ID RENDSZER - Nincs több duplikáció!
// ===================================================================

// RÉGI RENDSZER ELTÁVOLÍTVA (2025.06.11):
// - FMScreenButtonIDs namespace (~8 sor duplikált kód)
// - FMScreenButtonIDStruct wrapper (~17 sor template komplexitás)
//
// ÚJ RENDSZER:
// - Univerzális VerticalButtonIDs namespace (CommonVerticalButtons.h-ban)
// - Egyszerűsített factory hívás (template és struct nélkül)
// - Közös gombkezelési logika az AMScreen-nel

// ===================================================================
// Vízszintes gombsor azonosítók - Képernyő-specifikus navigáció
// ===================================================================

/**
 * @brief Vízszintes gombsor gomb azonosítók (FM képernyő specifikus)
 * @details Alsó navigációs gombsor - képernyőváltáshoz használt gombok
 *
 * **ID tartomány**: 20-22 (nem ütközik a univerzális 10-17 tartománnyal)
 * **Funkció**: Képernyők közötti navigáció (AM, Test, Setup)
 * **Gomb típus**: Pushable (egyszeri nyomás → képernyőváltás)
 */
namespace FMScreenHorizontalButtonIDs {
static constexpr uint8_t AM_BUTTON = 20;    ///< AM képernyőre váltás (pushable)
static constexpr uint8_t TEST_BUTTON = 21;  ///< Test képernyőre váltás (pushable)
static constexpr uint8_t SETUP_BUTTON = 22; ///< Setup képernyőre váltás (pushable)
} // namespace FMScreenHorizontalButtonIDs

// ===================================================================
// Konstruktor és inicializálás
// ===================================================================

/**
 * @brief FMScreen konstruktor - FM rádió képernyő létrehozása
 * @param tft TFT display referencia
 * @param si4735Manager Si4735 rádió chip kezelő referencia
 *
 * @details Inicializálja az FM rádió képernyőt:
 * - Si4735 chip inicializálása
 * - UI komponensek elrendezése (gombsorok, kijelzők)
 * - Event-driven gombkezelés beállítása
 */
FMScreen::FMScreen(TFT_eSPI &tft, Si4735Manager &si4735Manager) : UIScreen(tft, SCREEN_NAME_FM, &si4735Manager) {
    si4735Manager.init(); // Si4735 rádió chip inicializálása
    layoutComponents();   // UI komponensek elhelyezése
}

// ===================================================================
// UI komponensek layout és elhelyezés
// ===================================================================

/**
 * @brief UI komponensek létrehozása és képernyőn való elhelyezése
 * @details Létrehozza és elhelyezi az összes UI elemet:
 * - Állapotsor (felül)
 * - Frekvencia kijelző (középen)
 * - S-Meter (jelerősség mérő)
 * - Függőleges gombsor (jobb oldal)
 * - Vízszintes gombsor (alul)
 */
void FMScreen::layoutComponents() {

    // Állapotsor komponens létrehozása (felső sáv)
    UIScreen::createStatusLine();

    // ===================================================================
    // Frekvencia kijelző pozicionálás (képernyő közép)
    // ===================================================================
    uint16_t freqDisplayHeight = 38 + 20 + 10;                    // 7-szegmenses + mértékegység + aláhúzás + margók
    uint16_t freqDisplayWidth = 240;                              // Optimális szélesség a képernyő közepén
    uint16_t freqDisplayX = (tft.width() - freqDisplayWidth) / 2; // Vízszintes középre igazítás
    uint16_t freqDisplayY = 60;                                   // Állapotsor alatti pozíció
    Rect freqBounds(freqDisplayX, freqDisplayY, freqDisplayWidth, freqDisplayHeight);
    UIScreen::createFreqDisplay(freqBounds);

    // ===================================================================
    // S-Meter (jelerősség mérő) pozicionálás
    // ===================================================================
    uint16_t smeterX = 2;                                     // Kis margó a bal szélről
    uint16_t smeterY = freqDisplayY + freqDisplayHeight + 10; // Frekvencia kijelző alatti pozíció
    uint16_t smeterWidth = 240;                               // S-Meter szélessége
    uint16_t smeterHeight = 60;                               // S-Meter optimális magassága
    Rect smeterBounds(smeterX, smeterY, smeterWidth, smeterHeight);
    ColorScheme smeterColors = ColorScheme::defaultScheme();
    smeterColors.background = TFT_COLOR_BACKGROUND;     // Fekete háttér a designhoz
    UIScreen::createSMeter(smeterBounds, smeterColors); // ===================================================================
    // Gombsorok létrehozása - Event-driven architektúra
    // ===================================================================
    createCommonVerticalButtons(pSi4735Manager, getScreenManager()); // ButtonsGroupManager alapú függőleges gombsor
    createHorizontalButtonBar();                                     // Alsó vízszintes gombsor
}

// ===================================================================
// Felhasználói események kezelése - Event-driven architektúra
// ===================================================================

/**
 * @brief Rotary encoder eseménykezelés - Frekvencia hangolás
 * @param event Rotary encoder esemény (forgatás irány, érték, gombnyomás)
 * @return true ha sikeresen kezelte az eseményt, false egyébként
 *
 * @details FM frekvencia hangolás logika:
 * - Csak akkor reagál, ha nincs aktív dialógus
 * - Rotary klikket figyelmen kívül hagyja (más funkciókhoz)
 * - Frekvencia léptetés és mentés a band táblába
 * - Frekvencia kijelző azonnali frissítése
 */
bool FMScreen::handleRotary(const RotaryEvent &event) {

    // Biztonsági ellenőrzés: csak aktív dialógus nélkül és nem klikk eseménykor
    if (!isDialogActive() && event.buttonState != RotaryEvent::ButtonState::Clicked) {

        // Frekvencia léptetés és automatikus mentés a band táblába
        pSi4735Manager->stepFrequency(event.value);

        // Frekvencia kijelző azonnali frissítése
        if (freqDisplayComp) {
            uint16_t currentRadioFreq = pSi4735Manager->getCurrentBand().currFreq;
            freqDisplayComp->setFrequency(currentRadioFreq);
        }

        return true; // Esemény sikeresen kezelve
    }

    // Ha nem kezeltük az eseményt, továbbítjuk a szülő osztálynak (dialógusokhoz)
    return UIScreen::handleRotary(event);
}

// ===================================================================
// Loop ciklus - Optimalizált teljesítmény
// ===================================================================

/**
 * @brief Folyamatos loop hívás - Csak valóban szükséges frissítések
 * @details Event-driven architektúra: NINCS folyamatos gombállapot pollozás!
 *
 * Csak az alábbi komponenseket frissíti minden ciklusban:
 * - S-Meter (jelerősség) - valós idejű adat
 *
 * Gombállapotok frissítése CSAK:
 * - Képernyő aktiválásakor (activate() metódus)
 * - Specifikus eseményekkor (eseménykezelőkben)
 */
void FMScreen::handleOwnLoop() {

    // ===================================================================
    // S-Meter (jelerősség) valós idejű frissítése
    // ===================================================================
    if (smeterComp) {
        // Cache-elt jelerősség adatok lekérése a Si4735Manager-től
        SignalQualityData signalCache = pSi4735Manager->getSignalQuality();
        if (signalCache.isValid) {
            // RSSI és SNR megjelenítése FM módban
            smeterComp->showRSSI(signalCache.rssi, signalCache.snr, true /* FM mód */);
        }
    }
}

// ===================================================================
// Képernyő rajzolás és aktiválás
// ===================================================================

/**
 * @brief Statikus képernyő tartalom kirajzolása
 * @details Csak a statikus elemeket rajzolja ki (nem változó tartalom):
 * - S-Meter skála (vonalak, számok)
 *
 * A dinamikus tartalom (pl. S-Meter érték) a loop()-ban frissül.
 */
void FMScreen::drawContent() {
    // S-Meter statikus skála kirajzolása (egyszer, a kezdetekkor)
    if (smeterComp) {
        smeterComp->drawSmeterScale();
    }
}

/**
 * @brief Képernyő aktiválása - Event-driven gombállapot szinkronizálás
 * @details Meghívódik, amikor a felhasználó erre a képernyőre vált.
 *
 * Ez az EGYETLEN hely, ahol a gombállapotokat szinkronizáljuk a rendszer állapotával:
 * - Mute gomb szinkronizálása rtv::muteStat-tal
 * - AM gomb szinkronizálása aktuális band típussal
 * - További állapotok szinkronizálása (AGC, Attenuator, stb.)
 */
void FMScreen::activate() {
    DEBUG("FMScreen activated - syncing button states\n");

    // Szülő osztály activate() hívása (UIScreen lifecycle)
    UIScreen::activate(); // ===================================================================
    // Event-driven gombállapot szinkronizálás - CSAK AKTIVÁLÁSKOR!
    // ===================================================================
    updateAllVerticalButtonStates(pSi4735Manager); // Függőleges gombsor szinkronizálás (mixin method)
    updateHorizontalButtonStates();                // Vízszintes gombsor szinkronizálás
}

// ===================================================================
// Függőleges gombsor - Jobb oldali funkcionális gombok
// ===================================================================

/**
 * @brief Függőleges gombsor létrehozása a képernyő jobb oldalán
 * @details 8 funkcionális gomb elhelyezése függőleges elrendezésben:
 *
 * Gombsor pozíció: Jobb felső sarok, teljes magasság
 * Gombok (felülről lefelé):
 * 1. Mute (Némítás) - Toggleable
 * 2. Volume (Hangerő) - Pushable → Dialog
 * 3. AGC (Auto Gain Control) - Toggleable
 * 4. Att (Attenuator/Csillapító) - Toggleable
 * 5. Sql (Squelch/Zajzár) - Pushable → Dialog
 * 6. Freq (Frekvencia input) - Pushable → Dialog
 * 7. Setup (Beállítások) - Pushable → Screen switch
 * 8. Memo (Memória funkciók) - Pushable → Dialog
 */
/**
 * @brief Függőleges gombsor létrehozása - Univerzális factory pattern használatával
 * @details Egyszerűsített implementáció az univerzális gomb ID rendszer segítségével.
 *          A teljes gombkezelési logika a CommonVerticalButtons osztályba került.
 *
 * **Változások a korábbi verzióhoz képest**:
 * - Template paraméter eltávolítva (ButtonIDStruct már nem szükséges)
 * - 5 paraméterről 4-re csökkentve (buttonIds paraméter eliminálva)
 * - ~25 sor duplikált kód eltávolítva (FMScreenButtonIDs, FMScreenButtonIDStruct)
 * - Közös gombkezelési logika az AMScreen-nel
 *
 * **Factory hívás**:
 * - createVerticalButtonBar(tft, screen, si4735Manager, screenManager)
 * - Automatikus gombkonfiguráció univerzális ID-kkal
 * - Band-független működés (Si4735Manager kezeli a rádió állapotokat)
 */
// ===================================================================
// Vízszintes gombsor - Alsó navigációs gombok
// ===================================================================

/**
 * @brief Vízszintes gombsor létrehozása a képernyő alján
 * @details 3 navigációs gomb elhelyezése vízszintes elrendezésben:
 *
 * Gombsor pozíció: Bal alsó sarok, 3 gomb szélessége
 * Gombok (balról jobbra):
 * 1. AM - AM képernyőre váltás (Pushable)
 * 2. Test - Test képernyőre váltás (Pushable)
 * 3. Setup - Setup képernyőre váltás (Pushable)
 */
void FMScreen::createHorizontalButtonBar() {

    // ===================================================================
    // Gombsor pozicionálás - Bal alsó sarok
    // ===================================================================
    const uint16_t buttonBarHeight = 35;                        // Optimális gombmagasság
    const uint16_t buttonBarX = 0;                              // Bal szélhez igazítva
    const uint16_t buttonBarY = tft.height() - buttonBarHeight; // Alsó szélhez igazítva
    const uint16_t buttonBarWidth = 220;                        // 3 gomb + margók optimális szélessége

    // ===================================================================
    // Gomb konfigurációk - Navigációs események
    // ===================================================================
    std::vector<UIHorizontalButtonBar::ButtonConfig> buttonConfigs = {

        // 1. AM - AM/MW/LW/SW képernyőre váltás
        {FMScreenHorizontalButtonIDs::AM_BUTTON, "AM", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleAMButton(event); }},

        // 2. TEST - Teszt és diagnosztikai képernyőre váltás
        {FMScreenHorizontalButtonIDs::TEST_BUTTON, "Test", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleTestButton(event); }},

        // 3. SETUP - Beállítások képernyőre váltás (duplikáció a függőleges gombbal)
        {FMScreenHorizontalButtonIDs::SETUP_BUTTON, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleSetupButtonHorizontal(event); }}};

    // ===================================================================
    // UIHorizontalButtonBar objektum létrehozása
    // ===================================================================
    horizontalButtonBar = std::make_shared<UIHorizontalButtonBar>(     //
        tft,                                                           // TFT display referencia
        Rect(buttonBarX, buttonBarY, buttonBarWidth, buttonBarHeight), // Gombsor pozíció és méret
        buttonConfigs,                                                 // Gomb konfigurációk
        70,                                                            // Egyedi gomb szélessége (pixel)
        30,                                                            // Egyedi gomb magassága (pixel)
        3                                                              // Gombok közötti távolság (pixel)
    );

    // Gombsor hozzáadása a képernyő komponens hierarchiájához
    addChild(horizontalButtonBar);
}

/**
 * @brief Vízszintes gombsor állapotainak szinkronizálása
 * @details Event-driven architektúra: CSAK aktiváláskor hívódik meg!
 *
 * Szinkronizált állapotok:
 * - AM gomb ↔ Aktuális band típus (FM vs AM/MW/LW/SW)
 *
 * A Test és Setup gombok pushable típusúak, állapotuk nem változik.
 */
void FMScreen::updateHorizontalButtonStates() {
    if (!horizontalButtonBar) {
        return; // Biztonsági ellenőrzés
    }

    // ===================================================================
    // AM gomb állapot szinkronizálása - Band típus alapján
    // ===================================================================
    uint8_t currentBandType = pSi4735Manager->getCurrentBand().bandType;

    // AM gomb világít, ha NEM FM módban vagyunk (tehát AM, MW, LW, SW módban)
    // Ez vizuális visszajelzést ad, hogy melyik band családban vagyunk
    bool isAMMode = (currentBandType != FM_BAND_TYPE);
    horizontalButtonBar->setButtonState(FMScreenHorizontalButtonIDs::AM_BUTTON, isAMMode ? UIButton::ButtonState::On : UIButton::ButtonState::Off);

    // Test és Setup gombok pushable típusúak - állapotuk mindig Off marad
    // (Pushable gombok nem tartanak állapotot, csak eseményeket generálnak)
}

// ===================================================================
// Vízszintes gomb eseménykezelők - Navigációs funkciók
// ===================================================================

/**
 * @brief AM gomb eseménykezelő - AM családú képernyőre váltás
 * @param event Gomb esemény (Clicked)
 *
 * @details Pushable gomb: AM/MW/LW/SW képernyőre navigálás
 * Váltás az FM-től eltérő band családra
 */
void FMScreen::handleAMButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Switching to AM band family screen\n");
        // ScreenManager-en keresztül AM képernyőre váltás
        UIScreen::getScreenManager()->switchToScreen(SCREEN_NAME_AM);
    }
}

/**
 * @brief TEST gomb eseménykezelő - Teszt képernyőre váltás
 * @param event Gomb esemény (Clicked)
 *
 * @details Pushable gomb: Test és diagnosztikai képernyőre navigálás
 * Fejlesztői és diagnosztikai funkciókhoz
 */
void FMScreen::handleTestButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Switching to Test screen\n");
        // ScreenManager-en keresztül Test képernyőre váltás
        UIScreen::getScreenManager()->switchToScreen(SCREEN_NAME_TEST);
    }
}

/**
 * @brief SETUP gomb eseménykezelő (vízszintes) - Beállítások képernyőre váltás
 * @param event Gomb esemény (Clicked)
 *
 * @details Pushable gomb: Setup képernyőre navigálás
 * Azonos funkcionalitás a függőleges Setup gombbal (duplikáció a kényelemért)
 */
void FMScreen::handleSetupButtonHorizontal(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Switching to Setup screen (from horizontal button)\n");
        // ScreenManager-en keresztül Setup képernyőre váltás
        UIScreen::getScreenManager()->switchToScreen(SCREEN_NAME_SETUP);
    }
}