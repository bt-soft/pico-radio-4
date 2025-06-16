// src/FreqDisplay.cpp
/**
 * @file FreqDisplay.cpp
 * @brief Újratervezett frekvencia kijelző komponens implementáció
 *
 * Egyszerűsített és tiszta implementáció az alábbi megjelenítési módokhoz:
 * 1) FM/LW/AM: Mértékegység jobbra, frekvencia tőle balra
 * 2) SSB/CW: Frekvencia jobbra, finomhangolás aláhúzás, mértékegység alul
 * 3) Képernyővédő: Mint fent, de nincs finomhangolás
 */

#include "FreqDisplay.h"
#include "DSEG7_Classic_Mini_Regular_34.h" // 7-szegmenses font
#include "UIColorPalette.h"                // Centralizált színkonstansok
#include "defines.h"                       // TFT_COLOR_BACKGROUND
#include "utils.h"                         // Utils::beepError

// === Globális színkonfigurációk ===
const FreqSegmentColors defaultNormalColors = UIColorPalette::createNormalFreqColors();
const FreqSegmentColors defaultBfoColors = UIColorPalette::createBfoFreqColors();

// === Karakterszélesség konstansok (DSEG7_Classic_Mini_Regular_34 font) ===
// Valós mért értékek a font-ból
constexpr static int CHAR_WIDTH_DIGIT = 25; // '0'-'9' karakterek szélessége
constexpr static int CHAR_WIDTH_DOT = 3;    // '.' karakter szélessége
constexpr static int CHAR_WIDTH_SPACE = 1;  // ' ' karakter szélessége
constexpr static int CHAR_WIDTH_DASH = 23;  // '-' karakter szélessége

/**
 * @brief FreqDisplay konstruktor - inicializálja a frekvencia kijelző komponenst
 */
FreqDisplay::FreqDisplay(TFT_eSPI &tft_param, const Rect &bounds_param, Si4735Manager *pSi4735Manager)
    : UIComponent(tft_param, bounds_param), pSi4735Manager(pSi4735Manager), spr(&(this->tft)), normalColors(defaultNormalColors), bfoColors(defaultBfoColors),
      customColors(defaultNormalColors), useCustomColors(false), currentDisplayFrequency(0), hideUnderline(false), lastUpdateTime(0), needsFullClear(true) {

    // Alapértelmezett háttérszín beállítása
    this->colors.background = TFT_COLOR_BACKGROUND; // Érintési területek inicializálása
    for (int i = 0; i < 3; i++) {
        ssbCwTouchDigitAreas[i][0] = 0;
        ssbCwTouchDigitAreas[i][1] = 0;
    }

    // Explicit újrarajzolás kérése az első megjelenítéshez
    markForRedraw();

    // Explicit újrarajzolás kérése az első megjelenítéshez
    markForRedraw();
}

/**
 * @brief Beállítja a megjelenítendő frekvenciát
 */
void FreqDisplay::setFrequency(uint16_t freq, bool forceRedraw) {
    if (forceRedraw || currentDisplayFrequency != freq) {
        unsigned long currentTime = millis();

        // Villogás csökkentése: csak akkor frissítünk, ha legalább 50ms eltelt az előző frissítés óta
        // KIVÉVE ha forceRedraw = true vagy jelentős változás van (>10 egység)
        if (forceRedraw || (currentTime - lastUpdateTime > 50) || abs((int16_t)freq - (int16_t)currentDisplayFrequency) > 10) {

            currentDisplayFrequency = freq;
            lastUpdateTime = currentTime;
            markForRedraw();
        } else {
            // Csak a frekvencia értéket frissítjük, de nem rajzolunk újra azonnal
            currentDisplayFrequency = freq;
        }
    }
}

/**
 * @brief Beállítja a megjelenítendő frekvenciát teljes újrarajzolással
 */
void FreqDisplay::setFrequencyWithFullDraw(uint16_t freq, bool hideUnderline) {
    currentDisplayFrequency = freq;
    this->hideUnderline = hideUnderline;
    needsFullClear = true; // Teljes háttér törlés szükséges
    markForRedraw();
}

/**
 * @brief Beállítja az egyedi színkonfigurációt (pl. képernyővédő módhoz)
 */
