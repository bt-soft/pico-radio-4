// src/FreqDisplay.cpp
#include "FreqDisplay.h"
#include "DSEG7_Classic_Mini_Regular_34.h" // 7-szegmenses font
#include "defines.h"                       // TFT_COLOR_BACKGROUND
#include "utils.h"                         // Utils::beepError

namespace FreqDisplayConstants {
// FREQ_7SEGMENT_HEIGHT a SevenSegmentFreq.h-ból
constexpr int FREQ_7SEGMENT_HEIGHT = 38;

// Digit pozíciók és méretek az aláhúzáshoz (bounds.x, bounds.y relatív)
constexpr int DigitXStart[] = {141, 171, 200}; // Ezek az X értékek a komponens bal szélétől (bounds.x) számítódnak
constexpr int DigitWidth = 25;
constexpr int DigitHeight = FREQ_7SEGMENT_HEIGHT;
constexpr int DigitYStart = 20;                                 // Relatív Y a bounds.y-hoz képest
constexpr int UnderlineYOffset = DigitYStart + DigitHeight + 2; // Aláhúzás Y pozíciója a számjegyek alatt
constexpr int UnderlineHeight = 5;

// Sprite és Unit pozíciók (bounds.y relatív)
constexpr uint16_t SpriteYOffset = 20; // A 7-szegmenses kijelző Y eltolása a komponens tetejétől (bounds.y)
constexpr uint16_t UnitXOffset = 5;    // Az egység ("kHz", "MHz") X eltolása a frekvencia kijelző jobb szélétől

// Referencia X pozíciók a jobb igazításhoz (bounds.x relatív)
// Ezek az értékek a 7-szegmenses kijelző jobb szélének X pozícióját jelentik a komponens bal széléhez (bounds.x) képest.
// A tényleges sprite rajzolási X pozíciója (spritePushX) ebből és a sprite szélességéből (contentWidth) számolódik.
constexpr uint16_t RefXDefault = 222;
constexpr uint16_t RefXSeek = 144;
constexpr uint16_t RefXBfo = 115;
constexpr uint16_t RefXFmAm = 190;

// BFO mód specifikus pozíciók és méretek (bounds.x, bounds.y relatív)
constexpr uint16_t BfoLabelRectXOffset = 156;
constexpr uint16_t BfoLabelRectYOffset = 21;
constexpr uint16_t BfoLabelRectW = 42;
constexpr uint16_t BfoLabelRectH = 20;
// constexpr uint16_t BfoLabelTextXOffset = 160; // Nem használjuk, a téglalap közepére rajzolunk
// constexpr uint16_t BfoLabelTextYOffset = 40;  // Nem használjuk
constexpr uint16_t BfoHzLabelXOffset = 120;                                      // "Hz" felirat X pozíciója a BFO frekvencia mellett
constexpr uint16_t BfoHzLabelYOffset = SpriteYOffset + FREQ_7SEGMENT_HEIGHT;     // "Hz" felirat Y pozíciója
constexpr uint16_t BfoMiniFreqX = 220;                                           // A kis méretű (háttér) frekvencia X pozíciója BFO módban
constexpr uint16_t BfoMiniFreqY = SpriteYOffset + FREQ_7SEGMENT_HEIGHT + 5;      // A kis méretű (háttér) frekvencia Y pozíciója
constexpr uint16_t BfoMiniUnitXOffset = 20;                                      // A kis méretű "kHz" X eltolása BFO módban
constexpr uint16_t SsbCwUnitXOffset = 215;                                       // "kHz" felirat X pozíciója SSB/CW módban (nem BFO)
constexpr uint16_t SsbCwUnitYOffset = SpriteYOffset + FREQ_7SEGMENT_HEIGHT + 20; // "kHz" felirat Y pozíciója SSB/CW módban (nem BFO)

} // namespace FreqDisplayConstants

// Színek
const FreqSegmentColors defaultNormalColors = {TFT_GOLD, TFT_COLOR(50, 50, 50), TFT_YELLOW};
const FreqSegmentColors defaultBfoColors = {TFT_ORANGE, TFT_BROWN, TFT_ORANGE};

