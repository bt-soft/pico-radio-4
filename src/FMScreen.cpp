/**
 * @file FMScreen.cpp
 * @brief FM rádió vezérlő képernyő implementáció
 * @details Event-driven gombállapot kezeléssel és optimalizált teljesítménnyel
 *
 * Fő funkciók:
 * - FM frekvencia hangolás rotary encoder-rel
 * - S-Meter (jelerősség) megjelenítés
 * - Függőleges gombsor: Mute, Volume, AGC, Attenuator, Squelch, Freq, Setup, Memory
 * - Vízszintes gombsor: AM, Test, Setup gombok
 * - Event-driven gombállapot szinkronizálás (csak aktiváláskor és eseményekkor)
 */

#include "FMScreen.h"
#include "Band.h"
#include "FreqDisplay.h"
#include "SMeter.h"
#include "StatusLine.h"
#include "UIColorPalette.h"
#include "UIHorizontalButtonBar.h"
#include "UIVerticalButtonBar.h"
#include "rtVars.h"

// ===================================================================
// Gomb azonosítók - Event-driven architektúrához
// ===================================================================

/**
 * @brief Függőleges gombsor gomb azonosítók
 * @details Jobb oldali gombsor - 8 funkcionális gomb
 */
namespace FMScreenButtonIDs {
static constexpr uint8_t MUTE = 10;    ///< Némítás gomb (toggleable)
static constexpr uint8_t VOLUME = 11;  ///< Hangerő beállítás gomb (pushable)
static constexpr uint8_t AGC = 12;     ///< Automatikus erősítés szabályozás (toggleable)
static constexpr uint8_t ATT = 13;     ///< Csillapító (toggleable)
static constexpr uint8_t SQUELCH = 14; ///< Zajzár beállítás (pushable)
static constexpr uint8_t FREQ = 15;    ///< Frekvencia input (pushable)
static constexpr uint8_t SETUP = 16;   ///< Beállítások képernyő (pushable)
static constexpr uint8_t MEMO = 17;    ///< Memória funkciók (pushable)
} // namespace FMScreenButtonIDs

