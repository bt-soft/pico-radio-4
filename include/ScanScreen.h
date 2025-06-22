/**
 * @file ScanScreen.h
 * @brief Spektrum analizátor scan képernyő
 * @details Grafikus frekvencia pásztázás RSSI/SNR megjelenítéssel
 */

#ifndef __SCANSCREEN_H
#define __SCANSCREEN_H

#include "Config.h"
#include "Si4735Manager.h"
#include "UIButton.h"
#include "UIHorizontalButtonBar.h"
#include "UIScreen.h"
#include <memory>
#include <vector>

// Képernyő azonosító
#define SCREEN_NAME_SCAN "ScanScreen"

/**
 * @brief Scan képernyő állapotok
 */
enum class ScanState {
    Idle,    ///< Nincs aktív scan
    Scanning ///< Aktívan pásztáz
};

/**
 * @brief Scan módok
 */
enum class ScanMode {
    Spectrum, ///< Spektrum analizátor mód
    Seek,     ///< Gyors seek-based pásztázás
    Memory    ///< Mentett állomások pásztázása
};

/**
 * @brief Spektrum analizátor scan képernyő
 * @details Grafikus frekvencia pásztázás valós idejű RSSI/SNR megjelenítéssel
 */
class ScanScreen : public UIScreen {
  public:
    /**
     * @brief ScanScreen konstruktor
     * @param tft TFT display referencia
     * @param si4735Manager Si4735Manager referencia
     */
    ScanScreen(TFT_eSPI &tft, Si4735Manager *si4735Manager);

    /**
     * @brief Destruktor
     */
    virtual ~ScanScreen() = default;

    // UIScreen interface implementáció
    void activate() override;
    void deactivate() override;
    void drawContent() override;
    void handleOwnLoop() override;
    bool handleTouch(const TouchEvent &event) override;
    bool handleRotary(const RotaryEvent &event) override;

  private:
    static constexpr uint8_t BACK_BUTTON_ID = 40;

    std::shared_ptr<UIButton> backButton;

    // Metódusok
    void layoutComponents();
    void createHorizontalButtonBar();
};

#endif // __SCANSCREEN_H
