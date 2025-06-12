/**
 * @file CommonVerticalButtons_Refactored.h
 * @brief ButtonsGroupManager alapú refaktorált CommonVerticalButtons osztály
 * @details A CommonVerticalButtons átalakítva ButtonsGroupManager használatára
 *          az eredeti funkciók megtartásával
 *
 * **Refaktorálás célja**:
 * - ButtonsGroupManager template rendszer kihasználása
 * - Kód duplikáció további csökkentése
 * - Egységes gombkezelési architektúra
 * - UIVerticalButtonBar helyett ButtonsGroupManager használata
 *
 * @author Rádió projekt
 * @version 4.0 - ButtonsGroupManager integráció (2025.06.12)
 */

#ifndef __COMMON_VERTICAL_BUTTONS_REFACTORED_H
#define __COMMON_VERTICAL_BUTTONS_REFACTORED_H

#include "ButtonsGroupManager.h"
#include "IScreenManager.h"
#include "Si4735Manager.h"
#include "UIButton.h"
#include "UIContainerComponent.h"
#include "defines.h"
#include "rtVars.h"
#include "utils.h"

// ===================================================================
// UNIVERZÁLIS GOMB AZONOSÍTÓK - Egységes ID rendszer (változatlan)
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
 * @brief ButtonsGroupManager alapú CommonVerticalButtons implementáció
 * @details Mixin pattern használatával integrálja a ButtonsGroupManager-t
 *          a screen osztályokba, megtartva az eredeti funkcionalitást
 */
class CommonVerticalButtons {
  public:
    // =====================================================================
    // GOMBDEFINÍCIÓK KÉSZÍTÉSE - ButtonGroupDefinition formátumban
    // =====================================================================

    /**
     * @brief Függőleges gombok definíciójának előkészítése ButtonsGroupManager-hez
     * @param screen Screen objektum referencia (callback-ekhez)
     * @param si4735Manager Si4735Manager referencia
     * @param screenManager IScreenManager referencia
     * @return ButtonGroupDefinition vektor a ButtonsGroupManager számára
     */
    static std::vector<ButtonGroupDefinition> createButtonDefinitions(UIScreen *screen, Si4735Manager *si4735Manager, IScreenManager *screenManager) {

        return {// 1. NÉMÍTÁS - Kapcsolható gomb (BE/KI állapottal)
                {
                    VerticalButtonIDs::MUTE, "Mute", UIButton::ButtonType::Toggleable, [si4735Manager](const UIButton::ButtonEvent &e) { handleMuteButton(e, si4735Manager); },
                    UIButton::ButtonState::Off,
                    0, // Auto width
                    32 // Fixed height
                },

                // 2. HANGERŐ - Nyomógomb (dialógus megnyitás)
                {
                    VerticalButtonIDs::VOLUME, "Vol", UIButton::ButtonType::Pushable, [si4735Manager](const UIButton::ButtonEvent &e) { handleVolumeButton(e, si4735Manager); },
                    UIButton::ButtonState::Off,
                    0, // Auto width
                    32 // Fixed height
                },

                // 3. AGC - Kapcsolható gomb (Automatikus erősítésszabályozás)
                {
                    VerticalButtonIDs::AGC, "AGC", UIButton::ButtonType::Toggleable, [si4735Manager](const UIButton::ButtonEvent &e) { handleAGCButton(e, si4735Manager); },
                    UIButton::ButtonState::Off,
                    0, // Auto width
                    32 // Fixed height
                },

                // 4. CSILLAPÍTÓ - Kapcsolható gomb (Jel csillapítás)
                {
                    VerticalButtonIDs::ATT, "Att", UIButton::ButtonType::Toggleable, [si4735Manager](const UIButton::ButtonEvent &e) { handleAttenuatorButton(e, si4735Manager); },
                    UIButton::ButtonState::Off,
                    0, // Auto width
                    32 // Fixed height
                },

                // 5. ZAJZÁR - Nyomógomb (Zajzár beállító dialógus)
                {
                    VerticalButtonIDs::SQUELCH, "Sql", UIButton::ButtonType::Pushable, [si4735Manager](const UIButton::ButtonEvent &e) { handleSquelchButton(e, si4735Manager); },
                    UIButton::ButtonState::Off,
                    0, // Auto width
                    32 // Fixed height
                },

                // 6. FREKVENCIA - Nyomógomb (Frekvencia input dialógus)
                {
                    VerticalButtonIDs::FREQ, "Freq", UIButton::ButtonType::Pushable, [si4735Manager](const UIButton::ButtonEvent &e) { handleFrequencyButton(e, si4735Manager); },
                    UIButton::ButtonState::Off,
                    0, // Auto width
                    32 // Fixed height
                },

                // 7. BEÁLLÍTÁSOK - Nyomógomb (Beállítások képernyőre váltás)
                {
                    VerticalButtonIDs::SETUP, "Setup", UIButton::ButtonType::Pushable, [screenManager](const UIButton::ButtonEvent &e) { handleSetupButton(e, screenManager); },
                    UIButton::ButtonState::Off,
                    0, // Auto width
                    32 // Fixed height
                },

                // 8. MEMÓRIA - Nyomógomb (Memória funkciók dialógus)
                {
                    VerticalButtonIDs::MEMO, "Memo", UIButton::ButtonType::Pushable, [si4735Manager](const UIButton::ButtonEvent &e) { handleMemoryButton(e, si4735Manager); },
                    UIButton::ButtonState::Off,
                    0, // Auto width
                    32 // Fixed height
                }};
    }

