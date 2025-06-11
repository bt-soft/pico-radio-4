// Példa: AMScreen függőleges gombsor implementáció
// Ez mutatja, hogyan lehet újrafelhasználni az UIVerticalButtonBar-t más képernyőkön

#include "AMScreen.h"
#include "UIVerticalButtonBar.h"
#include "rtVars.h"

// AM képernyő specifikus gomb ID-k
namespace AMScreenButtonIDs {
static constexpr uint8_t MUTE = 20;
static constexpr uint8_t VOLUME = 21;
static constexpr uint8_t AGC = 22;
static constexpr uint8_t ATT = 23;
static constexpr uint8_t BANDWIDTH = 24; // AM specifikus
static constexpr uint8_t FREQ = 25;
static constexpr uint8_t SETUP = 26;
static constexpr uint8_t MEMO = 27;
} // namespace AMScreenButtonIDs

void AMScreen::createVerticalButtonBar() {

    // Gombsor pozíciója (hasonló az FM-hez)
    const uint16_t buttonBarWidth = 65;
    const uint16_t buttonBarX = tft.width() - buttonBarWidth - 5;
    const uint16_t buttonBarY = 80;
    const uint16_t buttonBarHeight = 200;

    // AM specifikus gomb konfiguráció
    std::vector<UIVerticalButtonBar::ButtonConfig> buttonConfigs = {
        {AMScreenButtonIDs::MUTE, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleMuteButton(event); }},

        {AMScreenButtonIDs::VOLUME, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleVolumeButton(event); }},

        {AMScreenButtonIDs::AGC, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleAGCButton(event); }},

        {AMScreenButtonIDs::ATT, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleAttButton(event); }},

        // AM specifikus: sávszélesség gomb (FM-ben nincs)
        {AMScreenButtonIDs::BANDWIDTH, "BW", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleBandwidthButton(event); }},

        {AMScreenButtonIDs::FREQ, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleFreqButton(event); }},

        {AMScreenButtonIDs::SETUP, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleSetupButton(event); }},

        {AMScreenButtonIDs::MEMO, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleMemoButton(event); }}};

    // UIVerticalButtonBar létrehozása (azonos API)
    verticalButtonBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(buttonBarX, buttonBarY, buttonBarWidth, buttonBarHeight), buttonConfigs,
                                                              60, // gomb szélesség
                                                              32, // gomb magasság
                                                              4   // gap
    );

    addChild(verticalButtonBar);
}

// AM specifikus gomb eseménykezelők
void AMScreen::handleBandwidthButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("AMScreen: Bandwidth selection requested\n");
        // TODO: AM sávszélesség választó dialógus
        // showBandwidthDialog();
    }
}

// A többi eseménykezelő hasonló az FM-hez...
void AMScreen::handleMuteButton(const UIButton::ButtonEvent &event) {
    // Ugyanaz a logika, mint az FM-ben
    if (event.state == UIButton::EventButtonState::On) {
        DEBUG("AMScreen: Mute ON\n");
        rtv::muteStat = true;
        pSi4735Manager->getSi4735().setAudioMute(true);
    } else if (event.state == UIButton::EventButtonState::Off) {
        DEBUG("AMScreen: Mute OFF\n");
        rtv::muteStat = false;
        pSi4735Manager->getSi4735().setAudioMute(false);
    }
}

void AMScreen::updateVerticalButtonStates() {
    if (!verticalButtonBar) {
        return;
    }

    // Mute állapot szinkronizálás (közös az FM-mel)
    verticalButtonBar->setButtonState(AMScreenButtonIDs::MUTE, rtv::muteStat ? UIButton::ButtonState::On : UIButton::ButtonState::Off);

    // AM specifikus állapot szinkronizálások...
    // TODO: Bandwidth gomb aktuális értékének megjelenítése
    // TODO: AGC állapot szinkronizálás
    // TODO: Attenuator állapot szinkronizálás
}
