#include "ScreenSaverScreen.h"
#include "PicoSensorUtils.h"
#include "UIColorPalette.h"
#include "rtVars.h"

// Global Band and Config are passed via constructor

ScreenSaverScreen::ScreenSaverScreen(TFT_eSPI &tft_ref, Band &band_ref, Config &config_ref)
    : UIScreen(tft_ref, SCREEN_NAME_SCREENSAVER), activationTime(0), lastAnimationUpdateTime(0), saverAnimationX(0), saverAnimationY(0), currentFrequencyValue(0), posSaver(0),
      band(band_ref), config(config_ref), lastFullUpdateSaverTime(0) {
    // Pre-calculate line colors for the animation
    for (uint8_t i = 0; i < ScreenSaverConstants::SAVER_ANIMATION_LINE_LENGTH; i++) {
        // The formula (31 - abs(i - 31)) generates values from 0 to 31 and back to 0.
        // Original code: saverLineColors[i] = (31 - abs(i - SAVER_LINE_CENTER));
        saverLineColors[i] = (ScreenSaverConstants::SAVER_LINE_CENTER - std::abs(static_cast<int>(i) - ScreenSaverConstants::SAVER_LINE_CENTER));
    }

    // Initialize FreqDisplay
    // Initial bounds are placeholders, will be updated in updateFrequencyAndBatteryDisplay
    // Default FreqDisplay width can be around 220-240, height around 60-70.
    Rect initialFreqBounds(0, 0, 220, 70); // Placeholder, will be centered later
    freqDisplayComp = std::make_shared<FreqDisplay>(tft, initialFreqBounds, band, config);
    addChild(freqDisplayComp);
}

void ScreenSaverScreen::activate() {
    UIScreen::activate(); // Call base class activate
    DEBUG("ScreenSaverScreen activated.\n");
    activationTime = millis();
    lastAnimationUpdateTime = millis();
    lastFullUpdateSaverTime = millis(); // Reset timer for full update

    // Configure FreqDisplay for screen saver mode: blue colors and hidden underline
    freqDisplayComp->setCustomColors(UIColorPalette::createScreenSaverFreqColors());
    freqDisplayComp->setHideUnderline(true);

    // Initial placement of frequency and battery
    updateFrequencyAndBatteryDisplay(); // This will clear screen and set initial positions
    // markForRedraw() is called by updateFrequencyAndBatteryDisplay
}

void ScreenSaverScreen::deactivate() {
    UIScreen::deactivate(); // Call base class deactivate
    DEBUG("ScreenSaverScreen deactivated.\n");

    // Restore FreqDisplay to normal mode: default colors and show underline
    freqDisplayComp->resetToDefaultColors();
    freqDisplayComp->setHideUnderline(false);
}

void ScreenSaverScreen::handleOwnLoop() {
    uint32_t currentTime = millis();

    posSaver++;
    if (posSaver >= ScreenSaverConstants::SAVER_ANIMATION_STEPS) {
        posSaver = 0;
    }

    if (currentTime - lastFullUpdateSaverTime >= ScreenSaverConstants::SAVER_NEW_POS_INTERVAL_MSEC) {
        lastFullUpdateSaverTime = currentTime;
        updateFrequencyAndBatteryDisplay();
        // updateFrequencyAndBatteryDisplay calls markForRedraw()
    }

    // Az animáció minden frame-nél újrarajzolást igényel
    // A flag-et a loop végén állítjuk be, hogy biztosítsuk az animáció folytonosságát
    needsRedraw = true;
}

void ScreenSaverScreen::drawContent() {
    // FreqDisplay is a child component and will be drawn by UIScreen::draw() if its needsRedraw is true.
    // We draw only the animated border here.
    // The battery info is drawn only during full updates in updateFrequencyAndBatteryDisplay().

    drawAnimatedBorder();

    // Battery info is only drawn during full updates to avoid constant redrawing
    // It will be drawn in updateFrequencyAndBatteryDisplay() when positions change
}

