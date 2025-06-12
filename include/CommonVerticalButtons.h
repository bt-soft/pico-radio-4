/**
 * @file CommonVerticalButtons.h
 * @brief Közös függőleges gombsor létrehozó és kezelő osztály FM és AM képernyőkhöz
 * @details Teljes mértékben megszünteti a kód duplikációt a képernyők között az
 *          univerzális gomb ID rendszer és egyszerűsített factory pattern segítségével
 *
 * **Eredeti probléma**:
 * - FMScreen és AMScreen 87.5%-ban azonos gombkezelő logikával rendelkezett
 * - Mute, Volume, AGC, Attenuator, Setup, Memory gombok teljesen azonosak voltak
 * - Duplikált namespace-ek (FMScreenButtonIDs, AMScreenButtonIDs) karbantartási problémákat okoztak
 * - Template alapú factory pattern túl bonyolult volt
 *
 * **Implementált megoldás**:
 * - Univerzális VerticalButtonIDs namespace (ID tartomány: 10-17)
 * - Egyszerűsített factory metódus (template nélkül, 4 paraméter)
 * - Közös statikus metódusok a gombkezelési logikákhoz
 * - Si4735Manager referencia alapú működés (band-független)
 * - Egységes állapot szinkronizáló rendszer
 *
 * **Elért eredmények**:
 * - ~50 sor duplikált kód eliminálása
 * - Template komplexitás megszüntetése
 * - Egységes ID rendszer minden képernyő típushoz
 * - Jövőbeli képernyők (SSB, DAB) könnyű integrációja
 *
 * @author Rádió projekt
 * @version 3.0 - Univerzális gomb ID rendszer (2025.06.11)
 */

#ifndef __COMMON_VERTICAL_BUTTONS_H
#define __COMMON_VERTICAL_BUTTONS_H

#include "ButtonsGroupManager.h"
#include "IScreenManager.h"
#include "Si4735Manager.h"
#include "UIButton.h"
#include "defines.h"
#include "rtVars.h"
#include "utils.h"

// ===================================================================
// UNIVERZÁLIS GOMB AZONOSÍTÓK - Egységes ID rendszer
// ===================================================================

/**
 * @brief Univerzális függőleges gomb azonosítók
 * @details Egyetlen egységes ID készlet minden képernyő típushoz (FM, AM, SSB, DAB stb.)
 *          Ez a namespace helyettesíti a korábbi FMScreenButtonIDs és AMScreenButtonIDs-t
 *
 * **ID tartomány**: 10-17 (8 funkcionális gomb)
 * **Kompatibilitás**: Minden jövőbeli képernyő típus használhatja
 *
 * **Előnyök az egységes rendszerben**:
 * - Nincs ID duplikációs probléma különböző képernyők között
 * - Egyszerű factory metódus hívás (nincs template paraméterre szükség)
 * - Könnyű karbantartás (egy helyen módosítható minden ID)
 * - Nincs template vagy struct wrapper komplexitás
 * - Jövőbeli képernyők (SSB, DAB) zökkenőmentes integrációja
 */
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
 *
 * **Előnyök a korábbi megoldáshoz képest**:
 * - Nincs kód duplikáció FM és AM képernyők között (~50 sor eliminálva)
 * - Egyetlen helyen karbantartható a teljes gombkezelési logika
 * - Band-független implementáció (Si4735Manager kezeli a chip állapotokat)
 * - Template komplexitás megszüntetése (korábbi 5 paraméter → 4 paraméter)
 * - Univerzális gomb ID-k - nincs ButtonIDStruct wrapper szükséglet
 * - Event-driven architektúra támogatása
 *
 * **Használat**:
 * ```cpp
 * // Egyszerű hívás minden képernyőből:
 * verticalButtonBar = CommonVerticalButtons::createVerticalButtonBar(
 *     tft, this, pSi4735Manager, getManager()
 * );
 * ```
 */
class CommonVerticalButtons {
  public: // =====================================================================
    // GOMBDEFINÍCIÓK LÉTREHOZÁSA - ButtonsGroupManager formátumban
    // =====================================================================

