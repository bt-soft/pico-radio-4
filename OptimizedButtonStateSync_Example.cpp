/**
 * @file OptimizedButtonStateSync_Example.cpp
 * @brief Példa az optimalizált gombállapot szinkronizálásra
 * @date 2025-06-11
 *
 * Ez a megoldás csak akkor frissíti a gombállapotokat, amikor ténylegesen változtak.
 */

#include "FMScreen.h"

class FMScreen : public UIScreen {
  private:
    // Állapot cache változók
    bool lastMuteState = false;
    uint8_t lastBandType = 0xFF; // Érvénytelen kezdeti érték

  public:
    /**
     * @brief Optimalizált függőleges gombállapot frissítés
     * Csak akkor hívja meg a setButtonState-et, ha az állapot ténylegesen változott
     */
    void updateVerticalButtonStatesOptimized() {
        if (!verticalButtonBar) {
            return;
        }

        // Mute gomb állapot szinkronizálása - csak ha változott
        if (rtv::muteStat != lastMuteState) {
            verticalButtonBar->setButtonState(FMScreenButtonIDs::MUTE, rtv::muteStat ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
            lastMuteState = rtv::muteStat;
        }

        // További gombállapotok hasonlóan...
        // AGC, Attenuator stb.
    }

    /**
     * @brief Optimalizált vízszintes gombállapot frissítés
     */
    void updateHorizontalButtonStatesOptimized() {
        if (!horizontalButtonBar) {
            return;
        }

        // Aktuális sáv lekérdezése
        uint8_t currentBandType = pSi4735Manager->getCurrentBand().bandType;

        // AM gomb állapot szinkronizálása - csak ha változott
        if (currentBandType != lastBandType) {
            bool isAMMode = (currentBandType != FM_BAND_TYPE);
            horizontalButtonBar->setButtonState(FMScreenHorizontalButtonIDs::AM_BUTTON, isAMMode ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
            lastBandType = currentBandType;
        }
    }

    /**
     * @brief Mute gomb eseménykezelő - állapot cache frissítéssel
     */
    void handleMuteButtonOptimized(const UIButton::ButtonEvent &event) {
        if (event.state == UIButton::EventButtonState::On) {
            DEBUG("FMScreen: Mute ON\n");
            rtv::muteStat = true;
            lastMuteState = true; // Cache frissítése
            pSi4735Manager->getSi4735().setAudioMute(true);
        } else if (event.state == UIButton::EventButtonState::Off) {
            DEBUG("FMScreen: Mute OFF\n");
            rtv::muteStat = false;
            lastMuteState = false; // Cache frissítése
            pSi4735Manager->getSi4735().setAudioMute(false);
        }
    }
};

// ===============================================
// MÁSIK MEGKÖZELÍTÉS: Event-alapú szinkronizálás
// ===============================================

class FMScreenEventBased : public UIScreen {
  private:
    /**
     * @brief Mute állapot változás kezelése
     */
    void onMuteStateChanged(bool newMuteState) {
        // Csak akkor frissítjük a gombot, ha ténylegesen változott
        if (verticalButtonBar) {
            verticalButtonBar->setButtonState(FMScreenButtonIDs::MUTE, newMuteState ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
        }
    }

    /**
     * @brief Sáv változás kezelése
     */
    void onBandChanged(uint8_t newBandType) {
        // AM gomb frissítése
        if (horizontalButtonBar) {
            bool isAMMode = (newBandType != FM_BAND_TYPE);
            horizontalButtonBar->setButtonState(FMScreenHorizontalButtonIDs::AM_BUTTON, isAMMode ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
        }
    }

  public:
    /**
     * @brief Mute gomb eseménykezelő - event-alapú
     */
    void handleMuteButtonEventBased(const UIButton::ButtonEvent &event) {
        if (event.state == UIButton::EventButtonState::On) {
            DEBUG("FMScreen: Mute ON\n");
            rtv::muteStat = true;
            pSi4735Manager->getSi4735().setAudioMute(true);

            // Gomb állapotát NEM frissítjük itt - a toggleable gomb magától váltott!
            // Az updateVerticalButtonStates() pedig szinkronizálja a rtv::muteStat-tal

        } else if (event.state == UIButton::EventButtonState::Off) {
            DEBUG("FMScreen: Mute OFF\n");
            rtv::muteStat = false;
            pSi4735Manager->getSi4735().setAudioMute(false);

            // Itt sem frissítjük - a gomb magától váltott!
        }
    }

    /**
     * @brief AM gomb eseménykezelő
     */
    void handleAMButtonEventBased(const UIButton::ButtonEvent &event) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            DEBUG("FMScreen: Switching to AM screen\n");
            UIScreen::getManager()->switchToScreen(SCREEN_NAME_AM);

            // A sáv váltás automatikusan frissíti a gombot az updateHorizontalButtonStates()-ben
        }
    }
};

// ===============================================
// HARMADIK MEGKÖZELÍTÉS: Callback-alapú
// ===============================================

class CallbackBasedButtonSync {
  private:
    std::function<void(bool)> muteStateCallback;
    std::function<void(uint8_t)> bandChangeCallback;

  public:
    /**
     * @brief Mute állapot változás callback regisztrálása
     */
    void setMuteStateCallback(std::function<void(bool)> callback) { muteStateCallback = callback; }

    /**
     * @brief Mute állapot változtatása callback hívással
     */
    void setMuteState(bool muted) {
        rtv::muteStat = muted;
        pSi4735Manager->getSi4735().setAudioMute(muted);

        // Callback hívása - ez frissíti a gomb állapotát
        if (muteStateCallback) {
            muteStateCallback(muted);
        }
    }
};

// ===============================================
// ÖSSZEHASONLÍTÁS
// ===============================================

/*
JELENLEGI MEGOLDÁS (EGYSZERŰ):
+ Egyszerű implementáció
+ Mindig szinkronban van
+ Automatikusan kezeli a külső változásokat
+ A UIButton optimalizált (nem rajzol feleslegesen)
- Minden loop-ban hívódik (de ez nem probléma)

OPTIMALIZÁLT MEGOLDÁS (CACHE):
+ Minimális hívások
+ Jobb teljesítmény (elméletileg)
- Komplexebb kód
- Cache szinkronizálási problémák lehetősége
- Nehezebb hibakeresés

EVENT-ALAPÚ MEGOLDÁS:
+ Tiszta esemény kezelés
+ Minimális redundancia
- Komplexebb architektúra
- Több kód
- Nehezebb karbantartás

CALLBACK-ALAPÚ MEGOLDÁS:
+ Tiszta szeparáció
+ Flexibilis
- Még komplexebb
- Callback hell lehetősége
- Memória management

KONKLÚZIÓ:
A jelenlegi megoldás a LEGJOBB!
Egyszerű, megbízható és már optimalizált a UIButton szinten.
*/