FreqDisplay::FreqDisplay(TFT_eSPI &tft_param, const Rect &bounds_param, Band &band_ref, Config &config_ref)
    : UIComponent(tft_param, bounds_param), band(band_ref), config(config_ref), spr(&(this->tft)), normalColors(defaultNormalColors), bfoColors(defaultBfoColors),
      currentDisplayFrequency(0), bfoModeActiveLastDraw(rtv::bfoOn) {

    // Alapértelmezett háttérszín beállítása a globális háttérszínre
    this->colors.background = TFT_COLOR_BACKGROUND;

    // Kezdeti frekvencia beállítása
    // A BandTable.currFreq-t használjuk, mert az FMScreen::handleOwnLoop is ezt figyeli.
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
    // Az x_offset a 7-szegmenses kijelző jobb szélének X pozícióját jelenti
    // a komponens bal széléhez (bounds.x) képest.
    uint32_t x_offset_from_left = RefXDefault;

    if (rtv::SEEK) {
        x_offset_from_left = RefXSeek;
    } else if (rtv::bfoOn) {
        x_offset_from_left = RefXBfo;
    } else if (currentDemod == FM || currentDemod == AM) {
        x_offset_from_left = RefXFmAm;
    }
    // A sprite-ot jobb oldalra igazítjuk, tehát a sprite bal szélének pozíciója:
    // bounds.x + x_offset_from_left - sprite_szélessége
    // Ezt a drawFrequencyInternal-ban számoljuk ki, itt csak a referencia jobb szélét adjuk vissza.
    return x_offset_from_left;
}

void FreqDisplay::drawFrequencyInternal(const String &freq_str, const __FlashStringHelper *mask, const FreqSegmentColors &colors, const __FlashStringHelper *unit) {
    using namespace FreqDisplayConstants;

    spr.setFreeFont(&DSEG7_Classic_Mini_Regular_34);
    uint16_t contentWidth = spr.textWidth(mask); // A sprite szélessége a maszk alapján

    // A sprite jobb szélének X pozíciója a komponens bal széléhez képest
    uint32_t spriteRightEdgeX_relative = calcFreqSpriteXPosition();
    // A sprite bal szélének abszolút X pozíciója
    uint16_t spritePushX = bounds.x + spriteRightEdgeX_relative - contentWidth;
    // A sprite tetejének abszolút Y pozíciója
    uint16_t spritePushY = bounds.y + SpriteYOffset;

    spr.createSprite(contentWidth, FREQ_7SEGMENT_HEIGHT);
    spr.fillSprite(this->colors.background); // A komponens háttérszínét használjuk
    spr.setTextSize(1);
    spr.setTextPadding(0);
    spr.setFreeFont(&DSEG7_Classic_Mini_Regular_34);
    spr.setTextDatum(BR_DATUM); // Bottom Right datum a sprite-on belüli igazításhoz

    if (config.data.tftDigitLigth) {
        spr.setTextColor(colors.inactive);
        spr.drawString(mask, contentWidth, FREQ_7SEGMENT_HEIGHT); // A sprite jobb alsó sarkához igazítva
    }

    spr.setTextColor(colors.active);
    spr.drawString(freq_str, contentWidth, FREQ_7SEGMENT_HEIGHT); // A sprite jobb alsó sarkához igazítva

    spr.pushSprite(spritePushX, spritePushY);
    spr.deleteSprite();

    // Az egység ("kHz", "MHz") kirajzolása a sprite jobb oldalához
    if (unit != nullptr) {
        tft.setFreeFont(); // Alapértelmezett font az egységhez
        tft.setTextSize(2);
        tft.setTextDatum(BL_DATUM);                                  // Bottom Left datum
        tft.setTextColor(colors.indicator, this->colors.background); // Háttérszínnel rajzoljuk, hogy felülírja a régit
        // Az egység X pozíciója: a sprite abszolút jobb széle + UnitXOffset
        uint16_t unitX = spritePushX + contentWidth + UnitXOffset;
        // Az egység Y pozíciója: a sprite alja
        uint16_t unitY = spritePushY + FREQ_7SEGMENT_HEIGHT;
        tft.drawString(unit, unitX, unitY);
    }
}

void FreqDisplay::drawStepUnderline(const FreqSegmentColors &colors) {
    using namespace FreqDisplayConstants;
    // Az aláhúzás teljes területének X pozíciója és szélessége
    const int underlineAreaX_abs = bounds.x + DigitXStart[0];
    const int underlineAreaWidth = (DigitXStart[2] + DigitWidth) - DigitXStart[0];
    const int underlineAreaY_abs = bounds.y + UnderlineYOffset;

    if (isDisabled() || rtv::bfoOn) {
        // Ha le van tiltva vagy BFO módban van, töröljük az aláhúzást a komponens háttérszínével
        tft.fillRect(underlineAreaX_abs, underlineAreaY_abs, underlineAreaWidth, UnderlineHeight, this->colors.background);
        return;
    }

    // Először töröljük a teljes aláhúzási területet a háttérszínnel
    tft.fillRect(underlineAreaX_abs, underlineAreaY_abs, underlineAreaWidth, UnderlineHeight, this->colors.background);
    // Majd kirajzoljuk az aktív lépés aláhúzását az indikátor színnel
    // Az aktív aláhúzás X pozíciója: komponens bal széle + az adott digit relatív X pozíciója
    tft.fillRect(bounds.x + DigitXStart[rtv::freqstepnr], underlineAreaY_abs, DigitWidth, UnderlineHeight, colors.indicator);
}

