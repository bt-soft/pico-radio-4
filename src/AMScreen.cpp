#include "AMScreen.h"
#include "Band.h"
#include "CommonVerticalButtons.h"
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
    uint16_t currentFrequency = pSi4735Manager->getSi4735().getFrequency();

    if (pSi4735Manager->isCurrentDemodSSB()) {

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
        newFreq = pSi4735Manager->getSi4735().getFrequency();

        // SSB hangolás esetén a BFO eltolás beállítása
        const int16_t cwBaseOffset = (currentBand.currMod == CW) ? config.data.cwReceiverOffsetHz : 0;
        int16_t bfoToSet = cwBaseOffset + rtv::currentBFO + rtv::currentBFOmanu;
        pSi4735Manager->getSi4735().setSSBBfo(bfoToSet);

    } else {
        // Léptetjük a rádiót, ez el is menti a band táblába
        newFreq = pSi4735Manager->stepFrequency(event.value);
    }

    // AGC
    pSi4735Manager->checkAGC();

    // Frekvencia kijelző azonnali frissítése
    if (freqDisplayComp) {
        freqDisplayComp->setFrequency(newFreq);
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
    // *** OPTIMALIZÁLT ARCHITEKTÚRA - NINCS GOMBÁLLAPOT POLLING! ***
    // ===================================================================

    // ===================================================================
    // S-Meter (jelerősség) időzített frissítése - Közös RadioScreen implementáció
    // ===================================================================
    updateSMeter(false /* AM mód */);
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

    // Szülő osztály aktiválása (RadioScreen -> UIScreen)
    // *** Ez automatikusan meghívja a signal cache invalidálást ***
    RadioScreen::activate();

    // ===================================================================
    // *** EGYETLEN GOMBÁLLAPOT SZINKRONIZÁLÁSI PONT - Event-driven ***
    // ===================================================================
    updateAllVerticalButtonStates(pSi4735Manager); // Univerzális funkcionális gombok (mixin method)
    updateCommonHorizontalButtonStates();          // Közös gombok szinkronizálása
    updateHorizontalButtonStates();                // AM-specifikus gombok szinkronizálása
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

    // Először hívjuk az alap implementációt (stack cleanup, navigation logic)
    UIScreen::onDialogClosed(closedDialog);

    // Ha ez volt az utolsó dialógus, frissítsük a gombállapotokat
    if (!isDialogActive()) {
        updateAllVerticalButtonStates(pSi4735Manager); // Függőleges gombok szinkronizálása
        updateCommonHorizontalButtonStates();          // Közös gombok szinkronizálása
        updateHorizontalButtonStates();                // AM specifikus gombok szinkronizálása

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
    Rect freqBounds(30, FreqDisplayY, 200, FreqDisplay::FREQDISPLAY_HEIGHT);
    UIScreen::createFreqDisplay(freqBounds);
    freqDisplayComp->setHideUnderline(true); // Alulvonás elrejtése a frekvencia kijelzőn

    // ===================================================================
    // S-Meter komponens létrehozása - RadioScreen közös implementáció
    // ===================================================================
    uint16_t smeterWidth = UIComponent::SCREEN_W - 90; // 90px helyet hagyunk a jobb oldalon
    Rect smeterBounds(2, FreqDisplayY + FreqDisplay::FREQDISPLAY_HEIGHT + 20, smeterWidth, 60);
    createSMeterComponent(smeterBounds);

    createCommonVerticalButtons(pSi4735Manager); // ButtonsGroupManager használata
    createCommonHorizontalButtons();             // Alsó közös + AM specifikus vízszintes gombsor
}

/**
 * @brief AM specifikus gombok hozzáadása a közös gombokhoz
 * @param buttonConfigs A már meglévő gomb konfigurációk vektora
 * @details Felülírja az ős metódusát, hogy hozzáadja az AM specifikus gombokat
 */
void AMScreen::addSpecificHorizontalButtons(std::vector<UIHorizontalButtonBar::ButtonConfig> &buttonConfigs) {
    // AM specifikus gombok hozzáadása a közös gombok után

    // 1. BFO - Beat Frequency Oscillator
    buttonConfigs.push_back({AMScreenHorizontalButtonIDs::BFO_BUTTON, "BFO", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
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

    if (pSi4735Manager->isCurrentDemodSSB()) {
        // SSB/CW módban: csak akkor engedélyezett, ha BFO be van kapcsolva
        stepButtonState = rtv::bfoOn ? UIButton::ButtonState::Off : UIButton::ButtonState::Disabled;
        DEBUG("AMScreen::updateStepButtonState - SSB/CW mode detected, BFO: %s, Step button: %s\n", rtv::bfoOn ? "ON" : "OFF",
              stepButtonState == UIButton::ButtonState::Disabled ? "DISABLED" : "ENABLED");
    } else {
        // AM/egyéb módban: mindig engedélyezett
        stepButtonState = UIButton::ButtonState::Off;
        DEBUG("AMScreen::updateStepButtonState - Non-SSB mode, Step button: ENABLED\n");
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

    if (pSi4735Manager->isCurrentDemodSSB()) {
        // SSB/CW módban: BFO állapot szerint be/ki kapcsolva
        bfoButtonState = rtv::bfoOn ? UIButton::ButtonState::On : UIButton::ButtonState::Off;
        DEBUG("AMScreen::updateBFOButtonState - SSB/CW mode, BFO button: %s\n", rtv::bfoOn ? "ON" : "OFF");
    } else {
        // AM/egyéb módban: letiltva
        bfoButtonState = UIButton::ButtonState::Disabled;
        DEBUG("AMScreen::updateBFOButtonState - Non-SSB mode, BFO button: DISABLED\n");
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
    if (event.state == UIButton::EventButtonState::Clicked) {

        // Csak SSB/CW módban működik
        if (!pSi4735Manager->isCurrentDemodSSB()) {
            return;
        }

        // BFO állapot váltása
        rtv::bfoOn = !rtv::bfoOn;

        // BFO és Step gombok állapotának frissítése
        updateBFOButtonState();
        updateStepButtonState();

        DEBUG("AMScreen::handleBFOButton - BFO turned %s\n", rtv::bfoOn ? "ON" : "OFF");

        // TODO: További BFO funkcionalitás implementálása
        Serial.printf("AMScreen::handleBFOButton - BFO %s\n", rtv::bfoOn ? "ON" : "OFF");
    }
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
    uint8_t currMod = pSi4735Manager->getCurrentBand().currMod;
    // Jelenlegi sávszélesség felirata
    const char *currentBw = pSi4735Manager->getCurrentBandWidthLabel();

    // Megállapítjuk a lehetséges sávszélességek tömbjét
    const char *title;
    size_t labelsCount;
    const char **labels;
    uint16_t w = 250;
    uint16_t h = 170;

    if (currMod == FM) {
        title = "FM Filter in kHz";
        labels = pSi4735Manager->getBandWidthLabels(Band::bandWidthFM, labelsCount);

    } else if (currMod == AM) {
        title = "AM Filter in kHz";
        w = 350;
        h = 160;

        labels = pSi4735Manager->getBandWidthLabels(Band::bandWidthAM, labelsCount);

    } else {
        title = "SSB/CW Filter in kHz";
        w = 300;
        h = 150;

        labels = pSi4735Manager->getBandWidthLabels(Band::bandWidthSSB, labelsCount);
    }

    auto afBwDialog = std::make_shared<MultiButtonDialog>(
        this, this->tft,                                                                       // Képernyő referencia
        title, "",                                                                             // Dialógus címe és üzenete
        labels, labelsCount,                                                                   // Gombok feliratai és számuk
        [this, currMod](int buttonIndex, const char *buttonLabel, MultiButtonDialog *dialog) { // Gomb kattintás kezelése
            //

            if (currMod == AM) {
                config.data.bwIdxAM = pSi4735Manager->getBandWidthIndexByLabel(Band::bandWidthAM, buttonLabel);
            } else if (currMod == FM) {
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
 * @details AM specifikus funkcionalitás - alapértelmezett implementáció
 */
void AMScreen::handleAntCapButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        // TODO: AntCap funkcionalitás implementálása
        Serial.println("AMScreen::handleAntCapButton - AntCap funkció (TODO)");
    }
}

/**
 * @brief Demod gomb eseménykezelő - Demodulation
 * @param event Gomb esemény (Clicked)
 * @details AM specifikus funkcionalitás - alapértelmezett implementáció
 */
void AMScreen::handleDemodButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        // TODO: Demod funkcionalitás implementálása
        Serial.println("AMScreen::handleDemodButton - Demod funkció (TODO)");

        // A demod mód változása után frissítjük a BFO és Step gombok állapotát
        // (fontos, mert SSB/CW módban mindkét gomb állapota más)
        updateBFOButtonState();
        updateStepButtonState();
    }
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
    uint8_t currMod = pSi4735Manager->getCurrentBand().currMod;

    // Az aktuális freki lépés felirata
    const char *currentStepStr = pSi4735Manager->currentStepSizeStr();

    // Megállapítjuk a lehetséges lépések méretét
    const char *title;
    size_t labelsCount;
    const char **labels;
    uint16_t w = 200;
    uint16_t h = 130;

    if (rtv::bfoOn) {
        title = "Step tune BFO";
        labels = pSi4735Manager->getStepSizeLabels(Band::stepSizeBFO, labelsCount);

    } else if (currMod == FM) {
        title = "Step tune FM";
        labels = pSi4735Manager->getStepSizeLabels(Band::stepSizeFM, labelsCount);
        w = 300;
        h = 100;
    } else {
        title = "Step tune AM/SSB";
        labels = pSi4735Manager->getStepSizeLabels(Band::stepSizeAM, labelsCount);
        w = 300;
        h = 120;
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
            if (rtv::bfoOn && pSi4735Manager->isCurrentDemodSSB()) {

                // BFO step állítás
                rtv::currentBFOStep = pSi4735Manager->getStepSizeByIndex(Band::stepSizeBFO, buttonIndex);

                // SSB módban beállítjuk a BFO lépésközt
                pSi4735Manager->setBFOStep();

            } else { // Nem SSB + BFO módban vagyunk

                // Beállítjuk a konfigban a stepSize-t
                if (currMod == FM) {
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
