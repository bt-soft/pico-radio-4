#ifndef __FM_SCREEN_H
#define __FM_SCREEN_H

#include "FreqDisplay.h"
#include "SMeter.h"
#include "Si4735Manager.h"
#include "UIButton.h"
#include "UIScreen.h"

/**
 * @file FMScreen.h
 * @brief FM képernyő osztály
 * @details Ez az osztály kezeli az FM rádió vezérlő funkcióit
 */
class FMScreen : public UIScreen {
  private:
    Si4735Manager &si4735Manager;

    // Frekvencia kijelző komponens
    std::shared_ptr<FreqDisplay> freqDisplayComp;

    // S-Meter komponens
    std::shared_ptr<SMeter> smeterComp;

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
};

#endif // __FM_SCREEN_H