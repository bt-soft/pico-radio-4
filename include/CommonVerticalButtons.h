/**
 * @file CommonVerticalButtons.h
 * @brief Közös függőleges gombsor létrehozó és kezelő osztály FM és AM képernyőkhöz
 * @details Teljes mértékben megszünteti a kód duplikációt a képernyők között
 *
 * **Probléma**:
 * - FMScreen és AMScreen 87.5%-ban azonos gombkezelő logikával rendelkezik
 * - Mute, Volume, AGC, Attenuator, Setup, Memory gombok teljesen azonosak
 * - Kód duplikáció karbantartási problémákat okoz
 * * **Megoldás**:
 * - Közös statikus metódusok a gombkezelési logikákhoz
 * - Közös gombsor factory metódus - teljes gombsor létrehozás
 * - Si4735Manager referencia alapú működés
 * - Band-független implementáció (a chip állapotot kezeli)
 *
 * @author Rádió projekt
 * @version 2.0 - Complete vertical button bar factory
 */

#ifndef __COMMON_VERTICAL_BUTTONS_H
#define __COMMON_VERTICAL_BUTTONS_H

#include "IScreenManager.h"
#include "Si4735Manager.h"
#include "UIButton.h"
#include "UIVerticalButtonBar.h"
#include "defines.h"
#include "rtVars.h"
#include "utils.h"

/**
 * @brief Közös függőleges gombsor osztály
 * @details Statikus metódusok gyűjteménye a teljes gombsor létrehozásához és kezeléshez
 *
 * **Előnyök:**
 * - Nincs kód duplikáció FM és AM képernyők között
 * - Teljes gombsor factory metódus - egy helyen minden
 * - Egy helyen karbantartható a logika
 * - Band-független implementáció
 * - Si4735Manager automatikusan kezeli a chip állapotokat
 */
class CommonVerticalButtons {
  public:
    // =====================================================================
    // Gombsor factory metódus - teljes gombsor létrehozás
    // =====================================================================

