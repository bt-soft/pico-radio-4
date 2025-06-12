#ifndef __COMMON_VERTICAL_BUTTONS_H
#define __COMMON_VERTICAL_BUTTONS_H

#include "ButtonsGroupManager.h"
#include "IScreenManager.h"
#include "Si4735Manager.h"
#include "UIButton.h"
#include "ValueChangeDialog.h"
#include "defines.h"
#include "rtVars.h"
#include "utils.h"

// ===================================================================
// UNIVERZÁLIS GOMB AZONOSÍTÓK - Egységes ID rendszer
// ===================================================================

namespace VerticalButtonIDs {
static constexpr uint8_t MUTE = 10; ///< Némítás gomb (univerzális)
static constexpr uint8_t VOLUME;    ///< Hangerő beállítás gomb (univerzális)
static constexpr uint8_t AGC;       ///< Automatikus erősítés szabályozás (univerzális)
static constexpr uint8_t ATT;       ///< Csillapító (univerzális)
static constexpr uint8_t SQUELCH;   ///< Zajzár beállítás (univerzális)
static constexpr uint8_t FREQ;      ///< Frekvencia input (univerzális)
static constexpr uint8_t SETUP;     ///< Beállítások képernyő (univerzális)
static constexpr uint8_t MEMO;      ///< Memória funkciók (univerzális)
} // namespace VerticalButtonIDs

/**
 * @brief Közös függőleges gombsor statikus osztály
 * @details Statikus metódusok gyűjteménye a teljes gombsor létrehozásához és kezeléshez.
 *          Ez az osztály teljes mértékben kiváltja a korábbi screen-specifikus gombkezelést.
 *
 * **Architektúra**:
 * - Factory pattern: createVerticalButtonBar() - teljes gombsor létrehozás
 * - Event handlers: handleXButton() metódusok - közös gombkezelési logika
 * - State sync: updateXButtonState() metódusok - állapot szinkronizálás
 */
class CommonVerticalButtons {
  public:
    // =====================================================================
    // KÖZPONTI GOMB DEFINÍCIÓK STRUKTÚRA - Egyetlen forrás minden gomb adatához
    // =====================================================================
    /**
     * @brief Handler függvény típusok a különböző gombokhoz
     */
    using Si4735HandlerFunc = void (*)(const UIButton::ButtonEvent &, Si4735Manager *);
    using ScreenHandlerFunc = void (*)(const UIButton::ButtonEvent &, IScreenManager *);

    /**
     * @brief Gomb statikus adatok struktúrája
     * @details Minden gomb statikus tulajdonságait tartalmazza egy helyen,
     *          beleértve a handler függvény pointert is
     */
    struct ButtonDefinition {
        uint8_t id;                         ///< Gomb azonosító (VerticalButtonIDs namespace-ből)
        const char *label;                  ///< Gomb felirata
        UIButton::ButtonType type;          ///< Gomb típusa (Toggleable/Pushable)
        UIButton::ButtonState initialState; ///< Kezdeti állapot
        uint16_t height;                    ///< Gomb magassága
        Si4735HandlerFunc si4735Handler;    ///< Handler Si4735Manager-rel (nullptr ha nem használt)
        ScreenHandlerFunc screenHandler;    ///< Handler IScreenManager-rel (nullptr ha nem használt)
    };

    // =====================================================================
    // UNIVERZÁLIS GOMBKEZELŐ METÓDUSOK - Band-független implementáció
    // =====================================================================

