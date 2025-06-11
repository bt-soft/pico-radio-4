/*
 * GYORS REFERENCIA - Gombpozicionálás
 * ===================================
 */

// 1. FÜGGŐLEGES GOMBOK - Jobb felső sarok
auto verticalBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(tft.width() - 65, 0, 65, tft.height()), // x, y, width, height
                                                         buttonConfigs, 60, 32, 4                          // btn_w, btn_h, gap
);

// 2. VÍZSZINTES GOMBOK - Bal alsó sarok (1 gombos UIVerticalButtonBar-okkal)
const uint16_t bottomY = tft.height() - 30;
const uint16_t startX = 0;
uint16_t currentX = startX;

for (const auto &buttonData : horizontalButtons) {
    std::vector<UIVerticalButtonBar::ButtonConfig> config = {{id++, buttonData.label, UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, buttonData.callback}};

    auto button = std::make_shared<UIVerticalButtonBar>(tft, Rect(currentX, bottomY, 45, 30), config, 45, 30, 0);
    screen->addChild(button);
    currentX += 45 + 3; // width + gap
}

// 3. GOMB KONFIGURÁCIÓ PÉLDÁK
std::vector<UIVerticalButtonBar::ButtonConfig> configs = {
    // Toggleable gombok (on/off állapottal)
    {10, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, callback},
    {11, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, callback},

    // Pushable gombok (kattintás esemény)
    {20, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, callback},
    {21, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, callback}};

// 4. CALLBACK PÉLDÁK
[this](const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        // Pushable gomb kattintás
        DEBUG("Button clicked\n");
    } else if (event.state == UIButton::EventButtonState::On) {
        // Toggleable gomb bekapcsolva
        DEBUG("Button ON\n");
    } else if (event.state == UIButton::EventButtonState::Off) {
        // Toggleable gomb kikapcsolva
        DEBUG("Button OFF\n");
    }
}

// 5. DINAMIKUS GOMBKEZELÉS
buttonBar->addButton({50, "Extra", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, callback});
buttonBar->removeButton(50);
buttonBar->setButtonVisible(10, false);
buttonBar->setButtonState(10, UIButton::ButtonState::On);
buttonBar->relayoutButtons();

// 6. POZÍCIÓ SZÁMÍTÁSOK
// Jobb felső sarok:
Rect(tft.width() - width, 0, width, height)

    // Bal felső sarok:
    Rect(0, 0, width, height)

    // Jobb alsó sarok:
    Rect(tft.width() - width, tft.height() - height, width, height)

    // Bal alsó sarok:
    Rect(0, tft.height() - height, width, height)

    // 7. MÉRETEK AJÁNLÁSOK
    // Függőleges gombsor: 65x240 (8 gomb esetén)
    // Egy gomb: 60x32 + 4px gap
    // Vízszintes gombsor: 300x30 (7 gomb esetén)
    // Egy gomb: 45x30 + 3px gap
