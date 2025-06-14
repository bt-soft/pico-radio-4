#include "FMScreen.h"
#include "Band.h"
#include "CommonVerticalButtons.h"
#include "FreqDisplay.h"
#include "SMeter.h"
#include "StatusLine.h"
#include "UIColorPalette.h"
#include "UIHorizontalButtonBar.h"
#include "rtVars.h"

// ===================================================================
// Vízszintes gombsor azonosítók - Képernyő-specifikus navigáció
// ===================================================================
namespace FMScreenHorizontalButtonIDs {
static constexpr uint8_t SEEK_DOWN_BUTTON = 20; ///< Seek lefelé (pushable)
static constexpr uint8_t SEEK_UP_BUTTON = 21;   ///< Seek felfelé (pushable)
static constexpr uint8_t AM_BUTTON = 22;        ///< AM képernyőre váltás (pushable)
static constexpr uint8_t TEST_BUTTON = 23;      ///< Test képernyőre váltás (pushable)
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
FMScreen::FMScreen(TFT_eSPI &tft, Si4735Manager &si4735Manager) : RadioScreen(tft, SCREEN_NAME_FM, &si4735Manager) {
    layoutComponents(); // UI komponensek elhelyezése
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
    uint16_t FreqDisplayY = 20;
    Rect freqBounds(30, FreqDisplayY, 200, FreqDisplay::FREQDISPLAY_HEIGHT);
    UIScreen::createFreqDisplay(freqBounds);
    freqDisplayComp->setHideUnderline(true); // Alulvonás elrejtése a frekvencia kijelzőn

    // ===================================================================
    // S-Meter (jelerősség mérő) pozicionálás
    // ===================================================================
    // S-Meter szélesség korlátozása - helyet hagyunk a függőleges gombsornak
    uint16_t smeterWidth = UIComponent::SCREEN_W - 90; // 90px helyet hagyunk a jobb oldalon
    Rect smeterBounds(2, FreqDisplayY + FreqDisplay::FREQDISPLAY_HEIGHT + 20, smeterWidth, 60);
    ColorScheme smeterColors = ColorScheme::defaultScheme();
    smeterColors.background = TFT_COLOR_BACKGROUND; // Fekete háttér a designhoz
    UIScreen::createSMeter(smeterBounds, smeterColors);

    // ===================================================================
    // RDS komponens létrehozása és pozicionálása
    // ===================================================================
    uint16_t rdsY = smeterBounds.y + smeterBounds.height + 10;

    // RDS szélesség korlátozása - helyet hagyunk a függőleges gombsornak (72px + 10px margin)
    uint16_t rdsWidth = UIComponent::SCREEN_W - 90; // 90px helyet hagyunk a jobb oldalon
    Rect rdsBounds(5, rdsY, rdsWidth, RDSComponent::DEFAULT_HEIGHT);

    // RDS komponens létrehozása a RadioScreen factory metódusával
    createRDSComponent(rdsBounds);

    // RDS területek finomhangolása (opcionális - alapértelmezett layout elfogadható)
    // rdsComponent->setStationNameArea(Rect(10, rdsY, 200, 18));
    // rdsComponent->setProgramTypeArea(Rect(220, rdsY, 150, 18));
    // rdsComponent->setDateTimeArea(Rect(380, rdsY, 85, 18));
    // rdsComponent->setRadioTextArea(Rect(10, rdsY + 20, 460, 18));

    // ===================================================================
    // Gombsorok létrehozása - Event-driven architektúra
    // ===================================================================
    createCommonVerticalButtonsWithCustomMemo(); // ButtonsGroupManager alapú függőleges gombsor egyedi Memo kezelővel
    createHorizontalButtonBar();                 // Alsó vízszintes gombsor
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
        // Beállítjuk a chip-en és le is mentjük a konfigba a frekvenciát
        config.data.currentFrequency = pSi4735Manager->stepFrequency(event.value);

        // RDS cache törlése frekvencia változás miatt
        if (rdsComponent) {
            rdsComponent->clearRdsOnFrequencyChange();
        } // Frekvencia kijelző azonnali frissítése
        if (freqDisplayComp) {
            uint16_t currentRadioFreq = pSi4735Manager->getCurrentBand().currFreq;
            freqDisplayComp->setFrequency(currentRadioFreq);
        }

        // Memória státusz ellenőrzése és frissítése
        checkAndUpdateMemoryStatus();

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
    // ===================================================================
    // RDS adatok valós idejű frissítése (optimalizált)
    // ===================================================================
    if (rdsComponent) {
        // RDS frissítés gyakrabban, de az RDSComponent maga időzít (1-3s adaptívan)
        static uint32_t lastRdsCall = 0;
        uint32_t currentTime = millis();

        // 100ms = 10 Hz (nem túl gyakori, de elég a responsiveness-hez)
        if (currentTime - lastRdsCall >= 100) {
            rdsComponent->updateRDS();
            lastRdsCall = currentTime;
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
    // Szülő osztály aktiválása
    UIScreen::activate();

    // StatusLine frissítése
    checkAndUpdateMemoryStatus();
}

/**
 * @brief Dialógus bezárásának kezelése - Gombállapot szinkronizálás
 * @details Az utolsó dialógus bezárásakor frissíti a gombállapotokat
 *
 * Ez a metódus biztosítja, hogy a gombállapotok konzisztensek maradjanak
 * a dialógusok bezárása után. Különösen fontos a ValueChangeDialog-ok
 * (Volume, Attenuator, Squelch, Frequency) után.
 */
void FMScreen::onDialogClosed(UIDialogBase *closedDialog) {

    // Először hívjuk az alap implementációt (stack cleanup, navigation logic)
    UIScreen::onDialogClosed(closedDialog); // Ha ez volt az utolsó dialógus, frissítsük a gombállapotokat
    if (!isDialogActive()) {
        updateAllVerticalButtonStates(pSi4735Manager); // Függőleges gombok szinkronizálása
        updateHorizontalButtonStates();                // Vízszintes gombok szinkronizálása

        // A gombsor konténer teljes újrarajzolása, hogy biztosan megjelenjenek a gombok
        if (horizontalButtonBar) {
            horizontalButtonBar->markForCompleteRedraw();
        }
    }
}

// ===================================================================
// Függőleges gombsor - Jobb oldali funkcionális gombok
// ===================================================================

// ===================================================================
// Vízszintes gombsor - Alsó navigációs gombok
// ===================================================================

/**
 * @brief Vízszintes gombsor létrehozása a képernyő alján
 * @details 4 navigációs gomb elhelyezése vízszintes elrendezésben:
 *
 * Gombsor pozíció: Bal alsó sarok, 4 gomb szélessége
 * Gombok (balról jobbra):
 * 1. SEEK DOWN - Automatikus hangolás lefelé (Pushable)
 * 2. SEEK UP - Automatikus hangolás felfelé (Pushable)
 * 3. AM - AM képernyőre váltás (Pushable)
 * 4. Test - Test képernyőre váltás (Pushable)
 */
void FMScreen::createHorizontalButtonBar() {

    // ===================================================================
    // Gombsor pozicionálás - Bal alsó sarok
    // ===================================================================
    const uint16_t buttonBarHeight = 35;                                 // Optimális gombmagasság
    const uint16_t buttonBarX = 0;                                       // Bal szélhez igazítva
    const uint16_t buttonBarY = UIComponent::SCREEN_H - buttonBarHeight; // Alsó szélhez igazítva
    const uint16_t buttonBarWidth = 300;                                 // 4 gomb + margók optimális szélessége

    // ===================================================================
    // Gomb konfigurációk - Seek és navigációs események
    // ===================================================================
    std::vector<UIHorizontalButtonBar::ButtonConfig> buttonConfigs = {

        // 1. SEEK DOWN - Automatikus hangolás lefelé
        {FMScreenHorizontalButtonIDs::SEEK_DOWN_BUTTON, "Seek-", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleSeekDownButton(event); }},

        // 2. SEEK UP - Automatikus hangolás felfelé
        {FMScreenHorizontalButtonIDs::SEEK_UP_BUTTON, "Seek+", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleSeekUpButton(event); }},

        // 3. AM - AM/MW/LW/SW képernyőre váltás
        {FMScreenHorizontalButtonIDs::AM_BUTTON, "AM", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleAMButton(event); }},

        // 4. TEST - Teszt és diagnosztikai képernyőre váltás
        {FMScreenHorizontalButtonIDs::TEST_BUTTON, "Test", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleTestButton(event); }}

