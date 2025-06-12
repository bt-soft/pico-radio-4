/**
 * @file CommonVerticalButtons.h
 * @brief Közös függőleges gombsor létrehozó és kezelő osztály FM és AM képernyőkhöz
 * @details DIALÓGUS TÁMOGATÁSSAL - Gombkezelők megjelenítehetik a ValueChangeDialog-ot és egyéb dialógusokat
 */

#ifndef __COMMON_VERTICAL_BUTTONS_H
#define __COMMON_VERTICAL_BUTTONS_H

#include "ButtonsGroupManager.h"
#include "Config.h"
#include "IScreenManager.h"
#include "MessageDialog.h"
#include "Si4735Manager.h"
#include "UIButton.h"
#include "UIScreen.h"
#include "ValueChangeDialog.h"
#include "defines.h"
#include "rtVars.h"
#include "utils.h"

// ===================================================================
// UNIVERZÁLIS GOMB AZONOSÍTÓK - Egységes ID rendszer
// ===================================================================

namespace VerticalButtonIDs {
static constexpr uint8_t MUTE = 10;    ///< Némítás gomb (univerzális)
static constexpr uint8_t VOLUME = 11;  ///< Hangerő beállítás gomb (univerzális)
static constexpr uint8_t AGC = 12;     ///< Automatikus erősítés szabályozás (univerzális)
static constexpr uint8_t ATT = 13;     ///< Csillapító (univerzális)
static constexpr uint8_t SQUELCH = 14; ///< Zajzár beállítás (univerzális)
static constexpr uint8_t FREQ = 15;    ///< Frekvencia input (univerzális)
static constexpr uint8_t SETUP = 16;   ///< Beállítások képernyő (univerzális)
static constexpr uint8_t MEMO = 17;    ///< Memória funkciók (univerzális)
} // namespace VerticalButtonIDs

/**
 * @brief Közös függőleges gombsor statikus osztály dialógus támogatással
 * @details Handler függvények képesek dialógusokat megjeleníteni
 */
class CommonVerticalButtons {
  public:
    // =====================================================================
    // HANDLER FÜGGVÉNY TÍPUSOK - UIScreen pointerrel kiegészítve dialógusokhoz
    // =====================================================================
    using Si4735HandlerFunc = void (*)(const UIButton::ButtonEvent &, Si4735Manager *, UIScreen *);
    using ScreenHandlerFunc = void (*)(const UIButton::ButtonEvent &, IScreenManager *, UIScreen *);
    using DialogHandlerFunc = void (*)(const UIButton::ButtonEvent &, Si4735Manager *, UIScreen *);

    /**
     * @brief Gomb statikus adatok struktúrája
     */
    struct ButtonDefinition {
        uint8_t id;                         ///< Gomb azonosító
        const char *label;                  ///< Gomb felirata
        UIButton::ButtonType type;          ///< Gomb típusa
        UIButton::ButtonState initialState; ///< Kezdeti állapot
        uint16_t height;                    ///< Gomb magassága
        Si4735HandlerFunc si4735Handler;    ///< Handler Si4735Manager-rel
        ScreenHandlerFunc screenHandler;    ///< Handler IScreenManager-rel
        DialogHandlerFunc dialogHandler;    ///< Handler dialógusokhoz
    };

    // =====================================================================
    // UNIVERZÁLIS GOMBKEZELŐ METÓDUSOK - Dialógus támogatással
    // =====================================================================

    /**
     * @brief Segédmetódus gombállapot frissítéséhez UIScreen-en keresztül (RTTI-mentes)
     */
    static void updateButtonStateInScreen(UIScreen *screen, uint8_t buttonId, UIButton::ButtonState state) {
        if (!screen)
            return;

        // Vegigmegyünk a screen összes gyerek komponensén
        auto &components = screen->getChildren();
        for (auto &component : components) {

            // Próbáljuk meg UIButton-ként kezelni (raw pointer cast)
            UIButton *button = reinterpret_cast<UIButton *>(component.get());

            // Ellenőrizzük, hogy valóban UIButton-e az ID alapján
            // (Az UIButton ID-k a VerticalButtonIDs tartományban vannak)
            if (button && button->getId() == buttonId) {
                button->setButtonState(state);
                break;
            }
        }
    }