const FreqSegmentColors &FreqDisplay::getSegmentColors() const { return rtv::bfoOn ? bfoColors : normalColors; }

void FreqDisplay::displaySsbCwFrequency(uint16_t currentFrequencyValue, const FreqSegmentColors &colors) {
    using namespace FreqDisplayConstants;
    BandTable &currentBand = band.getCurrentBand();
    // A BandTable-ben a lastBFO már a varData alatt van
    uint32_t bfoOffset = currentBand.lastBFO; // Itt a BandTable::lastBFO-t kellene használni, nem a varData.lastBFO-t, ha az a cél
                                              // De a SevenSegmentFreq a varData.lastBFO-t használta. Maradjunk ennél.
                                              // Viszont a Band.h-ban a BandTable-nek nincs varData tagja, csak közvetlenül a lastBFO.
                                              // Feltételezem, hogy a band.getCurrentBand() egy BandTable referenciát ad vissza,
                                              // és annak van lastBFO tagja.
                                              // Ha a BandTable-nek van varData tagja, akkor currentBand.varData.lastBFO a helyes.
                                              // A megadott Band.h alapján a BandTable-nek közvetlenül van lastBFO tagja.
    uint32_t displayFreqHz = (uint32_t)currentFrequencyValue * 1000 - bfoOffset;

    char s[12];
    long khz_part = displayFreqHz / 1000;
    int hz_tens_part = abs((int)(displayFreqHz % 1000)) / 10; // Csak a tized és század Hz kell
    sprintf(s, "%ld.%02d", khz_part, hz_tens_part);

    if (!rtv::bfoOn || rtv::bfoTr) { // Ha nincs BFO, vagy BFO animáció van
        tft.setFreeFont();
        tft.setTextDatum(BR_DATUM); // Bottom Right
        tft.setTextColor(colors.indicator, this->colors.background);

        if (rtv::bfoTr) { // BFO animáció
            rtv::bfoTr = false;
            // Az animáció a teljes komponenst érintheti, ezért a draw() elején lévő fillRect töröl.
            // Itt csak a szöveget rajzoljuk felül.
            for (uint8_t i = 4; i > 1; i--) {
                tft.setTextSize(rtv::bfoOn ? i : (6 - i));
                // A szöveg jobb alsó sarkának abszolút pozíciója
                tft.drawString(String(s), bounds.x + BfoMiniFreqX, bounds.y + BfoMiniFreqY);
                delay(50); // Csökkentett delay az animációhoz
            }
        }

        if (!rtv::bfoOn) {                                                     // Normál SSB/CW kijelzés (nem BFO mód)
            drawFrequencyInternal(String(s), F("88 888.88"), colors, nullptr); // Nincs külön unit, a "kHz" alatta lesz
            tft.setTextDatum(BC_DATUM);                                        // Bottom Center
            tft.setFreeFont();
            tft.setTextSize(2);
            tft.setTextColor(colors.indicator, this->colors.background);
            // A "kHz" felirat abszolút pozíciója
            tft.drawString(F("kHz"), bounds.x + SsbCwUnitXOffset, bounds.y + SsbCwUnitYOffset);
        }
    }

    if (rtv::bfoOn) { // BFO mód aktív
        // BFO érték kijelzése (pl. "-120")
        drawFrequencyInternal(String(config.data.currentBFOmanu), F("-888"), colors, nullptr); // Nincs unit mellette
        tft.setTextSize(2);
        tft.setTextDatum(BL_DATUM); // Bottom Left
        tft.setTextColor(colors.indicator, this->colors.background);
        // "Hz" felirat abszolút pozíciója
        tft.drawString("Hz", bounds.x + BfoHzLabelXOffset, bounds.y + BfoHzLabelYOffset);

        // "BFO" címke rajzolása
        uint16_t bfoRectAbsX = bounds.x + BfoLabelRectXOffset;
        uint16_t bfoRectAbsY = bounds.y + BfoLabelRectYOffset;
        tft.fillRect(bfoRectAbsX, bfoRectAbsY, BfoLabelRectW, BfoLabelRectH, colors.active);
        tft.setTextColor(TFT_BLACK, colors.active); // Szöveg a kitöltött téglalapon
        tft.setTextDatum(MC_DATUM);                 // Middle Center
        tft.drawString("BFO", bfoRectAbsX + BfoLabelRectW / 2, bfoRectAbsY + BfoLabelRectH / 2);

        // Kis méretű (háttér) frekvencia kijelzése
        tft.setTextSize(2);
        tft.setTextDatum(BR_DATUM); // Bottom Right
        tft.setTextColor(colors.indicator, this->colors.background);
        tft.drawString(String(s), bounds.x + BfoMiniFreqX, bounds.y + BfoMiniFreqY);

        // Kis méretű "kHz" egység
        tft.setTextSize(1);
        // A "kHz" abszolút X pozíciója: a mini frekvencia jobb széle + eltolás
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
        mask_val = F("188.88"); // FM maszk
        float displayFreqMHz = currentFrequencyValue / 100.0f;
        freqStr_val = String(displayFreqMHz, 2);
    } else { // AM (beleértve LW, MW, SW AM módjait)
        unit_val = F("kHz");
        if (currentBandType == MW_BAND_TYPE || currentBandType == LW_BAND_TYPE) {
            mask_val = F("8888"); // MW/LW maszk (pl. 1602)
            freqStr_val = String(currentFrequencyValue);
        } else {                    // SW AM (feltételezve, hogy a frekvencia kHz-ben van, de MHz-ben jelenítjük meg 3 tizedessel)
            mask_val = F("88.888"); // SW AM maszk (pl. 15.770 MHz)
            freqStr_val = String(currentFrequencyValue / 1000.0f, 3);
            unit_val = F("MHz"); // SW AM esetén is MHz-ben írjuk ki
        }
    }
    drawFrequencyInternal(freqStr_val, mask_val, colors, unit_val);
}

