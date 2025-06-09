// src/FreqDisplay.cpp
#include "FreqDisplay.h"
#include "DSEG7_Classic_Mini_Regular_34.h" // 7-szegmenses font
#include "utils.h"                         // Utils::beepError

namespace FreqDisplayConstants {
// FREQ_7SEGMENT_HEIGHT a SevenSegmentFreq.h-ból
constexpr int FREQ_7SEGMENT_HEIGHT = 38;

// Digit pozíciók és méretek az aláhúzáshoz (bounds.x, bounds.y relatív)
constexpr int DigitXStart[] = {141, 171, 200};
constexpr int DigitWidth = 25;
constexpr int DigitHeight = FREQ_7SEGMENT_HEIGHT;
constexpr int DigitYStart = 20; // Relatív Y a bounds.y-hoz képest
constexpr int UnderlineYOffset = 60;
constexpr int UnderlineHeight = 5;

// Sprite és Unit pozíciók (bounds.y relatív)
constexpr uint16_t SpriteYOffset = 20;
constexpr uint16_t UnitXOffset = 5;

// Referencia X pozíciók a jobb igazításhoz (bounds.x relatív)
constexpr uint16_t RefXDefault = 222;
constexpr uint16_t RefXSeek = 144;
constexpr uint16_t RefXBfo = 115;
constexpr uint16_t RefXFmAm = 190;

// BFO mód specifikus pozíciók és méretek (bounds.x, bounds.y relatív)
constexpr uint16_t BfoLabelRectXOffset = 156;
constexpr uint16_t BfoLabelRectYOffset = 21;
constexpr uint16_t BfoLabelRectW = 42;
constexpr uint16_t BfoLabelRectH = 20;
constexpr uint16_t BfoLabelTextXOffset = 160;
constexpr uint16_t BfoLabelTextYOffset = 40;
constexpr uint16_t BfoHzLabelXOffset = 120;
constexpr uint16_t BfoHzLabelYOffset = 40;
constexpr uint16_t BfoMiniFreqX = 220;
constexpr uint16_t BfoMiniFreqY = 62;
constexpr uint16_t BfoMiniUnitXOffset = 20;
constexpr uint16_t SsbCwUnitXOffset = 215;
constexpr uint16_t SsbCwUnitYOffset = 80;

// Törlési terület konstansai (bounds.x, bounds.y relatív)
// Ezeket a clearPartialDisplayArea használja, a teljes bounds törlését a UIComponent végzi.
constexpr uint16_t ClearAreaBaseWidth = 240;
constexpr uint16_t ClearAreaHeightCorrection = UnderlineHeight + 15;
} // namespace FreqDisplayConstants

// Színek
const FreqSegmentColors defaultNormalColors = {TFT_GOLD, TFT_COLOR(50, 50, 50), TFT_YELLOW};
const FreqSegmentColors defaultBfoColors = {TFT_ORANGE, TFT_BROWN, TFT_ORANGE};

FreqDisplay::FreqDisplay(TFT_eSPI &tft_param, const Rect &bounds_param, Band &band_ref, Config &config_ref)
    : UIComponent(tft_param, bounds_param), band(band_ref), config(config_ref), spr(&(this->tft)), normalColors(defaultNormalColors), bfoColors(defaultBfoColors),
      currentDisplayFrequency(0), bfoModeActiveLastDraw(rtv::bfoOn) {
    // Kezdeti frekvencia beállítása (pl. a band objektumból)
    setFrequency(band.getCurrentBand().currFreq);
}

void FreqDisplay::setFrequency(uint16_t freq) {
    if (currentDisplayFrequency != freq) {
        currentDisplayFrequency = freq;
        markForRedraw();
    }
}

uint32_t FreqDisplay::calcFreqSpriteXPosition() const {
    using namespace FreqDisplayConstants;
    uint8_t currentDemod = band.getCurrentBand().currMod;
    uint32_t x_offset = RefXDefault; // Ez egy relatív X eltolás a bounds.x-hez képest

    if (rtv::SEEK) {
        x_offset = RefXSeek;
    } else if (rtv::bfoOn) {
        x_offset = RefXBfo;
    } else if (currentDemod == FM || currentDemod == AM) {
        x_offset = RefXFmAm;
    }
    return x_offset;
}

