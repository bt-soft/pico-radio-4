/**
 * @file HorizontalButtonBar_Complete_Example.cpp
 * @brief Teljes példa az UIHorizontalButtonBar használatára állapot szinkronizálással
 * @date 2025-06-11
 *
 * Ez a példa bemutatja:
 * - UIHorizontalButtonBar létrehozását
 * - Gomb eseménykezelőket
 * - Állapot szinkronizálást
 * - Automatikus frissítést
 */

#include "Band.h"
#include "FMScreen.h"
#include "UIHorizontalButtonBar.h"

// Vízszintes gomb ID konstansai
namespace ExampleHorizontalButtonIDs {
static constexpr uint8_t AM_BUTTON = 20;
static constexpr uint8_t TEST_BUTTON = 21;
static constexpr uint8_t SETUP_BUTTON = 22;
static constexpr uint8_t STEREO_BUTTON = 23; // További példa gomb
} // namespace ExampleHorizontalButtonIDs

class ExampleScreen : public UIScreen {
  private:
    std::shared_ptr<UIHorizontalButtonBar> horizontalButtonBar;

  public:
    void createHorizontalButtonBar() {
        // Gombsor pozíciója - bal alsó sarok
        const uint16_t buttonBarHeight = 35;
        const uint16_t buttonBarX = 0;
        const uint16_t buttonBarY = tft.height() - buttonBarHeight;
        const uint16_t buttonBarWidth = 280; // 4 gomb számára

        // Gomb konfiguráció
        std::vector<UIHorizontalButtonBar::ButtonConfig> buttonConfigs = {// AM/FM váltó gomb - toggleable
                                                                          {ExampleHorizontalButtonIDs::AM_BUTTON, "AM", UIButton::ButtonType::Toggleable,
                                                                           UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleAMButton(event); }},

                                                                          // Test gomb - pushable
                                                                          {ExampleHorizontalButtonIDs::TEST_BUTTON, "Test", UIButton::ButtonType::Pushable,
                                                                           UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleTestButton(event); }},

                                                                          // Setup gomb - pushable
                                                                          {ExampleHorizontalButtonIDs::SETUP_BUTTON, "Setup", UIButton::ButtonType::Pushable,
                                                                           UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleSetupButton(event); }},

                                                                          // Stereo indikátor - toggleable
                                                                          {ExampleHorizontalButtonIDs::STEREO_BUTTON, "Stereo", UIButton::ButtonType::Toggleable,
                                                                           UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleStereoButton(event); }}};

        // UIHorizontalButtonBar létrehozása
        horizontalButtonBar = std::make_shared<UIHorizontalButtonBar>(tft, Rect(buttonBarX, buttonBarY, buttonBarWidth, buttonBarHeight), buttonConfigs,
                                                                      65, // gomb szélessége
                                                                      30, // gomb magassága
                                                                      3   // gombok közötti távolság
        );

        // Hozzáadás a képernyőhöz
        addChild(horizontalButtonBar);
    }

    // =========================
    // Gomb eseménykezelők
    // =========================

    void handleAMButton(const UIButton::ButtonEvent &event) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("Switching between AM/FM modes\n");

            // Aktuális band lekérdezése
            uint8_t currentBandType = pSi4735Manager->getCurrentBand().bandType;

            if (currentBandType == FM_BAND_TYPE) {
                // FM-ről AM-re váltás
                pSi4735Manager->switchToBand(1); // MW band
            } else {
                // AM-ről FM-re váltás
                pSi4735Manager->switchToBand(0); // FM band
            }

            // Állapotok frissítése
            updateHorizontalButtonStates();
        }
    }

    void handleTestButton(const UIButton::ButtonEvent &event) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("Switching to Test screen\n");
            UIScreen::getManager()->switchToScreen(SCREEN_NAME_TEST);
        }
    }

    void handleSetupButton(const UIButton::ButtonEvent &event) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("Switching to Setup screen\n");
            UIScreen::getManager()->switchToScreen(SCREEN_NAME_SETUP);
        }
    }

    void handleStereoButton(const UIButton::ButtonEvent &event) {
        if (event.state == UIButton::EventButtonState::On) {
            DEBUG("Stereo mode enabled\n");
            // Stereo mód bekapcsolása
        } else if (event.state == UIButton::EventButtonState::Off) {
            DEBUG("Mono mode enabled\n");
            // Mono mód bekapcsolása
        }
    }

    // =========================
    // Állapot szinkronizálás
    // =========================

    void updateHorizontalButtonStates() {
        if (!horizontalButtonBar) {
            return;
        }

        // Aktuális sáv lekérdezése
        uint8_t currentBandType = pSi4735Manager->getCurrentBand().bandType;

        // AM gomb állapot szinkronizálása
        bool isAMMode = (currentBandType != FM_BAND_TYPE);
        horizontalButtonBar->setButtonState(ExampleHorizontalButtonIDs::AM_BUTTON, isAMMode ? UIButton::ButtonState::On : UIButton::ButtonState::Off);

        // Stereo gomb állapot szinkronizálása (csak FM módban)
        if (currentBandType == FM_BAND_TYPE) {
            bool isStereo = pSi4735Manager->isStereo();
            horizontalButtonBar->setButtonState(ExampleHorizontalButtonIDs::STEREO_BUTTON, isStereo ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
        } else {
            // AM módban a stereo gomb ki van kapcsolva
            horizontalButtonBar->setButtonState(ExampleHorizontalButtonIDs::STEREO_BUTTON, UIButton::ButtonState::Off);
        }

        // Test és Setup gombok pushable típusúak, állapotuk nem változik
    }

    // =========================
    // Loop és frissítés
    // =========================

    virtual void handleOwnLoop() override {
        // Egyéb logika...

        // Gombállapotok automatikus frissítése
        updateHorizontalButtonStates();
    }

    // =========================
    // Segéd metódusok
    // =========================

    /**
     * @brief Adott gomb állapotának lekérdezése
     */
    UIButton::ButtonState getButtonState(uint8_t buttonId) {
        if (horizontalButtonBar) {
            return horizontalButtonBar->getButtonState(buttonId);
        }
        return UIButton::ButtonState::Off;
    }

    /**
     * @brief Adott gomb referenciájának lekérdezése
     */
    std::shared_ptr<UIButton> getButton(uint8_t buttonId) {
        if (horizontalButtonBar) {
            return horizontalButtonBar->getButton(buttonId);
        }
        return nullptr;
    }

    /**
     * @brief Gomb engedélyezése/letiltása
     */
    void setButtonEnabled(uint8_t buttonId, bool enabled) {
        auto button = getButton(buttonId);
        if (button) {
            button->setEnabled(enabled);
        }
    }

    /**
     * @brief Gomb szövegének megváltoztatása
     */
    void setButtonLabel(uint8_t buttonId, const String &newLabel) {
        auto button = getButton(buttonId);
        if (button) {
            button->setLabel(newLabel);
        }
    }
};