/**
 * @brief Vízszintes gombsor gomb azonosítók
 * @details Alsó gombsor - navigációs gombok
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
    smeterColors.background = TFT_COLOR_BACKGROUND; // Fekete háttér a designhoz
    UIScreen::createSMeter(smeterBounds, smeterColors);

    // ===================================================================
    // Gombsorok létrehozása - Event-driven architektúra
    // ===================================================================
    createVerticalButtonBar();   // Jobb oldali függőleges gombsor
    createHorizontalButtonBar(); // Alsó vízszintes gombsor
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

    // ✅ OPTIMALIZÁLT: Gombállapotok már az eseménykezelőkben és activate()-ben frissülnek
    // ❌ RÉGI MÓDSZER: updateVerticalButtonStates(); updateHorizontalButtonStates();
    // Nincs szükség folyamatos pollozásra - Event-driven megközelítés!
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
    UIScreen::activate();

    // ===================================================================
    // Event-driven gombállapot szinkronizálás - CSAK AKTIVÁLÁSKOR!
    // ===================================================================
    updateVerticalButtonStates();   // Függőleges gombsor szinkronizálás
    updateHorizontalButtonStates(); // Vízszintes gombsor szinkronizálás
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
void FMScreen::createVerticalButtonBar() {

    // ===================================================================
    // Gombsor pozicionálás - Jobb felső sarok, teljes magasság
    // ===================================================================
    const uint16_t buttonBarWidth = 65;                       // Optimális gombméret + margók
    const uint16_t buttonBarX = tft.width() - buttonBarWidth; // Pontosan a jobb szélhez igazítva
    const uint16_t buttonBarY = 0;                            // Legfelső pixeltől kezdve
    const uint16_t buttonBarHeight = tft.height();            // Teljes képernyő magasság kihasználása

    // ===================================================================
    // Gomb konfigurációk - Event-driven eseménykezelőkkel
    // ===================================================================
    std::vector<UIVerticalButtonBar::ButtonConfig> buttonConfigs = {

        // 1. MUTE - Toggleable gomb (BE/KI állapottal)
        {FMScreenButtonIDs::MUTE, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleMuteButton(event); }},

        // 2. VOLUME - Pushable gomb (dialógus megnyitás)
        {FMScreenButtonIDs::VOLUME, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleVolumeButton(event); }},

        // 3. AGC - Toggleable gomb (Automatikus erősítésszabályozás)
        {FMScreenButtonIDs::AGC, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleAGCButton(event); }},

        // 4. ATTENUATOR - Toggleable gomb (Jel csillapítás)
        {FMScreenButtonIDs::ATT, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleAttButton(event); }},

        // 5. SQUELCH - Pushable gomb (Zajzár beállító dialógus)
        {FMScreenButtonIDs::SQUELCH, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleSquelchButton(event); }},

        // 6. FREQUENCY - Pushable gomb (Frekvencia input dialógus)
        {FMScreenButtonIDs::FREQ, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleFreqButton(event); }},

        // 7. SETUP - Pushable gomb (Beállítások képernyőre váltás)
        {FMScreenButtonIDs::SETUP, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleSetupButtonVertical(event); }},

        // 8. MEMORY - Pushable gomb (Memória funkciók dialógus)
        {FMScreenButtonIDs::MEMO, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleMemoButton(event); }}};

    // ===================================================================
    // UIVerticalButtonBar objektum létrehozása és konfiguráció
    // ===================================================================
    verticalButtonBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(buttonBarX, buttonBarY, buttonBarWidth, buttonBarHeight), buttonConfigs,
                                                              60, // Egyedi gomb szélessége (pixel)
                                                              32, // Egyedi gomb magassága (pixel)
                                                              4   // Gombok közötti távolság (pixel)
    );

    // Gombsor hozzáadása a képernyő komponens hierarchiájához
    addChild(verticalButtonBar);
}

/**
 * @brief Függőleges gombsor állapotainak szinkronizálása a rendszer állapotával
 * @details Event-driven architektúra: CSAK aktiváláskor hívódik meg!
 *
 * Szinkronizált állapotok:
 * - Mute gomb ↔ rtv::muteStat (globális némítás állapot)
 * - AGC gomb ↔ Si4735 AGC állapot (implementálandó)
 * - Attenuator gomb ↔ Si4735 attenuator állapot (implementálandó)
 *
 * @note Ez a metódus NEM hívódik meg minden loop ciklusban!
 */
void FMScreen::updateVerticalButtonStates() {
    if (!verticalButtonBar) {
        return; // Biztonsági ellenőrzés
    }

    // ===================================================================
    // MUTE gomb állapot szinkronizálása
    // ===================================================================
    // rtv::muteStat globális változó → Mute gomb vizuális állapot
    verticalButtonBar->setButtonState(FMScreenButtonIDs::MUTE, rtv::muteStat ? UIButton::ButtonState::On : UIButton::ButtonState::Off);

    // ===================================================================
    // AGC gomb állapot szinkronizálása (TODO: implementálásra vár)
    // ===================================================================
    // TODO: Si4735Manager AGC állapot lekérdezés implementálása
    // bool agcEnabled = pSi4735Manager->isAGCEnabled();
    // verticalButtonBar->setButtonState(FMScreenButtonIDs::AGC,
    //                                  agcEnabled ? UIButton::ButtonState::On : UIButton::ButtonState::Off);

    // ===================================================================
    // ATTENUATOR gomb állapot szinkronizálása (TODO: implementálásra vár)
    // ===================================================================
    // TODO: Si4735Manager Attenuator állapot lekérdezés implementálása
    // bool attEnabled = pSi4735Manager->isAttenuatorEnabled();
    // verticalButtonBar->setButtonState(FMScreenButtonIDs::ATT,
    //                                  attEnabled ? UIButton::ButtonState::On : UIButton::ButtonState::Off);

    // További gombállapotok szinkronizálása szükség szerint...
}