void FreqDisplay::setCustomColors(const FreqSegmentColors &colors) {
    customColors = colors;
    useCustomColors = true;
    needsFullClear = true; // Színváltásnál teljes háttér törlés szükséges
    markForRedraw();
}

/**
 * @brief Visszaállítja az alapértelmezett színkonfigurációt
 */
void FreqDisplay::resetToDefaultColors() {
    useCustomColors = false;
    markForRedraw();
}

/**
 * @brief Beállítja, hogy megjelenjen-e a finomhangolás aláhúzás (képernyővédő mód)
 */
void FreqDisplay::setHideUnderline(bool hide) {
    if (hideUnderline != hide) {
        hideUnderline = hide;
        markForRedraw();
    }
}

/**
 * @brief Visszaadja az aktuális színkonfigurációt
 */
const FreqSegmentColors &FreqDisplay::getSegmentColors() const {
    if (useCustomColors) {
        return customColors;
    }
    return rtv::bfoOn ? bfoColors : normalColors;
}

/**
 * @brief Ellenőrzi, hogy SSB/CW mód van-e aktív
 */
bool FreqDisplay::isInSsbCwMode() const {
    uint8_t currentDemod = pSi4735Manager->getCurrentBand().currMod;
    return (currentDemod == LSB || currentDemod == USB || currentDemod == CW);
}

/**
 * @brief Meghatározza a frekvencia formátumot és adatokat a mód alapján
 */
FreqDisplay::FrequencyDisplayData FreqDisplay::getFrequencyDisplayData(uint16_t frequency) {
    FrequencyDisplayData data;
    uint8_t demodMode = pSi4735Manager->getCurrentBand().currMod;
    uint8_t bandType = pSi4735Manager->getCurrentBandType();

    if (demodMode == FM) {
        // FM mód: 100.50 MHz
        data.unit = "MHz";
        data.mask = "188.88";
        float displayFreqMHz = frequency / 100.0f;
        data.freqStr = String(displayFreqMHz, 2);
    } else if (demodMode == AM) {
        if (bandType == MW_BAND_TYPE || bandType == LW_BAND_TYPE) {
            // MW/LW: 1440 kHz
            data.unit = "kHz";
            data.mask = "8888";
            data.freqStr = String(frequency);
        } else {
            // SW AM: 15.230 MHz
            data.unit = "MHz";
            data.mask = "88.888";
            data.freqStr = String(frequency / 1000.0f, 3);
        }
    } else if (demodMode == LSB || demodMode == USB || demodMode == CW) {
        // SSB/CW mód: finomhangolással korrigált frekvencia
        if (rtv::bfoOn) {
            // BFO mód: csak BFO értéket mutatunk
            data.unit = "Hz";
            data.mask = "-888";
            data.freqStr = String(rtv::currentBFOmanu);
        } else { // Normál SSB/CW: frekvencia formázás            // Normál SSB/CW: egyszerű formázás, mint a korábbi implementációban
            data.unit = "kHz";
            data.mask = "88 888.88";
            uint32_t displayFreqHz = (uint32_t)frequency * 1000 - rtv::freqDec;
            long khz_part = displayFreqHz / 1000;
            int hz_tens_part = abs((int)(displayFreqHz % 1000)) / 10; // Egyszerű formázás: space-szel a maszk formátumának megfelelően
            // Maszk: "88 888.88" -> Frekvencia: "xx xxx.xx" (space a 2. pozíció után)
            char buffer[16];

            // Először a kHz részt és Hz részt külön formázzuk
            // 7074 -> "7074", majd "7 074" lesz belőle
            char khz_str[8];
            sprintf(khz_str, "%ld", khz_part);

            // Space beszúrása a megfelelő helyre
            String khz_with_space = String(khz_str);
            int len = khz_with_space.length();

            if (len > 2) {
                // Ha 3 vagy több digit, akkor space-t teszünk a végétől számított 3. pozíció elé
                // "7074" -> "7 074", "12345" -> "12 345"
                khz_with_space = khz_with_space.substring(0, len - 3) + " " + khz_with_space.substring(len - 3);
            }

            // Összerakjuk a teljes frekvencia stringet
            sprintf(buffer, "%s.%02d", khz_with_space.c_str(), hz_tens_part);
            data.freqStr = String(buffer);
        }
    }

    return data;
}

