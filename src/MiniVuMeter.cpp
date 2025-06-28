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
    // Audio szint számítása
    float newLevel = calculateRMSLevel();

    // Simított szint frissítése
    if (newLevel > currentLevel_) {
        currentLevel_ = newLevel;
    } else {
        currentLevel_ = currentLevel_ * LEVEL_DECAY + newLevel * (1.0f - LEVEL_DECAY);
    }

    // Peak hold frissítése
    uint32_t currentTime = millis();
    if (currentLevel_ > peakLevel_) {
        peakLevel_ = currentLevel_;
        lastPeakTime_ = currentTime;
    } else if (currentTime - lastPeakTime_ > peakHoldTime_) {
        peakLevel_ = currentLevel_;
    }

    // Rajzolás a kiválasztott stílus szerint
    switch (style_) {
        case Style::HORIZONTAL_BAR:
            drawHorizontalBar();
            break;
        case Style::VERTICAL_BAR:
            drawVerticalBar();
            break;
        case Style::NEEDLE:
            drawNeedle();
            break;
        case Style::LED_STRIP:
            drawLedStrip();
            break;
    }
}

void MiniVuMeter::drawHorizontalBar() {
    const Rect &bounds = getBounds();

    // Háttér
    tft.fillRect(bounds.x, bounds.y, bounds.width, bounds.height, backgroundColor_);

    // Szint sáv
    uint16_t levelWidth = (uint16_t)(currentLevel_ * bounds.width);
    if (levelWidth > 0) {
        uint16_t levelColor = levelToColor(currentLevel_);
        tft.fillRect(bounds.x, bounds.y, levelWidth, bounds.height, levelColor);
    }

    // Peak vonal
    uint16_t peakX = bounds.x + (uint16_t)(peakLevel_ * bounds.width);
    if (peakX < bounds.x + bounds.width) {
        tft.drawFastVLine(peakX, bounds.y, bounds.height, TFT_WHITE);
    }

    // Keret
    tft.drawRect(bounds.x, bounds.y, bounds.width, bounds.height, primaryColor_);
}

void MiniVuMeter::drawVerticalBar() {
    const Rect &bounds = getBounds();

    // Háttér
    tft.fillRect(bounds.x, bounds.y, bounds.width, bounds.height, backgroundColor_);

    // Szint sáv (alulról felfelé)
    uint16_t levelHeight = (uint16_t)(currentLevel_ * bounds.height);
    if (levelHeight > 0) {
        uint16_t levelColor = levelToColor(currentLevel_);
        uint16_t levelY = bounds.y + bounds.height - levelHeight;
        tft.fillRect(bounds.x, levelY, bounds.width, levelHeight, levelColor);
    }

    // Peak vonal
    uint16_t peakY = bounds.y + bounds.height - (uint16_t)(peakLevel_ * bounds.height);
    if (peakY > bounds.y) {
        tft.drawFastHLine(bounds.x, peakY, bounds.width, TFT_WHITE);
    }

    // Keret
    tft.drawRect(bounds.x, bounds.y, bounds.width, bounds.height, primaryColor_);
}

void MiniVuMeter::drawNeedle() {
    const Rect &bounds = getBounds();

    // Háttér (kör vagy félkör)
    tft.fillRect(bounds.x, bounds.y, bounds.width, bounds.height, backgroundColor_);

    // Skála rajzolása
    int16_t centerX = bounds.x + bounds.width / 2;
    int16_t centerY = bounds.y + bounds.height - 5;
    int16_t radius = min(bounds.width, bounds.height) / 2 - 5;

    // Skála vonalak
    for (int i = 0; i <= 10; i++) {
        float angle = -PI + (PI * i / 10.0f);
        int16_t x1 = centerX + (radius - 5) * cos(angle);
        int16_t y1 = centerY + (radius - 5) * sin(angle);
        int16_t x2 = centerX + radius * cos(angle);
        int16_t y2 = centerY + radius * sin(angle);

        tft.drawLine(x1, y1, x2, y2, secondaryColor_);
    }

    // Tű rajzolása
    float needleAngle = -PI + (PI * currentLevel_);
    int16_t needleX = centerX + (radius - 2) * cos(needleAngle);
    int16_t needleY = centerY + (radius - 2) * sin(needleAngle);

    tft.drawLine(centerX, centerY, needleX, needleY, TFT_RED);
    tft.fillCircle(centerX, centerY, 2, TFT_WHITE);
}

void MiniVuMeter::drawLedStrip() {
    const Rect &bounds = getBounds();

    // LED-ek száma
    uint16_t ledCount = bounds.width / 4; // 4 pixel széles LED-ek
    uint16_t activeLeds = (uint16_t)(currentLevel_ * ledCount);
    uint16_t peakLed = (uint16_t)(peakLevel_ * ledCount);

    for (uint16_t i = 0; i < ledCount; i++) {
        uint16_t ledX = bounds.x + i * 4;
        uint16_t ledColor;

        if (i < activeLeds) {
            // Aktív LED színe a szint alapján
            float ledLevel = (float)i / ledCount;
            ledColor = levelToColor(ledLevel);
        } else if (i == peakLed) {
            // Peak LED
            ledColor = TFT_WHITE;
        } else {
            // Inaktív LED
            ledColor = backgroundColor_;
        }

        tft.fillRect(ledX, bounds.y, 3, bounds.height, ledColor);

        // LED közötti tér
        if (i < ledCount - 1) {
            tft.drawFastVLine(ledX + 3, bounds.y, bounds.height, TFT_BLACK);
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
