#include "MiniVuMeter.h"
#include "Core1Logic.h"
#include <cmath>

MiniVuMeter::MiniVuMeter(TFT_eSPI &tft, const Rect &bounds, Style style, const ColorScheme &colors)
    : MiniAudioDisplay(tft, bounds, colors), style_(style), currentLevel_(0.0f), peakLevel_(0.0f), peakHoldTime_(DEFAULT_PEAK_HOLD_TIME), lastPeakTime_(0) {}

void MiniVuMeter::setStyle(Style style) {
    if (style_ != style) {
        style_ = style;
        markForRedraw();
    }
}

void MiniVuMeter::setPeakHoldTime(uint32_t timeMs) { peakHoldTime_ = timeMs; }

void MiniVuMeter::drawContent() {
    // Kompatibilitási okból: a sprite-alapú rajzolás a drawContentToSprite-ban történik
    // Itt csak a régi hívásokat tartjuk meg, de minden tényleges rajzolás sprite-ra megy
}

void MiniVuMeter::drawContentToSprite(TFT_eSprite *sprite) {
    float newLevel = calculateRMSLevel();
    if (newLevel > currentLevel_) {
        currentLevel_ = newLevel;
    } else {
        currentLevel_ = currentLevel_ * LEVEL_DECAY + newLevel * (1.0f - LEVEL_DECAY);
    }
    uint32_t currentTime = millis();
    if (currentLevel_ > peakLevel_) {
        peakLevel_ = currentLevel_;
        lastPeakTime_ = currentTime;
    } else if (currentTime - lastPeakTime_ > peakHoldTime_) {
        peakLevel_ = currentLevel_;
    }
    switch (style_) {
        case Style::HORIZONTAL_BAR:
            drawHorizontalBar(sprite);
            break;
        case Style::VERTICAL_BAR:
            drawVerticalBar(sprite);
            break;
        case Style::NEEDLE:
            drawNeedle(sprite);
            break;
        case Style::LED_STRIP:
            drawLedStrip(sprite);
            break;
    }
}

void MiniVuMeter::drawHorizontalBar(TFT_eSprite *sprite) {
    const Rect &bounds = getBounds();
    sprite->fillRect(0, 0, bounds.width, bounds.height, backgroundColor_);
    uint16_t levelWidth = (uint16_t)(currentLevel_ * bounds.width);
    if (levelWidth > 0) {
        uint16_t levelColor = levelToColor(currentLevel_);
        sprite->fillRect(0, 0, levelWidth, bounds.height, levelColor);
    }
    uint16_t peakX = (uint16_t)(peakLevel_ * bounds.width);
    if (peakX < bounds.width) {
        sprite->drawFastVLine(peakX, 0, bounds.height, TFT_YELLOW); // Peak: sárga
    }
    sprite->drawRect(0, 0, bounds.width, bounds.height, primaryColor_);
}

void MiniVuMeter::drawVerticalBar(TFT_eSprite *sprite) {
    const Rect &bounds = getBounds();
    sprite->fillRect(0, 0, bounds.width, bounds.height, backgroundColor_);
    uint16_t levelHeight = (uint16_t)(currentLevel_ * bounds.height);
    if (levelHeight > 0) {
        uint16_t levelColor = levelToColor(currentLevel_);
        uint16_t levelY = bounds.height - levelHeight;
        sprite->fillRect(0, levelY, bounds.width, levelHeight, levelColor);
    }
    uint16_t peakY = bounds.height - (uint16_t)(peakLevel_ * bounds.height);
    if (peakY > 0) {
        sprite->drawFastHLine(0, peakY, bounds.width, TFT_YELLOW); // Peak: sárga
    }
    sprite->drawRect(0, 0, bounds.width, bounds.height, primaryColor_);
}

void MiniVuMeter::drawNeedle(TFT_eSprite *sprite) {
    const Rect &bounds = getBounds();
    sprite->fillRect(0, 0, bounds.width, bounds.height, backgroundColor_);
    int16_t centerX = bounds.width / 2;
    int16_t centerY = bounds.height - 5;
    int16_t radius = min(bounds.width, bounds.height) / 2 - 5;
    for (int i = 0; i <= 10; i++) {
        float angle = -PI + (PI * i / 10.0f);
        int16_t x1 = centerX + (radius - 5) * cos(angle);
        int16_t y1 = centerY + (radius - 5) * sin(angle);
        int16_t x2 = centerX + radius * cos(angle);
        int16_t y2 = centerY + radius * sin(angle);
        sprite->drawLine(x1, y1, x2, y2, secondaryColor_);
    }
    float needleAngle = -PI + (PI * currentLevel_);
    int16_t needleX = centerX + (radius - 2) * cos(needleAngle);
    int16_t needleY = centerY + (radius - 2) * sin(needleAngle);
    sprite->drawLine(centerX, centerY, needleX, needleY, TFT_RED);
    sprite->fillCircle(centerX, centerY, 2, TFT_WHITE);
    // Peak hold: kis sárga pötty a skálán
    float peakAngle = -PI + (PI * peakLevel_);
    int16_t peakX = centerX + (radius - 2) * cos(peakAngle);
    int16_t peakY = centerY + (radius - 2) * sin(peakAngle);
    sprite->fillCircle(peakX, peakY, 2, TFT_YELLOW);
}

void MiniVuMeter::drawLedStrip(TFT_eSprite *sprite) {
    const Rect &bounds = getBounds();
    uint16_t ledCount = bounds.width / 4;
    uint16_t activeLeds = (uint16_t)(currentLevel_ * ledCount);
    uint16_t peakLed = (uint16_t)(peakLevel_ * ledCount);
    for (uint16_t i = 0; i < ledCount; i++) {
        uint16_t ledX = i * 4;
        uint16_t ledColor;
        if (i < activeLeds) {
            float ledLevel = (float)i / ledCount;
            ledColor = levelToColor(ledLevel);
        } else if (i == peakLed) {
            ledColor = TFT_YELLOW;
        } else {
            ledColor = backgroundColor_;
        }
        sprite->fillRect(ledX, 0, 3, bounds.height, ledColor);
        if (i < ledCount - 1) {
            sprite->drawFastVLine(ledX + 3, 0, bounds.height, TFT_BLACK);
        }
    }
}

float MiniVuMeter::calculateRMSLevel() {
    AudioProcessor *processor = ::getAudioProcessor();
    if (!processor) {
        return 0.0f;
    }

    // Oszcilloszkóp adatok használata RMS számításhoz
    constexpr uint16_t SAMPLE_COUNT = 64;
    float samples[SAMPLE_COUNT];

    if (!processor->getOscilloscopeData(samples, SAMPLE_COUNT)) {
        return 0.0f;
    }

    // RMS számítás
    float sum = 0.0f;
    for (uint16_t i = 0; i < SAMPLE_COUNT; i++) {
        sum += samples[i] * samples[i];
    }

    float rms = sqrt(sum / SAMPLE_COUNT);

    // Normalizálás 0-1 tartományba
    return constrain(rms * 2.0f, 0.0f, 1.0f);
}

uint16_t MiniVuMeter::levelToColor(float level) {
    if (level < 0.3f) {
        return TFT_GREEN;
    } else if (level < 0.7f) {
        return TFT_YELLOW;
    } else if (level < 0.9f) {
        return TFT_ORANGE;
    } else {
        return TFT_RED;
    }
}