        //
    };

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
    bool isAMMode = (currentBandType != FM_BAND_TYPE);
    horizontalButtonBar->setButtonState(FMScreenHorizontalButtonIDs::AM_BUTTON, isAMMode ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
}

// ===================================================================
// Vízszintes gomb eseménykezelők - Seek és navigációs funkciók
// ===================================================================

/**
 * @brief SEEK DOWN gomb eseménykezelő - Automatikus hangolás lefelé
 * @param event Gomb esemény (Clicked)
 *
 * @details Pushable gomb: Automatikus állomáskeresés lefelé
 * A seek során valós időben frissíti a frekvencia kijelzőt
 */
void FMScreen::handleSeekDownButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        if (pSi4735Manager) {
            // RDS cache törlése seek indítása előtt
            clearRDSCache(); // Seek lefelé a RadioScreen metódusával
            seekStationDown();

            // Seek befejezése után: RDS és memória státusz frissítése
            clearRDSCache();
            checkAndUpdateMemoryStatus();
        }
    }
}

/**
 * @brief SEEK UP gomb eseménykezelő - Automatikus hangolás felfelé
 * @param event Gomb esemény (Clicked)
 *
 * @details Pushable gomb: Automatikus állomáskeresés felfelé
 * A seek során valós időben frissíti a frekvencia kijelzőt
 */