/**
 * @brief Segédmetódus szöveg rajzolásához
 */
void FreqDisplay::drawText(const String &text, int x, int y, int textSize, uint8_t datum, uint16_t color) {
    tft.setFreeFont();
    tft.setTextSize(textSize);
    tft.setTextDatum(datum);
    tft.setTextColor(color, this->colors.background);
    tft.drawString(text, x, y);
}

/**
 * @brief Rajzolja FM/AM/LW stílusú frekvencia kijelzőt (mértékegység jobbra)
 */
void FreqDisplay::drawFmAmLwStyle(const FrequencyDisplayData &data) {
    const FreqSegmentColors &colors = getSegmentColors(); // 1. Mértékegység pozicionálása: keret jobb szélénél, digitek alsó vonalával egy magasságban
    int unitX = bounds.x + bounds.width - 5;              // 5 pixel margin a jobb szélétől
    int unitY = bounds.y + FREQ_7SEGMENT_HEIGHT;          // Digitek alsó vonalával egy magasságban

    // Mértékegység szöveg szélességének mérése
    tft.setFreeFont();
    tft.setTextSize(UNIT_TEXT_SIZE);
    int unitWidth = tft.textWidth(data.unit);

    // Finális mértékegység pozíció (jobbra igazítva)
    int finalUnitX = unitX - unitWidth;

    // Mértékegység rajzolása
    drawText(data.unit, finalUnitX, unitY, UNIT_TEXT_SIZE, BL_DATUM, colors.indicator);

    // 2. Frekvencia sprite pozicionálása: mértékegység bal szélétől balra
    spr.setFreeFont(&DSEG7_Classic_Mini_Regular_34);
    int freqSpriteWidth = spr.textWidth(data.mask);

    // Frekvencia sprite jobb széle legyen a mértékegység bal szélétől balra (kis gap)
    int freqSpriteRightX = finalUnitX - 8; // 8 pixel gap
    int freqSpriteX = freqSpriteRightX - freqSpriteWidth;
    int freqSpriteY = bounds.y;

    // Frekvencia sprite létrehozása és rajzolása
    spr.createSprite(freqSpriteWidth, FREQ_7SEGMENT_HEIGHT);
    spr.fillSprite(this->colors.background);
    spr.setTextSize(1);
    spr.setTextPadding(0);
    spr.setFreeFont(&DSEG7_Classic_Mini_Regular_34);
    spr.setTextDatum(BR_DATUM); // Jobb alsó sarokhoz igazítás

    // Inaktív számjegyek rajzolása (ha engedélyezve van)
    if (config.data.tftDigitLigth) {
        spr.setTextColor(colors.inactive);
        spr.drawString(data.mask, freqSpriteWidth, FREQ_7SEGMENT_HEIGHT);
    }

    // Aktív frekvencia számok rajzolása
    spr.setTextColor(colors.active);
    spr.drawString(data.freqStr, freqSpriteWidth, FREQ_7SEGMENT_HEIGHT);

    // Sprite kirajzolása és memória felszabadítása
    spr.pushSprite(freqSpriteX, freqSpriteY);
    spr.deleteSprite();
}

/**
 * @brief Rajzolja SSB/CW stílusú frekvencia kijelzőt (maszk jobbra, finomhangolás, mértékegység alul)
 */