void FreqDisplay::drawFrequencyInternal(const String &freq_str, const __FlashStringHelper *mask, const FreqSegmentColors &colors, const __FlashStringHelper *unit) {
    using namespace FreqDisplayConstants;

    spr.setFreeFont(&DSEG7_Classic_Mini_Regular_34);
    uint16_t contentWidth = spr.textWidth(mask);

    uint32_t relative_x_offset = calcFreqSpriteXPosition();
    uint16_t spritePushX = bounds.x + relative_x_offset - contentWidth;
    uint16_t spritePushY = bounds.y + SpriteYOffset;

    spr.createSprite(contentWidth, FREQ_7SEGMENT_HEIGHT);
    spr.fillSprite(this->colors.background); // UIComponent háttérszínét használjuk
    spr.setTextSize(1);
    spr.setTextPadding(0);
    spr.setFreeFont(&DSEG7_Classic_Mini_Regular_34);
    spr.setTextDatum(BR_DATUM);

    if (config.data.tftDigitLigth) {
        spr.setTextColor(colors.inactive);
        spr.drawString(mask, contentWidth, FREQ_7SEGMENT_HEIGHT);
    }

    spr.setTextColor(colors.active);
    spr.drawString(freq_str, contentWidth, FREQ_7SEGMENT_HEIGHT);

    spr.pushSprite(spritePushX, spritePushY);
    spr.deleteSprite();

    uint16_t spriteRightEdgeX = spritePushX + contentWidth;

    if (unit != nullptr) {
        tft.setFreeFont();
        tft.setTextSize(2);
        tft.setTextDatum(BL_DATUM);
        tft.setTextColor(colors.indicator, this->colors.background);
        uint16_t unitX = spriteRightEdgeX + UnitXOffset;
        uint16_t unitY = spritePushY + FREQ_7SEGMENT_HEIGHT;
        tft.drawString(unit, unitX, unitY);
    }
}

void FreqDisplay::drawStepUnderline(const FreqSegmentColors &colors) {
    if (isDisabled() || rtv::bfoOn) {
        // Ha le van tiltva vagy BFO módban van, töröljük az aláhúzást
        const int underlineAreaWidth = (FreqDisplayConstants::DigitXStart[2] + FreqDisplayConstants::DigitWidth) - FreqDisplayConstants::DigitXStart[0];
        tft.fillRect(bounds.x + FreqDisplayConstants::DigitXStart[0], bounds.y + FreqDisplayConstants::UnderlineYOffset, underlineAreaWidth, FreqDisplayConstants::UnderlineHeight,
                     this->colors.background);
        return;
    }

    using namespace FreqDisplayConstants;
    const int underlineAreaWidth = (DigitXStart[2] + DigitWidth) - DigitXStart[0];
    tft.fillRect(bounds.x + DigitXStart[0], bounds.y + UnderlineYOffset, underlineAreaWidth, UnderlineHeight, this->colors.background);
    tft.fillRect(bounds.x + DigitXStart[rtv::freqstepnr], bounds.y + UnderlineYOffset, DigitWidth, UnderlineHeight, colors.indicator);
}

const FreqSegmentColors &FreqDisplay::getSegmentColors() const { return rtv::bfoOn ? bfoColors : normalColors; }

void FreqDisplay::clearPartialDisplayArea() {
    // Ez a metódus a frekvencia és unit területét törli, az aláhúzást külön kezeli a drawStepUnderline.
    // A teljes bounds törlését a UIComponent::draw() -> draw() elején kellene elvégezni.
    // Itt csak a dinamikusan változó részeket töröljük, ha szükséges.
    // A sprite maga fillSprite-tel törlődik. Az unit és BFO feliratoknak kell háttérszínnel rajzolódniuk.
    // A BFO animáció miatt a teljes területet törölni kellhet.
    // A UIComponent::draw() elején egy this->tft.fillRect(bounds.x, bounds.y, bounds.width, bounds.height, this->colors.background);
    // hívás gondoskodik a teljes komponens területének törléséről.
}

void FreqDisplay::displaySsbCwFrequency(uint16_t currentFrequencyValue, const FreqSegmentColors &colors) {
    using namespace FreqDisplayConstants;
    BandTable &currentBand = band.getCurrentBand();
    uint32_t bfoOffset = currentBand.lastBFO;
    uint32_t displayFreqHz = (uint32_t)currentFrequencyValue * 1000 - bfoOffset;

    char s[12];
    long khz_part = displayFreqHz / 1000;
    int hz_tens_part = abs((int)(displayFreqHz % 1000)) / 10;
    sprintf(s, "%ld.%02d", khz_part, hz_tens_part);

    if (!rtv::bfoOn || rtv::bfoTr) {
        tft.setFreeFont();
        tft.setTextDatum(BR_DATUM);
        tft.setTextColor(colors.indicator, this->colors.background);

        if (rtv::bfoTr) {
            rtv::bfoTr = false;
            for (uint8_t i = 4; i > 1; i--) {
                tft.setTextSize(rtv::bfoOn ? i : (6 - i));
                // A teljes komponens területét újra kell rajzolni az animációhoz
                // Ezért a drawSelf()-ben kell a fő törlést végezni.
                // Itt csak a specifikus részeket rajzoljuk.
                // A clearPartialDisplayArea() helyett a drawSelf() elején lévő fillRect töröl.
                tft.drawString(String(s), bounds.x + BfoMiniFreqX, bounds.y + BfoMiniFreqY);
                delay(100); // TODO: Ezt az animációt máshogy kellene megoldani, ne blokkoljon.
            }
        }

        if (!rtv::bfoOn) {
            drawFrequencyInternal(String(s), F("88 888.88"), colors, nullptr);
            tft.setTextDatum(BC_DATUM);
            tft.setFreeFont();
            tft.setTextSize(2);
            tft.setTextColor(colors.indicator, this->colors.background);
            tft.drawString(F("kHz"), bounds.x + SsbCwUnitXOffset, bounds.y + SsbCwUnitYOffset);
            // Az aláhúzást a drawSelf() végén rajzoljuk
        }
    }

    if (rtv::bfoOn) {
        drawFrequencyInternal(String(config.data.currentBFOmanu), F("-888"), colors, nullptr);
        tft.setTextSize(2);
        tft.setTextDatum(BL_DATUM);
        tft.setTextColor(colors.indicator, this->colors.background);
        tft.drawString("Hz", bounds.x + BfoHzLabelXOffset, bounds.y + BfoHzLabelYOffset);

        tft.setTextColor(TFT_BLACK, colors.active);
        tft.fillRect(bounds.x + BfoLabelRectXOffset, bounds.y + BfoLabelRectYOffset, BfoLabelRectW, BfoLabelRectH, colors.active);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("BFO", bounds.x + BfoLabelRectXOffset + BfoLabelRectW / 2, bounds.y + BfoLabelRectYOffset + BfoLabelRectH / 2);

        tft.setTextSize(2);
        tft.setTextDatum(BR_DATUM);
        tft.setTextColor(colors.indicator, this->colors.background);
        tft.drawString(String(s), bounds.x + BfoMiniFreqX, bounds.y + BfoMiniFreqY);

        tft.setTextSize(1);
        tft.drawString("kHz", bounds.x + BfoMiniFreqX + BfoMiniUnitXOffset, bounds.y + BfoMiniFreqY);
    }
}

