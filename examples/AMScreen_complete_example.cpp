// AMScreen implementáció példa a jobb felső sarokba pozicionált függőleges gombokkal
// Ezt a kódot az AMScreen.cpp fájlba kell integrálni

#include "UIVerticalButtonBar.h"

// AMScreen specifikus gomb ID-k
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

class AMScreen : public UIScreen {
  private:
    std::shared_ptr<UIVerticalButtonBar> verticalButtonBar;

  public:
    // Konstruktorban hívjuk meg
    void setupScreen() {
        // ... egyéb komponensek létrehozása ...
        createVerticalButtonBar();
    }

  private:
    /**
     * @brief Függőleges gombsor létrehozása a jobb felső sarokban
     */
    void createVerticalButtonBar() {
        // Gombsor pozíciója - jobb felső sarok
        const uint16_t buttonBarWidth = 65;
        const uint16_t buttonBarX = tft.width() - buttonBarWidth; // Jobb szélhez illesztve
        const uint16_t buttonBarY = 0;                            // Felső szélhez illesztve
        const uint16_t buttonBarHeight = tft.height();            // Teljes képernyő magasság

        // AM specifikus gomb konfiguráció
        std::vector<UIVerticalButtonBar::ButtonConfig> buttonConfigs = {
            {AMScreenButtonIDs::MUTE, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
             [this](const UIButton::ButtonEvent &event) { handleMuteButton(event); }},

            {AMScreenButtonIDs::VOLUME, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
             [this](const UIButton::ButtonEvent &event) { handleVolumeButton(event); }},

            {AMScreenButtonIDs::AGC, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleAGCButton(event); }},

            {AMScreenButtonIDs::ATT, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleAttButton(event); }},

            // AM specifikus: Sávszélesség beállítás
            {AMScreenButtonIDs::BANDWIDTH, "BW", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
             [this](const UIButton::ButtonEvent &event) { handleBandwidthButton(event); }},

            {AMScreenButtonIDs::FREQ, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleFreqButton(event); }},

            {AMScreenButtonIDs::SETUP, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
             [this](const UIButton::ButtonEvent &event) { handleSetupButton(event); }},

            {AMScreenButtonIDs::MEMO, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleMemoButton(event); }}};

        // UIVerticalButtonBar létrehozása
        verticalButtonBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(buttonBarX, buttonBarY, buttonBarWidth, buttonBarHeight), buttonConfigs,
                                                                  60, // gomb szélessége
                                                                  32, // gomb magassága
                                                                  4   // gombok közötti távolság
        );

        // Hozzáadás a képernyőhöz
        addChild(verticalButtonBar);
    }

    // ================================
    // AM specifikus gomb eseménykezelők
    // ================================

    void handleMuteButton(const UIButton::ButtonEvent &event) {
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

    void handleVolumeButton(const UIButton::ButtonEvent &event) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("AMScreen: Volume adjustment requested\n");
            // TODO: Volume dialog megjelenítése
        }
    }

    void handleAGCButton(const UIButton::ButtonEvent &event) {
        if (event.state == UIButton::EventButtonState::On) {
            DEBUG("AMScreen: AGC ON\n");
            // TODO: AM AGC bekapcsolása
            // pSi4735Manager->setAGC(true);
        } else if (event.state == UIButton::EventButtonState::Off) {
            DEBUG("AMScreen: AGC OFF\n");
            // TODO: AM AGC kikapcsolása
            // pSi4735Manager->setAGC(false);
        }
    }

    void handleAttButton(const UIButton::ButtonEvent &event) {
        if (event.state == UIButton::EventButtonState::On) {
            DEBUG("AMScreen: Attenuator ON\n");
            // TODO: Attenuátor bekapcsolása
        } else if (event.state == UIButton::EventButtonState::Off) {
            DEBUG("AMScreen: Attenuator OFF\n");
            // TODO: Attenuátor kikapcsolása
        }
    }

    void handleBandwidthButton(const UIButton::ButtonEvent &event) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("AMScreen: Bandwidth adjustment requested\n");
            // TODO: AM sávszélesség beállító dialógus
            // showBandwidthDialog(); // 0.5kHz, 1kHz, 2kHz, 4kHz opciókat kínálva
        }
    }

    void handleFreqButton(const UIButton::ButtonEvent &event) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("AMScreen: Frequency input requested\n");
            // TODO: Frekvencia input dialógus
        }
    }

    void handleSetupButton(const UIButton::ButtonEvent &event) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("AMScreen: Switching to Setup screen\n");
            UIScreen::getManager()->switchToScreen(SCREEN_NAME_SETUP);
        }
    }

    void handleMemoButton(const UIButton::ButtonEvent &event) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("AMScreen: Memory functions requested\n");
            // TODO: AM memória funkciók
        }
    }

    // Állapot szinkronizálás (a handleOwnLoop() metódusban hívjuk)
    void updateVerticalButtonStates() {
        if (!verticalButtonBar) {
            return;
        }

        // Mute gomb állapot szinkronizálása
        verticalButtonBar->setButtonState(AMScreenButtonIDs::MUTE, rtv::muteStat ? UIButton::ButtonState::On : UIButton::ButtonState::Off);

        // AGC gomb állapot szinkronizálása
        // TODO: Ha van AGC állapot lekérdezés
        // bool agcEnabled = pSi4735Manager->isAGCEnabled();
        // verticalButtonBar->setButtonState(AMScreenButtonIDs::AGC,
        //                                  agcEnabled ? UIButton::ButtonState::On : UIButton::ButtonState::Off);

        // További állapot szinkronizálások...
    }
};
