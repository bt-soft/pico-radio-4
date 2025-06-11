// AMScreen specifikus gombkonfiguráció példa
#include "UIVerticalButtonBar.h"

namespace AMScreenButtonIDs {
const uint8_t MUTE = 20;
const uint8_t VOLUME = 21;
const uint8_t AGC = 22;
const uint8_t ATT = 23;
const uint8_t BANDWIDTH = 24; // AM specifikus - sávszélesség
const uint8_t FREQ = 25;
const uint8_t SETUP = 26;
const uint8_t MEMO = 27;
} // namespace AMScreenButtonIDs

void createAMVerticalButtonBar(TFT_eSPI &tft, UIScreen *screen) {
    // AM specifikus gomb konfiguráció
    std::vector<UIVerticalButtonBar::ButtonConfig> amButtonConfigs = {
        {AMScreenButtonIDs::MUTE, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
         [](const UIButton::ButtonEvent &event) {
             // AM Mute logika (ugyanaz mint FM-nél)
         }},

        {AMScreenButtonIDs::VOLUME, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [](const UIButton::ButtonEvent &event) {
             // Volume beállítás
         }},

        {AMScreenButtonIDs::AGC, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
         [](const UIButton::ButtonEvent &event) {
             // AGC be/ki (AM-nél más lehet a beállítás)
         }},

        {AMScreenButtonIDs::ATT, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
         [](const UIButton::ButtonEvent &event) {
             // Attenuátor
         }},

        // AM specifikus: Sávszélesség helyett squelch
        {AMScreenButtonIDs::BANDWIDTH, "BW", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [](const UIButton::ButtonEvent &event) {
             DEBUG("AM: Bandwidth adjustment requested\n");
             // AM sávszélesség beállítás (0.5kHz, 1kHz, 2kHz, 4kHz stb.)
         }},

        {AMScreenButtonIDs::FREQ, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [](const UIButton::ButtonEvent &event) {
             // Frekvencia input
         }},

        {AMScreenButtonIDs::SETUP, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [](const UIButton::ButtonEvent &event) { UIScreen::getManager()->switchToScreen(SCREEN_NAME_SETUP); }},

        {AMScreenButtonIDs::MEMO, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [](const UIButton::ButtonEvent &event) {
             // AM memória funkciók
         }}}; // AM buttonbar létrehozása - jobb felső sarok
    auto amButtonBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(tft.width() - 65, 0, 65, tft.height()), // jobb felső sarok
                                                             amButtonConfigs, 60, 32, 4                        // ugyanazok a méretek
    );

    screen->addChild(amButtonBar);
}

// ============================================
// Screen-specifikus gombkezelés
// ============================================

class ScreenButtonManager {
  private:
    std::shared_ptr<UIVerticalButtonBar> buttonBar;

  public:
    // Különböző screen típusokhoz különböző konfigurációk
    enum ScreenType { FM_SCREEN, AM_SCREEN, SW_SCREEN, LW_SCREEN };

    void createForScreen(TFT_eSPI &tft, UIScreen *screen, ScreenType type) {
        std::vector<UIVerticalButtonBar::ButtonConfig> configs;

        switch (type) {
            case FM_SCREEN:
                configs = createFMConfigs();
                break;
            case AM_SCREEN:
                configs = createAMConfigs();
                break;
            case SW_SCREEN:
                configs = createSWConfigs();
                break;
            case LW_SCREEN:
                configs = createLWConfigs();
                break;
        }

        buttonBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(tft.width() - 65, 0, 65, tft.height()), configs, 60, 32, 4);
        screen->addChild(buttonBar);
    }

  private:
    std::vector<UIVerticalButtonBar::ButtonConfig> createFMConfigs() {
        return {{10, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},
                {11, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
                {12, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},
                {13, "RDS", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr}, // FM specifikus
                {14, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
                {15, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
                {16, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
                {17, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr}};
    }

    std::vector<UIVerticalButtonBar::ButtonConfig> createAMConfigs() {
        return {
            {20, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr}, {21, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
            {22, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},  {23, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},
            {24, "BW", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr}, // AM specifikus
            {25, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},   {26, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
            {27, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr}};
    }

    std::vector<UIVerticalButtonBar::ButtonConfig> createSWConfigs() {
        return {{30, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},
                {31, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
                {32, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},
                {33, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},
                {34, "BW", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
                {35, "Band", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr}, // SW specifikus
                {36, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
                {37, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr}};
    }

    std::vector<UIVerticalButtonBar::ButtonConfig> createLWConfigs() {
        return {// LW egyszerűbb lehet, kevesebb gomb
                {40, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},
                {41, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
                {42, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},
                {43, "BW", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
                {44, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr}};
    }
};
