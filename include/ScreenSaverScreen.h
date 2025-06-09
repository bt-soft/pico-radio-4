#ifndef __SCREEN_SAVER_SCREEN_H
#define __SCREEN_SAVER_SCREEN_H

#include "Band.h"        // For Band&
#include "Config.h"      // For Config&
#include "FreqDisplay.h" // For frequency display
#include "IScreenManager.h"
#include "UIScreen.h"
#include "defines.h" // SCREEN_NAME_SCREENSAVER miatt
#include "rtVars.h"  // For rtv::

// Forward declaration for Band and Config if not fully included
// class Band;
// class Config;

namespace ScreenSaverConstants {
// Copied from ScreenSaverDisplay.cpp
constexpr int SAVER_ANIMATION_STEPS = 500;
constexpr int SAVER_ANIMATION_LINE_LENGTH = 63;
constexpr int SAVER_LINE_CENTER = 31;
// ... other animation constants
constexpr int SAVER_NEW_POS_INTERVAL_MSEC = 15000;
constexpr int SAVER_COLOR_FACTOR = 64;
constexpr int SAVER_ANIMATION_STEP_JUMP = 3;

// Offsets for the animated line relative to saverAnimationX, saverAnimationY
// Restored original proportional values for proper rectangular border animation
constexpr int SAVER_X_OFFSET_1 = 10;  // Left edge of rectangle
constexpr int SAVER_Y_OFFSET_1 = 5;   // Top edge
constexpr int SAVER_X_OFFSET_2 = 189; // Right edge
constexpr int SAVER_Y_OFFSET_2 = 205; // Vertical movement offset for right edge
constexpr int SAVER_X_OFFSET_3 = 439; // Horizontal movement offset for bottom
constexpr int SAVER_Y_OFFSET_3 = 44;  // Bottom edge
constexpr int SAVER_X_OFFSET_4 = 10;  // Left edge again
constexpr int SAVER_Y_OFFSET_4 = 494; // Vertical movement offset for left edge

// Battery display constants (can be fine-tuned)
constexpr int BATTERY_RECT_X_OFFSET = 145; // Relative to FreqDisplay's top-left
constexpr int BATTERY_RECT_Y_OFFSET = 0;
constexpr int BATTERY_RECT_W = 38;
constexpr int BATTERY_RECT_H = 18;
constexpr int BATTERY_NUB_X_OFFSET = BATTERY_RECT_X_OFFSET + BATTERY_RECT_W + 1;
constexpr int BATTERY_NUB_Y_OFFSET = BATTERY_RECT_Y_OFFSET + 4;
constexpr int BATTERY_NUB_W = 2;
constexpr int BATTERY_NUB_H = 10;
constexpr int BATTERY_TEXT_X_OFFSET = BATTERY_RECT_X_OFFSET + BATTERY_RECT_W / 2;
constexpr int BATTERY_TEXT_Y_OFFSET = BATTERY_RECT_Y_OFFSET + BATTERY_RECT_H / 2 - 1; // Centered in the rect
} // namespace ScreenSaverConstants

/**
 * @file ScreenSaverScreen.h
 * @brief Képernyővédő osztály
 */
class ScreenSaverScreen : public UIScreen {
  private:
    uint32_t activationTime;
    uint32_t lastAnimationUpdateTime;

    virtual void activate() override;

    // Variables from ScreenSaverDisplay logic
    uint16_t saverAnimationX;
    uint16_t saverAnimationY;
    uint16_t currentFrequencyValue;

    uint16_t posSaver;
    uint8_t saverLineColors[ScreenSaverConstants::SAVER_ANIMATION_LINE_LENGTH];

    std::shared_ptr<FreqDisplay> freqDisplayComp;
    Band &band;
    Config &config;

    uint32_t lastFullUpdateSaverTime;

    void drawAnimatedBorder();
    void drawBatteryInfo(uint16_t baseX, uint16_t baseY);
    void updateFrequencyAndBatteryDisplay();

  public:
    ScreenSaverScreen(TFT_eSPI &tft, Band &band_ref, Config &config_ref);
    virtual ~ScreenSaverScreen() = default;

    // virtual void activate() override; // Moved to private for internal use via constructor
    virtual void deactivate() override;
    virtual void drawContent() override;
    virtual void handleOwnLoop() override;

    virtual bool handleTouch(const TouchEvent &event) override;
    virtual bool handleRotary(const RotaryEvent &event) override;
};

#endif // __SCREEN_SAVER_SCREEN_H