/**
 * @file ScanScreen.cpp
 * @brief Spektrum analizátor scan képernyő implementáció (átdolgozott, dinamikus pásztázással és zoommal)
 */

#include "ScanScreen.h"
#include "ScreenManager.h"
#include "defines.h"
#include "rtVars.h"

// ===================================================================
// Konstruktor és inicializálás
// ===================================================================

ScanScreen::ScanScreen(TFT_eSPI &tft, Si4735Manager *si4735Manager) : UIScreen(tft, SCREEN_NAME_SCAN, si4735Manager) {
    DEBUG("ScanScreen initialized\n");
    layoutComponents();
}

// ===================================================================
// UIScreen interface implementáció
// ===================================================================

void ScanScreen::activate() { UIScreen::activate(); }

void ScanScreen::deactivate() {
    UIScreen::deactivate();
    DEBUG("ScanScreen deactivated\n");
}

void ScanScreen::layoutComponents() {

    // Vízszintes gombsor létrehozása
    createHorizontalButtonBar();
}

void ScanScreen::createHorizontalButtonBar() {
    constexpr int16_t margin = 5;
    uint16_t buttonHeight = UIButton::DEFAULT_BUTTON_HEIGHT;
    uint16_t buttonY = UIComponent::SCREEN_H - UIButton::DEFAULT_BUTTON_HEIGHT - margin;

    // Back gomb külön, jobbra igazítva
    uint16_t backButtonWidth = 60;
    uint16_t backButtonX = UIComponent::SCREEN_W - backButtonWidth - margin;
    Rect backButtonRect(backButtonX, buttonY, backButtonWidth, buttonHeight);

    backButton = std::make_shared<UIButton>( //
        tft,                                 //
        BACK_BUTTON_ID,                      //
        backButtonRect,                      //
        "Back",                              //
        UIButton::ButtonType::Pushable,      //
        UIButton::ButtonState::Off,          //
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                // Visszatérés az előző képernyőre
                if (getScreenManager()) {
                    getScreenManager()->goBack();
                }
            }
        } //
    );
    addChild(backButton);
}

void ScanScreen::drawContent() {
    tft.fillScreen(TFT_BLACK);
    tft.setFreeFont();

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("SPECTRUM ANALYZER", tft.width() / 2, 10);
}

void ScanScreen::handleOwnLoop() {}

bool ScanScreen::handleTouch(const TouchEvent &event) { return UIScreen::handleTouch(event); }

bool ScanScreen::handleRotary(const RotaryEvent &event) { return false; }
