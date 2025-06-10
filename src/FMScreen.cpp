#include "FMScreen.h"
#include "FreqDisplay.h" // Új include

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
    uint16_t freqDisplayY = 60; // Példa Y pozíció (a gombok felett)
    Rect freqBounds(freqDisplayX, freqDisplayY, freqDisplayWidth, freqDisplayHeight);
    freqDisplayComp = std::make_shared<FreqDisplay>(tft, freqBounds, si4735Manager);
    addChild(freqDisplayComp);
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
    // Ezt a logikát a DisplayBase::loop() vagy az FmDisplay::displayLoop() már kezeli
    // a DisplayBase::frequencyChanged flag alapján.
    // Itt egy példa, ha közvetlenül a Band objektumot figyeljük:
    static uint16_t lastDisplayedFreq = 0;
    uint16_t currentRadioFreq = si4735Manager.getCurrentBand().currFreq; // Vagy si4735.getFrequency()

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
            freqDisplayComp->markForRedraw(); // Újrarajzolás kérése a BFO állapot változása miatt
        }
        lastBfoOnState = rtv::bfoOn;
    }

    // TODO: Figyelni kell az rtv::freqstepnr változását is, és ha változik,
    // akkor freqDisplayComp->markForRedraw() hívása.
}

/**
 * @brief Kirajzolja a képernyő saját tartalmát
 */
void FMScreen::drawContent() {
    // Szöveg középre igazítása
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_COLOR_BACKGROUND);
    tft.setFreeFont();
    tft.setTextSize(3);

    // Képernyő cím kirajzolása
    tft.drawString(SCREEN_NAME_FM, tft.width() / 2, tft.height() / 2 - 20);

    // Információs szöveg
    tft.setTextSize(1);
    tft.drawString("FM Radio Control functions and debugging", tft.width() / 2, tft.height() / 2 + 20);
}