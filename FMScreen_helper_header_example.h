// Példa FMScreen.h a helper megközelítéshez

#ifndef __FM_SCREEN_H
#define __FM_SCREEN_H
#include "UIButton.h"
#include "UIScreen.h"
#include <map>

/**
 * @file FMScreen.h
 * @brief FM képernyő osztály
 * @details Ez az osztály kezeli az FM rádió vezérlő funkcióit
 */
class FMScreen : public UIScreen {

  public:
    /**
     * @brief FMScreen konstruktor
     * @param tft TFT display referencia
     */
    FMScreen(TFT_eSPI &tft, Si4735Manager &si4735Manager);
    virtual ~FMScreen() = default;

    /**
     * @brief Rotary encoder eseménykezelés felülírása
     * @param event Rotary encoder esemény
     * @return true ha kezelte az eseményt, false egyébként
     */
    virtual bool handleRotary(const RotaryEvent &event) override;

    /**
     * @brief Loop hívás felülírása
     * animációs vagy egyéb saját logika végrehajtására
     * @note Ez a metódus nem hívja meg a gyerek komponensek loop-ját, csak saját logikát tartalmaz.
     */
    virtual void handleOwnLoop() override;

    /**
     * @brief Kirajzolja a képernyő saját tartalmát
     */
    virtual void drawContent() override;

  private:
    /**
     * @brief UI komponensek létrehozása és elhelyezése
     */
    void layoutComponents();

    /**
     * @brief Helper funkció függőleges gombok létrehozásához
     */
    void createVerticalButtonsHelper();

    /**
     * @brief Függőleges gomb állapotának beállítása
     * @param buttonId A gomb azonosítója
     * @param state Az új állapot
     */
    void setVerticalButtonState(uint8_t buttonId, UIButton::ButtonState state);

    /**
     * @brief Függőleges gomb állapotának lekérdezése
     * @param buttonId A gomb azonosítója
     * @return A gomb aktuális állapota
     */
    UIButton::ButtonState getVerticalButtonState(uint8_t buttonId) const;

    /**
     * @brief Függőleges gombok állapotának frissítése
     */
    void updateVerticalButtonStates();

    // Gomb eseménykezelők
    void handleMuteButton(const UIButton::ButtonEvent &event);
    void handleVolumeButton(const UIButton::ButtonEvent &event);
    void handleAGCButton(const UIButton::ButtonEvent &event);
    void handleAttButton(const UIButton::ButtonEvent &event);
    void handleSquelchButton(const UIButton::ButtonEvent &event);
    void handleFreqButton(const UIButton::ButtonEvent &event);
    void handleSetupButton(const UIButton::ButtonEvent &event);
    void handleMemoButton(const UIButton::ButtonEvent &event);

    // UI komponensek
    std::map<uint8_t, std::shared_ptr<UIButton>> verticalButtons; // ID -> gomb mapping
};

#endif // __FM_SCREEN_H
