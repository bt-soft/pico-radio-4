// Pozicionálás példák - jobb felső sarok (függőleges) és bal alsó sarok (vízszintes)
#include "UIVerticalButtonBar.h"

// ============================================
// Függőleges gombok - jobb felső sarok
// ============================================

void createRightTopVerticalButtons(TFT_eSPI &tft, UIScreen *screen) {
    std::vector<UIVerticalButtonBar::ButtonConfig> verticalButtons = {
        {10, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr}, {11, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
        {12, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},  {13, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},
        {14, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},    {15, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
        {16, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},  {17, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr}};

    // Jobb felső sarok - teljes képernyő széléhez illesztve
    const uint16_t buttonBarWidth = 65;
    auto verticalBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(tft.width() - buttonBarWidth, 0, buttonBarWidth, tft.height()), // x, y, width, height
                                                             verticalButtons, 60, 32, 4                                                // button width, height, gap
    );

    screen->addChild(verticalBar);
}

// ============================================
// Vízszintes gombok - bal alsó sarok
// ============================================

void createLeftBottomHorizontalButtons(TFT_eSPI &tft, UIScreen *screen) {
    // Vízszintes gombok: minden gomb külön UIVerticalButtonBar (1 gombbal)
    const uint16_t buttonWidth = 45;
    const uint16_t buttonHeight = 30;
    const uint16_t gap = 3;
    const uint16_t bottomY = tft.height() - buttonHeight; // Alsó szélhez illesztve
    const uint16_t startX = 0;                            // Bal szélhez illesztve

    std::vector<std::pair<std::string, uint8_t>> horizontalButtons = {{"RDS", 20}, {"Memo", 21}, {"Scan", 22}, {"Band", 23}, {"Step", 24}};

    uint16_t currentX = startX;

    for (const auto &buttonData : horizontalButtons) {
        std::vector<UIVerticalButtonBar::ButtonConfig> singleButtonConfig = {
            {buttonData.second, buttonData.first.c_str(), UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr}};

        auto singleButton = std::make_shared<UIVerticalButtonBar>(tft, Rect(currentX, bottomY, buttonWidth, buttonHeight), singleButtonConfig, buttonWidth, buttonHeight,
                                                                  0 // 0 gap mert csak 1 gomb van
        );

        screen->addChild(singleButton);
        currentX += buttonWidth + gap;
    }
}

// ============================================
// Kombinált layout - mindkét pozícióban
// ============================================

void createCornerButtonLayout(TFT_eSPI &tft, UIScreen *screen) {
    // 1. Jobb felső sarok - fő funkciókat (függőleges)
    std::vector<UIVerticalButtonBar::ButtonConfig> mainButtons = {{10, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},
                                                                  {11, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
                                                                  {12, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},
                                                                  {13, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr}};

    auto verticalBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(tft.width() - 50, 0, 50, 180), // jobb felső sarok
                                                             mainButtons, 45, 35, 5);
    screen->addChild(verticalBar);

    // 2. Bal alsó sarok - kiegészítő funkciók (vízszintes)
    const uint16_t buttonWidth = 40;
    const uint16_t buttonHeight = 25;
    const uint16_t gap = 2;
    const uint16_t bottomY = tft.height() - buttonHeight;

    std::vector<std::pair<std::string, uint8_t>> secondaryButtons = {{"RDS", 20}, {"Memo", 21}, {"Setup", 22}};

    uint16_t currentX = 0;
    for (const auto &buttonData : secondaryButtons) {
        std::vector<UIVerticalButtonBar::ButtonConfig> config = {
            {buttonData.second, buttonData.first.c_str(), UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr}};

        auto button = std::make_shared<UIVerticalButtonBar>(tft, Rect(currentX, bottomY, buttonWidth, buttonHeight), config, buttonWidth, buttonHeight, 0);
        screen->addChild(button);
        currentX += buttonWidth + gap;
    }
}

// ============================================
// Adaptív pozicionálás
// ============================================

class CornerButtonManager {
  public:
    enum CornerPosition {
        TOP_RIGHT,    // Jobb felső
        TOP_LEFT,     // Bal felső
        BOTTOM_RIGHT, // Jobb alsó
        BOTTOM_LEFT   // Bal alsó
    };

    static Rect getCornerRect(TFT_eSPI &tft, CornerPosition position, uint16_t width, uint16_t height) {
        switch (position) {
            case TOP_RIGHT:
                return Rect(tft.width() - width, 0, width, height);
            case TOP_LEFT:
                return Rect(0, 0, width, height);
            case BOTTOM_RIGHT:
                return Rect(tft.width() - width, tft.height() - height, width, height);
            case BOTTOM_LEFT:
            default:
                return Rect(0, tft.height() - height, width, height);
        }
    }

    static void createCornerButtons(TFT_eSPI &tft, UIScreen *screen, CornerPosition position, const std::vector<UIVerticalButtonBar::ButtonConfig> &configs) {
        uint16_t buttonBarWidth = 65;
        uint16_t buttonBarHeight = 200;

        Rect bounds = getCornerRect(tft, position, buttonBarWidth, buttonBarHeight);

        auto buttonBar = std::make_shared<UIVerticalButtonBar>(tft, bounds, configs, 60, 32, 4);
        screen->addChild(buttonBar);
    }
};

// Használat:
// CornerButtonManager::createCornerButtons(tft, screen,
//     CornerButtonManager::TOP_RIGHT, buttonConfigs);
