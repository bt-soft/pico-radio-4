#include "FMScreen.h"
#include "Band.h"
#include "CommonVerticalButtons.h"
#include "Config.h"
#include "DmaAudioProcessor.h"
#include "FreqDisplay.h"
#include "StatusLine.h"
#include "UIColorPalette.h"
#include "UIHorizontalButtonBar.h"
#include "rtVars.h"

// ===================================================================
// FM képernyő specifikus vízszintes gomb azonosítók
// ===================================================================
namespace FMScreenHorizontalButtonIDs {
static constexpr uint8_t SEEK_DOWN_BUTTON = 60; ///< Seek lefelé (pushable) - FM specifikus
static constexpr uint8_t SEEK_UP_BUTTON = 61;   ///< Seek felfelé (pushable) - FM specifikus
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

    // UI komponensek elhelyezése
    layoutComponents();
}

/**
 * @brief FMScreen destruktor - MiniAudioDisplay parent pointer törlése
 * @details Biztosítja, hogy az MiniAudioDisplay ne próbáljon hozzáférni
 * a törölt screen objektumhoz képernyőváltáskor
 */
FMScreen::~FMScreen() {
    DEBUG("FMScreen::~FMScreen() - Destruktor hívása\n");
    if (miniAudioDisplay) {
        DEBUG("FMScreen::~FMScreen() - miniAudioDisplay parent pointer törlése\n");
        miniAudioDisplay->clearParentScreen();
    }
    DEBUG("FMScreen::~FMScreen() - Destruktor befejezve\n");
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
    Rect freqBounds(0, FreqDisplayY, FreqDisplay::FREQDISPLAY_WIDTH - 60, FreqDisplay::FREQDISPLAY_HEIGHT);
    UIScreen::createFreqDisplay(freqBounds);
    freqDisplayComp->setHideUnderline(true); // Alulvonás elrejtése a frekvencia kijelzőn

    // ===================================================================
    // STEREO/MONO jelző létrehozása
    // ===================================================================
    uint16_t stereoY = FreqDisplayY;
    Rect stereoBounds(FreqDisplay::FREQDISPLAY_WIDTH - 130, stereoY, 50, 20);
    stereoIndicator = std::make_shared<StereoIndicator>(tft, stereoBounds);
    addChild(stereoIndicator);

    // ===================================================================
    // RDS komponens létrehozása és pozicionálása
    // ===================================================================
    createRDSComponent(); // RDS komponens létrehozása

    // RDS Állomásnév közvetlenül a frekvencia kijelző alatt
    uint16_t currentY = FreqDisplayY + FreqDisplay::FREQDISPLAY_HEIGHT - 15;
    rdsComponent->setStationNameArea(Rect(2, currentY, 180, 32));

    // RDS Program típus közvetlenül az állomásnév alatt
    currentY += 32 + 5; // 18px magasság + 5px kisebb hézag
    rdsComponent->setProgramTypeArea(Rect(2, currentY, 135, 18));

    // Dátum/idő
    rdsComponent->setDateTimeArea(Rect(2 + 130 + 5, currentY, 105, 18)); // Ugyanaz az Y pozíció, mint a program típus

    // RDS Radio text
    currentY += 18 + 5;
    rdsComponent->setRadioTextArea(Rect(2, currentY, SMeterConstants::SMETER_WIDTH, 24)); // ===================================================================
    // S-Meter komponens létrehozása - RadioScreen közös implementáció
    // ===================================================================
    currentY += 24 + 5;
    Rect smeterBounds(2, currentY, SMeterConstants::SMETER_WIDTH, 60);
    createSMeterComponent(smeterBounds);

    // ===================================================================
    // Mini Audio Display komponens létrehozása
    // ===================================================================

    // Globális audio processor deklarálása (extern)
    extern DmaAudioProcessor *g_audioProcessor;                     // A S-Meter jobb oldalán helyezzük el, de a függőleges gombsor bal oldalától 10px-el balra
    uint16_t rightButtonsStartX = 480 - 60;                         // Feltételezve 60px széles függőleges gombok
    uint16_t miniAudioX = SMeterConstants::SMETER_WIDTH + 10;       // 10px hézag az S-Meter után
    uint16_t miniAudioWidth = rightButtonsStartX - miniAudioX - 10; // 10px hézag a gombokig
    uint16_t miniAudioHeight = 120; // Kétszer olyan magas mint az S-Meter    // EREDETI pozíció visszaállítása - S-Meter jobb oldalán, 50px-el feljebb emelve
    Rect miniAudioBounds(miniAudioX, currentY - 50, miniAudioWidth, miniAudioHeight);
    miniAudioDisplay = std::make_shared<MiniAudioDisplay>(tft, miniAudioBounds,
                                                          g_audioProcessor,               // audio data provider
                                                          config.data.miniAudioFftModeFm, // config mód referencia
                                                          this,                           // parent screen referencia
                                                          20000.0f                        // max freq Hz (20kHz FM audio)
    );

    // ===================================================================
    // Gombsorok létrehozása - Event-driven architektúra
    // ===================================================================
    createCommonVerticalButtonsWithCustomMemo(); // ButtonsGroupManager alapú függőleges gombsor egyedi Memo kezelővel
    createCommonHorizontalButtons();             // Alsó közös + FM specifikus vízszintes gombsor

    // MiniAudioDisplay hozzáadása UTOLJÁRA a Z-order miatt (hogy felül legyen)
    addChild(miniAudioDisplay);
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
        // Beállítjuk a chip-en és le is mentjük a band táblába a frekvenciát
        uint16_t currFreq = pSi4735Manager->stepFrequency(event.value); // Léptetjük a rádiót
        pSi4735Manager->getCurrentBand().currFreq = currFreq;           // Beállítjuk a band táblában a frekit

        // RDS cache törlése frekvencia változás miatt
        if (rdsComponent) {
            rdsComponent->clearRdsOnFrequencyChange();
        }

        // Frekvencia kijelző azonnali frissítése
        if (freqDisplayComp) {
            freqDisplayComp->setFrequency(currFreq);
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
    // S-Meter (jelerősség) időzített frissítése - Közös RadioScreen implementáció
    // ===================================================================
    updateSMeter(true /* FM mód */); // ===================================================================
    // RDS adatok valós idejű frissítése
    // ===================================================================
    if (rdsComponent) {
        static uint32_t lastRdsCall = 0;
        uint32_t currentTime = millis();

        // 500ms frissítési időköz az RDS adatokhoz
        if (currentTime - lastRdsCall >= 500) {
            rdsComponent->updateRDS();
            lastRdsCall = currentTime;
        }
    }

    // Néhány adatot csak ritkábban frissítünk
#define SCREEN_COMPS_REFRESH_TIME_MSEC 1000                                  // Frissítési időköz
    static uint32_t elapsedTimedValues = 0;                                  // Kezdőérték nulla
    if ((millis() - elapsedTimedValues) >= SCREEN_COMPS_REFRESH_TIME_MSEC) { // ===================================================================
        // STEREO/MONO jelző frissítése
        // ===================================================================
        if (stereoIndicator && pSi4735Manager) {
            // Si4735 stereo állapot lekérdezése
            bool isStereo = pSi4735Manager->getSi4735().getCurrentPilot();
            stereoIndicator->setStereo(isStereo);
        }

        // ===================================================================
        // Mini Audio Display frissítése
        // ===================================================================
        if (miniAudioDisplay) {
            miniAudioDisplay->update();
        }

        // Frissítjük az időbélyeget
        elapsedTimedValues = millis();
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

    // TESZT kód eltávolítva - nem kell többé direkt rajzolás
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

    DEBUG("FMScreen::activate() - Képernyő aktiválása\n");

    // Szülő osztály aktiválása (RadioScreen -> UIScreen)
    RadioScreen::activate();

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

    // Először hívjuk a RadioScreen implementációt (band váltás kezelés)
    RadioScreen::onDialogClosed(closedDialog);

    // Ha ez volt az utolsó dialógus, frissítsük a gombállapotokat
    if (!isDialogActive()) {
        updateAllVerticalButtonStates(pSi4735Manager); // Függőleges gombok szinkronizálása
        updateCommonHorizontalButtonStates();          // Közös gombok szinkronizálása
        updateHorizontalButtonStates();                // FM specifikus gombok szinkronizálása

        // A gombsor konténer teljes újrarajzolása, hogy biztosan megjelenjenek a gombok
        if (horizontalButtonBar) {
            horizontalButtonBar->markForCompleteRedraw();
        }
    }
}

// ===================================================================
// Vízszintes gombsor - Alsó navigációs gombok
// ===================================================================

/**
 * @brief FM specifikus gombok hozzáadása a közös gombokhoz
 * @param buttonConfigs A már meglévő gomb konfigurációk vektora
 * @details Felülírja az ős metódusát, hogy hozzáadja az FM specifikus gombokat
 */
void FMScreen::addSpecificHorizontalButtons(std::vector<UIHorizontalButtonBar::ButtonConfig> &buttonConfigs) {
    // FM specifikus gombok hozzáadása a közös gombok után

    // 1. SEEK DOWN - Automatikus hangolás lefelé
    buttonConfigs.push_back({FMScreenHorizontalButtonIDs::SEEK_DOWN_BUTTON, //
                             "Seek-",                                       //
                             UIButton::ButtonType::Pushable,                //
                             UIButton::ButtonState::Off,                    //
                             [this](const UIButton::ButtonEvent &event) {   //
                                 handleSeekDownButton(event);
                             }});

    // 2. SEEK UP - Automatikus hangolás felfelé
    buttonConfigs.push_back({                                             //
                             FMScreenHorizontalButtonIDs::SEEK_UP_BUTTON, //
                             "Seek+",                                     //
                             UIButton::ButtonType::Pushable,              //
                             UIButton::ButtonState::Off,                  //
                             [this](const UIButton::ButtonEvent &event) { //
                                 handleSeekUpButton(event);
                             }});
}

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
/**
 * @brief FM specifikus vízszintes gombsor állapotainak szinkronizálása
 * @details Event-driven architektúra: CSAK aktiváláskor hívódik meg!
 *
 * Szinkronizált állapotok:
 * - FM specifikus gombok (Seek-, Seek+) alapértelmezett állapotai
 */
void FMScreen::updateHorizontalButtonStates() {
    if (!horizontalButtonBar) {
        return; // Biztonsági ellenőrzés
    }

    // ===================================================================
    // FM specifikus gombok állapot szinkronizálása
    // ===================================================================

    // Seek gombok alapértelmezett állapotban (kikapcsolva)
    horizontalButtonBar->setButtonState(FMScreenHorizontalButtonIDs::SEEK_DOWN_BUTTON, UIButton::ButtonState::Off);
    horizontalButtonBar->setButtonState(FMScreenHorizontalButtonIDs::SEEK_UP_BUTTON, UIButton::ButtonState::Off);
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

    auto screenManager = getScreenManager();

    // Ellenőrizzük, hogy az aktuális állomás már a memóriában van-e
    bool isInMemory = checkCurrentFrequencyInMemory();              // RDS állomásnév lekérése (ha van)
    String rdsStationName = pSi4735Manager->getCachedStationName(); // Ha új állomás és van RDS név, akkor automatikus hozzáadás

    if (!isInMemory && rdsStationName.length() > 0) {
        // ScreenManager biztonságos paraméter beállítása
        screenManager->setMemoryScreenParams(true, rdsStationName.c_str());

        // MemoryScreen megnyitása a ScreenManager buffer-ből
        screenManager->switchToMemoryScreen();
    } else {
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