    /**
     * @brief Univerzális MUTE gomb kezelő - Összes képernyő típushoz
     * @param event Gomb esemény (On/Off toggleable állapot)
     * @param si4735Manager Si4735 rádió chip manager referencia
     */
    static void handleMuteButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager) {
        if (event.state != UIButton::EventButtonState::On && event.state != UIButton::EventButtonState::Off) {
            return;
        }
        rtv::muteStat = event.state == UIButton::EventButtonState::On;
        si4735Manager->getSi4735().setAudioMute(rtv::muteStat);
    }

    /**
     * @brief Univerzális VOLUME gomb kezelő - Hangerő beállító dialógus
     * @param event Gomb esemény (Clicked pushable)
     * @param si4735Manager Si4735 rádió chip manager referencia
     */
    static void handleVolumeButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager) {
        if (event.state != UIButton::EventButtonState::Clicked) {
            return;
        }
    }

    /**
     * @brief Univerzális AGC gomb kezelő - Automatikus erősítésszabályozás
     * @param event Gomb esemény (On/Off toggleable állapot)
     * @param si4735Manager Si4735 rádió chip manager referencia
     *
     * @details AGC (Automatic Gain Control) kezelés:
     * - ON állapot: Automatikus erősítésszabályozás bekapcsolása
     * - OFF állapot: Manuális erősítésszabályozás (fix gain)
     * - Band-specifikus optimalizált működés:
     *   * FM: Natív AGC algoritmus
     *   * AM: Zaj-optimalizált AGC
     *   * SSB: Gyors AGC válasz
     * - TODO: Si4735Manager AGC metódusok implementálása
     * - Állapot visszajelzés a gomb vizuális frissítéséhez
     */
    static void handleAGCButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager) {
        if (event.state == UIButton::EventButtonState::On) {
            DEBUG("CommonVerticalHandler: AGC ON\n");
            // TODO: Si4735 AGC bekapcsolása (band-független)
            // si4735Manager->setAGC(true);
        } else if (event.state == UIButton::EventButtonState::Off) {
            DEBUG("CommonVerticalHandler: AGC OFF\n");
            // TODO: Si4735 AGC kikapcsolása
            // si4735Manager->setAGC(false);
        }
    }

    /**
     * @brief Univerzális ATTENUATOR gomb kezelő - Jel csillapítás
     * @param event Gomb esemény (On/Off toggleable állapot)
     * @param si4735Manager Si4735 rádió chip manager referencia
     *
     * @details Attenuator (jel csillapítás) kezelés:
     * - ON állapot: Erős jelek csillapítása (túlvezérlés megelőzése)
     * - OFF állapot: Normál érzékenység (gyenge jelek vételéhez)
     * - Használati esetek:
     *   * Erős helyi adók közelében
     *   * Túlvezérlés és torzítás csökkentése
     *   * AM/FM/SSB módokban egyaránt hasznos
     * - Tipikus csillapítás: 6-12 dB
     * - TODO: Si4735Manager attenuator metódusok implementálása
     */
    static void handleAttenuatorButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager) {
        if (event.state == UIButton::EventButtonState::On) {
            DEBUG("CommonVerticalHandler: Attenuator ON\n");
            // TODO: Si4735 attenuator bekapcsolása
            // si4735Manager->setAttenuator(true);
        } else if (event.state == UIButton::EventButtonState::Off) {
            DEBUG("CommonVerticalHandler: Attenuator OFF\n");
            // TODO: Si4735 attenuator kikapcsolása
            // si4735Manager->setAttenuator(false);
        }
    }

    /**
     * @brief Univerzális FREQUENCY gomb kezelő - Közvetlen frekvencia megadás
     * @param event Gomb esemény (Clicked pushable)
     * @param si4735Manager Si4735 rádió chip manager referencia
     *
     * @details Frekvencia input dialógus funkcionalitás:
     * - Clicked állapot: Band-aware frekvencia input dialógus megjelenítése
     * - Band-specifikus frekvencia tartományok:
     *   * FM: 87.5 - 108.0 MHz (0.1 MHz lépésekkel)
     *   * AM/MW: 520 - 1710 kHz (9/10 kHz lépésekkel)
     *   * LW: 150 - 281 kHz
     *   * SW: 1.7 - 30.0 MHz (5 kHz lépésekkel)
     * - Validáció: Csak érvényes tartományban engedélyezett
     * - Automatikus band váltás ha szükséges
     * - TODO: showFrequencyInputDialog() implementálása
     */
    static void handleFrequencyButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("CommonVerticalHandler: Frequency input dialog requested\n");
            // TODO: Band-aware frekvencia input dialógus
            // A Si4735Manager tudja, milyen band aktív és milyen tartomány érvényes
            // showFrequencyInputDialog(si4735Manager);
        }
    }

    /**
     * @brief Univerzális SETUP gomb kezelő - Beállítások képernyő megnyitás
     * @param event Gomb esemény (Clicked pushable)
     * @param screenManager Screen manager referencia a képernyőváltáshoz
     *
     * @details Setup képernyő navigáció:
     * - Clicked állapot: Átváltás a beállítások képernyőre
     * - SCREEN_NAME_SETUP konstans használata
     * - Univerzális működés minden képernyő típusból
     * - Setup kategóriák:
     *   * Si4735 chip beállítások
     *   * Display/UI beállítások
     *   * Rendszer beállítások
     *   * Decoder/RDS beállítások
     * - Visszatérés lehetőség az eredeti képernyőre
     */
    static void handleSetupButton(const UIButton::ButtonEvent &event, IScreenManager *screenManager) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("CommonVerticalHandler: Switching to Setup screen\n");
            screenManager->switchToScreen(SCREEN_NAME_SETUP);
        }
    }

    /**
     * @brief Univerzális MEMORY gomb kezelő - Memória funkciók dialógus
     * @param event Gomb esemény (Clicked pushable)
     * @param si4735Manager Si4735 rádió chip manager referencia
     *
     * @details Band-aware memória funkciók:
     * - Clicked állapot: Memória műveletek dialógus megjelenítése
     * - Funkciók:
     *   * Aktuális frekvencia mentése
     *   * Mentett frekvenciák böngészése/betöltése
     *   * Állomás név szerkesztése
     *   * Memória helyek törlése
     * - Band-specifikus memória kezelés:
     *   * FM: RDS állomásnevek automatikus detektálása
     *   * AM/SW: Manuális állomás címkézés
     * - Memória kapacitás: ~100 állomás/band
     * - TODO: showMemoryDialog() implementálása
     */
    static void handleMemoryButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("CommonVerticalHandler: Memory functions dialog requested\n");
            // TODO: Band-aware memória funkciók dialógus
            // A Si4735Manager tudja, milyen band aktív
            // showMemoryDialog(si4735Manager);
        }
    }

    /**
     * @brief Univerzális SQUELCH gomb kezelő - Band-optimalizált zajzár
     * @param event Gomb esemény (Clicked pushable)
     * @param si4735Manager Si4735 rádió chip manager referencia
     *
     * @details Band-specifikus squelch (zajzár) implementáció:
     * - Clicked állapot: Zajzár beállító dialógus megjelenítése
     * - FM mód: Natív squelch algoritmus
     *   * RSSI és multipath noise detektálás
     *   * Sztereo/mono automatikus váltás
     *   * Beállítási tartomány: 0-127
     * - AM/SW mód: RSSI alapú squelch
     *   * Csak jelerősség alapú szűrés
     *   * Zaj-specifikus algoritmus
     *   * Beállítási tartomány: 0-63
     * - Automatikus band-detektálás Si4735Manager-en keresztül
     * - TODO: showSquelchDialog() implementálása
     */
    static void handleSquelchButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("CommonVerticalHandler: Squelch adjustment dialog requested\n");

            // A Si4735Manager tudja, milyen band aktív
            // if (si4735Manager->getCurrentBandType() == FM_BAND) {
            //     // FM natív squelch
            //     showFMSquelchDialog(si4735Manager);
            // } else {
            //     // AM RSSI alapú squelch
            //     showAMSquelchDialog(si4735Manager);
            // }
        }
    }

    /**
     * @brief Központi gomb definíciók getter - minden gomb statikus adata egy helyen
     * @details Runtime függvény, amely visszaadja az összes függőleges gomb definícióját.
     *          Módosítás esetén csak itt kell változtatni, automatikusan frissül minden metódusban.
     *          Minden gombhoz megadott a megfelelő handler függvény is.
     */
    static const std::vector<ButtonDefinition> &getButtonDefinitions() {
        static const std::vector<ButtonDefinition> BUTTON_DEFINITIONS = {
            // ID, Label, Type, InitialState, Height, Si4735Handler, ScreenHandler
            {VerticalButtonIDs::MUTE, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, 32, handleMuteButton, nullptr},
            {VerticalButtonIDs::VOLUME, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, 32, handleVolumeButton, nullptr},
            {VerticalButtonIDs::AGC, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, 32, handleAGCButton, nullptr},
            {VerticalButtonIDs::ATT, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, 32, handleAttenuatorButton, nullptr},
            {VerticalButtonIDs::SQUELCH, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, 32, handleSquelchButton, nullptr},
            {VerticalButtonIDs::FREQ, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, 32, handleFrequencyButton, nullptr},
            {VerticalButtonIDs::SETUP, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, 32, nullptr, handleSetupButton},
            {VerticalButtonIDs::MEMO, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, 32, handleMemoryButton, nullptr}};
        return BUTTON_DEFINITIONS;
    }

    static constexpr size_t BUTTON_COUNT = 8; // Number of buttons

    // =====================================================================
    // GOMBDEFINÍCIÓK LÉTREHOZÁSA - ButtonsGroupManager formátumban
    // =====================================================================

    /**
     * @brief Kiszámítja a maximális szélességet az összes gomb számára egységes megjelenés érdekében
     * @param tft TFT kijelző referencia a szövegméret kalkulációhoz
     * @param buttonHeight A gombok magassága
     * @return A legnagyobb szükséges gombszélesség
     */
    template <typename TFTType> static uint16_t calculateUniformButtonWidth(TFTType &tft, uint16_t buttonHeight = 32) {
        uint16_t maxWidth = 0;
        const auto &buttonDefs = getButtonDefinitions();

        for (size_t i = 0; i < buttonDefs.size(); i++) {
            uint16_t width = UIButton::calculateWidthForText(tft, buttonDefs[i].label, false /*useMiniFont*/, buttonHeight);
            maxWidth = std::max(maxWidth, width);
        }
        return maxWidth;
    }

  private:
    /**
     * @brief Belső segédmetódus a gombdefiníciók létrehozásához
     * @param si4735Manager A Si4735Manager referencia a rádió kezelőhöz
     * @param screenManager Az IScreenManager referencia a képernyő kezelőhöz
     * @param buttonWidth A gombok szélessége (0 = automatikus méretezés)
     * @return ButtonGroupDefinition vektor
     */
    static std::vector<ButtonGroupDefinition> createButtonDefinitionsInternal(Si4735Manager *si4735Manager, IScreenManager *screenManager, uint16_t buttonWidth) {
        std::vector<ButtonGroupDefinition> definitions;
        const auto &buttonDefs = getButtonDefinitions();
        definitions.reserve(buttonDefs.size());

        for (size_t i = 0; i < buttonDefs.size(); i++) {
            const auto &def = buttonDefs[i];

            // Lambda callback létrehozása a struktúrában megadott handler alapján
            std::function<void(const UIButton::ButtonEvent &)> callback;

            if (def.si4735Handler != nullptr) {
                // Si4735Manager handler használata
                callback = [si4735Manager, handler = def.si4735Handler](const UIButton::ButtonEvent &e) { handler(e, si4735Manager); };
            } else if (def.screenHandler != nullptr) {
                // IScreenManager handler használata
                callback = [screenManager, handler = def.screenHandler](const UIButton::ButtonEvent &e) { handler(e, screenManager); };
            } else {
                // Fallback - nem kellene előfordulnia
                callback = [](const UIButton::ButtonEvent &e) { /* no-op */ };
            }

            definitions.push_back({def.id, def.label, def.type, callback, def.initialState, buttonWidth, def.height});
        }

        return definitions;
    }

  public:
    /**
     * @brief Függőleges gombok definícióinak létrehozása ButtonsGroupManager számára (automatikus szélességgel)
     * @param si4735Manager A Si4735Manager referencia a rádió kezelőhöz
     * @param screenManager Az IScreenManager referencia a képernyő kezelőhöz
     * @return ButtonGroupDefinition vektor automatikus gombszélességgel
     */
    static std::vector<ButtonGroupDefinition> createButtonDefinitions(Si4735Manager *si4735Manager, IScreenManager *screenManager) {
        return createButtonDefinitionsInternal(si4735Manager, screenManager, 0);
    }

    /**
     * @brief Egységes szélességű gombdefiníciók létrehozása
     * @param si4735Manager A rádió kezelő pointer
     * @param screenManager A képernyő kezelő pointer
     * @param tft TFT kijelző referencia a szélességkalkulációhoz
     * @return ButtonGroupDefinition vektor egységes szélességű gombokkal
     */
    template <typename TFTType>
    static std::vector<ButtonGroupDefinition> createUniformButtonDefinitions(Si4735Manager *si4735Manager, IScreenManager *screenManager, TFTType &tft) {
        uint16_t uniformWidth = calculateUniformButtonWidth(tft, 32);
        return createButtonDefinitionsInternal(si4735Manager, screenManager, uniformWidth);
    }

    // =====================================================================
    // MIXIN TEMPLATE - Screen osztályok kiegészítéséhez
    // =====================================================================

    template <typename ScreenType> class Mixin : public ButtonsGroupManager<ScreenType> {
      protected:
        std::vector<std::shared_ptr<UIButton>> createdVerticalButtons;

        /**
         * @brief Közös függőleges gombok létrehozása egységes szélességgel
         * @param si4735Manager A rádió kezelő pointer
         * @param screenManager A képernyő kezelő pointer
         */
        void createCommonVerticalButtons(Si4735Manager *si4735Manager, IScreenManager *screenManager) {
            ScreenType *self = static_cast<ScreenType *>(this);
            auto buttonDefs = CommonVerticalButtons::createUniformButtonDefinitions(si4735Manager, screenManager, self->getTFT());
            this->layoutVerticalButtonGroup(buttonDefs, &createdVerticalButtons, 0, 0, 5, 60, 32, 3, 4);
        }

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