void FreqDisplay::drawSsbCwStyle(const FrequencyDisplayData &data) {
    const FreqSegmentColors &colors = getSegmentColors();

    // 1. Frekvencia sprite pozicionálása: keret jobb szélénél (maszk bal oldala jobbra igazítva)
    int freqSpriteRightX = bounds.x + bounds.width - 5; // 5 pixel margin a jobb szélétől

    // Sprite szélesség számítása space-ekkel együtt
    int freqSpriteWidth = calculateSpriteWidthWithSpaces(data.mask);
    int freqSpriteX = freqSpriteRightX - freqSpriteWidth;
    int freqSpriteY = bounds.y;

    // Frekvencia sprite rajzolása space-ekkel
    drawFrequencySpriteWithSpaces(data, freqSpriteX, freqSpriteY, freqSpriteWidth);

    // 2. Finomhangolás aláhúzás rajzolása (ha nem elrejtett és nem BFO mód)
    if (!hideUnderline && !rtv::bfoOn) {
        drawFineTuningUnderline(freqSpriteX, freqSpriteWidth);

        // Érintési területek kiszámítása az aláhúzáshoz
        calculateSsbCwTouchAreas(freqSpriteX, freqSpriteWidth);
    }

    // 3. Mértékegység pozicionálása: finomhangolás alatt, jobbra igazítva
    int unitY = bounds.y + FREQ_7SEGMENT_HEIGHT + UNIT_Y_OFFSET_SSB_CW;
    if (rtv::bfoOn) {
        // BFO módban nincs aláhúzás, ezért a mértékegység közelebb van
        unitY = bounds.y + FREQ_7SEGMENT_HEIGHT + 8;
    }

    drawText(data.unit, freqSpriteRightX, unitY, UNIT_TEXT_SIZE, BR_DATUM, colors.indicator);
}

/**
 * @brief Kiszámítja a sprite szélességét space karakterekkel együtt
 */
int FreqDisplay::calculateSpriteWidthWithSpaces(const char *mask) {
    // Temporary sprite létrehozása a pontos méréshez
    TFT_eSprite tempSpr(&tft);
    tempSpr.setFreeFont(&DSEG7_Classic_Mini_Regular_34);
    tempSpr.setTextSize(1);

    const int SPACE_GAP_WIDTH = 8; // Space karakter helyett használt gap
    int totalWidth = 0;
    int maskLen = strlen(mask);
    for (int i = 0; i < maskLen; i++) {
        if (mask[i] == ' ') {
            totalWidth += SPACE_GAP_WIDTH;
        } else {
            // Optimalizált karakterszélesség lekérdezés
            totalWidth += getCharacterWidth(mask[i]);
        }
    }

    return totalWidth;
}

/**
 * @brief Rajzolja a frekvencia sprite-ot space karakterekkel
 */
void FreqDisplay::drawFrequencySpriteWithSpaces(const FrequencyDisplayData &data, int x, int y, int width) {
    const FreqSegmentColors &colors = getSegmentColors();

    // Sprite létrehozása
    spr.createSprite(width, FREQ_7SEGMENT_HEIGHT);
    spr.fillSprite(this->colors.background);
    spr.setTextSize(1);
    spr.setTextPadding(0);
    spr.setFreeFont(&DSEG7_Classic_Mini_Regular_34);
    spr.setTextDatum(BR_DATUM); // Jobb alsó sarokhoz igazítás (mint a korábbi implementációban)

    // Inaktív számjegyek rajzolása (ha engedélyezve van)
    if (config.data.tftDigitLigth) {
        spr.setTextColor(colors.inactive);
        spr.drawString(data.mask, width, FREQ_7SEGMENT_HEIGHT); // Jobb szélre igazítva
    }

    // Aktív frekvencia számok rajzolása
    spr.setTextColor(colors.active);
    spr.drawString(data.freqStr, width, FREQ_7SEGMENT_HEIGHT); // Jobb szélre igazítva

    // Sprite kirajzolása és memória felszabadítása
    spr.pushSprite(x, y);
    spr.deleteSprite();
}

/**
 * @brief Rajzolja a finomhangolás aláhúzást SSB/CW módban
 */
