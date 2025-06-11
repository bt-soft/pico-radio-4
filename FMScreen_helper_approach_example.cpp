// Alternatív megoldás: Helper funkció megközelítés
// Ez egyszerűbb, ha nem szeretnél új komponenst létrehozni

#include "FMScreen.h"
#include "UIButton.h"

// Gomb ID-k konstansai
namespace FMButtonIDs {
static constexpr uint8_t MUTE = 20;
static constexpr uint8_t VOLUME = 21;
static constexpr uint8_t AGC = 22;
static constexpr uint8_t ATT = 23;
static constexpr uint8_t SQUELCH = 24;
static constexpr uint8_t FREQ = 25;
static constexpr uint8_t SETUP = 26;
static constexpr uint8_t MEMO = 27;
} // namespace FMButtonIDs

void FMScreen::layoutComponents() {

    // ... egyéb komponensek ...

    // Függőleges gombok létrehozása helper funkcióval
    createVerticalButtonsHelper();

    // ... egyéb komponensek ...
}

void FMScreen::createVerticalButtonsHelper() {

    // Alapbeállítások
    const uint16_t buttonWidth = 60;
    const uint16_t buttonHeight = 32;
    const uint16_t buttonGap = 4;
    const uint16_t rightMargin = 5;

    // Pozíció számítás
    const uint16_t startX = tft.width() - buttonWidth - rightMargin;
    const uint16_t startY = 80; // StatusLine és FreqDisplay után

    uint16_t currentY = startY;

    // Gomb struktúra a könnyebb kezeléshez
    struct ButtonDef {
        uint8_t id;
        const char *label;
        UIButton::ButtonType type;
        UIButton::ButtonState initialState;
        std::function<void(const UIButton::ButtonEvent &)> callback;
    };

    std::vector<ButtonDef> buttonDefs = {
        {FMButtonIDs::MUTE, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](auto &event) { handleMuteButton(event); }},

        {FMButtonIDs::VOLUME, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleVolumeButton(event); }},

        {FMButtonIDs::AGC, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](auto &event) { handleAGCButton(event); }},

        {FMButtonIDs::ATT, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](auto &event) { handleAttButton(event); }},

        {FMButtonIDs::SQUELCH, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleSquelchButton(event); }},

        {FMButtonIDs::FREQ, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleFreqButton(event); }},

        {FMButtonIDs::SETUP, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleSetupButton(event); }},

        {FMButtonIDs::MEMO, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](auto &event) { handleMemoButton(event); }}};

    // Gombok létrehozása egy ciklusban
    for (const auto &buttonDef : buttonDefs) {

        // Ellenőrizzük, hogy belefér-e még a képernyőre
        if (currentY + buttonHeight > tft.height() - 50) { // 50px margó alulról
            DEBUG("FMScreen: Not enough space for button '%s'\n", buttonDef.label);
            break;
        }

        auto button = std::make_shared<UIButton>(tft, buttonDef.id, Rect(startX, currentY, buttonWidth, buttonHeight), buttonDef.label, buttonDef.type, buttonDef.initialState,
                                                 buttonDef.callback);

        // Opcionális: kisebb font használata a függőleges gombokhoz
        button->setUseMiniFont(true);

        // Hozzáadás a képernyőhöz
        addChild(button);

        // Tárolás a könnyebb eléréshez (opcionális)
        verticalButtons[buttonDef.id] = button;

        // Következő gomb pozíciója
        currentY += buttonHeight + buttonGap;
    }
}

// Segédfunkció gomb állapot beállításához ID alapján
void FMScreen::setVerticalButtonState(uint8_t buttonId, UIButton::ButtonState state) {
    auto it = verticalButtons.find(buttonId);
    if (it != verticalButtons.end()) {
        it->second->setButtonState(state);
    } else {
        DEBUG("FMScreen: Vertical button with ID %d not found\n", buttonId);
    }
}

// Segédfunkció gomb állapot lekérdezéséhez ID alapján
UIButton::ButtonState FMScreen::getVerticalButtonState(uint8_t buttonId) const {
    auto it = verticalButtons.find(buttonId);
    if (it != verticalButtons.end()) {
        return it->second->getButtonState();
    }
    DEBUG("FMScreen: Vertical button with ID %d not found\n", buttonId);
    return UIButton::ButtonState::Disabled;
}

// Státusz frissítés a loop-ban
void FMScreen::handleOwnLoop() {

    // S-Meter frissítése (eredeti kód)
    if (smeterComp) {
        SignalQualityData signalCache = pSi4735Manager->getSignalQuality();
        if (signalCache.isValid) {
            smeterComp->showRSSI(signalCache.rssi, signalCache.snr, true);
        }
    }

    // Gombállapotok frissítése (új)
    updateVerticalButtonStates();
}

void FMScreen::updateVerticalButtonStates() {
    // Mute gomb állapot szinkronizálása
    bool isMuted = pSi4735Manager->isMuted();
    setVerticalButtonState(FMButtonIDs::MUTE, isMuted ? UIButton::ButtonState::On : UIButton::ButtonState::Off);

    // AGC gomb állapot szinkronizálása
    bool agcEnabled = pSi4735Manager->isAGCEnabled();
    setVerticalButtonState(FMButtonIDs::AGC, agcEnabled ? UIButton::ButtonState::On : UIButton::ButtonState::Off);

    // További gombállapotok...
}