// ===================================================================
// Függőleges gomb eseménykezelők - Event-driven architektúra
// ===================================================================

/**
 * @brief MUTE gomb eseménykezelő - Audió némítás BE/KI kapcsolás
 * @param event Gomb esemény (On/Off állapotváltozás)
 *
 * @details Toggleable gomb: Minden kattintásra vált BE/KI között
 *
 * Funkciók:
 * - rtv::muteStat globális állapot frissítése
 * - Si4735 chip audió kimenet némítása/engedélyezése
 * - Event-driven: Gomb vizuális állapotát a UIButton automatikusan kezeli
 */
void FMScreen::handleMuteButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::On) {
        DEBUG("FMScreen: Mute ON - Audio muted\n");
        rtv::muteStat = true;                           // Globális némítás állapot beállítása
        pSi4735Manager->getSi4735().setAudioMute(true); // Si4735 chip némítása
    } else if (event.state == UIButton::EventButtonState::Off) {
        DEBUG("FMScreen: Mute OFF - Audio enabled\n");
        rtv::muteStat = false;                           // Globális némítás állapot törlése
        pSi4735Manager->getSi4735().setAudioMute(false); // Si4735 chip hang engedélyezése
    }

    // ✅ Event-driven optimalizáció:
    // A gomb vizuális állapotát a UIButton automatikusan beállítja az eseménykezelés során
    // Nincs szükség manuális setButtonState() hívásra - a UI automatikusan szinkronban marad
}

/**
 * @brief VOLUME gomb eseménykezelő - Hangerő beállító dialógus megnyitása
 * @param event Gomb esemény (Clicked)
 *
 * @details Pushable gomb: Minden kattintásra dialógust nyit
 * TODO: ValueChangeDialog implementálása hangerő beállításhoz
 */
void FMScreen::handleVolumeButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Volume adjustment dialog requested\n");

        // TODO: Hangerő beállító dialógus megjelenítése
        // Implementálandó funkciók:
        // - ValueChangeDialog használata
        // - Si4735 volume beállítás (0-63 tartomány)
        // - Rotary encoder navigáció a dialógusban
        // showVolumeDialog();
    }
}

/**
 * @brief AGC gomb eseménykezelő - Automatikus erősítésszabályozás BE/KI
 * @param event Gomb esemény (On/Off állapotváltozás)
 *
 * @details Toggleable gomb: AGC (Automatic Gain Control) funkció
 * TODO: Si4735Manager AGC funkciók implementálása
 */
void FMScreen::handleAGCButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::On) {
        DEBUG("FMScreen: AGC ON - Automatic Gain Control enabled\n");

        // TODO: Si4735 AGC bekapcsolása
        // Implementálandó funkciók:
        // - Si4735Manager::setAGC(true) metódus
        // - AGC paraméterek beállítása
        // - Állapot mentése a konfigurációba
        // pSi4735Manager->setAGC(true);

    } else if (event.state == UIButton::EventButtonState::Off) {
        DEBUG("FMScreen: AGC OFF - Manual gain control\n");

        // TODO: Si4735 AGC kikapcsolása
        // pSi4735Manager->setAGC(false);
    }
}

/**
 * @brief ATTENUATOR gomb eseménykezelő - Jel csillapítás BE/KI
 * @param event Gomb esemény (On/Off állapotváltozás)
 *
 * @details Toggleable gomb: RF attenuator funkció (erős jelek csillapítása)
 * TODO: Si4735Manager Attenuator funkciók implementálása
 */