    /**
     * @brief Közös függőleges gombsor létrehozása
     * @param tft TFT display referencia
     * @param screen Képernyő referencia (lambda capturehez)
     * @param si4735Manager Si4735 manager referencia
     * @param screenManager Screen manager referencia
     * @param buttonIds Gomb ID struktúra (template paraméter)
     * @return Létrehozott UIVerticalButtonBar smart pointer
     *
     * @details Ez a metódus teljes mértékben kiváltja a createVerticalButtonBar()
     * metódusokat az AM és FM képernyőkben. Minden gombsor konfiguráció itt található.
     */
    template <typename ButtonIDStruct>
    static std::shared_ptr<UIVerticalButtonBar> createVerticalButtonBar(TFT_eSPI &tft, UIScreen *screen, Si4735Manager *si4735Manager, IScreenManager *screenManager,
                                                                        const ButtonIDStruct &buttonIds) {
        // ===================================================================
        // Gombsor pozicionálás - Egységes minden képernyőre
        // ===================================================================
        const uint16_t buttonBarWidth = 65;                       // Optimális gombméret + margók
        const uint16_t buttonBarX = tft.width() - buttonBarWidth; // Pontosan a jobb szélhez igazítva (dinamikus)
        const uint16_t buttonBarY = 0;                            // Legfelső pixeltől kezdve
        const uint16_t buttonBarHeight = tft.height();            // Teljes képernyő magasság kihasználása (dinamikus)

        // ===================================================================
        // Gomb konfigurációk - Minden képernyőre azonos logika
        // ===================================================================
        std::vector<UIVerticalButtonBar::ButtonConfig> configs = {

            // 1. MUTE - Toggleable gomb (BE/KI állapottal)
            {buttonIds.MUTE, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
             [screen, si4735Manager](const UIButton::ButtonEvent &e) { CommonVerticalButtons::handleMuteButton(e, si4735Manager); }},

            // 2. VOLUME - Pushable gomb (dialógus megnyitás)
            {buttonIds.VOLUME, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
             [screen, si4735Manager](const UIButton::ButtonEvent &e) { CommonVerticalButtons::handleVolumeButton(e, si4735Manager); }},

            // 3. AGC - Toggleable gomb (Automatikus erősítésszabályozás)
            {buttonIds.AGC, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
             [screen, si4735Manager](const UIButton::ButtonEvent &e) { CommonVerticalButtons::handleAGCButton(e, si4735Manager); }},

            // 4. ATTENUATOR - Toggleable gomb (Jel csillapítás)
            {buttonIds.ATT, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
             [screen, si4735Manager](const UIButton::ButtonEvent &e) { CommonVerticalButtons::handleAttenuatorButton(e, si4735Manager); }},

            // 5. SQUELCH - Pushable gomb (Zajzár beállító dialógus)
            {buttonIds.SQUELCH, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
             [screen, si4735Manager](const UIButton::ButtonEvent &e) { CommonVerticalButtons::handleSquelchButton(e, si4735Manager); }},

            // 6. FREQUENCY - Pushable gomb (Frekvencia input dialógus)
            {buttonIds.FREQ, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
             [screen, si4735Manager](const UIButton::ButtonEvent &e) { CommonVerticalButtons::handleFrequencyButton(e, si4735Manager); }},

            // 7. SETUP - Pushable gomb (Beállítások képernyőre váltás)
            {buttonIds.SETUP, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
             [screen, screenManager](const UIButton::ButtonEvent &e) { CommonVerticalButtons::handleSetupButton(e, screenManager); }},

            // 8. MEMORY - Pushable gomb (Memória funkciók dialógus)
            {buttonIds.MEMO, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
             [screen, si4735Manager](const UIButton::ButtonEvent &e) { CommonVerticalButtons::handleMemoryButton(e, si4735Manager); }}};

        // ===================================================================
        // UIVerticalButtonBar objektum létrehozása és konfiguráció
        // ===================================================================
        return std::make_shared<UIVerticalButtonBar>(tft, Rect(buttonBarX, buttonBarY, buttonBarWidth, buttonBarHeight), configs,
                                                     60, // Egyedi gomb szélessége (pixel)
                                                     32, // Egyedi gomb magassága (pixel)
                                                     4   // Gombok közötti távolság (pixel)
        );
    }

    // =====================================================================
    // Közös gombkezelő metódusok
    // ===================================================================

