#ifndef __SCREEN_BUTTONS_MANAGER_H
#define __SCREEN_BUTTONS_MANAGER_H

#include <functional>
#include <memory>
#include <vector>

#include "UIButton.h"
#include "UIContainerComponent.h"
#include "UIScreen.h" // Szükséges lehet, ha a DerivedScreen mindig UIScreen leszármazott
#include "defines.h"  // A DEBUG makróhoz

// Struktúra a gombok definíciójához a csoportos elrendezéshez
// Ezt a struktúrát használják a layout metódusok.
struct ButtonDefinition {
    uint8_t id;                                                      // Gomb egyedi azonosítója
    const char *label;                                               // Gomb felirata
    UIButton::ButtonType type;                                       // Gomb típusa (Pushable, Toggleable)
    std::function<void(const UIButton::ButtonEvent &)> callback;     // Visszahívási függvény eseménykezeléshez
    UIButton::ButtonState initialState = UIButton::ButtonState::Off; // Kezdeti állapot (alapértelmezetten Off)
    uint16_t width = 0;                                              // Egyedi szélesség (0 = UIButton::DEFAULT_BUTTON_WIDTH)
    uint16_t height = 0;                                             // Egyedi magasság (0 = UIButton::DEFAULT_BUTTON_HEIGHT)
};

template <typename DerivedScreen> // Curiously Recurring Template Pattern (CRTP): A DerivedScreen lesz a konkrét képernyő osztály (pl. TestScreen)
class ScreenButtonsManager {
  protected:
    // A konstruktor és destruktor lehet default, mivel ez az osztály
    // elsősorban metódusokat biztosít, nem saját állapotot.
    ScreenButtonsManager() = default;
    virtual ~ScreenButtonsManager() = default;

    /**
     * @brief Gombokat rendez el függőlegesen, a képernyő jobb széléhez igazítva.
     * A metódus a leszármazott képernyő (ami UIScreen-ből is származik)
     * tft referenciáját és addChild metódusát használja.
     */
    void layoutVerticalButtonGroup(const std::vector<ButtonDefinition> &buttonDefs, std::vector<std::shared_ptr<UIButton>> *out_createdButtons = nullptr, int16_t marginRight = 5,
                                   int16_t marginTop = 5, int16_t marginBottom = 5, int16_t defaultButtonWidthRef = UIButton::DEFAULT_BUTTON_WIDTH,
                                   int16_t defaultButtonHeightRef = UIButton::DEFAULT_BUTTON_HEIGHT, int16_t columnGap = 3, int16_t buttonGap = 3) {

        DerivedScreen *self = static_cast<DerivedScreen *>(this);

        if (buttonDefs.empty()) {
            return;
        }

        // A 'self->tft' a DerivedScreen (pl. TestScreen) UIScreen ősosztályából
        // (pontosabban UIComponent-ből) örökölt 'tft' referenciája lesz.
        // Az 'self->addChild' pedig az UIContainerComponent public metódusa.
        const int16_t screenHeight = self->getTft().height();
        const int16_t screenWidth = self->getTft().width();
        const int16_t maxColumnHeight = screenHeight - marginTop - marginBottom;

        int16_t currentButtonWidth = defaultButtonWidthRef;

        if (!buttonDefs.empty() && buttonDefs[0].width > 0) {
            currentButtonWidth = buttonDefs[0].width;
        }

        int16_t currentX = screenWidth - marginRight - currentButtonWidth;
        int16_t currentY = marginTop;

        if (out_createdButtons) {
            out_createdButtons->clear();
        }

        for (const auto &def : buttonDefs) {
            int16_t btnWidth = (def.width > 0) ? def.width : defaultButtonWidthRef;
            int16_t btnHeight = (def.height > 0) ? def.height : defaultButtonHeightRef;

            if (currentY + btnHeight > maxColumnHeight && currentY != marginTop) {
                currentX -= (currentButtonWidth + columnGap);
                currentY = marginTop;
                if (currentX < 0) {
                    DEBUG("ScreenButtonsManager::layoutVerticalButtonGroup: Out of horizontal space!\n");
                    break;
                }
            }
            if (currentY + btnHeight > maxColumnHeight && currentY == marginTop) {
                DEBUG("ScreenButtonsManager::layoutVerticalButtonGroup: Button too tall for column!\n");
                continue;
            }

            Rect bounds(currentX, currentY, btnWidth, btnHeight);
            auto button = std::make_shared<UIButton>(self->getTft(), def.id, bounds, def.label, def.type, def.initialState, def.callback);

            self->addChild(button);

            if (out_createdButtons) {
                out_createdButtons->push_back(button);
            }

            currentY += btnHeight + buttonGap;
        }
    }

    /**
     * @brief Gombokat rendez el vízszintesen, a képernyő aljához igazítva.
     * A metódus a leszármazott képernyő (ami UIScreen-ből is származik)
     * tft referenciáját és addChild metódusát használja.
     */
    void layoutHorizontalButtonGroup(const std::vector<ButtonDefinition> &buttonDefs, std::vector<std::shared_ptr<UIButton>> *out_createdButtons = nullptr, int16_t marginLeft = 5,
                                     int16_t marginRight = 5, int16_t marginBottom = 5, int16_t defaultButtonWidthRef = UIButton::DEFAULT_BUTTON_WIDTH,
                                     int16_t defaultButtonHeightRef = UIButton::DEFAULT_BUTTON_HEIGHT, int16_t rowGap = 3, int16_t buttonGap = 3) {

        DerivedScreen *self = static_cast<DerivedScreen *>(this);

        if (buttonDefs.empty()) {
            return;
        }

        const int16_t screenHeight = self->getTft().height();
        const int16_t screenWidth = self->getTft().width();
        const int16_t maxRowWidth = screenWidth - marginLeft - marginRight;

        int16_t currentButtonHeight = defaultButtonHeightRef;
        if (!buttonDefs.empty() && buttonDefs[0].height > 0) {
            currentButtonHeight = buttonDefs[0].height;
        }

        int16_t currentY = screenHeight - marginBottom - currentButtonHeight;
        int16_t currentX = marginLeft;

        if (out_createdButtons) {
            out_createdButtons->clear();
        }

        for (const auto &def : buttonDefs) {
            int16_t btnWidth = (def.width > 0) ? def.width : defaultButtonWidthRef;
            int16_t btnHeight = (def.height > 0) ? def.height : defaultButtonHeightRef;

            if (currentX + btnWidth > maxRowWidth && currentX != marginLeft) {
                currentY -= (currentButtonHeight + rowGap);
                currentX = marginLeft;
                if (currentY < 0) {
                    DEBUG("ScreenButtonsManager::layoutHorizontalButtonGroup: Out of vertical space!\n");
                    break;
                }
            }
            if (currentX + btnWidth > maxRowWidth && currentX == marginLeft) {
                DEBUG("ScreenButtonsManager::layoutHorizontalButtonGroup: Button too wide for row!\n");
                continue;
            }

            Rect bounds(currentX, currentY, btnWidth, btnHeight);
            auto button = std::make_shared<UIButton>(self->getTft(), def.id, bounds, def.label, def.type, def.initialState, def.callback);
            self->addChild(button);

            if (out_createdButtons) {
                out_createdButtons->push_back(button);
            }

            currentX += btnWidth + buttonGap;
        }
    }
};

#endif // __SCREEN_BUTTONS_MANAGER_H