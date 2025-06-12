#ifndef __COMMON_VERTICAL_BUTTONS_H
#define __COMMON_VERTICAL_BUTTONS_H

#include "ButtonsGroupManager.h"
#include "IScreenManager.h"
#include "MessageDialog.h"
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
     *@brief Handler függvény típusok a különböző gombokhoz */
    using Si4735HandlerFunc = void (*)(const UIButton::ButtonEvent &, Si4735Manager *, UIScreen *);
    using ScreenHandlerFunc = void (*)(const UIButton::ButtonEvent &, IScreenManager *, UIScreen *);

    /**
     * @brief Dialógus megjelenítéshez szükséges handler típus
     */
    using DialogHandlerFunc = void (*)(const UIButton::ButtonEvent &, Si4735Manager *, UIScreen *);

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
        DialogHandlerFunc dialogHandler;    ///< Handler dialógusokhoz UIScreen-nel (nullptr ha nem használt)
    };

    // =====================================================================
    // UNIVERZÁLIS GOMBKEZELŐ METÓDUSOK - Band-független implementáció
    // =====================================================================
    /**
     *@brief Univerzális MUTE gomb kezelő - Összes képernyő típushoz
     *@param event Gomb esemény(On / Off toggleable állapot)
     *@param si4735Manager Si4735 rádió chip manager referencia
     *@param screen A képernyő objektum(opcionális, dialógusokhoz)
     */
    static void handleMuteButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager, UIScreen *screen = nullptr) {
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
     * @param screen A képernyő objektum a dialógus megjelenítéséhez
     */
    static void handleVolumeButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager, UIScreen *screen) {
        if (event.state != UIButton::EventButtonState::Clicked || !screen) {
            return;
        }

        // ValueChangeDialog létrehozása hangerő beállításhoz
        auto volumeDialog = std::make_shared<ValueChangeDialog>(
            screen, screen->getTFT(),                                         //
            "Volume Control", "Adjust radio volume (0-63):",                  //
            &config.data.currVolume, Si4735Constants::SI4735_MIN_VOLUME,      //
            Si4735Constants::SI4735_MAX_VOLUME, 1,                            // Min, max, step
            [si4735Manager](const std::variant<int, float, bool> &newValue) { // Callback a hangerő változásra
                if (std::holds_alternative<int>(newValue)) {
                    int volume = std::get<int>(newValue);
                    DEBUG("CommonVerticalHandler: Volume changed to: %d\n", volume);
                    si4735Manager->getSi4735().setVolume(static_cast<uint8_t>(volume));
                }
            },
            nullptr,             // Nincs külön dialog callback
            Rect(-1, -1, 280, 0) // Auto-magasság
        );

        // Dialógus megjelenítése
        screen->showDialog(volumeDialog);
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
     */    /**
     * @brief Univerzális AGC gomb kezelő - Automatikus erősítésszabályozás
     * @param event Gomb esemény (On/Off toggleable állapot)
     * @param si4735Manager Si4735 rádió chip manager referencia
     * @param screen A képernyő objektum (opcionális, dialógusokhoz)
     */
    static void handleAGCButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager, UIScreen *screen = nullptr) {
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
     */    /**
     * @brief Univerzális ATTENUATOR gomb kezelő - Jel csillapítás
     * @param event Gomb esemény (On/Off toggleable állapot)
     * @param si4735Manager Si4735 rádió chip manager referencia
     * @param screen A képernyő objektum (opcionális, dialógusokhoz)
     */
    static void handleAttenuatorButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager, UIScreen *screen = nullptr) {
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
     */    /**
     * @brief Univerzális FREQUENCY gomb kezelő - Közvetlen frekvencia megadás
     * @param event Gomb esemény (Clicked pushable)
     * @param si4735Manager Si4735 rádió chip manager referencia
     * @param screen A képernyő objektum a dialógus megjelenítéséhez
     */
    static void handleFrequencyButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager, UIScreen *screen) {
        if (event.state != UIButton::EventButtonState::Clicked || !screen) {
            return;
        }

        DEBUG("CommonVerticalHandler: Frequency input dialog requested\n");

        // TODO: Band-aware frekvencia input dialógus implementálása
        // Példa implementáció ValueChangeDialog-gal:

        // Band-specifikus frekvencia tartomány meghatározása
        float minFreq = 87.5f;  // FM alapértelmezett
        float maxFreq = 108.0f; // FM alapértelmezett
        float stepSize = 0.1f;  // FM alapértelmezett

        // Aktuális frekvencia lekérése
        float currentFreq = static_cast<float>(si4735Manager->getSi4735().getFrequency()) / 100.0f;

        Rect dlgBounds(-1, -1, 300, 0);
        auto freqDialog = std::make_shared<ValueChangeDialog>(
            screen, screen->getTFT(), "Frequency Input", "Enter frequency (MHz):", static_cast<float *>(nullptr), minFreq, maxFreq, stepSize,
            [si4735Manager](const std::variant<int, float, bool> &newValue) {
                if (std::holds_alternative<float>(newValue)) {
                    float freq = std::get<float>(newValue);
                    uint16_t freqValue = static_cast<uint16_t>(freq * 100);
                    DEBUG("CommonVerticalHandler: Frequency changed to: %.1f MHz\n", freq);
                    si4735Manager->getSi4735().setFrequency(freqValue);
                }
            },
            nullptr, dlgBounds);

        screen->showDialog(freqDialog);
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
     */    /**
     * @brief Univerzális SETUP gomb kezelő - Beállítások képernyő megnyitás
     * @param event Gomb esemény (Clicked pushable)
     * @param screenManager Screen manager referencia a képernyőváltáshoz
     * @param screen A képernyő objektum (nem használt ennél a handlernél)
     */
    static void handleSetupButton(const UIButton::ButtonEvent &event, IScreenManager *screenManager, UIScreen *screen = nullptr) {
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
     */    /**
     * @brief Univerzális MEMORY gomb kezelő - Memória funkciók dialógus
     * @param event Gomb esemény (Clicked pushable)
     * @param si4735Manager Si4735 rádió chip manager referencia
     * @param screen A képernyő objektum a dialógus megjelenítéséhez
     */
    static void handleMemoryButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager, UIScreen *screen) {
        if (event.state != UIButton::EventButtonState::Clicked || !screen) {
            return;
        }

        DEBUG("CommonVerticalHandler: Memory functions dialog requested\n");
    }

    /**
     * @brief Univerzális SQUELCH gomb kezelő - Band-optimalizált zajzár
     * @param event Gomb esemény (Clicked pushable)
     * @param si4735Manager Si4735 rádió chip manager referencia
     */
    static void handleSquelchButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager, UIScreen *screen) {
        if (event.state != UIButton::EventButtonState::Clicked || !screen) {
            return;
        }

        DEBUG("CommonVerticalHandler: Squelch adjustment dialog requested\n");

        // TODO: Band-specifikus squelch implementáció
        // Példa implementáció ValueChangeDialog-gal:

        // Band-alapú squelch tartomány (FM: 0-127, AM: 0-63)
        int minSquelch = 0;
        int maxSquelch = 127;    // FM alapértelmezett
        int currentSquelch = 20; // Alapértelmezett érték

        Rect dlgBounds(-1, -1, 280, 0);
        auto squelchDialog = std::make_shared<ValueChangeDialog>(
            screen, screen->getTFT(), "Squelch Control", "Adjust squelch level (0=off):", static_cast<int *>(nullptr), minSquelch, maxSquelch, 1,
            [si4735Manager](const std::variant<int, float, bool> &newValue) {
                if (std::holds_alternative<int>(newValue)) {
                    int squelch = std::get<int>(newValue);
                    DEBUG("CommonVerticalHandler: Squelch changed to: %d\n", squelch);
                    // TODO: si4735Manager->setSquelch(squelch);
                }
            },
            nullptr, dlgBounds);

        screen->showDialog(squelchDialog);
    }

    /**
     * @brief Központi gomb definíciók getter - minden gomb statikus adata egy helyen
     * @details Runtime függvény, amely visszaadja az összes függőleges gomb definícióját.
     *          Módosítás esetén csak itt kell változtatni, automatikusan frissül minden metódusban.
     *          Minden gombhoz megadott a megfelelő handler függvény is.
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
     * @param screen A UIScreen referencia dialógusok megjelenítéséhez
     * @param buttonWidth A gombok szélessége (0 = automatikus méretezés)
     * @return ButtonGroupDefinition vektor
     */
    static std::vector<ButtonGroupDefinition> createButtonDefinitionsInternal(Si4735Manager *si4735Manager, IScreenManager *screenManager, UIScreen *screen, uint16_t buttonWidth) {
        std::vector<ButtonGroupDefinition> definitions;
        const auto &buttonDefs = getButtonDefinitions();
        definitions.reserve(buttonDefs.size());

        for (size_t i = 0; i < buttonDefs.size(); i++) {
            const auto &def = buttonDefs[i];

            // Lambda callback létrehozása a struktúrában megadott handler alapján
            std::function<void(const UIButton::ButtonEvent &)> callback;

            if (def.si4735Handler != nullptr) {
                // Si4735Manager handler használata
                callback = [si4735Manager, screen, handler = def.si4735Handler](const UIButton::ButtonEvent &e) { handler(e, si4735Manager, screen); };
            } else if (def.screenHandler != nullptr) {
                // IScreenManager handler használata
                callback = [screenManager, screen, handler = def.screenHandler](const UIButton::ButtonEvent &e) { handler(e, screenManager, screen); };
            } else if (def.dialogHandler != nullptr) {
                // DialogHandler használata (dialógus megjelenítéshez)
                callback = [si4735Manager, screen, handler = def.dialogHandler](const UIButton::ButtonEvent &e) { handler(e, si4735Manager, screen); };
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
     * @param screen A UIScreen referencia dialógusok megjelenítéséhez
     * @return ButtonGroupDefinition vektor automatikus gombszélességgel
     */
    static std::vector<ButtonGroupDefinition> createButtonDefinitions(Si4735Manager *si4735Manager, IScreenManager *screenManager, UIScreen *screen) {
        return createButtonDefinitionsInternal(si4735Manager, screenManager, screen, 0);
    }

    /**
     * @brief Egységes szélességű gombdefiníciók létrehozása
     * @param si4735Manager A rádió kezelő pointer
     * @param screenManager A képernyő kezelő pointer
     * @param screen A UIScreen referencia dialógusok megjelenítéséhez
     * @param tft TFT kijelző referencia a szélességkalkulációhoz
     * @return ButtonGroupDefinition vektor egységes szélességű gombokkal
     */
    template <typename TFTType>
    static std::vector<ButtonGroupDefinition> createUniformButtonDefinitions(Si4735Manager *si4735Manager, IScreenManager *screenManager, UIScreen *screen, TFTType &tft) {
        uint16_t uniformWidth = calculateUniformButtonWidth(tft, 32);
        return createButtonDefinitionsInternal(si4735Manager, screenManager, screen, uniformWidth);
    } // =====================================================================
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
            auto buttonDefs = CommonVerticalButtons::createUniformButtonDefinitions(si4735Manager, screenManager, self, self->getTFT());
            ButtonsGroupManager<ScreenType>::layoutVerticalButtonGroup(buttonDefs, &createdVerticalButtons, 0, 0, 5, 60, 32, 3, 4);
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