    /**
     * @brief Általános MUTE gomb kezelő
     * @param event Gomb esemény (On/Off)
     * @param si4735Manager Si4735 manager referencia
     * @details Band-független mute kezelés - a chip tudja, milyen módban van
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
     * @brief Általános VOLUME gomb kezelő
     * @param event Gomb esemény (Clicked)
     * @param si4735Manager Si4735 manager referencia
     * @details TODO: ValueChangeDialog implementálása hangerő beállításhoz
     */
    static void handleVolumeButton(const UIButton::ButtonEvent &event, Si4735Manager *si4735Manager) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("CommonVerticalHandler: Volume adjustment dialog requested\n");
            // TODO: Hangerő beállító dialógus megjelenítése
            // showVolumeDialog(si4735Manager);
        }
    }

    /**
     * @brief Általános AGC gomb kezelő
     * @param event Gomb esemény (On/Off)
     * @param si4735Manager Si4735 manager referencia
     * @details Band-független AGC kezelés
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
     * @brief Általános ATTENUATOR gomb kezelő
     * @param event Gomb esemény (On/Off)
     * @param si4735Manager Si4735 manager referencia
     * @details Band-független attenuator kezelés
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
     * @brief Általános FREQUENCY gomb kezelő
     * @param event Gomb esemény (Clicked)
     * @param si4735Manager Si4735 manager referencia
     * @details Frekvencia input dialógus - band-specifikus tartományokkal
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
     * @brief Általános SETUP gomb kezelő
     * @param event Gomb esemény (Clicked)
     * @param screenManager Screen manager referencia
     */
    static void handleSetupButton(const UIButton::ButtonEvent &event, IScreenManager *screenManager) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("CommonVerticalHandler: Switching to Setup screen\n");
            screenManager->switchToScreen(SCREEN_NAME_SETUP);
        }
    }

    /**
     * @brief Általános MEMORY gomb kezelő
     * @param event Gomb esemény (Clicked)
     * @param si4735Manager Si4735 manager referencia
     * @details Band-aware memória funkciók
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
    // Band-specifikus gombkezelők (ha szükséges)
    // =====================================================================

    /**
     * @brief SQUELCH gomb kezelő - FM specifikus optimalizálással
     * @param event Gomb esemény (Clicked)
     * @param si4735Manager Si4735 manager referencia
     * @details FM-ben natív squelch, AM-ben RSSI alapú implementáció
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

    // =====================================================================
    // Gombállapot szinkronizáló metódusok
    // =====================================================================

    /**
     * @brief Mute gomb állapot szinkronizálás
     * @param buttonBar Gombsor referencia
     * @param muteButtonId Mute gomb ID
     */
    static void updateMuteButtonState(UIVerticalButtonBar *buttonBar, uint8_t muteButtonId) {
        if (buttonBar) {
            buttonBar->setButtonState(muteButtonId, rtv::muteStat ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
        }
    }

    /**
     * @brief AGC gomb állapot szinkronizálás
     * @param buttonBar Gombsor referencia
     * @param agcButtonId AGC gomb ID
     * @param si4735Manager Si4735 manager referencia
     */
    static void updateAGCButtonState(UIVerticalButtonBar *buttonBar, uint8_t agcButtonId, Si4735Manager *si4735Manager) {
        if (buttonBar) {
            // TODO: Si4735Manager AGC állapot lekérdezés
            // bool agcEnabled = si4735Manager->isAGCEnabled();
            // buttonBar->setButtonState(agcButtonId, agcEnabled ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
        }
    }

    /**
     * @brief Attenuator gomb állapot szinkronizálás
     * @param buttonBar Gombsor referencia
     * @param attButtonId Attenuator gomb ID
     * @param si4735Manager Si4735 manager referencia
     */
    static void updateAttenuatorButtonState(UIVerticalButtonBar *buttonBar, uint8_t attButtonId, Si4735Manager *si4735Manager) {
        if (buttonBar) {
            // TODO: Si4735Manager attenuator állapot lekérdezés
            // bool attEnabled = si4735Manager->isAttenuatorEnabled();
            // buttonBar->setButtonState(attButtonId, attEnabled ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
        }
    }

    // =====================================================================
    // Helper metódusok
    // =====================================================================

    /**
     * @brief Teljes gombsor állapot szinkronizálás
     * @param buttonBar Gombsor referencia
     * @param buttonIds Gomb ID-k struktúra
     * @param si4735Manager Si4735 manager referencia
     * @param screenManager Screen manager referencia
     */
    template <typename ButtonIDStruct>
    static void updateAllButtonStates(UIVerticalButtonBar *buttonBar, const ButtonIDStruct &buttonIds, Si4735Manager *si4735Manager, IScreenManager *screenManager) {
        if (!buttonBar)
            return;

        // Mute állapot szinkronizálás
        updateMuteButtonState(buttonBar, buttonIds.MUTE);

        // AGC állapot szinkronizálás
        updateAGCButtonState(buttonBar, buttonIds.AGC, si4735Manager);

        // Attenuator állapot szinkronizálás
        updateAttenuatorButtonState(buttonBar, buttonIds.ATT, si4735Manager);

        // További állapotok szükség szerint...
    }
};

#endif // __COMMON_VERTICAL_BUTTONS_H