void FreqDisplay::drawFineTuningUnderline(int freqSpriteX, int freqSpriteWidth) {
    const FreqSegmentColors &colors = getSegmentColors(); // Az utolsó 3 digit pozíciójának meghatározása
    // A maszk: "88 888.88"
    // Pozíciók:  01234567 8
    //            0: '8' (10kHz), 1: '8' (1kHz), 2: ' ' (space)
    //            3: '8' (100Hz), 4: '8' (10Hz), 5: '8' (1Hz), 6: '.' (pont)
    //            7: '8' (0.1Hz), 8: '8' (0.01Hz) - ezek nem kellenek finomhangoláshoz    // A finomhangolás digitjei a maszkban (jobbról balra):
    // "88 888.88" - pozíciók: 0:ezres, 1:ezres, 2:space, 3:százas, 4:tízes, 5:egyes, 6:pont, 7:tized, 8:század
    // Finomhangolás digitek jobbról balra: pozíció 8 (10Hz), 7 (100Hz), 5 (1kHz)
    // JAVÍTÁS: Próbáljuk vissza pozíció 5-öt az 1kHz-hez

    const char *mask = "88 888.88";
    int maskLen = strlen(mask);
    const int SPACE_GAP_WIDTH = 8; // A finomhangolási digit indexei a maszkban (0-alapú)
    // Helyes elemzés:
    // Maszk: "88 888.88" → pozíciók: 0,1,2(' '),3,4,5,6('.'),7,8
    // Frekvencia: "7 074.00" → pozíciók: 0(' '),1('7'),2(' '),3('0'),4('7'),5('4'),6('.'),7('0'),8('0')
    // Finomhangolás digitek: 5=1kHz('4'), 7=100Hz('0'), 8=10Hz('0')
    int lastThreeDigitIndices[3] = {5, 7, 8}; // 5=1kHz, 7=100Hz, 8=10Hz
    int digitPositions[3];                    // [0] = 1kHz digit, [1] = 100Hz digit, [2] = 10Hz digit
    int digitWidths[3];                       // A digitek tényleges szélességei

    // Pozíciók kiszámítása
    int currentX = 0;
    for (int i = 0; i < maskLen; i++) {
        // X pozíció frissítése ELŐTT - számoljuk ki a karakterszélességet
        int charWidth = 0;
        if (mask[i] == ' ') {
            charWidth = SPACE_GAP_WIDTH;
        } else {
            // Optimalizált karakterszélesség lekérdezés
            charWidth = getCharacterWidth(mask[i]);
        } // Ellenőrizzük, hogy ez az utolsó 3 digit egyike-e
        for (int j = 0; j < 3; j++) {
            if (i == lastThreeDigitIndices[j]) {
                // A digit pozíciója: jelenlegi X + fél karakterszélesség (középre igazítás)
                digitPositions[j] = freqSpriteX + currentX + (charWidth / 2);
                digitWidths[j] = charWidth; // Eltároljuk a digit szélességét is
                break;
            }
        }

        // X pozíció frissítése a következő karakterhez
        currentX += charWidth;
    } // Aláhúzás rajzolása a kiválasztott digit alatt
    if (rtv::freqstepnr >= 0 && rtv::freqstepnr < 3) {
        int digitCenter = digitPositions[rtv::freqstepnr];
        int digitWidth = digitWidths[rtv::freqstepnr]; // Tényleges digit szélesség
        int underlineY = bounds.y + FREQ_7SEGMENT_HEIGHT + UNDERLINE_Y_OFFSET;

        // Aláhúzás középre igazítva a digit alatt (tényleges digit szélességgel)
        int underlineX = digitCenter - (digitWidth / 2);

        // Előbb töröljük az egész aláhúzási területet
        int totalUnderlineWidth = digitPositions[2] - digitPositions[0] + digitWidths[2];
        int clearStartX = digitPositions[0] - (digitWidths[0] / 2);

        tft.fillRect(clearStartX, underlineY, totalUnderlineWidth, UNDERLINE_HEIGHT, this->colors.background);

        // Aztán rajzoljuk az aktív aláhúzást (tényleges digit szélességgel)
        tft.fillRect(underlineX, underlineY, digitWidth, UNDERLINE_HEIGHT, colors.indicator);
    }
}

/**
 * @brief Kiszámítja az SSB/CW frekvencia érintési területeket
 */