void ScreenSaverScreen::updateFrequencyAndBatteryDisplay() {
    tft.fillScreen(TFT_COLOR_BACKGROUND);

    uint16_t freqDisplayW = 220;
    uint16_t freqDisplayH = 70; // Calculate actual battery dimensions for proper positioning
    using namespace ScreenSaverConstants;
    uint16_t batteryTotalW = BATTERY_RECT_X_OFFSET + BATTERY_RECT_W + BATTERY_NUB_W + 5; // Total battery width
    uint16_t batteryTotalH = BATTERY_RECT_H + 10;                                        // Battery height with some margin

    // Add margin for animated border and battery positioning
    const uint16_t borderMargin = 80;                 // Increased space for animated border
    const uint16_t batterySpace = batteryTotalH + 10; // Extra space for battery below FreqDisplay    // Calculate safe positioning bounds
    uint16_t minX = borderMargin;
    uint16_t maxX = tft.width() - std::max(freqDisplayW, batteryTotalW) - borderMargin;
    uint16_t minY = borderMargin;
    uint16_t maxY = tft.height() - freqDisplayH - batterySpace - borderMargin;

    // Ensure we have valid ranges
    if (maxX <= minX)
        maxX = minX + 10;
    if (maxY <= minY)
        maxY = minY + 10;

    uint16_t saverFreqDisplayX = random(maxX - minX) + minX;
    uint16_t saverFreqDisplayY = random(maxY - minY) + minY;

    currentFrequencyValue = band.getCurrentBand().currFreq;
    if (freqDisplayComp) {
        freqDisplayComp->setBounds(Rect(saverFreqDisplayX, saverFreqDisplayY, freqDisplayW, freqDisplayH));
        freqDisplayComp->setFrequency(currentFrequencyValue);
        freqDisplayComp->markForRedraw(); // Ensure FreqDisplay redraws
    } // Center animation around FreqDisplay center
    saverAnimationX = saverFreqDisplayX + freqDisplayW / 2;
    saverAnimationY = saverFreqDisplayY + freqDisplayH / 2;

    // Draw battery info below FreqDisplay (only during full updates)
    if (freqDisplayComp) {
        Rect fdBounds = freqDisplayComp->getBounds();
        drawBatteryInfo(fdBounds.x, fdBounds.y);
    }

    markForRedraw();
}

void ScreenSaverScreen::drawAnimatedBorder() {
    using namespace ScreenSaverConstants;
    uint16_t t = posSaver;

    // Calculate animation offsets based on actual screen size
    uint16_t screenW = tft.width();
    uint16_t screenH = tft.height();

    // Calculate safe animation offsets - simple rectangular border around freq display
    int16_t borderSize = 60; // Fixed border size around frequency display

    // Calculate rectangle bounds around the frequency display
    int16_t rectLeft = saverAnimationX - (220 / 2) - borderSize / 2;
    int16_t rectRight = saverAnimationX + (220 / 2) + borderSize / 2;
    int16_t rectTop = saverAnimationY - (70 / 2) - borderSize / 2;
    int16_t rectBottom = saverAnimationY + (70 / 2) + borderSize / 2;

    // Ensure rectangle stays within screen bounds
    if (rectLeft < 0)
        rectLeft = 0;
    if (rectRight >= screenW)
        rectRight = screenW - 1;
    if (rectTop < 0)
        rectTop = 0;
    if (rectBottom >= screenH)
        rectBottom = screenH - 1;

    int16_t rectWidth = rectRight - rectLeft;
    int16_t rectHeight = rectBottom - rectTop;

    for (uint8_t i = 0; i < SAVER_ANIMATION_LINE_LENGTH; i++) {
        uint8_t c_val = saverLineColors[i];
        uint16_t pixel_color = (c_val * SAVER_COLOR_FACTOR) + c_val;

        // Calculate pixel positions for rectangular border animation
        int16_t pixelX, pixelY;

        // Total perimeter steps: top + right + bottom + left
        uint16_t totalSteps = SAVER_ANIMATION_STEPS;
        uint16_t topSteps = totalSteps / 4;
        uint16_t rightSteps = totalSteps / 4;
        uint16_t bottomSteps = totalSteps / 4;
        uint16_t leftSteps = totalSteps / 4;

        if (t < topSteps) {
            // Top edge: move from left to right
            pixelX = rectLeft + (t * rectWidth) / topSteps;
            pixelY = rectTop;
        } else if (t < topSteps + rightSteps) {
            // Right edge: move from top to bottom
            pixelX = rectRight;
            pixelY = rectTop + ((t - topSteps) * rectHeight) / rightSteps;
        } else if (t < topSteps + rightSteps + bottomSteps) {
            // Bottom edge: move from right to left
            pixelX = rectRight - ((t - topSteps - rightSteps) * rectWidth) / bottomSteps;
            pixelY = rectBottom;
        } else {
            // Left edge: move from bottom to top
            pixelX = rectLeft;
            pixelY = rectBottom - ((t - topSteps - rightSteps - bottomSteps) * rectHeight) / leftSteps;
        }

        // Only draw pixel if it's within screen bounds
        if (pixelX >= 0 && pixelX < screenW && pixelY >= 0 && pixelY < screenH) {
            tft.drawPixel(pixelX, pixelY, pixel_color);
        }

        t += SAVER_ANIMATION_STEP_JUMP;
        if (t >= SAVER_ANIMATION_STEPS) {
            t -= SAVER_ANIMATION_STEPS;
        }
    }
}