void FreqDisplay::draw() {
    // Ha az animáció aktív (bfoTr), akkor mindenképp rajzolunk.
    // Egyébként csak akkor, ha a needsRedraw flag (UIComponent-ből örökölt) be van állítva.
    if (!rtv::bfoTr && !needsRedraw) {
        return;
    }

    // Teljes komponens területének törlése a beállított háttérszínnel
    tft.fillRect(bounds.x, bounds.y, bounds.width, bounds.height, this->colors.background);

    const FreqSegmentColors &segment_colors = getSegmentColors();
    const uint8_t currDemod = band.getCurrentBand().currMod;

    if (currDemod == LSB || currDemod == USB || currDemod == CW) {
        displaySsbCwFrequency(currentDisplayFrequency, segment_colors);
    } else { // FM, AM
        displayFmAmFrequency(currentDisplayFrequency, segment_colors);
    }

    // Aláhúzás rajzolása (csak ha nincs BFO és engedélyezve van a komponens)
    drawStepUnderline(segment_colors);

    // BFO mód változásának detektálása a következő rajzoláshoz (nem feltétlenül szükséges itt,
    // mert a markForRedraw() gondoskodik róla, ha a bfoOn változik)
    // bfoModeActiveLastDraw = rtv::bfoOn;

    tft.setTextDatum(BC_DATUM); // Alapértelmezett text datum visszaállítása a TFT-n
    needsRedraw = false;        // Rajzolás után töröljük a flag-et
}

bool FreqDisplay::handleTouch(const TouchEvent &event) {
    if (isDisabled() || rtv::bfoOn) { // Ha le van tiltva vagy BFO módban van, nem kezeljük a digit érintést
        return false;
    }

    // A UIComponent::handleTouch már ellenőrzi a bounds-ot, de itt a specifikus digit területeket nézzük.
    // Ha a kattintás nem a komponensen belül van, akkor nem kezeljük.
    if (!bounds.contains(event.x, event.y)) {
        return false;
    }

    using namespace FreqDisplayConstants;
    bool eventHandled = false;

    // Az érintés Y koordinátája a komponens tetejéhez képest
    uint16_t relativeTouchY = event.y - bounds.y;

    // Ellenőrizzük, hogy az érintés a számjegyek magasságában történt-e
    if (relativeTouchY >= DigitYStart && relativeTouchY <= DigitYStart + DigitHeight) {
        for (int i = 0; i <= 2; ++i) { // 0, 1, 2 indexű digitek
            // Az adott digit X kezdőpozíciója abszolút koordinátában
            uint16_t digitStartX_abs = bounds.x + DigitXStart[i];
            // Ellenőrizzük, hogy az érintés X koordinátája az adott digit területén belül van-e
            if (event.x >= digitStartX_abs && event.x < digitStartX_abs + DigitWidth) {
                if (rtv::freqstepnr != i) { // Csak akkor csinálunk valamit, ha másik digitre kattintottunk
                    rtv::freqstepnr = i;
                    // A freqstep beállítása a SevenSegmentFreq logika alapján
                    if (rtv::freqstepnr == 0)
                        rtv::freqstep = 1000; // 1kHz
                    else if (rtv::freqstepnr == 1)
                        rtv::freqstep = 100; // 100Hz
                    else
                        rtv::freqstep = 10; // 10Hz
                    markForRedraw();        // Aláhúzás frissítéséhez kérünk újrarajzolást
                }
                eventHandled = true; // Az eseményt kezeltük
                break;               // Kilépünk a ciklusból, mert megtaláltuk az érintett digitet
            }
        }
    }
    return eventHandled;
}