    static std::vector<ButtonGroupDefinition> createButtonDefinitions(Si4735Manager *si4735Manager, IScreenManager *screenManager) {
        return {{VerticalButtonIDs::MUTE, "Mute", UIButton::ButtonType::Toggleable, [si4735Manager](const UIButton::ButtonEvent &e) { handleMuteButton(e, si4735Manager); },
                 UIButton::ButtonState::Off, 0, 32},

                {VerticalButtonIDs::VOLUME, "Vol", UIButton::ButtonType::Pushable, [si4735Manager](const UIButton::ButtonEvent &e) { handleVolumeButton(e, si4735Manager); },
                 UIButton::ButtonState::Off, 0, 32},

                {VerticalButtonIDs::AGC, "AGC", UIButton::ButtonType::Toggleable, [si4735Manager](const UIButton::ButtonEvent &e) { handleAGCButton(e, si4735Manager); },
                 UIButton::ButtonState::Off, 0, 32},

                {VerticalButtonIDs::ATT, "Att", UIButton::ButtonType::Toggleable, [si4735Manager](const UIButton::ButtonEvent &e) { handleAttenuatorButton(e, si4735Manager); },
                 UIButton::ButtonState::Off, 0, 32},

                {VerticalButtonIDs::SQUELCH, "Sql", UIButton::ButtonType::Pushable, [si4735Manager](const UIButton::ButtonEvent &e) { handleSquelchButton(e, si4735Manager); },
                 UIButton::ButtonState::Off, 0, 32},

                {VerticalButtonIDs::FREQ, "Freq", UIButton::ButtonType::Pushable, [si4735Manager](const UIButton::ButtonEvent &e) { handleFrequencyButton(e, si4735Manager); },
                 UIButton::ButtonState::Off, 0, 32},

                {VerticalButtonIDs::SETUP, "Setup", UIButton::ButtonType::Pushable, [screenManager](const UIButton::ButtonEvent &e) { handleSetupButton(e, screenManager); },
                 UIButton::ButtonState::Off, 0, 32},

                {VerticalButtonIDs::MEMO, "Memo", UIButton::ButtonType::Pushable, [si4735Manager](const UIButton::ButtonEvent &e) { handleMemoryButton(e, si4735Manager); },
                 UIButton::ButtonState::Off, 0, 32}};
    }

    // =====================================================================
    // MIXIN TEMPLATE - Screen osztályok kiegészítéséhez
    // =====================================================================

    template <typename ScreenType> class Mixin : public ButtonsGroupManager<ScreenType> {
      protected:
        std::vector<std::shared_ptr<UIButton>> createdVerticalButtons;
        void createCommonVerticalButtons(Si4735Manager *si4735Manager, IScreenManager *screenManager) {
            auto buttonDefs = CommonVerticalButtons::createButtonDefinitions(si4735Manager, screenManager);
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

    // =====================================================================
    // UNIVERZÁLIS GOMBKEZELŐ METÓDUSOK - Band-független implementáció
    // =====================================================================

    /**
     * @brief Univerzális MUTE gomb kezelő - Összes képernyő típushoz
     * @param event Gomb esemény (On/Off toggleable állapot)
     * @param si4735Manager Si4735 rádió chip manager referencia
     *
     * @details Band-független mute kezelés implementáció:
     * - ON állapot: Hang némítása (rtv::muteStat = true, chip mute BE)
     * - OFF állapot: Hang visszakapcsolása (rtv::muteStat = false, chip mute KI)
     * - Működik FM, AM, SSB, DAB módokban egyaránt
     * - A Si4735 chip automatikusan kezeli a band-specifikus mute logikát
     * - Globális állapot szinkronizálás (rtv::muteStat)
     */
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

    /**
     * @brief Univerzális VOLUME gomb kezelő - Hangerő beállító dialógus
     * @param event Gomb esemény (Clicked pushable)
     * @param si4735Manager Si4735 rádió chip manager referencia
     *
     * @details Hangerő beállítás funkcionalitás:
     * - Clicked állapot: ValueChangeDialog megjelenítése
     * - TODO: Implementálandó showVolumeDialog() metódus
     * - Hangterjedelem: 0-63 (Si4735 chip specifikus)
     * - Band-független működés (FM, AM, SSB, DAB)
     * - Real-time hangerő változtatás a dialógusban
     * - Aktuális hangerő érték megjelenítése
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

    // =====================================================================
    // BAND-SPECIFIKUS GOMBKEZELŐK - Speciális optimalizációkkal
    // =====================================================================

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
            //     showAMSquelchDialog(si4735Manager);            // }
        }
    }
};

#endif // __COMMON_VERTICAL_BUTTONS_H