void FreqDisplay::displayFmAmFrequency(uint16_t currentFrequencyValue, const FreqSegmentColors &colors) {
    String freqStr_val;
    const __FlashStringHelper *unit_val = nullptr;
    const __FlashStringHelper *mask_val = nullptr;
    uint8_t currentBandType = band.getCurrentBandType();
    uint8_t currDemod = band.getCurrentBand().currMod;

    if (currDemod == FM) {
        unit_val = F("MHz");
        mask_val = F("188.88");
        float displayFreqMHz = currentFrequencyValue / 100.0f;
        freqStr_val = String(displayFreqMHz, 2);
    } else {
        unit_val = F("kHz");
        mask_val = (currentBandType == MW_BAND_TYPE || currentBandType == LW_BAND_TYPE) ? F("8888") : F("88.888");
        freqStr_val = (currentBandType == MW_BAND_TYPE || currentBandType == LW_BAND_TYPE) ? String(currentFrequencyValue) : String(currentFrequencyValue / 1000.0f, 3);
        if (currentBandType != MW_BAND_TYPE && currentBandType != LW_BAND_TYPE)
            unit_val = F("MHz");
    }
    drawFrequencyInternal(freqStr_val, mask_val, colors, unit_val);
}

void FreqDisplay::draw() {
    // Teljes komponens területének törlése a háttérszínnel
    tft.fillRect(bounds.x, bounds.y, bounds.width, bounds.height, this->colors.background);

    const FreqSegmentColors &segment_colors = getSegmentColors();
    const uint8_t currDemod = band.getCurrentBand().currMod;

    if (currDemod == LSB || currDemod == USB || currDemod == CW) {
        displaySsbCwFrequency(currentDisplayFrequency, segment_colors);
    } else {
        displayFmAmFrequency(currentDisplayFrequency, segment_colors);
    }

    // Aláhúzás rajzolása (csak ha nincs BFO és engedélyezve van)
    drawStepUnderline(segment_colors);

    // BFO mód változásának detektálása a következő rajzoláshoz
    bfoModeActiveLastDraw = rtv::bfoOn;
    tft.setTextDatum(BC_DATUM); // Alapértelmezett visszaállítása
}

bool FreqDisplay::handleTouch(const TouchEvent &event) {
    if (isDisabled() || rtv::bfoOn) {
        return false;
    }

    // Csak akkor kezeljük, ha a komponens határain belül van az érintés
    // A UIComponent::handleTouch már ellenőrzi a bounds-ot, de itt a specifikus digit területeket nézzük.
    if (!bounds.contains(event.x, event.y)) {
        return false;
    }

    using namespace FreqDisplayConstants;
    bool eventHandled = false;

    // Az érintés Y koordinátája a komponens tetejéhez képest
    uint16_t relativeTy = event.y - bounds.y;

    if (relativeTy >= DigitYStart && relativeTy <= DigitYStart + DigitHeight) {
        for (int i = 0; i <= 2; ++i) {
            uint16_t digitStartX_abs = bounds.x + DigitXStart[i];
            if (event.x >= digitStartX_abs && event.x < digitStartX_abs + DigitWidth) {
                if (rtv::freqstepnr != i) {
                    rtv::freqstepnr = i;
                    if (rtv::freqstepnr == 0)
                        rtv::freqstep = 1000;
                    else if (rtv::freqstepnr == 1)
                        rtv::freqstep = 100;
                    else
                        rtv::freqstep = 10;
                    markForRedraw(); // Aláhúzás frissítéséhez
                }
                eventHandled = true;
                break;
            }
        }
    }
    return eventHandled;
}
