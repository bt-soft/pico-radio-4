#include "AMScreen.h"
#include "Band.h"
#include "CommonVerticalButtons.h"
#include "Config.h"
#include "FreqDisplay.h"
#include "MultiButtonDialog.h"
#include "StatusLine.h"
#include "defines.h"
#include "rtVars.h"
#include "utils.h"
#include <algorithm>

// ===================================================================
// Vízszintes gombsor azonosítók - Képernyő-specifikus navigáció
// ===================================================================

/**
 * @brief AM képernyő specifikus vízszintes gomb azonosítók
 * @details Alsó vízszintes gombsor - AM specifikus funkcionalitás
 *
 * **ID tartomány**: 70-74 (nem ütközik a közös 50-52 és FM 60-61 tartománnyal)
 * **Funkció**: AM specifikus rádió funkciók
 * **Gomb típus**: Pushable (egyszeri nyomás → funkció végrehajtása)
 */
namespace AMScreenHorizontalButtonIDs {
static constexpr uint8_t BFO_BUTTON = 70;    ///< Beat Frequency Oscillator
static constexpr uint8_t AFBW_BUTTON = 71;   ///< Audio Filter Bandwidth
static constexpr uint8_t ANTCAP_BUTTON = 72; ///< Antenna Capacitor
static constexpr uint8_t DEMOD_BUTTON = 73;  ///< Demodulation
static constexpr uint8_t STEP_BUTTON = 74;   ///< Frequency Step
} // namespace AMScreenHorizontalButtonIDs

// =====================================================================
// Konstruktor és inicializálás
// =====================================================================

/**
 * @brief AMScreen konstruktor implementáció - RadioScreen alaposztályból származik
 * @param tft TFT display referencia
 * @param si4735Manager Si4735 rádió chip kezelő referencia
 */
AMScreen::AMScreen(TFT_eSPI &tft, Si4735Manager &si4735Manager) : RadioScreen(tft, SCREEN_NAME_AM, &si4735Manager) {

    // UI komponensek létrehozása és elhelyezése
    layoutComponents();
}

/**
 * @brief AMScreen destruktor - MiniAudioDisplay parent pointer törlése
 * @details Biztosítja, hogy az MiniAudioDisplay ne próbáljon hozzáférni
 * a törölt screen objektumhoz képernyőváltáskor
 */
AMScreen::~AMScreen() { DEBUG("AMScreen::~AMScreen() - Destruktor hívása\n"); }

// =====================================================================
// UIScreen interface megvalósítás
// =====================================================================

/**
 * @brief Rotary encoder eseménykezelés - AM frekvencia hangolás implementáció
 * @param event Rotary encoder esemény (forgatás irány, érték, gombnyomás)
 * @return true ha sikeresen kezelte az eseményt, false egyébként
 *
 * @details AM frekvencia hangolás logika:
 * - Csak akkor reagál, ha nincs aktív dialógus
 * - Rotary klikket figyelmen kívül hagyja (más funkciókhoz)
 * - AM/MW/LW/SW frekvencia léptetés és mentés a band táblába
 * - Frekvencia kijelző azonnali frissítése
 * - Hasonló az FMScreen rotary kezeléshez, de AM-specifikus tartományokkal
 */
