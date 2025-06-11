// Gyakorlati példák a UIVerticalButtonBar rugalmas használatára

#include "FMScreen.h"
#include "UIVerticalButtonBar.h"

// ===============================================
// PÉLDA 1: Minimális gombsor (csak a legfontosabbak)
// ===============================================
void FMScreen::createMinimalButtonBar() {
    std::vector<UIVerticalButtonBar::ButtonConfig> minimalConfigs = {
        {10, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](auto &event) { handleMuteButton(event); }},
        {11, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleVolumeButton(event); }},
        {12, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleSetupButtonVertical(event); }}};

    verticalButtonBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(255, 80, 65, 120), minimalConfigs, 60, 32, 4);
    addChild(verticalButtonBar);
}

// ===============================================
// PÉLDA 2: Teljes gombsor (minden funkcióval)
// ===============================================
void FMScreen::createFullButtonBar() {
    std::vector<UIVerticalButtonBar::ButtonConfig> fullConfigs = {
        {10, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](auto &event) { handleMuteButton(event); }},
        {11, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleVolumeButton(event); }},
        {12, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](auto &event) { handleAGCButton(event); }},
        {13, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](auto &event) { handleAttButton(event); }},
        {14, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleSquelchButton(event); }},
        {15, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleFreqButton(event); }},
        {16, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleSetupButtonVertical(event); }},
        {17, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleMemoButton(event); }}};

    verticalButtonBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(255, 80, 65, 280), fullConfigs, 60, 32, 4);
    addChild(verticalButtonBar);
}

// ===============================================
// PÉLDA 3: AM képernyő specifikus gombsor
// ===============================================
void AMScreen::createAMSpecificButtonBar() {
    std::vector<UIVerticalButtonBar::ButtonConfig> amConfigs = {
        {20, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](auto &event) { handleMuteButton(event); }},
        {21, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleVolumeButton(event); }},
        // Squelch NINCS - AM-ben nem releváns
        {22, "BW", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleBandwidthButton(event); }}, // AM specifikus
        {23, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](auto &event) { handleAGCButton(event); }},
        {24, "Filter", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleNoiseFilterButton(event); }}, // AM specifikus
        {25, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleFreqButton(event); }},
        {26, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleSetupButtonVertical(event); }},
        {27, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleMemoButton(event); }}};

    verticalButtonBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(255, 80, 65, 280), amConfigs, 60, 32, 4);
    addChild(verticalButtonBar);
}

// ===============================================
// PÉLDA 4: Dinamikus gomb hozzáadás/eltávolítás
// ===============================================
void FMScreen::createDynamicButtonBar() {
    // Alapvető gombok létrehozása
    std::vector<UIVerticalButtonBar::ButtonConfig> basicConfigs = {
        {10, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](auto &event) { handleMuteButton(event); }},
        {11, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleVolumeButton(event); }},
        {16, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleSetupButtonVertical(event); }}};

    verticalButtonBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(255, 80, 65, 280), basicConfigs, 60, 32, 4);
    addChild(verticalButtonBar);

    // Dinamikusan hozzáadunk gombokat egy feltétel alapján
    if (config.data.showAdvancedButtons) {
        // AGC gomb hozzáadása
        verticalButtonBar->addButton({12, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](auto &event) { handleAGCButton(event); }});

        // Squelch gomb hozzáadása
        verticalButtonBar->addButton({14, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleSquelchButton(event); }});
    }

    if (config.data.showMemoryButtons) {
        // Memory gomb hozzáadása
        verticalButtonBar->addButton({17, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleMemoButton(event); }});
    }
}

// ===============================================
// PÉLDA 5: Gomb elrejtés/megjelenítés futásidőben
// ===============================================
void FMScreen::toggleAdvancedButtons(bool show) {
    if (verticalButtonBar) {
        verticalButtonBar->setButtonVisible(12, show); // AGC
        verticalButtonBar->setButtonVisible(13, show); // ATT
        verticalButtonBar->setButtonVisible(14, show); // Squelch
    }
}

// ===============================================
// PÉLDA 6: Gomb eltávolítás
// ===============================================
void FMScreen::removeAdvancedButtons() {
    if (verticalButtonBar) {
        verticalButtonBar->removeButton(12); // AGC eltávolítása
        verticalButtonBar->removeButton(13); // ATT eltávolítása
        verticalButtonBar->removeButton(14); // Squelch eltávolítása
        // A gombok automatikusan újrarendeződnek
    }
}

// ===============================================
// PÉLDA 7: Képernyő mód alapú gombsor
// ===============================================
void FMScreen::createModeBasedButtonBar(ScreenMode mode) {
    std::vector<UIVerticalButtonBar::ButtonConfig> configs;

    // Alapvető gombok minden módban
    configs.push_back({10, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](auto &event) { handleMuteButton(event); }});
    configs.push_back({11, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleVolumeButton(event); }});

    switch (mode) {
        case ScreenMode::BEGINNER:
            // Csak alapvető gombok
            configs.push_back({16, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleSetupButtonVertical(event); }});
            break;

        case ScreenMode::ADVANCED:
            // Haladó gombok
            configs.push_back({12, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](auto &event) { handleAGCButton(event); }});
            configs.push_back({14, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleSquelchButton(event); }});
            configs.push_back({15, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleFreqButton(event); }});
            configs.push_back({16, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleSetupButtonVertical(event); }});
            break;

        case ScreenMode::EXPERT:
            // Minden gomb
            configs.push_back({12, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](auto &event) { handleAGCButton(event); }});
            configs.push_back({13, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](auto &event) { handleAttButton(event); }});
            configs.push_back({14, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleSquelchButton(event); }});
            configs.push_back({15, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleFreqButton(event); }});
            configs.push_back({16, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleSetupButtonVertical(event); }});
            configs.push_back({17, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleMemoButton(event); }});
            break;
    }

    verticalButtonBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(255, 80, 65, 280), configs, 60, 32, 4);
    addChild(verticalButtonBar);
}