    /**
     * @brief MUTE gomb kezelő
     */
    static void handleMuteButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager, UIScreen *screen = nullptr) {
        if (event.state != UIButton::EventButtonState::On && event.state != UIButton::EventButtonState::Off) {
            return;
        }
        rtv::muteStat = event.state == UIButton::EventButtonState::On;
        si4735Manager->getSi4735().setAudioMute(rtv::muteStat);
    }

    /**
     * @brief VOLUME gomb kezelő - ValueChangeDialog megjelenítése
     */
    static void handleVolumeButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager, UIScreen *screen) {
        if (event.state != UIButton::EventButtonState::Clicked || !screen) {
            return;
        }

        // ValueChangeDialog létrehozása a statikus változó pointerével
        auto volumeDialog = std::make_shared<ValueChangeDialog>(
            screen, screen->getTFT(),                                                  //
            "Volume Control", "Adjust radio volume (0-63):",                           // Cím, felirat
            &config.data.currVolume,                                                   // Pointer a statikus változóra
            Si4735Constants::SI4735_MIN_VOLUME, Si4735Constants::SI4735_MAX_VOLUME, 1, // Min, Max, Step
            [si4735Manager](const std::variant<int, float, bool> &newValue) {          // Callback a változásra
                if (std::holds_alternative<int>(newValue)) {
                    int volume = std::get<int>(newValue);
                    DEBUG("Volume changed to: %d\n", volume);
                    si4735Manager->getSi4735().setVolume(static_cast<uint8_t>(volume));
                }
            },
            nullptr,             // Nincs külön dialog bezárás callback
            Rect(-1, -1, 280, 0) // Auto-magasság
        );
        screen->showDialog(volumeDialog);
    }

    /**
     * @brief AGC gomb kezelő
     */
    static void handleAGCButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager, UIScreen *screen = nullptr) {

        if (event.state != UIButton::EventButtonState::On && event.state != UIButton::EventButtonState::Off) {
            return;
        }

        if (event.state == UIButton::EventButtonState::On) {
            // Az attenuátor gomb OFF állapotba helyezése, ha az AGC be van kapcsolva
            updateButtonStateInScreen(screen, VerticalButtonIDs::ATT, UIButton::ButtonState::Off);
            config.data.agcGain = static_cast<uint8_t>(Si4735Runtime::AgcGainMode::Automatic); // AGC Automatic
        } else if (event.state == UIButton::EventButtonState::Off) {
            config.data.agcGain = static_cast<uint8_t>(Si4735Runtime::AgcGainMode::Off); // AGC OFF
        }

        // AGC beállítása
        si4735Manager->checkAGC();

        // StatusLine update
        if (screen->getStatusLineComp()) {
            screen->getStatusLineComp()->updateAgc();
        }
    }

    /**
     * @brief ATTENUATOR gomb kezelő
     */
    static void handleAttenuatorButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager, UIScreen *screen = nullptr) {
        if (event.state == UIButton::EventButtonState::On) {

            // Az AGC gomb OFF állapotba helyezése, ha az Attenuator be van kapcsolva
            updateButtonStateInScreen(screen, VerticalButtonIDs::AGC, UIButton::ButtonState::Off);

            config.data.agcGain = static_cast<uint8_t>(Si4735Runtime::AgcGainMode::Manual); // AGC ->Manual

            // ValueChangeDialog létrehozása a statikus változó pointerével
            uint8_t maxGain = si4735Manager->isCurrentDemodFM() ? Si4735Constants::SI4735_MAX_ATTENNUATOR_FM : Si4735Constants::SI4735_MAX_ATTENNUATOR_AM;

            auto attDialog = std::make_shared<ValueChangeDialog>(
                screen, screen->getTFT(),                                                 //
                "RF attenuation", "Adjust attenuation:",                                  // Cím, felirat
                &config.data.currentAGCgain,                                              // Pointer a statikus változóra
                Si4735Constants::SI4735_MIN_ATTENNUATOR, maxGain, 1,                      // Min, Max, Step
                [si4735Manager, screen](const std::variant<int, float, bool> &newValue) { // Callback a változásra
                    if (std::holds_alternative<int>(newValue)) {

                        DEBUG("Attenuation changed to: %d\n", std::get<int>(newValue));

                        // AGC beállítása
                        si4735Manager->checkAGC();

                        // StatusLine update
                        if (screen->getStatusLineComp()) {
                            screen->getStatusLineComp()->updateAgc();
                        }
                    }
                },
                nullptr,             // Nincs külön dialog bezárás callback
                Rect(-1, -1, 280, 0) // Auto-magasság
            );
            screen->showDialog(attDialog);

        } else if (event.state == UIButton::EventButtonState::Off) {
            config.data.agcGain = static_cast<uint8_t>(Si4735Runtime::AgcGainMode::Off); // AGC OFF

            // AGC beállítása
            si4735Manager->checkAGC();

            // StatusLine update
            if (screen->getStatusLineComp()) {
                screen->getStatusLineComp()->updateAgc();
            }
        }
    }

    /**
     * @brief FREQUENCY gomb kezelő - ValueChangeDialog megjelenítése
     */
    static void handleFrequencyButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager, UIScreen *screen) {
        if (event.state != UIButton::EventButtonState::Clicked || !screen) {
            return;
        }

        DEBUG("Frequency input dialog requested\n");

        // Band-specifikus frekvencia tartomány (példa FM-hez)
        float minFreq = 87.5f;
        float maxFreq = 108.0f;
        float stepSize = 0.1f;
        // Aktuális frekvencia lekérése és statikus változóba tárolása
        uint16_t currentFreqRaw = si4735Manager->getSi4735().getFrequency();
        static float currentFreq = 100.0f; // Alapértelmezett érték
        currentFreq = static_cast<float>(currentFreqRaw) / 100.0f;

        auto freqDialog = std::make_shared<ValueChangeDialog>(
            screen, screen->getTFT(), "Frequency Input", "Enter frequency (MHz):",
            &currentFreq, // Pointer a statikus változóra
            minFreq, maxFreq, stepSize,
            [si4735Manager](const std::variant<int, float, bool> &newValue) {
                if (std::holds_alternative<float>(newValue)) {
                    float freq = std::get<float>(newValue);
                    uint16_t freqValue = static_cast<uint16_t>(freq * 100);
                    DEBUG("Frequency changed to: %.1f MHz\n", freq);
                    si4735Manager->getSi4735().setFrequency(freqValue);
                }
            },
            nullptr, Rect(-1, -1, 300, 0));
        // Aktuális frekvencia beállítása a konstruktorban történik a valuePtr alapján
        screen->showDialog(freqDialog);
    }

    /**
     * @brief SETUP gomb kezelő
     */
    static void handleSetupButton(const UIButton::ButtonEvent &event, IScreenManager *screenManager, UIScreen *screen = nullptr) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("Switching to Setup screen\n");
            screenManager->switchToScreen(SCREEN_NAME_SETUP);
        }
    }

    /**
     * @brief MEMORY gomb kezelő - MessageDialog megjelenítése
     */
    static void handleMemoryButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager, UIScreen *screen) {
        if (event.state != UIButton::EventButtonState::Clicked || !screen) {
            return;
        }

        DEBUG("Memory functions dialog requested\n");
        // Egyszerű tájékoztató dialógus - használjuk a MessageDialog ButtonsType::Ok-ot
        auto messageDialog = std::make_shared<MessageDialog>(
            screen, screen->getTFT(), Rect(-1, -1, 300, 0), // Auto-magasság
            "Memory Functions",
            "Memory management not yet implemented.\n\nPlanned features:\n- Save current frequency\n- Load saved stations\n- Edit station names\n- Delete stations",
            MessageDialog::ButtonsType::Ok);

        screen->showDialog(messageDialog);
    }

    /**
     * @brief SQUELCH gomb kezelő - ValueChangeDialog megjelenítése
     */
    static void handleSquelchButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager, UIScreen *screen) {
        if (event.state != UIButton::EventButtonState::Clicked || !screen) {
            return;
        }

        DEBUG("Squelch adjustment dialog requested\n");
        // Squelch beállítás statikus változóval
        int minSquelch = 0;
        int maxSquelch = 127;           // FM alapértelmezett
        static int currentSquelch = 20; // Statikus alapértelmezett érték

        auto squelchDialog = std::make_shared<ValueChangeDialog>(
            screen, screen->getTFT(), "Squelch Control", "Adjust squelch level (0=off):",
            &currentSquelch, // Pointer a statikus változóra
            minSquelch, maxSquelch, 1,
            [si4735Manager](const std::variant<int, float, bool> &newValue) {
                if (std::holds_alternative<int>(newValue)) {
                    int squelch = std::get<int>(newValue);
                    DEBUG("Squelch changed to: %d\n", squelch);
                    // TODO: si4735Manager->setSquelch(squelch);
                }
            },
            nullptr, Rect(-1, -1, 280, 0));
        // Aktuális squelch beállítása a konstruktorban történik a valuePtr alapján
        screen->showDialog(squelchDialog);
    }

    /**
     * @brief Központi gomb definíciók
     */
    static const std::vector<ButtonDefinition> &getButtonDefinitions() {
        static const std::vector<ButtonDefinition> BUTTON_DEFINITIONS = {
            // ID, Label, Type, InitialState, Height, Si4735Handler, ScreenHandler, DialogHandler
            {VerticalButtonIDs::MUTE, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, 32, handleMuteButton, nullptr, nullptr},
            {VerticalButtonIDs::VOLUME, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, 32, nullptr, nullptr, handleVolumeButton},
            {VerticalButtonIDs::AGC, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, 32, handleAGCButton, nullptr, nullptr},
            {VerticalButtonIDs::ATT, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, 32, handleAttenuatorButton, nullptr, nullptr},
            {VerticalButtonIDs::SQUELCH, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, 32, nullptr, nullptr, handleSquelchButton},
            {VerticalButtonIDs::FREQ, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, 32, nullptr, nullptr, handleFrequencyButton},
            {VerticalButtonIDs::SETUP, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, 32, nullptr, handleSetupButton, nullptr},
            {VerticalButtonIDs::MEMO, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, 32, nullptr, nullptr, handleMemoryButton}};
        return BUTTON_DEFINITIONS;
    }

    // =====================================================================
    // FACTORY METÓDUSOK
    // =====================================================================

    /**
     * @brief Maximális gombszélesség kalkuláció
     */
    template <typename TFTType> static uint16_t calculateUniformButtonWidth(TFTType &tft, uint16_t buttonHeight = 32) {
        uint16_t maxWidth = 0;
        const auto &buttonDefs = getButtonDefinitions();

        for (const auto &def : buttonDefs) {
            uint16_t width = UIButton::calculateWidthForText(tft, def.label, false, buttonHeight);
            maxWidth = std::max(maxWidth, width);
        }
        return maxWidth;
    }

  private:
    /**
     * @brief Belső gombdefiníció létrehozó metódus
     */
    static std::vector<ButtonGroupDefinition> createButtonDefinitionsInternal(Si4735Manager *si4735Manager, IScreenManager *screenManager, UIScreen *screen, uint16_t buttonWidth) {

        std::vector<ButtonGroupDefinition> definitions;
        const auto &buttonDefs = getButtonDefinitions();
        definitions.reserve(buttonDefs.size());

        for (const auto &def : buttonDefs) {
            std::function<void(const UIButton::ButtonEvent &)> callback;

            if (def.si4735Handler != nullptr) {
                callback = [si4735Manager, screen, handler = def.si4735Handler](const UIButton::ButtonEvent &e) { handler(e, si4735Manager, screen); };
            } else if (def.screenHandler != nullptr) {
                callback = [screenManager, screen, handler = def.screenHandler](const UIButton::ButtonEvent &e) { handler(e, screenManager, screen); };
            } else if (def.dialogHandler != nullptr) {
                callback = [si4735Manager, screen, handler = def.dialogHandler](const UIButton::ButtonEvent &e) { handler(e, si4735Manager, screen); };
            } else {
                callback = [](const UIButton::ButtonEvent &e) { /* no-op */ };
            }

            definitions.push_back({def.id, def.label, def.type, callback, def.initialState, buttonWidth, def.height});
        }

        return definitions;
    }

  public:
    /**
     * @brief Gombdefiníciók létrehozása automatikus szélességgel
     */
    static std::vector<ButtonGroupDefinition> createButtonDefinitions(Si4735Manager *si4735Manager, IScreenManager *screenManager, UIScreen *screen) {
        return createButtonDefinitionsInternal(si4735Manager, screenManager, screen, 0);
    }

    /**
     * @brief Egységes szélességű gombdefiníciók létrehozása
     */
    template <typename TFTType>
    static std::vector<ButtonGroupDefinition> createUniformButtonDefinitions(Si4735Manager *si4735Manager, IScreenManager *screenManager, UIScreen *screen, TFTType &tft) {
        uint16_t uniformWidth = calculateUniformButtonWidth(tft, 32);
        return createButtonDefinitionsInternal(si4735Manager, screenManager, screen, uniformWidth);
    }

    // =====================================================================
    // MIXIN TEMPLATE - Screen osztályok kiegészítéséhez
    // =====================================================================

    template <typename ScreenType> class Mixin : public ButtonsGroupManager<ScreenType> {
      protected:
        std::vector<std::shared_ptr<UIButton>> createdVerticalButtons;

        /**
         * @brief Közös függőleges gombok létrehozása egységes szélességgel
         */
        void createCommonVerticalButtons(Si4735Manager *si4735Manager, IScreenManager *screenManager) {
            ScreenType *self = static_cast<ScreenType *>(this);
            auto buttonDefs = CommonVerticalButtons::createUniformButtonDefinitions(si4735Manager, screenManager, self, self->getTFT());
            ButtonsGroupManager<ScreenType>::layoutVerticalButtonGroup(buttonDefs, &createdVerticalButtons, 0, 0, 5, 60, 32, 3, 4);
        }

        /**
         *
         */
        void updateVerticalButtonState(uint8_t buttonId, UIButton::ButtonState state) {
            for (auto &button : createdVerticalButtons) {
                if (button && button->getId() == buttonId) {
                    button->setButtonState(state);
                    break;
                }
            }
        }

        void updateAllVerticalButtonStates(Si4735Manager *si4735Manager) {
            updateVerticalButtonState(VerticalButtonIDs::MUTE, rtv::muteStat ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
        }
    };
}; // CommonVerticalButtons osztály bezárása

#endif // __COMMON_VERTICAL_BUTTONS_H