// =========================
// Használati példák
// =========================

void example_basic_usage() {
    // Alapvető használat
    ExampleScreen screen;
    screen.createHorizontalButtonBar();

    // Gombállapot lekérdezése
    UIButton::ButtonState amState = screen.getButtonState(ExampleHorizontalButtonIDs::AM_BUTTON);

    // Gomb letiltása
    screen.setButtonEnabled(ExampleHorizontalButtonIDs::TEST_BUTTON, false);

    // Gomb szövegének változtatása
    screen.setButtonLabel(ExampleHorizontalButtonIDs::AM_BUTTON, "FM");
}

void example_dynamic_button_management() {
    ExampleScreen screen;
    screen.createHorizontalButtonBar();

    // Band váltás esemény kezelése
    auto onBandChanged = [&screen](uint8_t newBandType) {
        // Gombállapotok frissítése
        screen.updateHorizontalButtonStates();

        // Dinamikus címke változtatás
        if (newBandType == FM_BAND_TYPE) {
            screen.setButtonLabel(ExampleHorizontalButtonIDs::AM_BUTTON, "AM");
        } else {
            screen.setButtonLabel(ExampleHorizontalButtonIDs::AM_BUTTON, "FM");
        }
    };

    // Használat: onBandChanged(newBandType);
}

// =========================
// Fejlett funkciók
// =========================

void example_advanced_features() {
    ExampleScreen screen;

    // Különböző gomb típusok és állapotok
    std::vector<UIHorizontalButtonBar::ButtonConfig> advancedConfigs = {
        // Toggle gomb kezdeti ON állapottal
        {1, "Power", UIButton::ButtonType::Toggleable, UIButton::ButtonState::On, [](const UIButton::ButtonEvent &event) { /* ... */ }},

        // Pushable gomb
        {2, "Scan", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [](const UIButton::ButtonEvent &event) { /* ... */ }},

        // További toggle gomb
        {3, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [](const UIButton::ButtonEvent &event) { /* ... */ }}};

    // Egyedi méretek
    auto buttonBar = std::make_shared<UIHorizontalButtonBar>(screen.getTft(), Rect(10, 200, 300, 40), // Egyedi pozíció és méret
                                                             advancedConfigs,
                                                             90, // Szélesebb gombok
                                                             35, // Magasabb gombok
                                                             5   // Nagyobb térköz
    );
}
