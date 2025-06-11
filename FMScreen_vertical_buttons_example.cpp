// Példa implementáció: FMScreen függőleges gombsor használata
// Ez egy példa bemutatja, hogyan integráld a UIVerticalButtonBar-t az FMScreen-be

#include "FMScreen.h"
#include "UIVerticalButtonBar.h"
#include "defines.h"

// Gomb ID-k konstansai a könnyebb kezeléshez
namespace FMScreenButtonIDs {
static constexpr uint8_t MUTE = 10;
static constexpr uint8_t VOLUME = 11;
static constexpr uint8_t AGC = 12;
static constexpr uint8_t ATT = 13;
static constexpr uint8_t SQUELCH = 14;
static constexpr uint8_t FREQ = 15;
static constexpr uint8_t SETUP = 16;
static constexpr uint8_t MEMO = 17;
} // namespace FMScreenButtonIDs

void FMScreen::layoutComponents() {

    // ... egyéb komponensek (StatusLine, FreqDisplay, SMeter stb.) ...

    // Függőleges gombsor létrehozása a jobb oldalon
    createVerticalButtonBar();

    // ... egyéb gombok (AM, Test, Setup vízszintesen alul) ...
}

void FMScreen::createVerticalButtonBar() {

    // Gombsor pozíciója és mérete
    const uint16_t buttonBarWidth = 65;
    const uint16_t buttonBarX = tft.width() - buttonBarWidth - 5; // 5px margó jobbról
    const uint16_t buttonBarY = 80;                               // StatusLine és FreqDisplay után
    const uint16_t buttonBarHeight = 200;                         // Vagy tft.height() - buttonBarY - 50

    // Gomb konfiguráció
    std::vector<UIVerticalButtonBar::ButtonConfig> buttonConfigs = {
        {FMScreenButtonIDs::MUTE, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleMuteButton(event); }},

        {FMScreenButtonIDs::VOLUME, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleVolumeButton(event); }},

        {FMScreenButtonIDs::AGC, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleAGCButton(event); }},

        {FMScreenButtonIDs::ATT, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleAttButton(event); }},

        {FMScreenButtonIDs::SQUELCH, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleSquelchButton(event); }},

        {FMScreenButtonIDs::FREQ, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleFreqButton(event); }},

        {FMScreenButtonIDs::SETUP, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleSetupButton(event); }},

        {FMScreenButtonIDs::MEMO, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleMemoButton(event); }}};

    // UIVerticalButtonBar létrehozása
    verticalButtonBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(buttonBarX, buttonBarY, buttonBarWidth, buttonBarHeight), buttonConfigs,
                                                              60, // gomb szélessége
                                                              32, // gomb magassága
                                                              4   // gombok közötti távolság
    );

    // Hozzáadás a képernyőhöz
    addChild(verticalButtonBar);
}

// Gomb eseménykezelők példái
void FMScreen::handleMuteButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::On) {
        DEBUG("FMScreen: Mute ON\n");
        pSi4735Manager->setMute(true);
    } else if (event.state == UIButton::EventButtonState::Off) {
        DEBUG("FMScreen: Mute OFF\n");
        pSi4735Manager->setMute(false);
    }
}

void FMScreen::handleVolumeButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Volume dialog\n");
        // Ide jöhet egy ValueChangeDialog a hangerő beállításához
        // showVolumeDialog();
    }
}

void FMScreen::handleAGCButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::On) {
        DEBUG("FMScreen: AGC ON\n");
        // AGC bekapcsolása
    } else if (event.state == UIButton::EventButtonState::Off) {
        DEBUG("FMScreen: AGC OFF\n");
        // AGC kikapcsolása
    }
}

void FMScreen::handleAttButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::On) {
        DEBUG("FMScreen: Attenuator ON\n");
        // Attenuátor bekapcsolása
    } else if (event.state == UIButton::EventButtonState::Off) {
        DEBUG("FMScreen: Attenuator OFF\n");
        // Attenuátor kikapcsolása
    }
}

void FMScreen::handleSquelchButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Squelch dialog\n");
        // Squelch beállító dialógus
        // showSquelchDialog();
    }
}

void FMScreen::handleFreqButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Frequency input dialog\n");
        // Frekvencia input dialógus
        // showFrequencyInputDialog();
    }
}

void FMScreen::handleSetupButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Switching to Setup screen\n");
        UIScreen::getManager()->switchToScreen(SCREEN_NAME_SETUP);
    }
}

void FMScreen::handleMemoButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Memory functions\n");
        // Memória funkciók (save/recall állomások)
        // showMemoryDialog();
    }
}

// Segédfunkciók a gombállapotok frissítéséhez
void FMScreen::updateMuteButtonState() {
    if (verticalButtonBar) {
        bool isMuted = pSi4735Manager->isMuted();
        verticalButtonBar->setButtonState(FMScreenButtonIDs::MUTE, isMuted ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
    }
}

void FMScreen::updateAGCButtonState() {
    if (verticalButtonBar) {
        bool agcEnabled = pSi4735Manager->isAGCEnabled();
        verticalButtonBar->setButtonState(FMScreenButtonIDs::AGC, agcEnabled ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
    }
}
