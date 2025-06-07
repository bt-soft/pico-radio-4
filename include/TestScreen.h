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

        // Előre definiált feliratok a vízszintes gombokhoz
        const char *horizontalLabels[] = {"HBtn1", "HBtn2",  "HBtn3",  "HBtn4",  "HBtn5",  "HBtn6",  "HBtn7", "HBtn8",
                                          "HBtn9", "HBtn10", "HBtn11", "HBtn12", "HBtn13", "HBtn14", "HBtn15"};
        const size_t numHorizontalButtons = sizeof(horizontalLabels) / sizeof(horizontalLabels[0]);

        // Vízszintes gombok tesztelése (alsó sor)
        std::vector<ButtonDefinition> horizontalButtonDefs;
        for (size_t i = 0; i < numHorizontalButtons; ++i) {
            horizontalButtonDefs.push_back({static_cast<uint8_t>(i + 1), // ID
                                            horizontalLabels[i],         // Felirat a tömbből
                                            UIButton::ButtonType::Pushable,
                                            [this, id = i + 1](const UIButton::ButtonEvent &event) {
                                                if (event.state == UIButton::EventButtonState::Clicked) {
                                                    DEBUG("TestScreen: Horizontal Button %d clicked\n", id);
                                                }
                                            },
                                            UIButton::ButtonState::Off});
        }

        std::vector<std::shared_ptr<UIButton>> createdHorizontalButtons;
        // A layoutHorizontalButtonGroup a ScreenButtonsManager-ből öröklődik és a TestScreen (ami UIScreen is) tft-jét és addChild-ját használja.
        layoutHorizontalButtonGroup(horizontalButtonDefs, &createdHorizontalButtons);

        // Előre definiált feliratok a függőleges gombokhoz
        const char *verticalLabels[] = {"VBtn1", "VBtn2", "VBtn3", "VBtn4", "VBtn5", "VBtn6", "VBtn7", "VBtn8", "VBtn9", "VBtn10", "VBtn11", "VBtn12"};
        const size_t numVerticalButtons = sizeof(verticalLabels) / sizeof(verticalLabels[0]);

        // Függőleges gombok tesztelése (jobb oldali oszlop)
        std::vector<ButtonDefinition> verticalButtonDefs;
        for (size_t i = 0; i < numVerticalButtons; ++i) {
            verticalButtonDefs.push_back({
                static_cast<uint8_t>(100 + i + 1), // Eltérő ID tartomány
                verticalLabels[i],                 // Felirat a tömbből
                UIButton::ButtonType::Toggleable,
                [this, id = 100 + i + 1](const UIButton::ButtonEvent &event) {
                    if (event.state == UIButton::EventButtonState::On || event.state == UIButton::EventButtonState::Off) {
                        DEBUG("TestScreen: Vertical Button %d toggled to %s\n", id, event.state == UIButton::EventButtonState::On ? "ON" : "OFF");
                    }
                },
                (i % 2 == 0) ? UIButton::ButtonState::Off : UIButton::ButtonState::On, // Váltakozó kezdeti állapot
            });
        }

        std::vector<std::shared_ptr<UIButton>> createdVerticalButtons;
        layoutVerticalButtonGroup(verticalButtonDefs, &createdVerticalButtons, 5, 5,
                                  5 + (UIButton::DEFAULT_BUTTON_HEIGHT + 3) * 1); // Alsó margó növelése, hogy ne ütközzön a vízszintes gombokkal

        // std::vector<ButtonDefinition> bottomButtonDefs = {
        //     {1, "AM", UIButton::ButtonType::Pushable,
        //      [this](const UIButton::ButtonEvent &event) {
        //          if (event.state == UIButton::EventButtonState::Clicked) {
        //              DEBUG("TestScreen: Switching to AM screen\n");
        //              getManager()->switchToScreen(SCREEN_NAME_AM); // UIScreen::getManager()
        //          }
        //      },
        //      UIButton::ButtonState::Disabled},
        //     {2, "FM", UIButton::ButtonType::Pushable,
        //      [this](const UIButton::ButtonEvent &event) {
        //          if (event.state == UIButton::EventButtonState::Clicked) {
        //              DEBUG("TestScreen: Switching to FM screen\n");
        //              getManager()->switchToScreen(SCREEN_NAME_FM); // UIScreen::getManager()
        //          }
        //      }}
        //     // ...további gombok...
        // };

        // std::vector<std::shared_ptr<UIButton>> createdBottomButtons;
        // // A layoutHorizontalButtonGroup a ScreenButtonsManager-ből öröklődik és a TestScreen (ami UIScreen is) tft-jét és addChild-ját használja.
        // layoutHorizontalButtonGroup(bottomButtonDefs, &createdBottomButtons);
    }
};

#endif // __TEST_SCREEN_H