void ScreenSaverScreen::drawBatteryInfo(uint16_t baseX, uint16_t baseY) {
    using namespace ScreenSaverConstants;

    float vSupply = PicoSensorUtils::readVBus();
    uint8_t bat_percent = map(static_cast<int>(vSupply * 100), MIN_BATTERY_VOLTAGE, MAX_BATTERY_VOLTAGE, 0, 100);
    bat_percent = constrain(bat_percent, 0, 100);

    uint16_t colorBatt = TFT_DARKCYAN;
    if (bat_percent < 5) {
        colorBatt = UIColorPalette::TFT_COLOR_DRAINED_BATTERY;
    } else if (bat_percent < 15) {
        colorBatt = UIColorPalette::TFT_COLOR_SUBMERSIBLE_BATTERY;
    }

    // Calculate total battery width for bounds checking
    uint16_t batteryTotalW = BATTERY_RECT_X_OFFSET + BATTERY_RECT_W + BATTERY_NUB_W + 5;      // Position battery below FreqDisplay, centered horizontally
    uint16_t batteryAreaX = baseX + (freqDisplayComp->getBounds().width - batteryTotalW) / 2; // Center horizontally
    uint16_t batteryAreaY = baseY + freqDisplayComp->getBounds().height + 5;                  // Position below FreqDisplay

    // Ensure battery doesn't go off screen
    if (batteryAreaY + BATTERY_RECT_H > tft.height() - 5) {
        batteryAreaY = tft.height() - 5 - BATTERY_RECT_H;
    }
    if (batteryAreaX + batteryTotalW > tft.width() - 5) {
        batteryAreaX = tft.width() - 5 - batteryTotalW;
    }
    // Ensure battery doesn't go off left edge
    if (batteryAreaX < 5) {
        batteryAreaX = 5;
    }

    tft.fillRect(batteryAreaX + BATTERY_RECT_X_OFFSET, batteryAreaY + BATTERY_RECT_Y_OFFSET, BATTERY_RECT_W, BATTERY_RECT_H, TFT_BLACK); // Clear area first
    tft.drawRect(batteryAreaX + BATTERY_RECT_X_OFFSET, batteryAreaY + BATTERY_RECT_Y_OFFSET, BATTERY_RECT_W, BATTERY_RECT_H, colorBatt);
    tft.drawRect(batteryAreaX + BATTERY_NUB_X_OFFSET, batteryAreaY + BATTERY_NUB_Y_OFFSET, BATTERY_NUB_W, BATTERY_NUB_H, colorBatt);

    tft.setFreeFont();
    tft.setTextSize(1);
    tft.setTextColor(colorBatt, TFT_COLOR_BACKGROUND);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(String(bat_percent) + "%", batteryAreaX + BATTERY_TEXT_X_OFFSET, batteryAreaY + BATTERY_TEXT_Y_OFFSET);
}

bool ScreenSaverScreen::handleTouch(const TouchEvent &event) {
    if (event.pressed) {
        DEBUG("ScreenSaverScreen: Touch event, waking up.\n");
        if (getManager()) {
            getManager()->goBack();
        }
        return true;
    }
    return false;
}

bool ScreenSaverScreen::handleRotary(const RotaryEvent &event) {
    // Any rotary event (turn or click) should wake up
    DEBUG("ScreenSaverScreen: Rotary event, waking up.\n");
    if (getManager()) {
        getManager()->goBack();
    }
    return true;
}