bool AMScreen::handleRotary(const RotaryEvent &event) {

    // Biztonsági ellenőrzés: csak aktív dialógus nélkül és nem klikk eseménykor
    if (isDialogActive() || event.buttonState == RotaryEvent::ButtonState::Clicked) {
        // Nem kezeltük az eseményt, továbbítjuk a szülő osztálynak (dialógusokhoz)
        return UIScreen::handleRotary(event);
    }

    uint16_t newFreq;

    BandTable &currentBand = pSi4735Manager->getCurrentBand();
    // Az SI4735 osztály cache-ból olvassuk az aktuális frekvenciát, nem használunk chip olvasást
    uint16_t currentFrequency = pSi4735Manager->getSi4735().getCurrentFrequency();

    bool isCurrentDemodSSBorCW = pSi4735Manager->isCurrentDemodSSBorCW();

    if (isCurrentDemodSSBorCW) {

        if (rtv::bfoOn) {

            int16_t step = rtv::currentBFOStep;
            rtv::currentBFOmanu += (event.direction == RotaryEvent::Direction::Up) ? step : -step;
            rtv::currentBFOmanu = constrain(rtv::currentBFOmanu, -999, 999);

        } else {

            // Hangolás felfelé
            if (event.direction == RotaryEvent::Direction::Up) {

                rtv::freqDec = rtv::freqDec - rtv::freqstep;
                uint32_t freqTot = (uint32_t)(currentFrequency * 1000) + (rtv::freqDec * -1);

                if (freqTot > (uint32_t)(currentBand.maximumFreq * 1000)) {
                    pSi4735Manager->getSi4735().setFrequency(currentBand.maximumFreq);
                    rtv::freqDec = 0;
                }

                if (rtv::freqDec <= -16000) {
                    rtv::freqDec = rtv::freqDec + 16000;
                    int16_t freqPlus16 = currentFrequency + 16;
                    pSi4735Manager->hardwareAudioMuteOn();
                    pSi4735Manager->getSi4735().setFrequency(freqPlus16);
                    delay(10);
                }

            } else { // Hangolás lefelé

                rtv::freqDec = rtv::freqDec + rtv::freqstep;
                uint32_t freqTot = (uint32_t)(currentFrequency * 1000) - rtv::freqDec;
                if (freqTot < (uint32_t)(currentBand.minimumFreq * 1000)) {
                    pSi4735Manager->getSi4735().setFrequency(currentBand.minimumFreq);
                    rtv::freqDec = 0;
                }

                if (rtv::freqDec >= 16000) {
                    rtv::freqDec = rtv::freqDec - 16000;
                    int16_t freqMin16 = currentFrequency - 16;
                    pSi4735Manager->hardwareAudioMuteOn();
                    pSi4735Manager->getSi4735().setFrequency(freqMin16);
                    delay(10);
                }
            }
            rtv::currentBFO = rtv::freqDec;
            rtv::lastBFO = rtv::currentBFO;
        }

        // Lekérdezzük a beállított frekvenciát
        // Az SI4735 osztály cache-ból olvassuk az aktuális frekvenciát, nem használunk chip olvasást
        newFreq = pSi4735Manager->getSi4735().getCurrentFrequency();

        // SSB hangolás esetén a BFO eltolás beállítása
        const int16_t cwBaseOffset = (currentBand.currDemod == CW_DEMOD_TYPE) ? 750 : 0; // Ideiglenes konstans CW offset
        int16_t bfoToSet = cwBaseOffset + rtv::currentBFO + rtv::currentBFOmanu;
        pSi4735Manager->getSi4735().setSSBBfo(bfoToSet);

    } else {
        // Léptetjük a rádiót, ez el is menti a band táblába
        newFreq = pSi4735Manager->stepFrequency(event.value);
    } // AGC
    pSi4735Manager->checkAGC();

    // Frekvencia kijelző azonnali frissítése
    if (freqDisplayComp) {
        // SSB/CW módban mindig frissítjük a kijelzőt, mert a finomhangolás (rtv::freqDec)
        // változhat anélkül, hogy a chip frekvencia megváltozna
        freqDisplayComp->setFrequency(newFreq, isCurrentDemodSSBorCW);
    }

    // Memória státusz ellenőrzése és frissítése
    checkAndUpdateMemoryStatus();

    return true; // Esemény sikeresen kezelve
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
    // S-Meter (jelerősség) időzített frissítése - Közös RadioScreen implementáció
    // ===================================================================
    updateSMeter(false /* AM mód */);

    // ===================================================================
    // Mini Audio Display frissítése
    // ===================================================================
    if (miniAudioDisplay) {
        miniAudioDisplay->update();
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
    DEBUG("AMScreen::activate() - Képernyő aktiválása\n");

    // Szülő osztály aktiválása (RadioScreen -> UIScreen)
    RadioScreen::activate();

    // ===================================================================
    // *** EGYETLEN GOMBÁLLAPOT SZINKRONIZÁLÁSI PONT - Event-driven ***
    // ===================================================================
    updateAllVerticalButtonStates(pSi4735Manager); // Univerzális funkcionális gombok (mixin method)
    updateCommonHorizontalButtonStates();          // Közös gombok szinkronizálása
    updateHorizontalButtonStates();                // AM-specifikus gombok szinkronizálása
    updateFreqDisplayWidth();                      // FreqDisplay szélességének frissítése

    // MEGJEGYZÉS: A frekvencia kijelző frissítése nem szükséges itt,
    // mert a FreqDisplay konstruktor már beállította a helyes frekvenciát
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

    // Először hívjuk a RadioScreen implementációt (band váltás kezelés)
    RadioScreen::onDialogClosed(closedDialog);

    // Ha ez volt az utolsó dialógus, frissítsük a gombállapotokat
    if (!isDialogActive()) {
        updateAllVerticalButtonStates(pSi4735Manager); // Függőleges gombok szinkronizálása
        updateCommonHorizontalButtonStates();          // Közös gombok szinkronizálása
        updateHorizontalButtonStates();                // AM specifikus gombok szinkronizálása
        updateFreqDisplayWidth();                      // FreqDisplay szélességének frissítése

        // A gombsor konténer teljes újrarajzolása, hogy biztosan megjelenjenek a gombok
        if (horizontalButtonBar) {
            horizontalButtonBar->markForCompleteRedraw();
        }
    }
}

// =====================================================================
// UI komponensek layout és management
// =====================================================================

/**
 * @brief UI komponensek létrehozása és képernyőn való elhelyezése
 */
void AMScreen::layoutComponents() {

    // Állapotsor komponens létrehozása (felső sáv)
    UIScreen::createStatusLine();
    // ===================================================================
    // Frekvencia kijelző pozicionálás (képernyő közép)
    // ===================================================================
    uint16_t FreqDisplayY = 20;
    Rect freqBounds(0, FreqDisplayY, FreqDisplay::FREQDISPLAY_WIDTH, FreqDisplay::FREQDISPLAY_HEIGHT + 10);
    UIScreen::createFreqDisplay(freqBounds);

    // Dinamikus szélesség beállítása band típus alapján
    updateFreqDisplayWidth();

    // Finomhangolás jel (alulvonás) elrejtése a frekvencia kijelzőn, ha nem HAM sávban vagyunk
    freqDisplayComp->setHideUnderline(!pSi4735Manager->isCurrentHamBand());

    // ===================================================================
    // S-Meter komponens létrehozása - RadioScreen közös implementáció
    // ===================================================================

    Rect smeterBounds(2, FreqDisplayY + FreqDisplay::FREQDISPLAY_HEIGHT, SMeterConstants::SMETER_WIDTH, 60);
    createSMeterComponent(smeterBounds);

    // ===================================================================
    // Mini Audio Display komponens létrehozása
    // ===================================================================
    createMiniAudioDisplay();

    createCommonVerticalButtons(pSi4735Manager); // ButtonsGroupManager használata
    createCommonHorizontalButtons();             // Alsó közös + AM specifikus vízszintes gombsor
}

/**
 * @brief AM specifikus gombok hozzáadása a közös gombokhoz
 * @param buttonConfigs A már meglévő gomb konfigurációk vektora
 * @details Felülírja az ős metódusát, hogy hozzáadja az AM specifikus gombokat
 */
void AMScreen::addSpecificHorizontalButtons(std::vector<UIHorizontalButtonBar::ButtonConfig> &buttonConfigs) { // AM specifikus gombok hozzáadása a közös gombok után

    // 1. BFO - Beat Frequency Oscillator
    buttonConfigs.push_back({AMScreenHorizontalButtonIDs::BFO_BUTTON, "BFO", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
                             [this](const UIButton::ButtonEvent &event) { handleBFOButton(event); }});

    // 2. AfBW - Audio Filter Bandwidth
    buttonConfigs.push_back({AMScreenHorizontalButtonIDs::AFBW_BUTTON, "AfBW", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                             [this](const UIButton::ButtonEvent &event) { handleAfBWButton(event); }});

    // 3. AntCap - Antenna Capacitor
    buttonConfigs.push_back({AMScreenHorizontalButtonIDs::ANTCAP_BUTTON, "AntCap", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                             [this](const UIButton::ButtonEvent &event) { handleAntCapButton(event); }});

    // 4. Demod - Demodulation
    buttonConfigs.push_back({AMScreenHorizontalButtonIDs::DEMOD_BUTTON, "Demod", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                             [this](const UIButton::ButtonEvent &event) { handleDemodButton(event); }});

    // 5. Step - Frequency Step
    buttonConfigs.push_back({AMScreenHorizontalButtonIDs::STEP_BUTTON, "Step", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                             [this](const UIButton::ButtonEvent &event) { handleStepButton(event); }});
}

// =====================================================================
// EVENT-DRIVEN GOMBÁLLAPOT SZINKRONIZÁLÁS
// =====================================================================

/**
 * @brief AM specifikus vízszintes gombsor állapotainak szinkronizálása
 * @details Event-driven architektúra: CSAK aktiváláskor hívódik meg!
 *
 * Szinkronizált állapotok:
 * - AM specifikus gombok alapértelmezett állapotai
 */
void AMScreen::updateHorizontalButtonStates() {

    if (!horizontalButtonBar) {
        return; // Biztonsági ellenőrzés
    }

    // ===================================================================
    // AM specifikus gombok állapot szinkronizálása
    // ===================================================================

    // BFO és Step gombok speciális logikája: használjuk a dedikált metódusokat
    updateBFOButtonState();
    updateStepButtonState();

    // Többi AM specifikus gomb alapértelmezett állapotban
    horizontalButtonBar->setButtonState(AMScreenHorizontalButtonIDs::AFBW_BUTTON, UIButton::ButtonState::Off);
    horizontalButtonBar->setButtonState(AMScreenHorizontalButtonIDs::ANTCAP_BUTTON, UIButton::ButtonState::Off);
    horizontalButtonBar->setButtonState(AMScreenHorizontalButtonIDs::DEMOD_BUTTON, UIButton::ButtonState::Off);

    // ===================================================================
    // Step gomb speciális logika: használjuk a dedikált metódust
    // ===================================================================
    updateStepButtonState();
}

/**
 * @brief Step gomb állapotának frissítése
 * @details SSB/CW módban csak akkor engedélyezett, ha BFO be van kapcsolva
 */
void AMScreen::updateStepButtonState() {

    if (!horizontalButtonBar) {
        return; // Biztonsági ellenőrzés
    }

    UIButton::ButtonState stepButtonState = UIButton::ButtonState::Off;

    if (pSi4735Manager->isCurrentDemodSSBorCW()) {
        // SSB/CW módban: csak akkor engedélyezett, ha BFO be van kapcsolva
        stepButtonState = rtv::bfoOn ? UIButton::ButtonState::Off : UIButton::ButtonState::Disabled;
    } else {
        // AM/egyéb módban: mindig engedélyezett
        stepButtonState = UIButton::ButtonState::Off;
    }

    horizontalButtonBar->setButtonState(AMScreenHorizontalButtonIDs::STEP_BUTTON, stepButtonState);
}

/**
 * @brief BFO gomb állapotának frissítése
 * @details Csak SSB/CW módban engedélyezett
 */
void AMScreen::updateBFOButtonState() {
    if (!horizontalButtonBar) {
        return; // Biztonsági ellenőrzés
    }

    UIButton::ButtonState bfoButtonState;

    if (pSi4735Manager->isCurrentDemodSSBorCW()) {
        // SSB/CW módban: BFO állapot szerint be/ki kapcsolva
        bfoButtonState = rtv::bfoOn ? UIButton::ButtonState::On : UIButton::ButtonState::Off;
        // DEBUG("AMScreen::updateBFOButtonState - SSB/CW mode, BFO button: %s\n", rtv::bfoOn ? "ON" : "OFF");
    } else {
        // AM/egyéb módban: letiltva
        bfoButtonState = UIButton::ButtonState::Disabled;
        // DEBUG("AMScreen::updateBFOButtonState - Non-SSB mode, BFO button: DISABLED\n");
    }

    horizontalButtonBar->setButtonState(AMScreenHorizontalButtonIDs::BFO_BUTTON, bfoButtonState);
}

// =====================================================================
// AM specifikus gomb eseménykezelők
// =====================================================================

/**
 * @brief BFO gomb eseménykezelő - Beat Frequency Oscillator
 * @param event Gomb esemény (Clicked)
 * @details AM specifikus funkcionalitás - BFO állapot váltása és Step gomb frissítése
 */
void AMScreen::handleBFOButton(const UIButton::ButtonEvent &event) {

    // Csak véltoztatásra reagálunk, nem kattintásra
    if (event.state != UIButton::EventButtonState::On && event.state != UIButton::EventButtonState::Off) {
        return;
    }

    // Csak SSB/CW módban működik
    if (!pSi4735Manager->isCurrentDemodSSBorCW()) {
        return;
    } // BFO állapot váltása
    rtv::bfoOn = !rtv::bfoOn;
    rtv::bfoTr = true; // BFO animáció trigger beállítása

    // A Step gombok állapotának frissítése
    updateStepButtonState();

    // Frissítjük a frekvencia kijelzőt is, hogy BFO állapot változás volt
    freqDisplayComp->forceFullRedraw();
}

/**
 * @brief AfBW gomb eseménykezelő - Audio Frequency Bandwidth
 * @param event Gomb esemény (Clicked)
 * @details AM specifikus funkcionalitás - sávszélesség váltás
 */
void AMScreen::handleAfBWButton(const UIButton::ButtonEvent &event) {
    if (event.state != UIButton::EventButtonState::Clicked) {
        return; // Csak kattintásra reagálunk
    }

    // Aktuális demodulációs mód
    uint8_t currMod = pSi4735Manager->getCurrentBand().currDemod;
    // Jelenlegi sávszélesség felirata
    const char *currentBw = pSi4735Manager->getCurrentBandWidthLabel();

    // Megállapítjuk a lehetséges sávszélességek tömbjét
    const char *title;
    size_t labelsCount;
    const char **labels;
    uint16_t w = 250;
    uint16_t h = 170;

    if (currMod == FM_DEMOD_TYPE) {
        title = "FM Filter in kHz";
        labels = pSi4735Manager->getBandWidthLabels(Band::bandWidthFM, labelsCount);

    } else if (currMod == AM_DEMOD_TYPE) {
        title = "AM Filter in kHz";
        w = 350;
        h = 160;

        labels = pSi4735Manager->getBandWidthLabels(Band::bandWidthAM, labelsCount);

    } else {
        title = "SSB/CW Filter in kHz";
        w = 380;
        h = 130;

        labels = pSi4735Manager->getBandWidthLabels(Band::bandWidthSSB, labelsCount);
    }

    auto afBwDialog = std::make_shared<MultiButtonDialog>(
        this, this->tft,                                                                       // Képernyő referencia
        title, "",                                                                             // Dialógus címe és üzenete
        labels, labelsCount,                                                                   // Gombok feliratai és számuk
        [this, currMod](int buttonIndex, const char *buttonLabel, MultiButtonDialog *dialog) { // Gomb kattintás kezelése
            //

            if (currMod == AM_DEMOD_TYPE) {
                config.data.bwIdxAM = pSi4735Manager->getBandWidthIndexByLabel(Band::bandWidthAM, buttonLabel);
            } else if (currMod == FM_DEMOD_TYPE) {
                config.data.bwIdxFM = pSi4735Manager->getBandWidthIndexByLabel(Band::bandWidthFM, buttonLabel);
            } else {
                config.data.bwIdxSSB = pSi4735Manager->getBandWidthIndexByLabel(Band::bandWidthSSB, buttonLabel);
            }

            // Beállítjuk a rádió chip-en a kiválasztott HF sávszélességet
            pSi4735Manager->setAfBandWidth();
        },
        true,              // Automatikusan bezárja-e a dialógust gomb kattintáskor
        currentBw,         // Az alapértelmezett (jelenlegi) gomb felirata
        true,              // Ha true, az alapértelmezett gomb le van tiltva; ha false, csak vizuálisan kiemelve
        Rect(-1, -1, w, h) // Dialógus mérete (ha -1, akkor automatikusan a képernyő közepére igazítja)
    );
    this->showDialog(afBwDialog);
}

/**
 * @brief AntCap gomb eseménykezelő - Antenna Capacitor
 * @param event Gomb esemény (Clicked)
 */
void AMScreen::handleAntCapButton(const UIButton::ButtonEvent &event) {
    if (event.state != UIButton::EventButtonState::Clicked) {
        return;
    }
    BandTable &currband = pSi4735Manager->getCurrentBand(); // Kikeressük az aktuális Band rekordot

    // Store antCap value in a local int variable for the dialog to work with
    static int antCapTempValue; // Static to ensure it persists during dialog lifetime
    antCapTempValue = static_cast<int>(currband.antCap);

    auto antCapDialog = std::make_shared<ValueChangeDialog>(
        this, this->tft,                                                                                                             //
        "Antenna Tuning capacitor", "Capacitor value [pF]:",                                                                         //
        &antCapTempValue,                                                                                                            //
        1, currband.currDemod == FM_DEMOD_TYPE ? Si4735Constants::SI4735_MAX_ANT_CAP_FM : Si4735Constants::SI4735_MAX_ANT_CAP_AM, 1, //
        [this](const std::variant<int, float, bool> &liveNewValue) {
            if (std::holds_alternative<int>(liveNewValue)) {
                int currentDialogVal = std::get<int>(liveNewValue);
                pSi4735Manager->getCurrentBand().antCap = static_cast<uint16_t>(currentDialogVal);
                pSi4735Manager->getSi4735().setTuneFrequencyAntennaCapacitor(currentDialogVal);
            }
        },
        nullptr, // Callback a változásra
        Rect(-1, -1, 280, 0));
    this->showDialog(antCapDialog);
}

/**
 * @brief Demod gomb eseménykezelő - Demodulation
 * @param event Gomb esemény (Clicked)
 * @details AM specifikus funkcionalitás - alapértelmezett implementáció
 */
void AMScreen::handleDemodButton(const UIButton::ButtonEvent &event) {
    if (event.state != UIButton::EventButtonState::Clicked) {
        return;
    }

    uint8_t labelsCount;
    const char **labels = pSi4735Manager->getAmDemodulationModes(labelsCount);

    auto demodDialog = std::make_shared<MultiButtonDialog>(
        this, this->tft,                                                              // Képernyő referencia
        "Demodulation Mode", "",                                                      // Dialógus címe és üzenete
        labels, labelsCount,                                                          // Gombok feliratai és számuk
        [this](int buttonIndex, const char *buttonLabel, MultiButtonDialog *dialog) { // Gomb kattintás kezelése
            // Kikeressük az aktuális Band rekordot
            BandTable &currentband = pSi4735Manager->getCurrentBand();

            // Demodulációs mód bellítása
            currentband.currDemod = buttonIndex + 1; // Az FM  mód indexe 0, azt kihagyjuk

            // Újra beállítjuk a sávot az új móddal (false -> ne a preferáltat töltse)
            pSi4735Manager->bandSet(false);

            // A demod mód változása után frissítjük a BFO és Step gombok állapotát
            // (fontos, mert SSB/CW módban mindkét gomb állapota más)
            updateBFOButtonState();
            updateStepButtonState();

        },
        true,                                         // Automatikusan bezárja-e a dialógust gomb kattintáskor
        pSi4735Manager->getCurrentBandDemodModDesc(), // Az alapértelmezett (jelenlegi) gomb felirata
        true,                                         // Ha true, az alapértelmezett gomb le van tiltva; ha false, csak vizuálisan kiemelve
        Rect(-1, -1, 320, 130)                        // Dialógus mérete (ha -1, akkor automatikusan a képernyő közepére igazítja)
    );

    this->showDialog(demodDialog);
}

/**
 * @brief Step gomb eseménykezelő - Frequency Step
 * @param event Gomb esemény (Clicked)
 * @details AM specifikus funkcionalitás - alapértelmezett implementáció
 */
void AMScreen::handleStepButton(const UIButton::ButtonEvent &event) {
    if (event.state != UIButton::EventButtonState::Clicked) {
        return;
    }

    // Aktuális demodulációs mód
    uint8_t currMod = pSi4735Manager->getCurrentBand().currDemod;

    // Az aktuális freki lépés felirata
    const char *currentStepStr = pSi4735Manager->currentStepSizeStr();

    // Megállapítjuk a lehetséges lépések méretét
    const char *title;
    size_t labelsCount;
    const char **labels;
    uint16_t w = 290;
    uint16_t h = 130;

    if (rtv::bfoOn) {
        title = "Step tune BFO";
        labels = pSi4735Manager->getStepSizeLabels(Band::stepSizeBFO, labelsCount);

    } else if (currMod == FM_DEMOD_TYPE) {
        title = "Step tune FM";
        labels = pSi4735Manager->getStepSizeLabels(Band::stepSizeFM, labelsCount);
        w = 300;
        h = 100;
    } else {
        title = "Step tune AM/SSB";
        labels = pSi4735Manager->getStepSizeLabels(Band::stepSizeAM, labelsCount);
        w = 290;
        h = 130;
    }

    auto stepDialog = std::make_shared<MultiButtonDialog>(
        this, this->tft,                                                                       // Képernyő referencia
        title, "",                                                                             // Dialógus címe és üzenete
        labels, labelsCount,                                                                   // Gombok feliratai és számuk
        [this, currMod](int buttonIndex, const char *buttonLabel, MultiButtonDialog *dialog) { // Gomb kattintás kezelése
            // Kikeressük az aktuális Band rekordot
            BandTable &currentband = pSi4735Manager->getCurrentBand();

            // Kikeressük az aktuális Band típust
            uint8_t currentBandType = currentband.bandType;

            // SSB módban a BFO be van kapcsolva?
            if (rtv::bfoOn && pSi4735Manager->isCurrentDemodSSBorCW()) {

                // BFO step állítás - a buttonIndex közvetlenül használható
                rtv::currentBFOStep = pSi4735Manager->getStepSizeByIndex(Band::stepSizeBFO, buttonIndex);

            } else { // Nem SSB + BFO módban vagyunk

                // Beállítjuk a konfigban a stepSize-t - a buttonIndex közvetlenül használható
                if (currMod == FM_DEMOD_TYPE) {
                    // FM módban
                    config.data.ssIdxFM = buttonIndex;
                    currentband.currStep = pSi4735Manager->getStepSizeByIndex(Band::stepSizeFM, buttonIndex);

                } else {
                    // AM módban
                    if (currentBandType == MW_BAND_TYPE or currentBandType == LW_BAND_TYPE) {
                        // MW vagy LW módban
                        config.data.ssIdxMW = buttonIndex;
                    } else {
                        // Sima AM vagy SW módban
                        config.data.ssIdxAM = buttonIndex;
                    }
                }
                currentband.currStep = pSi4735Manager->getStepSizeByIndex(Band::stepSizeAM, buttonIndex);
            }
        },
        true,              // Automatikusan bezárja-e a dialógust gomb kattintáskor
        currentStepStr,    // Az alapértelmezett (jelenlegi) gomb felirata
        true,              // Ha true, az alapértelmezett gomb le van tiltva; ha false, csak vizuálisan kiemelve
        Rect(-1, -1, w, h) // Dialógus mérete (ha -1, akkor automatikusan a képernyő közepére igazítja)
    );

    this->showDialog(stepDialog);
}

/**
 * @brief Frissíti a FreqDisplay szélességét az aktuális band típus alapján
 * @details Dinamikusan állítja be a frekvencia kijelző szélességét
 */
void AMScreen::updateFreqDisplayWidth() {
    if (!freqDisplayComp) {
        return; // Biztonsági ellenőrzés
    }
    auto bandType = pSi4735Manager->getCurrentBandType();
    uint16_t newWidth;

    switch (bandType) {
        case MW_BAND_TYPE:
        case LW_BAND_TYPE:
            newWidth = FreqDisplay::AM_BAND_WIDTH;
            break;
        case FM_BAND_TYPE:
            newWidth = FreqDisplay::FM_BAND_WIDTH;
            break;
        case SW_BAND_TYPE:
            newWidth = FreqDisplay::SW_BAND_WIDTH;
            break;
        default:
            newWidth = FreqDisplay::FREQDISPLAY_WIDTH - 25; // Alapértelmezett
            break;
    }

    freqDisplayComp->setWidth(newWidth);
}

// ===================================================================
// Mini Audio Display komponens létrehozása
// ===================================================================

/**
 * @brief Mini Audio Display komponens létrehozása
 * @details A config alapján létrehozza a megfelelő típusú mini audio display-t
 */
void AMScreen::createMiniAudioDisplay() {
    extern Config config;

    if (!config.data.audioEnabled) {
        return; // Audio kikapcsolva
    }

    // Pozíció számítása - az S-Meter jobb oldalán
    Rect audioDisplayBounds(SMeterConstants::SMETER_WIDTH + 10, 110, 180, 60); // x, y, width, height

    // Audio display típus a config alapján
    MiniAudioDisplayType displayType = static_cast<MiniAudioDisplayType>(config.data.audioModeAM);

    switch (displayType) {
        case MiniAudioDisplayType::SPECTRUM_BARS:
        case MiniAudioDisplayType::SPECTRUM_LINE:
            miniAudioDisplay = std::make_shared<MiniSpectrumAnalyzer>(
                tft, audioDisplayBounds, (displayType == MiniAudioDisplayType::SPECTRUM_BARS) ? MiniSpectrumAnalyzer::DisplayMode::BARS : MiniSpectrumAnalyzer::DisplayMode::LINE);
            // AM mód frekvencia tartomány beállítása (kisebb sávszélesség)
            static_cast<MiniSpectrumAnalyzer *>(miniAudioDisplay.get())->setFrequencyRange(300.0f, 6000.0f);
            break;

        case MiniAudioDisplayType::VU_METER:
            miniAudioDisplay = std::make_shared<MiniVuMeter>(tft, audioDisplayBounds, MiniVuMeter::Style::HORIZONTAL_BAR);
            break;

        default:
            // Alapértelmezett spektrum analizátor
            miniAudioDisplay = std::make_shared<MiniSpectrumAnalyzer>(tft, audioDisplayBounds);
            static_cast<MiniSpectrumAnalyzer *>(miniAudioDisplay.get())->setFrequencyRange(300.0f, 6000.0f);
            break;
    }

    if (miniAudioDisplay) {
        addChild(miniAudioDisplay);

        // Touch callback beállítása a mód váltáshoz
        miniAudioDisplay->setModeChangeCallback([this]() { cycleThroughAudioModes(); });

        DEBUG("AMScreen: Mini audio display created (type: %d)\n", static_cast<int>(displayType));
    }
}

/**
 * @brief Audio display módok közötti váltás
 */
void AMScreen::cycleThroughAudioModes() {
    extern Config config;

    // Következő mód kiszámítása
    int currentMode = config.data.audioModeAM;
    int nextMode = currentMode + 1;

    // Ha elértük a végét, visszatérünk az elejére (NONE után SPECTRUM_BARS)
    if (nextMode > static_cast<int>(MiniAudioDisplayType::VU_METER)) {
        nextMode = static_cast<int>(MiniAudioDisplayType::SPECTRUM_BARS);
    }

    // Config frissítése
    config.data.audioModeAM = nextMode;

    // Mód név meghatározása
    String modeName = getAudioModeDisplayName(static_cast<MiniAudioDisplayType>(nextMode));

    // Régi display eltávolítása
    if (miniAudioDisplay) {
        removeChild(miniAudioDisplay);
        miniAudioDisplay.reset();
    }

    // Új display létrehozása
    createMiniAudioDisplay();

    // Mód kijelzés megjelenítése
    if (miniAudioDisplay) {
        miniAudioDisplay->showModeDisplay(modeName);
    }

    DEBUG("AMScreen: Audio mode changed to: %s (%d)\n", modeName.c_str(), nextMode);
}

/**
 * @brief Audio mód megjelenítendő nevének lekérése
 */
String AMScreen::getAudioModeDisplayName(MiniAudioDisplayType mode) {
    switch (mode) {
        case MiniAudioDisplayType::SPECTRUM_BARS:
            return "Spectrum Bars";
        case MiniAudioDisplayType::SPECTRUM_LINE:
            return "Spectrum Line";
        case MiniAudioDisplayType::VU_METER:
            return "VU Meter";
        case MiniAudioDisplayType::WATERFALL:
            return "Waterfall";
        case MiniAudioDisplayType::OSCILLOSCOPE:
            return "Oscilloscope";
        default:
            return "Audio Display";
    }
}
