#include "UIHorizontalButtonBar.h"
#include "defines.h"

/**
 * @brief Konstruktor - vízszintes gombsor létrehozása
 */
UIHorizontalButtonBar::UIHorizontalButtonBar(TFT_eSPI &tft, const Rect &bounds, const std::vector<ButtonConfig> &buttonConfigs, uint16_t buttonWidth, uint16_t buttonHeight,
                                             uint16_t buttonGap)
    : UIContainerComponent(tft, bounds), buttonWidth(buttonWidth), buttonHeight(buttonHeight), buttonGap(buttonGap) {

    createButtons(buttonConfigs);
}

/**
 * @brief Gombok létrehozása és vízszintes elhelyezése
 */
void UIHorizontalButtonBar::createButtons(const std::vector<ButtonConfig> &buttonConfigs) {
    uint16_t currentX = bounds.x;

    // Gombok függőleges középre igazítása a konténerben
    uint16_t buttonY = bounds.y + (bounds.height - buttonHeight) / 2;

    for (const auto &config : buttonConfigs) {
        // Ellenőrizzük, hogy a gomb még belefér-e a bounds-ba
        if (currentX + buttonWidth > bounds.x + bounds.width) {
            DEBUG("UIHorizontalButtonBar: Button '%s' doesn't fit in bounds, skipping\n", config.label);
            break;
        }

        // Gomb létrehozása - függőlegesen középre igazítva
        auto button =
            std::make_shared<UIButton>(tft, config.id, Rect(currentX, buttonY, buttonWidth, buttonHeight), config.label, config.type, config.initialState, config.callback);

        // Hozzáadás a konténerhez és a belső listához
        addChild(button);
        buttons.push_back(button);

        // Következő gomb pozíciója
        currentX += buttonWidth + buttonGap;
    }
}

/**
 * @brief Gomb állapotának beállítása ID alapján
 */
void UIHorizontalButtonBar::setButtonState(uint8_t buttonId, UIButton::ButtonState state) {
    for (auto &button : buttons) {
        if (button->getId() == buttonId) {
            button->setButtonState(state);
            return;
        }
    }
    DEBUG("UIHorizontalButtonBar: Button with ID %d not found\n", buttonId);
}

/**
 * @brief Gomb állapotának lekérdezése ID alapján
 */
UIButton::ButtonState UIHorizontalButtonBar::getButtonState(uint8_t buttonId) const {
    for (const auto &button : buttons) {
        if (button->getId() == buttonId) {
            return button->getButtonState();
        }
    }
    DEBUG("UIHorizontalButtonBar: Button with ID %d not found\n", buttonId);
    return UIButton::ButtonState::Disabled;
}

/**
 * @brief Gomb referenciájának megszerzése ID alapján
 */
std::shared_ptr<UIButton> UIHorizontalButtonBar::getButton(uint8_t buttonId) const {
    for (const auto &button : buttons) {
        if (button->getId() == buttonId) {
            return button;
        }
    }
    return nullptr;
}