    // =====================================================================
    // HELPER TEMPLATE MIXIN - Screen osztályok kiegészítéséhez
    // =====================================================================

    /**
     * @brief Template mixin ButtonsGroupManager integrációhoz
     * @tparam ScreenType A konkrét screen osztály (CRTP)
     * @details Ez a template lehetővé teszi, hogy bármely UIScreen alapú
     *          osztály könnyen használhassa a CommonVerticalButtons-t
     *          ButtonsGroupManager segítségével
     */
    template <typename ScreenType> class Mixin : public ButtonsGroupManager<ScreenType> {
      protected:
        std::vector<std::shared_ptr<UIButton>> createdVerticalButtons;

        /**
         * @brief Függőleges gombok létrehozása ButtonsGroupManager-rel
         * @param si4735Manager Si4735Manager referencia
         * @param screenManager IScreenManager referencia
         * @param marginRight Jobb margó (alapértelmezett: 5)
         * @param marginTop Felső margó (alapértelmezett: 5)
         * @param marginBottom Alsó margó (alapértelmezett: 5)
         */
        void createCommonVerticalButtons(Si4735Manager *si4735Manager, IScreenManager *screenManager, int16_t marginRight = 5, int16_t marginTop = 5, int16_t marginBottom = 5) {

            ScreenType *self = static_cast<ScreenType *>(this);

            // Gombdefiníciók előkészítése
            auto buttonDefs = CommonVerticalButtons::createButtonDefinitions(self, si4735Manager, screenManager);

            // ButtonsGroupManager layoutVerticalButtonGroup használata
            this->layoutVerticalButtonGroup(buttonDefs,
                                            &createdVerticalButtons, // Létrehozott gombok tárolása
                                            marginRight,             // Jobb margó
                                            marginTop,               // Felső margó
                                            marginBottom,            // Alsó margó
                                            60,                      // Alapértelmezett gomb szélesség
                                            32,                      // Alapértelmezett gomb magasság
                                            3,                       // Oszlop gap
                                            4                        // Gomb gap
            );
        }

        /**
         * @brief Specifikus gomb állapotának frissítése
         * @param buttonId Gomb ID (VerticalButtonIDs namespace-ből)
         * @param state Új állapot
         */
        void updateVerticalButtonState(uint8_t buttonId, UIButton::ButtonState state) {
            for (auto &button : createdVerticalButtons) {
                if (button && button->getId() == buttonId) {
                    button->setState(state);
                    break;
                }
            }
        }

        /**
         * @brief Összes toggleable gomb állapotának szinkronizálása
         * @param si4735Manager Si4735Manager referencia állapotokhoz
         */
        void updateAllVerticalButtonStates(Si4735Manager *si4735Manager) {
            // Mute állapot szinkronizálás
            updateVerticalButtonState(VerticalButtonIDs::MUTE, rtv::muteStat ? UIButton::ButtonState::On : UIButton::ButtonState::Off);

            // TODO: AGC és Attenuator állapotok szinkronizálása
            // amikor Si4735Manager implementálja ezeket
            /*
            updateVerticalButtonState(
                VerticalButtonIDs::AGC,
                si4735Manager->isAGCEnabled() ? UIButton::ButtonState::On : UIButton::ButtonState::Off
            );

            updateVerticalButtonState(
                VerticalButtonIDs::ATT,
                si4735Manager->isAttenuatorEnabled() ? UIButton::ButtonState::On : UIButton::ButtonState::Off
            );
            */
        }

        /**
         * @brief Létrehozott függőleges gombok elérése
         * @return Gombok shared_ptr vektora
         */
        const std::vector<std::shared_ptr<UIButton>> &getVerticalButtons() const { return createdVerticalButtons; }
    };

