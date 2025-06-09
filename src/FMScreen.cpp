#include "FMScreen.h"

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
}

/**
 * @brief Rotary encoder eseménykezelés felülírása
 * @param event Rotary encoder esemény
 * @return true ha kezelte az eseményt, false egyébként
 */
bool FMScreen::handleRotary(const RotaryEvent &event) {

    // Ha van aktív dialógus, akkor a szülő implementációnak adjuk át
    if (isDialogActive()) {
        DEBUG("FMScreen: Dialog active, forwarding to UIScreen\n");
        return UIScreen::handleRotary(event);
    }

    // Saját rotary logika itt
    if (event.direction == RotaryEvent::Direction::Up) {
        DEBUG("FMScreen: Rotary Up\n");
        return true;
    } else if (event.direction == RotaryEvent::Direction::Down) {
        DEBUG("FMScreen: Rotary Down\n");
        return true;
    }

    if (event.buttonState == RotaryEvent::ButtonState::Clicked) {
        DEBUG("FMScreen: Rotary Clicked\n");
        return true;
    }

    return UIScreen::handleRotary(event);
}

/**
 * @brief Loop hívás felülírása
 * animációs vagy egyéb saját logika végrehajtására
 * @note Ez a metódus nem hívja meg a gyerek komponensek loop-ját, csak saját logikát tartalmaz.
 */
void FMScreen::handleOwnLoop() {}

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