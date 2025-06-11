// Egyszerűsített FMScreen implementáció a helper osztályok használatával
// Ez mutatja, hogyan lehet még egyszerűbben létrehozni függőleges gombsorokat

#include "FMScreen.h"
#include "VerticalButtonConfigs.h"

void FMScreen::createVerticalButtonBarSimplified() {

    // Automatikus pozíció számítás
    Rect position = VerticalButtonConfigHelper::calculateDefaultPosition(tft.width()); // Egyszerűsített gomb konfiguráció helper függvényekkel
    std::vector<UIVerticalButtonBar::ButtonConfig> buttonConfigs = {
        // Közös gombok helper függvényekkel
        VerticalButtonConfigHelper::createMuteButton(VerticalButtonIDs::FM::MUTE,
                                                     [this](const UIButton::ButtonEvent &event) { CommonVerticalButtons::handleMuteButton(event, pSi4735Manager); }),

        VerticalButtonConfigHelper::createVolumeButton(VerticalButtonIDs::FM::VOLUME, [this](const UIButton::ButtonEvent &event) { handleVolumeButton(event); }),

        VerticalButtonConfigHelper::createAGCButton(VerticalButtonIDs::FM::AGC, [this](const UIButton::ButtonEvent &event) { handleAGCButton(event); }),

        VerticalButtonConfigHelper::createAttButton(VerticalButtonIDs::FM::ATT, [this](const UIButton::ButtonEvent &event) { handleAttButton(event); }),

        // FM specifikus gomb
        {VerticalButtonIDs::FM::SQUELCH, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleSquelchButton(event); }},

        VerticalButtonConfigHelper::createFreqButton(VerticalButtonIDs::FM::FREQ, [this](const UIButton::ButtonEvent &event) { handleFreqButton(event); }),

        VerticalButtonConfigHelper::createSetupButton(VerticalButtonIDs::FM::SETUP,
                                                      [this](const UIButton::ButtonEvent &event) { CommonVerticalButtons::handleSetupButton(event, UIScreen::getManager()); }),

        VerticalButtonConfigHelper::createMemoButton(VerticalButtonIDs::FM::MEMO, [this](const UIButton::ButtonEvent &event) { handleMemoButton(event); })};

    // UIVerticalButtonBar létrehozása alapértelmezett méretekkel
    verticalButtonBar = std::make_shared<UIVerticalButtonBar>(tft, position, buttonConfigs, VerticalButtonLayout::DEFAULT_BUTTON_WIDTH, VerticalButtonLayout::DEFAULT_BUTTON_HEIGHT,
                                                              VerticalButtonLayout::DEFAULT_BUTTON_GAP);

    addChild(verticalButtonBar);
}

void FMScreen::updateVerticalButtonStatesSimplified() {
    // Közös mute állapot frissítés
    CommonVerticalButtons::updateMuteButtonState(verticalButtonBar.get(), VerticalButtonIDs::FM::MUTE);

    // További képernyő-specifikus állapot frissítések...
}

// Egyszerűsített AMScreen példa is:
void AMScreen::createVerticalButtonBarSimplified() {
    Rect position = VerticalButtonConfigHelper::calculateDefaultPosition(tft.width());
    std::vector<UIVerticalButtonBar::ButtonConfig> buttonConfigs = {
        VerticalButtonConfigHelper::createMuteButton(VerticalButtonIDs::AM::MUTE,
                                                     [this](const UIButton::ButtonEvent &event) { CommonVerticalButtons::handleMuteButton(event, pSi4735Manager); }),
        VerticalButtonConfigHelper::createVolumeButton(VerticalButtonIDs::AM::VOLUME, [this](const UIButton::ButtonEvent &event) { handleVolumeButton(event); }),
        VerticalButtonConfigHelper::createAGCButton(VerticalButtonIDs::AM::AGC, [this](const UIButton::ButtonEvent &event) { handleAGCButton(event); }),
        VerticalButtonConfigHelper::createAttButton(VerticalButtonIDs::AM::ATT, [this](const UIButton::ButtonEvent &event) { handleAttButton(event); }),

        // AM specifikus: Bandwidth gomb
        {VerticalButtonIDs::AM::BANDWIDTH, "BW", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleBandwidthButton(event); }},

        VerticalButtonConfigHelper::createFreqButton(VerticalButtonIDs::AM::FREQ, [this](const UIButton::ButtonEvent &event) { handleFreqButton(event); }),
        VerticalButtonConfigHelper::createSetupButton(VerticalButtonIDs::AM::SETUP,
                                                      [this](const UIButton::ButtonEvent &event) { CommonVerticalButtons::handleSetupButton(event, UIScreen::getManager()); }),
        VerticalButtonConfigHelper::createMemoButton(VerticalButtonIDs::AM::MEMO, [this](const UIButton::ButtonEvent &event) { handleMemoButton(event); })};

    verticalButtonBar = std::make_shared<UIVerticalButtonBar>(tft, position, buttonConfigs, VerticalButtonLayout::DEFAULT_BUTTON_WIDTH, VerticalButtonLayout::DEFAULT_BUTTON_HEIGHT,
                                                              VerticalButtonLayout::DEFAULT_BUTTON_GAP);

    addChild(verticalButtonBar);
}
