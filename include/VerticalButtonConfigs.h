// Közös gomb definíciók és konfiguráció a függőleges gombsorokhoz
// Ez a fájl tartalmazza a gyakran használt gomb konfigurációkat

#ifndef __VERTICAL_BUTTON_CONFIGS_H
#define __VERTICAL_BUTTON_CONFIGS_H

#include "UIVerticalButtonBar.h"
#include "rtVars.h"

/**
 * @brief Közös gomb ID namespace-ek elkerülése érdekében
 */
namespace VerticalButtonIDs {
// FM Screen gombok (10-19)
namespace FM {
static constexpr uint8_t MUTE = 10;
static constexpr uint8_t VOLUME = 11;
static constexpr uint8_t AGC = 12;
static constexpr uint8_t ATT = 13;
static constexpr uint8_t SQUELCH = 14;
static constexpr uint8_t FREQ = 15;
static constexpr uint8_t SETUP = 16;
static constexpr uint8_t MEMO = 17;
} // namespace FM

// AM Screen gombok (20-29)
namespace AM {
static constexpr uint8_t MUTE = 20;
static constexpr uint8_t VOLUME = 21;
static constexpr uint8_t AGC = 22;
static constexpr uint8_t ATT = 23;
static constexpr uint8_t BANDWIDTH = 24;
static constexpr uint8_t FREQ = 25;
static constexpr uint8_t SETUP = 26;
static constexpr uint8_t MEMO = 27;
} // namespace AM

// SSB Screen gombok (30-39)
namespace SSB {
static constexpr uint8_t MUTE = 30;
static constexpr uint8_t VOLUME = 31;
static constexpr uint8_t AGC = 32;
static constexpr uint8_t ATT = 33;
static constexpr uint8_t BFO = 34;
static constexpr uint8_t BANDWIDTH = 35;
static constexpr uint8_t FREQ = 36;
static constexpr uint8_t SETUP = 37;
static constexpr uint8_t MEMO = 38;
} // namespace SSB
} // namespace VerticalButtonIDs

/**
 * @brief Közös gomb pozíció konstansok
 */
namespace VerticalButtonLayout {
static constexpr uint16_t DEFAULT_BUTTON_WIDTH = 60;
static constexpr uint16_t DEFAULT_BUTTON_HEIGHT = 32;
static constexpr uint16_t DEFAULT_BUTTON_GAP = 4;
static constexpr uint16_t DEFAULT_RIGHT_MARGIN = 5;
static constexpr uint16_t DEFAULT_BAR_WIDTH = 65;
static constexpr uint16_t DEFAULT_START_Y = 80;
static constexpr uint16_t DEFAULT_BAR_HEIGHT = 200;
} // namespace VerticalButtonLayout

/**
 * @brief Gyakran használt gomb konfiguráció helper függvények
 */
class VerticalButtonConfigHelper {
  public:
    /**
     * @brief Mute gomb konfiguráció létrehozása
     */
    template <typename CallbackType> static UIVerticalButtonBar::ButtonConfig createMuteButton(uint8_t id, CallbackType callback) {
        return {id, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, callback};
    }

    /**
     * @brief Volume gomb konfiguráció létrehozása
     */
    template <typename CallbackType> static UIVerticalButtonBar::ButtonConfig createVolumeButton(uint8_t id, CallbackType callback) {
        return {id, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, callback};
    }

    /**
     * @brief AGC gomb konfiguráció létrehozása
     */
    template <typename CallbackType> static UIVerticalButtonBar::ButtonConfig createAGCButton(uint8_t id, CallbackType callback) {
        return {id, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, callback};
    }

    /**
     * @brief Attenuator gomb konfiguráció létrehozása
     */
    template <typename CallbackType> static UIVerticalButtonBar::ButtonConfig createAttButton(uint8_t id, CallbackType callback) {
        return {id, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, callback};
    }

    /**
     * @brief Frequency gomb konfiguráció létrehozása
     */
    template <typename CallbackType> static UIVerticalButtonBar::ButtonConfig createFreqButton(uint8_t id, CallbackType callback) {
        return {id, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, callback};
    }

    /**
     * @brief Setup gomb konfiguráció létrehozása
     */
    template <typename CallbackType> static UIVerticalButtonBar::ButtonConfig createSetupButton(uint8_t id, CallbackType callback) {
        return {id, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, callback};
    }

    /**
     * @brief Memory gomb konfiguráció létrehozása
     */
    template <typename CallbackType> static UIVerticalButtonBar::ButtonConfig createMemoButton(uint8_t id, CallbackType callback) {
        return {id, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, callback};
    }

    /**
     * @brief Alapértelmezett pozíció számítása a képernyő szélességéből
     */
    static Rect calculateDefaultPosition(uint16_t screenWidth) {
        uint16_t x = screenWidth - VerticalButtonLayout::DEFAULT_BAR_WIDTH - VerticalButtonLayout::DEFAULT_RIGHT_MARGIN;
        return Rect(x, VerticalButtonLayout::DEFAULT_START_Y, VerticalButtonLayout::DEFAULT_BAR_WIDTH, VerticalButtonLayout::DEFAULT_BAR_HEIGHT);
    }
};

/**
 * @brief Közös mute gomb eseménykezelő
 */
class CommonVerticalButtonHandlers {
  public:
    /**
     * @brief Általános mute gomb kezelő
     */
    static void handleMuteButton(const UIButton::ButtonEvent &event, Si4735Manager *manager) {
        if (event.state == UIButton::EventButtonState::On) {
            DEBUG("Mute ON\n");
            rtv::muteStat = true;
            manager->getSi4735().setAudioMute(true);
        } else if (event.state == UIButton::EventButtonState::Off) {
            DEBUG("Mute OFF\n");
            rtv::muteStat = false;
            manager->getSi4735().setAudioMute(false);
        }
    }

    /**
     * @brief Általános setup gomb kezelő
     */
    static void handleSetupButton(const UIButton::ButtonEvent &event, IScreenManager *manager) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("Switching to Setup screen\n");
            manager->switchToScreen(SCREEN_NAME_SETUP);
        }
    }

    /**
     * @brief Mute állapot szinkronizálás
     */
    static void updateMuteButtonState(UIVerticalButtonBar *buttonBar, uint8_t muteButtonId) {
        if (buttonBar) {
            buttonBar->setButtonState(muteButtonId, rtv::muteStat ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
        }
    }
};

#endif // __VERTICAL_BUTTON_CONFIGS_H
