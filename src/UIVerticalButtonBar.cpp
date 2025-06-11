#include "UIVerticalButtonBar.h"
#include "defines.h"

/**
 * @brief Konstruktor - függőleges gombsor létrehozása
 */
UIVerticalButtonBar::UIVerticalButtonBar(TFT_eSPI &tft, const Rect &bounds, const std::vector<ButtonConfig> &buttonConfigs, uint16_t buttonWidth, uint16_t buttonHeight,
                                         uint16_t buttonGap)
    : UIContainerComponent(tft, bounds), buttonWidth(buttonWidth), buttonHeight(buttonHeight), buttonGap(buttonGap) {

    createButtons(buttonConfigs);
}

/**
 * @brief Gombok létrehozása és függőleges elhelyezése
 */
void UIVerticalButtonBar::createButtons(const std::vector<ButtonConfig> &buttonConfigs) {
    uint16_t currentY = bounds.y;

    for (const auto &config : buttonConfigs) {
        // Ellenőrizzük, hogy a gomb még belefér-e a bounds-ba
        if (currentY + buttonHeight > bounds.y + bounds.height) {
            DEBUG("UIVerticalButtonBar: Button '%s' doesn't fit in bounds, skipping\n", config.label);
            break;
        }

        // Gomb létrehozása
        auto button =
            std::make_shared<UIButton>(tft, config.id, Rect(bounds.x, currentY, buttonWidth, buttonHeight), config.label, config.type, config.initialState, config.callback);

        // Hozzáadás a konténerhez és a belső listához
        addChild(button);
        buttons.push_back(button);

        // Következő gomb pozíciója
        currentY += buttonHeight + buttonGap;
    }
}

/**
 * @brief Gomb állapotának beállítása ID alapján
 */
void UIVerticalButtonBar::setButtonState(uint8_t buttonId, UIButton::ButtonState state) {
    for (auto &button : buttons) {
        if (button->getId() == buttonId) {
            button->setButtonState(state);
            return;
        }
    }
    DEBUG("UIVerticalButtonBar: Button with ID %d not found\n", buttonId);
}

/**
 * @brief Gomb állapotának lekérdezése ID alapján
 */
UIButton::ButtonState UIVerticalButtonBar::getButtonState(uint8_t buttonId) const {
    for (const auto &button : buttons) {
        if (button->getId() == buttonId) {
            return button->getButtonState();
        }
    }
    DEBUG("UIVerticalButtonBar: Button with ID %d not found\n", buttonId);
    return UIButton::ButtonState::Disabled;
}

/**
 * @brief Gomb referenciájának megszerzése ID alapján
 */
std::shared_ptr<UIButton> UIVerticalButtonBar::getButton(uint8_t buttonId) const {
    for (const auto &button : buttons) {
        if (button->getId() == buttonId) {
            return button;
        }
    }
    return nullptr;
}

/**
 * @brief Gomb hozzáadása futásidőben
 */
bool UIVerticalButtonBar::addButton(const ButtonConfig &config) {
    // Ellenőrizzük, hogy van-e hely
    uint16_t requiredHeight = (buttons.size() + 1) * (buttonHeight + buttonGap) - buttonGap;
    if (requiredHeight > bounds.height) {
        DEBUG("UIVerticalButtonBar: No space for new button '%s'\n", config.label);
        return false;
    }

    // Ellenőrizzük, hogy nincs-e már ilyen ID
    if (getButton(config.id) != nullptr) {
        DEBUG("UIVerticalButtonBar: Button with ID %d already exists\n", config.id);
        return false;
    }

    // Új gomb pozíciójának számítása
    uint16_t newButtonY = bounds.y + buttons.size() * (buttonHeight + buttonGap);

    // Új gomb létrehozása
    auto button =
        std::make_shared<UIButton>(tft, config.id, Rect(bounds.x, newButtonY, buttonWidth, buttonHeight), config.label, config.type, config.initialState, config.callback);

    // Hozzáadás a konténerhez és a belső listához
    addChild(button);
    buttons.push_back(button);

    DEBUG("UIVerticalButtonBar: Added button '%s' with ID %d\n", config.label, config.id);
    return true;
}

/**
 * @brief Gomb eltávolítása ID alapján
 */
bool UIVerticalButtonBar::removeButton(uint8_t buttonId) {
    for (auto it = buttons.begin(); it != buttons.end(); ++it) {
        if ((*it)->getId() == buttonId) {
            // Eltávolítás a konténerből
            removeChild(*it);
            // Eltávolítás a belső listából
            buttons.erase(it);
            // Gombok újrarendezése
            relayoutButtons();
            DEBUG("UIVerticalButtonBar: Removed button with ID %d\n", buttonId);
            return true;
        }
    }
    DEBUG("UIVerticalButtonBar: Button with ID %d not found for removal\n", buttonId);
    return false;
}

/**
 * @brief Gomb láthatóságának beállítása
 */
void UIVerticalButtonBar::setButtonVisible(uint8_t buttonId, bool visible) {
    auto button = getButton(buttonId);
    if (button) {
        button->setDisabled(!visible);
        if (!visible) {
            // Rejtett gomb esetén a bounds-t nullázhatjuk
            button->setBounds(Rect(0, 0, 0, 0));
        }
        relayoutButtons();
    } else {
        DEBUG("UIVerticalButtonBar: Button with ID %d not found for visibility change\n", buttonId);
    }
}

/**
 * @brief Gombok újrarendezése
 */
void UIVerticalButtonBar::relayoutButtons() {
    uint16_t currentY = bounds.y;

    for (auto &button : buttons) {
        // Csak a látható gombok kapnak új pozíciót
        if (!button->isDisabled()) {
            button->setBounds(Rect(bounds.x, currentY, buttonWidth, buttonHeight));
            currentY += buttonHeight + buttonGap;
        }
    }

    // Újrarajzolás kérése
    markForRedraw();
}
