#ifndef __EMPTY_SCREEN_H
#define __EMPTY_SCREEN_H

#include "UIButton.h"
#include "UIScreen.h"

/**
 * @file FMScreen.h
 * @brief FM képernyő osztály
 */
class EmptyScreen : public UIScreen {
  private:
    // Navigációs gombok
    std::shared_ptr<UIButton> amButton; ///< AM képernyőre váltás gomb
    std::shared_ptr<UIButton> fmButton; ///< FM képernyőre váltás gomb

  public:
    /**
     * @brief Konstruktor
     * @param tft TFT display referencia
     */
    EmptyScreen(TFT_eSPI &tft) : UIScreen(tft, SCREEN_NAME_TEST) { layoutComponents(); }
    virtual ~EmptyScreen() = default;

    /**
     * @brief Rotary encoder eseménykezelés felülírása
     * @param event Rotary encoder esemény
     * @return true ha kezelte az eseményt, false egyébként
     */
    virtual bool handleRotary(const RotaryEvent &event) override {
        DEBUG("EmptyScreen handleRotary: direction=%d, button=%d\n", (int)event.direction, (int)event.buttonState);

        // // Ha van aktív dialógus, akkor a szülő implementációnak adjuk át
        // if (isDialogActive()) {
        //     DEBUG("FMScreen: Dialog active, forwarding to UIScreen\n");
        //     return UIScreen::handleRotary(event);
        // }

        // Saját rotary logika itt
        if (event.direction == RotaryEvent::Direction::Up) {
            DEBUG("TestScreen: Rotary Up\n");
            return true;
        } else if (event.direction == RotaryEvent::Direction::Down) {
            DEBUG("TestScreen: Rotary Down\n");
            return true;
        }

        if (event.buttonState == RotaryEvent::ButtonState::Clicked) {
            DEBUG("TestScreen: Rotary Clicked\n");
            return true;
        }

        return UIScreen::handleRotary(event);
    }

    /**
     * @brief Loop hívás felülírása
     * animációs vagy egyéb saját logika végrehajtására
     * @note Ez a metódus nem hívja meg a gyerek komponensek loop-ját, csak saját logikát tartalmaz.
     */
    virtual void handleOwnLoop() override {}

    /**
     * @brief Kirajzolja a képernyő saját tartalmát
     */
    virtual void drawSelf() override {
        // Szöveg középre igazítása
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_COLOR_BACKGROUND);
        tft.setTextSize(3);

        // Képernyő cím kirajzolása
        tft.drawString(SCREEN_NAME_TEST, tft.width() / 2, tft.height() / 2 - 20);

        // Információs szöveg
        tft.setTextSize(1);
        tft.drawString("EmptyScreen  for debugging", tft.width() / 2, tft.height() / 2 + 20);
    }

  private:
    /**
     * @brief UI komponensek létrehozása és elhelyezése
     */
    void layoutComponents() {
        const int16_t screenHeight = tft.height();
        const int16_t buttonHeight = UIButton::DEFAULT_BUTTON_HEIGHT;
        const int16_t gap = 3;
        const int16_t margin = 5;
        const int16_t buttonWidth = UIButton::DEFAULT_BUTTON_WIDTH;
        const int16_t buttonY = screenHeight - buttonHeight - margin;

        int16_t currentX = margin;

        // AM gomb létrehozása
        amButton = std::make_shared<UIButton>(tft,
                                              1,                                                  // ID
                                              Rect(currentX, buttonY, buttonWidth, buttonHeight), // rect
                                              "AM",                                               // label
                                              UIButton::ButtonType::Pushable,                     // type
                                              UIButton::ButtonState::Disabled,                    // initial state
                                              [this](const UIButton::ButtonEvent &event) {        // callback
                                                  if (event.state == UIButton::EventButtonState::Clicked) {
                                                      DEBUG("EmptyScreen: Switching to AM screen\n");
                                                      // Képernyőváltás AM-re
                                                      UIScreen::getManager()->switchToScreen(SCREEN_NAME_AM);
                                                  }
                                              });
        addChild(amButton);

        currentX += buttonWidth + gap;

        // FM gomb létrehozása
        fmButton = std::make_shared<UIButton>(tft,
                                              2,                                                  // Id
                                              Rect(currentX, buttonY, buttonWidth, buttonHeight), // rect
                                              "FM",                                               // label
                                              UIButton::ButtonType::Pushable,                     // type
                                              [this](const UIButton::ButtonEvent &event) {        // callback
                                                  if (event.state == UIButton::EventButtonState::Clicked) {
                                                      DEBUG("EmptyScreen: Switching to FM screen\n");
                                                      // Képernyőváltás FM-re
                                                      UIScreen::getManager()->switchToScreen(SCREEN_NAME_FM);
                                                  }
                                              });
        addChild(fmButton);
    }
};

#endif // __EMPTY_SCREEN_H