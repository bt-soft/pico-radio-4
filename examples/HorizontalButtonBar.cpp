// Horizontális gombsor példa (alul elhelyezve)
#include "UIVerticalButtonBar.h"

void createHorizontalBottomButtons(TFT_eSPI &tft, UIScreen *screen) {
    // Horizontális gombsor bal alsó sarok - több kisebb UIVerticalButtonBar használatával
    // Minden gomb külön "oszlop" lesz

    const uint16_t buttonWidth = 45;
    const uint16_t buttonHeight = 30;
    const uint16_t gap = 3;
    const uint16_t bottomY = tft.height() - buttonHeight; // Pontosan az alsó szélhez illesztve
    const uint16_t startX = 0;                            // Pontosan a bal szélhez illesztve

    // Minden gomb külön UIVerticalButtonBar lesz 1 gombbal
    std::vector<std::pair<std::string, std::function<void(const UIButton::ButtonEvent &)>>> horizontalButtons = {
        {"Mute", [](const UIButton::ButtonEvent &event) { DEBUG("Horizontal Mute\n"); }},
        {"Vol", [](const UIButton::ButtonEvent &event) { DEBUG("Horizontal Volume\n"); }},
        {"AGC", [](const UIButton::ButtonEvent &event) { DEBUG("Horizontal AGC\n"); }},
        {"Att", [](const UIButton::ButtonEvent &event) { DEBUG("Horizontal Att\n"); }},
        {"Sql", [](const UIButton::ButtonEvent &event) { DEBUG("Horizontal Squelch\n"); }},
        {"Freq", [](const UIButton::ButtonEvent &event) { DEBUG("Horizontal Freq\n"); }},
        {"Setup", [](const UIButton::ButtonEvent &event) { UIScreen::getManager()->switchToScreen(SCREEN_NAME_SETUP); }}};

    uint16_t currentX = startX;
    uint8_t buttonId = 50; // Kezdő ID

    for (const auto &buttonData : horizontalButtons) {
        // Egyetlen gomb konfigurációja
        std::vector<UIVerticalButtonBar::ButtonConfig> singleButtonConfig = {
            {buttonId++, buttonData.first.c_str(), UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, buttonData.second}};

        // "Függőleges" gombsor 1 gombbal = horizontális gomb
        auto singleButton = std::make_shared<UIVerticalButtonBar>(tft, Rect(currentX, bottomY, buttonWidth, buttonHeight), singleButtonConfig, buttonWidth, buttonHeight,
                                                                  0 // 0 gap mert csak 1 gomb van
        );

        screen->addChild(singleButton);
        currentX += buttonWidth + gap;
    }
}

// ============================================
// Valódi horizontális gombsor osztály
// ============================================

class UIHorizontalButtonBar : public UIContainerComponent {
  public:
    struct ButtonConfig {
        uint8_t id;
        const char *label;
        UIButton::ButtonType type;
        UIButton::ButtonState initialState;
        std::function<void(const UIButton::ButtonEvent &)> callback;
    };

    UIHorizontalButtonBar(TFT_eSPI &tft, const Rect &bounds, const std::vector<ButtonConfig> &buttonConfigs, uint16_t buttonWidth = 45, uint16_t buttonHeight = 30,
                          uint16_t buttonGap = 3)
        : UIContainerComponent(tft, bounds), buttonWidth(buttonWidth), buttonHeight(buttonHeight), buttonGap(buttonGap) {
        createButtons(buttonConfigs);
    }

  private:
    uint16_t buttonWidth;
    uint16_t buttonHeight;
    uint16_t buttonGap;
    std::vector<std::shared_ptr<UIButton>> buttons;

    void createButtons(const std::vector<ButtonConfig> &buttonConfigs) {
        uint16_t currentX = bounds.x;

        for (const auto &config : buttonConfigs) {
            // Ellenőrizzük, hogy a gomb még belefér-e
            if (currentX + buttonWidth > bounds.x + bounds.width) {
                DEBUG("UIHorizontalButtonBar: Button '%s' doesn't fit, skipping\n", config.label);
                break;
            }

            // Gomb létrehozása
            auto button =
                std::make_shared<UIButton>(tft, config.id, Rect(currentX, bounds.y, buttonWidth, buttonHeight), config.label, config.type, config.initialState, config.callback);

            addChild(button);
            buttons.push_back(button);

            currentX += buttonWidth + buttonGap;
        }
    }
};

// Használat:
void createTrueHorizontalButtons(TFT_eSPI &tft, UIScreen *screen) {
    std::vector<UIHorizontalButtonBar::ButtonConfig> horizontalConfigs = {
        {60, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr}, {61, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
        {62, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},  {63, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},
        {64, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},    {65, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
        {66, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr}};

    auto horizontalBar = std::make_shared<UIHorizontalButtonBar>(tft, Rect(0, tft.height() - 30, 300, 30), // bal alsó sarok
                                                                 horizontalConfigs, 45, 30, 3              // gomb méretek és gap
    );

    screen->addChild(horizontalBar);
}

// ============================================
// Kombinált megoldás: függőleges + horizontális
// ============================================

void createCombinedButtonLayout(TFT_eSPI &tft, UIScreen *screen) {
    // 1. Függőleges gombsor jobb felső sarok (főbb funkciók)
    std::vector<UIVerticalButtonBar::ButtonConfig> mainButtons = {{10, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},
                                                                  {11, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
                                                                  {12, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},
                                                                  {13, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr}};

    auto verticalBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(tft.width() - 45, 0, 45, 160), mainButtons, 40, 35, 5);
    screen->addChild(verticalBar);

    // 2. Horizontális gombsor bal alsó sarok (ritkábban használt funkciók)
    std::vector<UIHorizontalButtonBar::ButtonConfig> secondaryButtons = {{20, "RDS", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, nullptr},
                                                                         {21, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr},
                                                                         {22, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, nullptr}};

    auto horizontalBar = std::make_shared<UIHorizontalButtonBar>(tft, Rect(0, tft.height() - 30, 200, 30), secondaryButtons, 45, 25, 5);
    screen->addChild(horizontalBar);
}
