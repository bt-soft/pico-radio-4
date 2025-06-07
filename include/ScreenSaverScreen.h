#ifndef __SCREEN_SAVER_SCREEN_H
#define __SCREEN_SAVER_SCREEN_H

#include "IScreenManager.h"
#include "UIScreen.h"
#include "defines.h" // SCREEN_NAME_SCREENSAVER miatt

/**
 * @file ScreenSaverScreen.h
 * @brief Képernyővédő osztály
 */
class ScreenSaverScreen : public UIScreen {
  private:
    uint32_t activationTime;

  public:
    /**
     * @brief ScreenSaverScreen konstruktor
     * @param tft TFT display referencia
     */
    ScreenSaverScreen(TFT_eSPI &tft) : UIScreen(tft, SCREEN_NAME_SCREENSAVER) { activationTime = millis(); }

    virtual ~ScreenSaverScreen() = default;

    virtual void activate() override {
        DEBUG("ScreenSaverScreen activated.\n");
        activationTime = millis();
        tft.fillScreen(TFT_BLACK); // Tiszta lappal indulunk
        markForRedraw();
    }

    virtual void deactivate() override { DEBUG("ScreenSaverScreen deactivated.\n"); }

    /**
     * @brief Kirajzolja a képernyővédő tartalmát
     */
    virtual void drawSelf() override {
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextSize(2);
        tft.drawString("Screen Saver Active", tft.width() / 2, tft.height() / 2 - 10);

        // Egyszerű animáció: másodpercenként váltakozó pontok
        uint32_t secondsActive = (millis() - activationTime) / 1000;
        String dots = ".";
        for (uint8_t i = 0; i < (secondsActive % 3); ++i) {
            dots += ".";
        }
        tft.setTextSize(1);
        tft.drawString(dots, tft.width() / 2, tft.height() / 2 + 20);
        markForRedraw(); // Folyamatos újrarajzolás az animációhoz
    }

    virtual bool handleTouch(const TouchEvent &event) override {
        DEBUG("ScreenSaverScreen: Touch event, waking up.\n");
        if (getManager()) {
            getManager()->goBack(); // Vissza az előző képernyőre
        }
        return true; // Esemény kezelve
    }

    virtual bool handleRotary(const RotaryEvent &event) override {
        DEBUG("ScreenSaverScreen: Rotary event, waking up.\n");
        if (getManager()) {
            getManager()->goBack(); // Vissza az előző képernyőre
        }
        return true; // Esemény kezelve
    }
};

#endif // __SCREEN_SAVER_SCREEN_H