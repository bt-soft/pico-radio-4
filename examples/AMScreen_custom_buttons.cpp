// Példa: AM képernyő egyedi gomb konfiguráció
// Bizonyos gombok elhagyása/módosítása

void AMScreen::createVerticalButtonBar() {

    // Alapértelmezett pozíció
    const uint16_t buttonBarWidth = 65;
    const uint16_t buttonBarX = tft.width() - buttonBarWidth - 5;
    const uint16_t buttonBarY = 80;
    const uint16_t buttonBarHeight = 200;

    // AM specifikus gomb konfiguráció - bizonyos gombok elhagyva
    std::vector<UIVerticalButtonBar::ButtonConfig> buttonConfigs = {
        // Megtartott gombok az FM-ből
        {AMScreenButtonIDs::MUTE, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleMuteButton(event); }},

        {AMScreenButtonIDs::VOLUME, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleVolumeButton(event); }},

        // Squelch gomb ELHAGYVA - AM-ben nincs squelch
        // {AMScreenButtonIDs::SQUELCH, "Sql", ...} - NEM SZEREPEL

        // AM specifikus új gomb - Bandwidth
        {AMScreenButtonIDs::BANDWIDTH, "BW", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleBandwidthButton(event); }},

        // AGC megtartva
        {AMScreenButtonIDs::AGC, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleAGCButton(event); }},

        // Attenuator ELHAGYVA - AM-ben ritkán használt
        // {AMScreenButtonIDs::ATT, "Att", ...} - NEM SZEREPEL

        {AMScreenButtonIDs::FREQ, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleFreqButton(event); }},

        {AMScreenButtonIDs::SETUP, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleSetupButtonVertical(event); }},

        {AMScreenButtonIDs::MEMO, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleMemoButton(event); }}};

    // Kevesebb gombbal létrehozás
    verticalButtonBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(buttonBarX, buttonBarY, buttonBarWidth, buttonBarHeight),
                                                              buttonConfigs, // Ez a vektor határozza meg, hogy mely gombok jelennek meg
                                                              60, 32, 4);

    addChild(verticalButtonBar);
}
