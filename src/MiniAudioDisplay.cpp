#include "MiniAudioDisplay.h"
#include "Core1Logic.h"
#include "UIColorPalette.h"

MiniAudioDisplay::MiniAudioDisplay(TFT_eSPI &tft, const Rect &bounds, const ColorScheme &colors)
    : UIComponent(tft, bounds, colors), primaryColor_(UIColorPalette::audioSpectrumPrimary), secondaryColor_(UIColorPalette::audioSpectrumSecondary),
      backgroundColor_(UIColorPalette::audioSpectrumBackground), lastUpdateTime_(0) {}

void MiniAudioDisplay::draw() {
    if (isDisabled()) {
        return;
    }

    if (isRedrawNeeded()) {
        const Rect &bounds = getBounds();

        // Háttér törlése
        tft.fillRect(bounds.x, bounds.y, bounds.width, bounds.height, backgroundColor_);

// Keret rajzolása debug módban
#ifdef DRAW_DEBUG_FRAMES
        tft.drawRect(bounds.x, bounds.y, bounds.width, bounds.height, TFT_RED);
#endif

        drawContent();
        needsRedraw = false;
    }
}

void MiniAudioDisplay::update() {
    if (isDisabled()) {
        return;
    }

    uint32_t currentTime = millis();
    if (currentTime - lastUpdateTime_ >= UPDATE_INTERVAL_MS) {
        markForRedraw();
        lastUpdateTime_ = currentTime;
    }
}

void MiniAudioDisplay::setEnabled(bool enabled) {
    setDisabled(!enabled);

    if (enabled) {
        markForRedraw();
    } else {
        // Terület törlése ha letiltjuk
        const Rect &bounds = getBounds();
        tft.fillRect(bounds.x, bounds.y, bounds.width, bounds.height, backgroundColor_);
    }
}

void MiniAudioDisplay::setColorScheme(uint16_t primary, uint16_t secondary, uint16_t background) {
    primaryColor_ = primary;
    secondaryColor_ = secondary;
    backgroundColor_ = background;
    markForRedraw();
}

AudioProcessor *MiniAudioDisplay::getAudioProcessor() {
    return ::getAudioProcessor(); // Globális függvény hívása a Core1Logic.h-ból
}
