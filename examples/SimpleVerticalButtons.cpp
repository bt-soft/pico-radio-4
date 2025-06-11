// Egyszerű 4 gombos konfiguráció példa
#include "FMScreen.h" // FMScreenButtonIDs namespace-ért
#include "UIVerticalButtonBar.h"

void createSimpleButtonBar(TFT_eSPI &tft, UIScreen *screen) {
    // Csak 4 alapvető gomb
    std::vector<UIVerticalButtonBar::ButtonConfig> simpleConfig = {{10, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
                                                                    [](const UIButton::ButtonEvent &event) {
                                                                        // Mute logika
                                                                        DEBUG("Simple Mute button pressed\n");
                                                                    }},

                                                                   {11, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                                    [](const UIButton::ButtonEvent &event) {
                                                                        // Volume logika
                                                                        DEBUG("Simple Volume button pressed\n");
                                                                    }},

                                                                   {12, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                                    [](const UIButton::ButtonEvent &event) {
                                                                        // Setup logika
                                                                        UIScreen::getManager()->switchToScreen(SCREEN_NAME_SETUP);
                                                                    }},

                                                                   {13, "Back", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [](const UIButton::ButtonEvent &event) {
                                                                        // Vissza az előző képernyőre
                                                                        DEBUG("Back button pressed\n");
                                                                    }}}; // Kisebb gombsor létrehozása - jobb felső sarok
    auto buttonBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(tft.width() - 40, 0, 40, 160), // jobb felső sarok
                                                           simpleConfig,
                                                           35, // kisebb gomb szélesség
                                                           30, // kisebb gomb magasság
                                                           5   // nagyobb gap
    );

    screen->addChild(buttonBar);
}

// ============================================
// Dinamikus gombkezelés példák
// ============================================

void demonstrateDynamicButtons(std::shared_ptr<UIVerticalButtonBar> buttonBar) {

    // 1. Új gomb hozzáadása
    UIVerticalButtonBar::ButtonConfig extraButton{50, "Extra", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                  [](const UIButton::ButtonEvent &event) { DEBUG("Extra button clicked!\n"); }};

    if (buttonBar->addButton(extraButton)) {
        DEBUG("Extra button successfully added\n");
    }

    // 2. Kondicionális gomb megjelenítés
    bool advancedMode = true; // valamilyen beállítás alapján

    if (advancedMode) {
        // Fejlett gombok megjelenítése
        buttonBar->setButtonVisible(12, true); // Setup látható
        buttonBar->setButtonVisible(13, true); // Back látható
    } else {
        // Egyszerű mód - csak alapgombok
        buttonBar->setButtonVisible(12, false); // Setup rejtve
        buttonBar->setButtonVisible(13, false); // Back rejtve
    }

    // 3. Gomb állapot változtatása
    buttonBar->setButtonState(10, UIButton::ButtonState::On); // Mute bekapcsolva

    // 4. Teljes újrakonfigurálás (gomb eltávolítással)
    buttonBar->removeButton(50); // Extra gomb eltávolítása
}

// ============================================
// Adaptív gombsor példák különböző helyzetekre
// ============================================

class AdaptiveButtonManager {
  public:
    enum Mode {
        BASIC_MODE,    // Csak alapgombok
        ADVANCED_MODE, // Minden gomb
        COMPACT_MODE   // Csak a legfontosabbak
    };

    static void configureForMode(std::shared_ptr<UIVerticalButtonBar> buttonBar, Mode mode) {
        switch (mode) {
            case BASIC_MODE:
                // Csak mute és setup
                buttonBar->setButtonVisible(FMScreenButtonIDs::MUTE, true);
                buttonBar->setButtonVisible(FMScreenButtonIDs::VOLUME, false);
                buttonBar->setButtonVisible(FMScreenButtonIDs::AGC, false);
                buttonBar->setButtonVisible(FMScreenButtonIDs::ATT, false);
                buttonBar->setButtonVisible(FMScreenButtonIDs::SQUELCH, false);
                buttonBar->setButtonVisible(FMScreenButtonIDs::FREQ, false);
                buttonBar->setButtonVisible(FMScreenButtonIDs::SETUP, true);
                buttonBar->setButtonVisible(FMScreenButtonIDs::MEMO, false);
                break;

            case ADVANCED_MODE:
                // Minden gomb látható
                for (int id = FMScreenButtonIDs::MUTE; id <= FMScreenButtonIDs::MEMO; id++) {
                    buttonBar->setButtonVisible(id, true);
                }
                break;

            case COMPACT_MODE:
                // Kompakt mód - csak a leggyakrabban használtak
                buttonBar->setButtonVisible(FMScreenButtonIDs::MUTE, true);
                buttonBar->setButtonVisible(FMScreenButtonIDs::VOLUME, true);
                buttonBar->setButtonVisible(FMScreenButtonIDs::AGC, false);
                buttonBar->setButtonVisible(FMScreenButtonIDs::ATT, false);
                buttonBar->setButtonVisible(FMScreenButtonIDs::SQUELCH, true);
                buttonBar->setButtonVisible(FMScreenButtonIDs::FREQ, true);
                buttonBar->setButtonVisible(FMScreenButtonIDs::SETUP, false);
                buttonBar->setButtonVisible(FMScreenButtonIDs::MEMO, false);
                break;
        }

        // Újrarendezés hogy a látható gombok szép sorrendben legyenek
        buttonBar->relayoutButtons();
    }
};

// Használat:
// AdaptiveButtonManager::configureForMode(verticalButtonBar, AdaptiveButtonManager::COMPACT_MODE);