void FMScreen::handleSeekUpButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        if (pSi4735Manager) {
            // RDS cache törlése seek indítása előtt
            clearRDSCache(); // Seek felfelé a RadioScreen metódusával
            seekStationUp();

            // Seek befejezése után: RDS és memória státusz frissítése
            clearRDSCache();
            checkAndUpdateMemoryStatus();
        }
    }
}

/**
 * @brief AM gomb eseménykezelő - AM családú képernyőre váltás
 * @param event Gomb esemény (Clicked)
 *
 * @details Pushable gomb: AM/MW/LW/SW képernyőre navigálás
 * Váltás az FM-től eltérő band családra
 */
void FMScreen::handleAMButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
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
        UIScreen::getScreenManager()->switchToScreen(SCREEN_NAME_TEST);
    }
}

/**
 * @brief Egyedi MEMO gomb eseménykezelő - Intelligens memória kezelés
 * @param event Gomb esemény (Clicked)
 *
 * @details Ha az aktuális állomás még nincs a memóriában és van RDS állomásnév,
 * akkor automatikusan megnyitja a MemoryScreen-t név szerkesztő dialógussal
 */
void FMScreen::handleMemoButton(const UIButton::ButtonEvent &event) {
    if (event.state != UIButton::EventButtonState::Clicked) {
        return;
    }

    DEBUG("FM Custom Memo button clicked\n");

    auto screenManager = getScreenManager();
    if (!screenManager) {
        DEBUG("ERROR: Could not get screenManager from FMScreen!\n");
        return;
    }

    // Ellenőrizzük, hogy az aktuális állomás már a memóriában van-e
    bool isInMemory = checkCurrentFrequencyInMemory(); // RDS állomásnév lekérése (ha van)
    String rdsStationName = pSi4735Manager->getCachedStationName();

    // Ha új állomás és van RDS név, akkor automatikus hozzáadás
    if (!isInMemory && rdsStationName.length() > 0) {
        DEBUG("Station not in memory and has RDS name: %s\n", rdsStationName.c_str());

        // ScreenManager biztonságos paraméter beállítása
        screenManager->setMemoryScreenParams(true, rdsStationName.c_str());

        // MemoryScreen megnyitása a ScreenManager buffer-ből
        screenManager->switchToMemoryScreen();
    } else {
        DEBUG("Station already in memory or no RDS name - opening Memory screen normally\n");

        // Normál MemoryScreen megnyitása paraméterek nélkül
        screenManager->switchToScreen(SCREEN_NAME_MEMORY);
    }
}

/**
 * @brief Egyedi függőleges gombok létrehozása - Memo gomb override-dal
 * @details Felülírja a CommonVerticalButtons alapértelmezett Memo kezelőjét
 */
void FMScreen::createCommonVerticalButtonsWithCustomMemo() {
    // Alapértelmezett gombdefiníciók lekérése
    const auto &baseDefs = CommonVerticalButtons::getButtonDefinitions();

    // Egyedi gombdefiníciók lista létrehozása
    std::vector<ButtonGroupDefinition> customDefs;
    customDefs.reserve(baseDefs.size());

    // Végigmegyünk az alapértelmezett definíciókon
    for (const auto &def : baseDefs) {
        std::function<void(const UIButton::ButtonEvent &)> callback;

        // Memo gomb speciális kezelése
        if (def.id == VerticalButtonIDs::MEMO) {
            // Egyedi Memo handler használata
            callback = [this](const UIButton::ButtonEvent &e) { this->handleMemoButton(e); };
        } else if (def.handler != nullptr) {
            // Többi gomb: eredeti handler használata
            callback = [si4735Manager = pSi4735Manager, screen = this, handler = def.handler](const UIButton::ButtonEvent &e) { handler(e, si4735Manager, screen); };
        } else {
            // No-op callback üres handlerekhez
            callback = [](const UIButton::ButtonEvent &e) { /* no-op */ };
        }

        // Gombdefiníció hozzáadása a listához
        customDefs.push_back({def.id, def.label, def.type, callback, def.initialState,
                              60, // uniformWidth
                              def.height});
    }

    // Gombok létrehozása és elhelyezése
    ButtonsGroupManager<FMScreen>::layoutVerticalButtonGroup(customDefs, &createdVerticalButtons, 0, 0, 5, 60, 32, 3, 4);
}
