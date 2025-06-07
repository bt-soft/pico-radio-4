#ifndef __TEST_SCREEN_H
#define __TEST_SCREEN_H

#include "ScreenButtonsManager.h"
#include "UIScreen.h"

/**
 * @file TestScreen.h
 * @brief Test képernyő osztály, amely használja a ScreenButtonsManager-t
 */
class TestScreen : public UIScreen, public ScreenButtonsManager<TestScreen> {
  public:
    /**
     * @brief TestScreen konstruktor
     * @param tft TFT display referencia
     */
    TestScreen(TFT_eSPI &tft) : UIScreen(tft, SCREEN_NAME_TEST) { layoutComponents(); }
    virtual ~TestScreen() = default;

    /**
     * @brief Rotary encoder eseménykezelés felülírása
     * @param event Rotary encoder esemény
     * @return true ha kezelte az eseményt, false egyébként
     */
    virtual bool handleRotary(const RotaryEvent &event) override {
        DEBUG("TestScreen handleRotary: direction=%d, button=%d\n", (int)event.direction, (int)event.buttonState);

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
        tft.drawString("TestScreen for debugging", tft.width() / 2, tft.height() / 2 + 20);
    }

  private:
    /**
     * @brief UI komponensek létrehozása és elhelyezése
     */
    void layoutComponents() {
        std::vector<ButtonDefinition> bottomButtonDefs = {
            {1, "AM", UIButton::ButtonType::Pushable,
             [this](const UIButton::ButtonEvent &event) {
                 if (event.state == UIButton::EventButtonState::Clicked) {
                     DEBUG("TestScreen: Switching to AM screen\n");
                     getManager()->switchToScreen(SCREEN_NAME_AM); // UIScreen::getManager()
                 }
             },
             UIButton::ButtonState::Disabled},
            {2, "FM", UIButton::ButtonType::Pushable,
             [this](const UIButton::ButtonEvent &event) {
                 if (event.state == UIButton::EventButtonState::Clicked) {
                     DEBUG("TestScreen: Switching to FM screen\n");
                     getManager()->switchToScreen(SCREEN_NAME_FM); // UIScreen::getManager()
                 }
             }}
            // ...további gombok...
        };

        std::vector<std::shared_ptr<UIButton>> createdBottomButtons;
        // A layoutHorizontalButtonGroup a ScreenButtonsManager-ből öröklődik
        // és a TestScreen (ami UIScreen is) tft-jét és addChild-ját használja.
        layoutHorizontalButtonGroup(bottomButtonDefs, &createdBottomButtons);
    }
};

#endif // __TEST_SCREEN_H