void FreqDisplay::calculateSsbCwTouchAreas(int freqSpriteX, int freqSpriteWidth) {
    // Ugyanaz a logika, mint a finomhangolás aláhúzásnál
    const char *mask = "88 888.88";
    const int SPACE_GAP_WIDTH = 8;            // Az utolsó 3 digit indexei a maszkban (0-alapú) - a finomhangoláshoz
    int lastThreeDigitIndices[3] = {5, 7, 8}; // 5=1kHz, 7=100Hz, 8=10Hz// Pozíciók kiszámítása
    int currentX = 0;
    for (int i = 0; i < strlen(mask); i++) {
        // X pozíció frissítése ELŐTT - számoljuk ki a karakterszélességet
        int charWidth = 0;
        if (mask[i] == ' ') {
            charWidth = SPACE_GAP_WIDTH;
        } else {
            // Optimalizált karakterszélesség lekérdezés
            charWidth = getCharacterWidth(mask[i]);
        } // Ellenőrizzük, hogy ez az utolsó 3 digit egyike-e
        for (int j = 0; j < 3; j++) {
            if (i == lastThreeDigitIndices[j]) {
                // Touch terület: digit közepe körül (ugyanúgy, mint az aláhúzásnál)
                int digitCenter = freqSpriteX + currentX + (charWidth / 2);
                ssbCwTouchDigitAreas[j][0] = digitCenter - (charWidth / 2); // X start (bal széle)
                ssbCwTouchDigitAreas[j][1] = digitCenter + (charWidth / 2); // X end (jobb széle)

                break;
            }
        } // X pozíció frissítése a következő karakterhez
        currentX += charWidth;
    }
}

/**
 * @brief Rajzolja a frekvencia kijelzőt a megadott mód szerint
 */
void FreqDisplay::drawFrequencyDisplay(const FrequencyDisplayData &data) {
    if (isInSsbCwMode()) {
        drawSsbCwStyle(data);
    } else {
        drawFmAmLwStyle(data);
    }
}

/**
 * @brief Fő rajzolási metódus
 */
void FreqDisplay::draw() {
    if (!needsRedraw) {
        return;
    }

    // Csak akkor töröljük a hátteret, ha szükséges (pl. első rajzolás, mód váltás)
    if (needsFullClear) {
        tft.fillRect(bounds.x, bounds.y, bounds.width, bounds.height, this->colors.background);
        needsFullClear = false; // Reset a flag
    }

    // Frekvencia adatok meghatározása
    FrequencyDisplayData data = getFrequencyDisplayData(currentDisplayFrequency);

    // Frekvencia rajzolása
    drawFrequencyDisplay(data);

    // Debug keret - segít az optimalizálásban
    tft.drawRect(bounds.x, bounds.y, bounds.width, bounds.height, TFT_RED);

    needsRedraw = false;
}

/**
 * @brief Érintési esemény kezelése
 */
bool FreqDisplay::handleTouch(const TouchEvent &event) {
    // Csak SSB/CW módban és ha nincs elrejtve az aláhúzás
    if (!isInSsbCwMode() || hideUnderline || rtv::bfoOn) {
        return false;
    }

    // Pozíció ellenőrzése
    if (!bounds.contains(event.x, event.y)) {
        return false;
    }

    // Digit érintés ellenőrzése
    for (int i = 0; i < 3; i++) {
        if (event.x >= ssbCwTouchDigitAreas[i][0] && event.x < ssbCwTouchDigitAreas[i][1]) {
            // Digit kiválasztása
            if (rtv::freqstepnr != i) {
                rtv::freqstepnr = i; // Frekvencia lépés beállítása
                // i=0: 1kHz digit, i=1: 100Hz digit, i=2: 10Hz digit
                switch (i) {
                    case 0:
                        rtv::freqstep = 1000;
                        break; // 1kHz
                    case 1:
                        rtv::freqstep = 100;
                        break; // 100Hz
                    case 2:
                        rtv::freqstep = 10;
                        break; // 10Hz
                    default:
                        break;
                }

                markForRedraw();
            }
            return true;
        }
    }

    return false;
}

/**
 * @brief Visszaadja egy karakter szélességét konstansok alapján
 */
int FreqDisplay::getCharacterWidth(char c) {
    if (c >= '0' && c <= '9') {
        return CHAR_WIDTH_DIGIT;
    }
    switch (c) {
        case '.':
            return CHAR_WIDTH_DOT;
        case ' ':
            return CHAR_WIDTH_SPACE;
        case '-':
            return CHAR_WIDTH_DASH;
        default:
            return CHAR_WIDTH_DIGIT; // Biztonsági alapértelmezett
    }
}