void FMScreen::handleAttButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::On) {
        DEBUG("FMScreen: Attenuator ON - RF signal attenuation enabled\n");

        // TODO: Si4735 attenuátor bekapcsolása
        // Implementálandó funkciók:
        // - Si4735Manager::setAttenuator(true) metódus
        // - Attenuator szint beállítása (általában 10-30dB)
        // - Állapot mentése
        // pSi4735Manager->setAttenuator(true);

    } else if (event.state == UIButton::EventButtonState::Off) {
        DEBUG("FMScreen: Attenuator OFF - Full RF sensitivity\n");

        // TODO: Si4735 attenuátor kikapcsolása
        // pSi4735Manager->setAttenuator(false);
    }
}

/**
 * @brief SQUELCH gomb eseménykezelő - Zajzár beállító dialógus
 * @param event Gomb esemény (Clicked)
 *
 * @details Pushable gomb: Squelch (zajzár) szint beállító dialógus
 * TODO: Squelch beállító dialógus implementálása
 */
void FMScreen::handleSquelchButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Squelch adjustment dialog requested\n");

        // TODO: Zajzár beállító dialógus megjelenítése
        // Implementálandó funkciók:
        // - ValueChangeDialog squelch szinthez (0-255)
        // - RSSI alapú squelch logika
        // - Valós idejű előnézet a beállítás közben
        // showSquelchDialog();
    }
}

/**
 * @brief FREQUENCY gomb eseménykezelő - Frekvencia közvetlen input
 * @param event Gomb esemény (Clicked)
 *
 * @details Pushable gomb: Frekvencia közvetlen megadása numerikus input-tal
 * TODO: Frekvencia input dialógus implementálása
 */
void FMScreen::handleFreqButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Direct frequency input dialog requested\n");

        // TODO: Frekvencia input dialógus megjelenítése
        // Implementálandó funkciók:
        // - Numerikus input dialógus (88.0 - 108.0 MHz FM tartomány)
        // - Band límit ellenőrzés
        // - Frekvencia validáció és beállítás
        // - Si4735Manager frekvencia váltás
        // showFrequencyInputDialog();
    }
}

/**
 * @brief SETUP gomb eseménykezelő (függőleges) - Beállítások képernyőre váltás
 * @param event Gomb esemény (Clicked)
 *
 * @details Pushable gomb: Setup képernyőre navigálás
 * Azonos funkcionalitás a vízszintes Setup gombbal
 */
void FMScreen::handleSetupButtonVertical(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Switching to Setup screen (from vertical button)\n");
        // ScreenManager-en keresztül képernyő váltás
        UIScreen::getManager()->switchToScreen(SCREEN_NAME_SETUP);
    }
}

/**
 * @brief MEMORY gomb eseménykezelő - Memória funkciók dialógus
 * @param event Gomb esemény (Clicked)
 *
 * @details Pushable gomb: Állomás memória kezelő dialógus
 * TODO: Memória funkciók dialógus implementálása
 */
void FMScreen::handleMemoButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Memory functions dialog requested\n");

        // TODO: Memória funkciók dialógus megjelenítése
        // Implementálandó funkciók:
        // - Aktuális frekvencia mentése memória slotba
        // - Mentett állomások visszahívása
        // - Memória slot lista megjelenítése
        // - Memória slot törlése/szerkesztése
        // showMemoryDialog();
    }
}

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
    horizontalButtonBar = std::make_shared<UIHorizontalButtonBar>(tft, Rect(buttonBarX, buttonBarY, buttonBarWidth, buttonBarHeight), buttonConfigs,
                                                                  70, // Egyedi gomb szélessége (pixel)
                                                                  30, // Egyedi gomb magassága (pixel)
                                                                  3   // Gombok közötti távolság (pixel)
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
        UIScreen::getManager()->switchToScreen(SCREEN_NAME_AM);
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
        UIScreen::getManager()->switchToScreen(SCREEN_NAME_TEST);
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
        UIScreen::getManager()->switchToScreen(SCREEN_NAME_SETUP);
    }
}