    // =====================================================================
    // UNIVERZÁLIS GOMBKEZELŐ METÓDUSOK - Változatlan funkcionalitás
    // =====================================================================

    static void handleMuteButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager) {
        if (event.state == UIButton::EventButtonState::On) {
            DEBUG("CommonVerticalHandler: Mute ON\n");
            rtv::muteStat = true;
            si4735Manager->getSi4735().setAudioMute(true);
        } else if (event.state == UIButton::EventButtonState::Off) {
            DEBUG("CommonVerticalHandler: Mute OFF\n");
            rtv::muteStat = false;
            si4735Manager->getSi4735().setAudioMute(false);
        }
    }

    static void handleVolumeButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager) {
        if (event.state != UIButton::EventButtonState::Clicked) {
            return;
        }
        // TODO: Volume dialog implementation
    }

    static void handleAGCButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager) {
        if (event.state == UIButton::EventButtonState::On) {
            DEBUG("CommonVerticalHandler: AGC ON\n");
            // TODO: Si4735 AGC bekapcsolása
        } else if (event.state == UIButton::EventButtonState::Off) {
            DEBUG("CommonVerticalHandler: AGC OFF\n");
            // TODO: Si4735 AGC kikapcsolása
        }
    }

    static void handleAttenuatorButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager) {
        if (event.state == UIButton::EventButtonState::On) {
            DEBUG("CommonVerticalHandler: Attenuator ON\n");
            // TODO: Si4735 attenuator bekapcsolása
        } else if (event.state == UIButton::EventButtonState::Off) {
            DEBUG("CommonVerticalHandler: Attenuator OFF\n");
            // TODO: Si4735 attenuator kikapcsolása
        }
    }

    static void handleFrequencyButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("CommonVerticalHandler: Frequency input dialog requested\n");
            // TODO: Frequency input dialog
        }
    }

    static void handleSetupButton(const UIButton::ButtonEvent &event, IScreenManager *screenManager) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("CommonVerticalHandler: Switching to Setup screen\n");
            screenManager->switchToScreen(SCREEN_NAME_SETUP);
        }
    }

    static void handleMemoryButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("CommonVerticalHandler: Memory functions dialog requested\n");
            // TODO: Memory dialog
        }
    }

    static void handleSquelchButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("CommonVerticalHandler: Squelch adjustment dialog requested\n");
            // TODO: Squelch dialog
        }
    }
};

#endif // __COMMON_VERTICAL_BUTTONS_REFACTORED_H
