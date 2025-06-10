#include "FMScreen.h"
#include "FreqDisplay.h" // Új include
#include "SMeter.h"
#include "UIColorPalette.h" // TFT_COLOR_BACKGROUND makróhoz

FMScreen::FMScreen(TFT_eSPI &tft, Si4735Manager &si4735Manager) : UIScreen(tft, SCREEN_NAME_FM), si4735Manager(si4735Manager) {

    si4735Manager.init(); // Si4735 inicializálása

    layoutComponents();
}

/**
 * @brief UI komponensek létrehozása és elhelyezése
 */
void FMScreen::layoutComponents() {
    const int16_t screenHeight = tft.height();
    const int16_t buttonHeight = UIButton::DEFAULT_BUTTON_HEIGHT;
    const int16_t gap = 3;
    const int16_t margin = 5;
    const int16_t buttonWidth = UIButton::DEFAULT_BUTTON_WIDTH;
    const int16_t buttonY = screenHeight - buttonHeight - margin;

    int16_t currentX = margin;

    // AM gomb létrehozása
    std::shared_ptr<UIButton> amButton = std::make_shared<UIButton>( //
        tft,
        1,                                                  // ID
        Rect(currentX, buttonY, buttonWidth, buttonHeight), // rect
        "AM",                                               // label
        UIButton::ButtonType::Pushable,                     // type
        UIButton::ButtonState::Disabled,                    // initial state
        [this](const UIButton::ButtonEvent &event) {        // callback
            if (event.state == UIButton::EventButtonState::Clicked) {
                DEBUG("FMScreen: Switching to AM screen\n");
                // Képernyőváltás AM-re
                UIScreen::getManager()->switchToScreen(SCREEN_NAME_AM);
            }
        });
    addChild(amButton);

    currentX += buttonWidth + gap;

    // Test gomb létrehozása
    std::shared_ptr<UIButton> testButton = std::make_shared<UIButton>( //
        tft,
        2,                                                  // Id
        Rect(currentX, buttonY, buttonWidth, buttonHeight), // rect
        "Test",                                             // label
        UIButton::ButtonType::Pushable,                     // type
        [this](const UIButton::ButtonEvent &event) {        // callback
            if (event.state == UIButton::EventButtonState::Clicked) {
                DEBUG("FMScreen: Switching to Test screen\n");
                // Képernyőváltás Test-re
                UIScreen::getManager()->switchToScreen(SCREEN_NAME_TEST);
            }
        });
    addChild(testButton);

    currentX += buttonWidth + gap;

    // Test gomb létrehozása
    std::shared_ptr<UIButton> setupButton = std::make_shared<UIButton>( //
        tft,
        2,                                                  // Id
        Rect(currentX, buttonY, buttonWidth, buttonHeight), // rect
        "Setup",                                            // label
        UIButton::ButtonType::Pushable,                     // type
        [this](const UIButton::ButtonEvent &event) {        // callback
            if (event.state == UIButton::EventButtonState::Clicked) {
                DEBUG("FMScreen: Switching to Setup screen\n");
                // Képernyőváltás Setup-re
                UIScreen::getManager()->switchToScreen(SCREEN_NAME_SETUP);
            }
        });
    addChild(setupButton);

    // FreqDisplay komponens
    uint16_t freqDisplayHeight = 38 + 20 + 10; // Szegmens + unit + aláhúzás + margók (becslés)
    uint16_t freqDisplayWidth = 240;           // Becsült szélesség
    uint16_t freqDisplayX = (tft.width() - freqDisplayWidth) / 2;
    uint16_t freqDisplayY = 60;
    Rect freqBounds(freqDisplayX, freqDisplayY, freqDisplayWidth, freqDisplayHeight);
    freqDisplayComp = std::make_shared<FreqDisplay>(tft, freqBounds, si4735Manager);
    addChild(freqDisplayComp);

    // S-Meter komponens
    uint16_t smeterX = 2;                                     // Kis margó balról
    uint16_t smeterY = freqDisplayY + freqDisplayHeight + 10; // FreqDisplay alatt 10px réssel
    uint16_t smeterWidth = 240;                               // S-Meter szélessége
    uint16_t smeterHeight = 60;                               // S-Meter magassága (becsült)
    Rect smeterBounds(smeterX, smeterY, smeterWidth, smeterHeight);
    ColorScheme smeterColors = ColorScheme::defaultScheme();
    smeterColors.background = TFT_COLOR_BACKGROUND; // Fekete háttér
    smeterComp = std::make_shared<SMeter>(tft, smeterBounds, smeterColors);
    addChild(smeterComp);
}

/**
 * @brief Rotary encoder eseménykezelés felülírása
 * @param event Rotary encoder esemény
 * @return true ha kezelte az eseményt, false egyébként
 */
bool FMScreen::handleRotary(const RotaryEvent &event) {

    // Csak ha nincs aktív dialóg és nem egy rotary klikk hangzott el
    if (!isDialogActive() && event.buttonState != RotaryEvent::ButtonState::Clicked) {

        // Léptetjük (és el is mentjük a bandtable-ba) a frekvenciát a rotary értéke alapján
        si4735Manager.stepFrequency(event.value);

        return true;
    }

    // Ha nem kezeltük az eseményt, akkor továbbítjuk a UIScreen-nek, ami továbbítja a lehetséges további a gyerek komponenseknek
    return UIScreen::handleRotary(event);
}

/**
 * @brief Loop hívás felülírása
 * animációs vagy egyéb saját logika végrehajtására
 * @note Ez a metódus nem hívja meg a gyerek komponensek loop-ját, csak saját logikát tartalmaz.
 */
void FMScreen::handleOwnLoop() {
    // Frekvencia frissítése, ha változott
    static uint16_t lastDisplayedFreq = 0;
    uint16_t currentRadioFreq = si4735Manager.getCurrentBand().currFreq;

    if (currentRadioFreq != lastDisplayedFreq) {
        if (freqDisplayComp) {
            freqDisplayComp->setFrequency(currentRadioFreq);
        }
        lastDisplayedFreq = currentRadioFreq;
    }

    // BFO állapot változásának figyelése
    static bool lastBfoOnState = rtv::bfoOn;
    if (rtv::bfoOn != lastBfoOnState) {
        if (freqDisplayComp) {
            freqDisplayComp->markForRedraw();
        }
        lastBfoOnState = rtv::bfoOn;
    }

    // S-Meter frissítése
    if (smeterComp) {
        // Si4735-től lekérjük az RSSI és SNR értékeket
        uint8_t rssi = si4735Manager.getSi4735().getCurrentRSSI();
        uint8_t snr = si4735Manager.getSi4735().getCurrentSNR();
        bool isFMMode = (si4735Manager.getCurrentBandType() == FM_BAND_TYPE);

        smeterComp->showRSSI(rssi, snr, isFMMode);
    }
}

/**
 * @brief Kirajzolja a képernyő saját tartalmát
 */
void FMScreen::drawContent() {
    // S-Meter skála kirajzolása (statikus rész)
    if (smeterComp) {
        smeterComp->drawSmeterScale();
    }
}