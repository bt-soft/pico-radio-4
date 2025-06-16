// src/FreqDisplay.cpp
/**
 * @file FreqDisplay.cpp
 * @brief Frekvencia kijelző komponens implementáció
 *
 * Ez a fájl tartalmazza a FreqDisplay osztály teljes implementációját,
 * amely különböző rádiós demodulációs módokhoz optimalizált frekvencia
 * megjelenítést biztosít 7-szegmenses digitális kijelző stílusban.
 */

#include "FreqDisplay.h"
#include "DSEG7_Classic_Mini_Regular_34.h" // 7-szegmenses font
#include "UIColorPalette.h"                // Centralizált színkonstansok
#include "defines.h"                       // TFT_COLOR_BACKGROUND
#include "utils.h"                         // Utils::beepError

/**
 * @namespace FreqDisplayConstants
 * @brief Frekvencia kijelző konstansok és pozícionálási adatok
 *
 * Ez a névtér tartalmazza az összes pozícionálási konstanst, méretet
 * és eltolási értéket, ami a komponens különböző elemeinek (digitek,
 * címkék, egységek, aláhúzások) pontos elhelyezéséhez szükséges.
 */
namespace FreqDisplayConstants {
// === 7-szegmenses kijelző alapadatok ===
constexpr int FREQ_7SEGMENT_HEIGHT = 38; ///< A 7-szegmenses font magassága pixelben

// === Referencia X pozíciók különböző módokhoz (jobb igazításhoz) ===
// Ezek az értékek a 7-szegmenses kijelző jobb szélének X pozícióját jelentik
// a komponens bal széléhez (bounds.x) viszonyítva. A tényleges sprite rajzolási
// X pozíciója (spritePushX) ebből és a sprite szélességéből számolódik.
// TODO: ezt majd javítani, ez így nem jó!!
#warning "RefXDefault, RefXFmAm, RefXSeek, RefXBfo értékek hardcoded értékek, nem dinamikusan számoltak, javítani!"
constexpr uint16_t RefXDefault = 200; ///< SSB/CW mód
constexpr uint16_t RefXFmAm = 160;    ///< FM/AM módokban
constexpr uint16_t RefXSeek = 144;    ///< SEEK mód során
constexpr uint16_t RefXBfo = 115;     ///< BFO mód során

// === Digit pozíciók és méretek az érintéses aláhúzáshoz ===
// Megjegyzés: Minden pozíció a komponens bounds.x, bounds.y koordinátájához relatív
constexpr int DigitWidth = 25;                    ///< Egy digit szélessége pixelben
constexpr int DigitHeight = FREQ_7SEGMENT_HEIGHT; ///< Digitek magassága (azonos a font magasságával)
constexpr int DigitYStart = 0;                    ///< Digitek Y kezdőpozíciója (komponens tetejétől)

constexpr int FreqStepDigitXPositions[] = {141, 171, 200};      ///< Frekvencia lépésköz digitek X pozíciói (komponens bal szélétől)
constexpr int UnderlineYOffset = DigitYStart + DigitHeight + 2; ///< Aláhúzás Y pozíciója
constexpr int UnderlineHeight = 5;                              ///< Aláhúzás magassága pixelben

// === Sprite és egység pozicionálás ===
constexpr uint16_t SpriteYOffset = 0; ///< 7-szegmenses kijelző Y eltolása a komponens tetejétől
constexpr uint16_t UnitXOffset = 5;   ///< Egység ("kHz", "MHz") X eltolása a frekvencia jobb szélétől

// === BFO mód specifikus pozíciók és méretek ===
// Minden pozíció relatív a komponens bounds.x, bounds.y koordinátájához
constexpr uint16_t BfoLabelRectXOffset = 156; ///< "BFO" címke téglalap X pozíciója
constexpr uint16_t BfoLabelRectYOffset = 1;   ///< "BFO" címke téglalap Y pozíciója
constexpr uint16_t BfoLabelRectW = 42;        ///< "BFO" címke téglalap szélessége
constexpr uint16_t BfoLabelRectH = 20;        ///< "BFO" címke téglalap magassága

constexpr uint16_t BfoHzLabelXOffset = 120;                                  ///< "Hz" felirat X pozíciója BFO érték mellett
constexpr uint16_t BfoHzLabelYOffset = SpriteYOffset + FREQ_7SEGMENT_HEIGHT; ///< "Hz" felirat Y pozíciója

constexpr uint16_t BfoMiniFreqX = 220;                                      ///< Kis méretű háttér frekvencia X pozíciója BFO módban
constexpr uint16_t BfoMiniFreqY = SpriteYOffset + FREQ_7SEGMENT_HEIGHT + 5; ///< Kis méretű háttér frekvencia Y pozíciója
constexpr uint16_t BfoMiniUnitXOffset = 20;                                 ///< Kis méretű "kHz" egység X eltolása BFO módban

constexpr uint16_t SsbCwUnitXOffset = 215;                                       ///< "kHz" felirat X pozíciója normál SSB/CW módban
constexpr uint16_t SsbCwUnitYOffset = SpriteYOffset + FREQ_7SEGMENT_HEIGHT + 20; ///< "kHz" felirat Y pozíciója normál SSB/CW módban

} // namespace FreqDisplayConstants

// === Globális színkonfigurációk - UIColorPalette használatával ===
/// Alapértelmezett színkonfiguráció normál módhoz (nem BFO)
const FreqSegmentColors defaultNormalColors = UIColorPalette::createNormalFreqColors();
/// Alapértelmezett színkonfiguráció BFO módhoz
const FreqSegmentColors defaultBfoColors = UIColorPalette::createBfoFreqColors();

/**
 * @brief FreqDisplay konstruktor - inicializálja a frekvencia kijelző komponenst
 *
 * A konstruktor beállítja az alapértelmezett értékeket, színkonfigurációkat,
 * és biztosítja, hogy az első rajzolás teljes újrarajzolásként történjen meg.
 *
 * @param tft_param TFT kijelző referencia
 * @param bounds_param Komponens terület (pozíció és méret)
 * @param band_ref Sávkezelő objektum referencia
 * @param config_ref Konfiguráció objektum referencia
 */
FreqDisplay::FreqDisplay(TFT_eSPI &tft_param, const Rect &bounds_param, Si4735Manager *pSi4735Manager)
    : UIComponent(tft_param, bounds_param), pSi4735Manager(pSi4735Manager), spr(&(this->tft)), normalColors(defaultNormalColors), bfoColors(defaultBfoColors),
      customColors(defaultNormalColors), useCustomColors(false), // Egyedi színek inicializálása
      currentDisplayFrequency(0),                                // Kezdetben 0, hogy az első setFrequency biztosan frissítsen
      bfoModeActiveLastDraw(rtv::bfoOn),                         // BFO mód állapota az utolsó rajzoláskor
      redrawOnlyFrequencyDigits(false),                          // Alapértelmezetten false, hogy az első rajzolás teljes legyen
      hideUnderline(false) {                                     // Alapértelmezetten megjelenik az aláhúzás

    // Alapértelmezett háttérszín beállítása a globális háttérszínre
    this->colors.background = TFT_COLOR_BACKGROUND;

    // Explicit újrarajzolás kérése az első megjelenítéshez
    markForRedraw(); // Biztosítjuk, hogy az első rajzolás megtörténjen
}

/**
 * @brief Beállítja a megjelenítendő frekvenciát
 *
 * Ez a metódus frissíti a kijelzendő frekvenciát, és optimalizált
 * újrarajzolást kér, ha a frekvencia valóban megváltozott, vagy ha
 * a forceRedraw paraméter true.
 *
 * @param freq Az új frekvencia érték
 * @param forceRedraw Ha true, akkor újrarajzol, még ha a frekvencia nem változott is
 */
void FreqDisplay::setFrequency(uint16_t freq, bool forceRedraw) {
    if (forceRedraw || currentDisplayFrequency != freq) {
        currentDisplayFrequency = freq;
        // Optimalizált rajzolás engedélyezése: csak a frekvencia számjegyek
        // frissítése, nem a teljes komponens újrarajzolása
        redrawOnlyFrequencyDigits = true;
        markForRedraw();
    }
}

/**
 * @brief Beállítja a megjelenítendő frekvenciát, teljes újrarajzolással
 * @param freq Az új frekvencia érték
 * @brief Ez a metódus teljes újrarajzolást kér, függetlenül attól, hogy a frekvencia megváltozott-e.
 */
void FreqDisplay::setFrequencyWithFullDraw(uint16_t freq, bool hideUnderline) {
    // Teljes újrarajzolás kérése, függetlenül attól, hogy a frekvencia megváltozott-e
    currentDisplayFrequency = freq;
    redrawOnlyFrequencyDigits = false; // Teljes újrarajzolás lesz

    this->hideUnderline = hideUnderline; // Aláhúzás állapotának beállítása

    markForRedraw(); // Teljes újrarajzolás indítása
}

/**
 * @brief Beállítja az egyedi színkonfigurációt (pl. képernyővédő módhoz)
 * @param colors Az új színkonfiguráció
 */
void FreqDisplay::setCustomColors(const FreqSegmentColors &colors) {
    customColors = colors;
    useCustomColors = true;
    markForRedraw(); // Teljes újrarajzolás szükséges a színváltás miatt
}

/**
 * @brief Visszaállítja az alapértelmezett színkonfigurációt
 */
void FreqDisplay::resetToDefaultColors() {
    useCustomColors = false;
    markForRedraw(); // Teljes újrarajzolás szükséges a színváltás miatt
}

/**
 * @brief Beállítja, hogy megjelenjen-e a Finomhangolás jel (aláhúzás)
 * @param hide Ha true, az aláhúzás elrejtve
 */
void FreqDisplay::setHideUnderline(bool hide) {
    if (hideUnderline != hide) {
        hideUnderline = hide;

        // Teljes újrarajzolás szükséges az aláhúzás váltás miatt
        markForRedraw();
    }
}

/**
 * @brief Kiszámítja a frekvencia sprite X pozícióját az aktuális mód alapján
 *
 * A különböző rádiós módokban (SEEK, BFO, FM/AM, normál) a frekvencia kijelző
 * sprite-ja különböző pozíciókon jelenik meg a komponensen belül.
 *
 * @return A sprite jobb szélének X pozíciója a komponens bal széléhez képest
 */
uint32_t FreqDisplay::calcFreqSpriteXPosition() const {
    using namespace FreqDisplayConstants;
    uint8_t currentDemod = pSi4735Manager->getCurrentBand().currMod;

    // Alapértelmezett pozíció (normál SSB/CW mód)
    uint32_t x_offset_from_left = RefXDefault;

    // Mód-specifikus pozíció beállítások
    if (rtv::SEEK) {
        x_offset_from_left = RefXSeek; // SEEK mód során más pozíció
    } else if (rtv::bfoOn) {
        x_offset_from_left = RefXBfo; // BFO mód során kompaktabb elrendezés
    } else if (currentDemod == FM || currentDemod == AM) {
        x_offset_from_left = RefXFmAm; // FM/AM módokhoz optimált pozíció
    }

    // Megjegyzés: A sprite jobb oldalra van igazítva, a tényleges bal széle:
    // bounds.x + x_offset_from_left - sprite_szélessége
    // A pontos számítás a drawFrequencyInternal metódusban történik
    return x_offset_from_left;
}

/**
 * @brief Optimalizált sprite rajzolás - csak a frekvencia számjegyek frissítése
 *
 * Ez a metódus csak a 7-szegmenses frekvencia sprite-ot rajzolja újra,
 * az egységek és egyéb UI elemek nélkül. Teljesítmény optimalizáláshoz használt.
 *
 * @param freq_str A formázott frekvencia string
 * @param mask A 7-szegmenses maszk pattern
 * @param colors A használandó színkonfiguráció
 */
void FreqDisplay::drawFrequencySpriteOnly(const String &freq_str, const char *mask, const FreqSegmentColors &colors) {
    using namespace FreqDisplayConstants;

    // Font beállítása a maszk szélességének meghatározásához (SevenSegmentFreq stílusban)
    spr.setFreeFont(&DSEG7_Classic_Mini_Regular_34);
    uint16_t contentWidth = spr.textWidth(mask); // Sprite szélessége a maszk alapján
    // uint16_t freqWidth = spr.textWidth(freq_str); // Frekvencia string szélessége

    // Sprite pozícionálás számítása
    uint32_t spriteRightEdgeX_relative = calcFreqSpriteXPosition();
    uint16_t spritePushX = bounds.x + spriteRightEdgeX_relative - contentWidth; // Bal széle
    uint16_t spritePushY = bounds.y + SpriteYOffset;                            // Teteje

    // Sprite létrehozása és konfigurálása (SevenSegmentFreq stílusban)
    spr.createSprite(contentWidth, FREQ_7SEGMENT_HEIGHT);
    spr.fillSprite(this->colors.background); // Háttér törlése
    spr.setTextSize(1);
    spr.setTextPadding(0);
    spr.setFreeFont(&DSEG7_Classic_Mini_Regular_34);
    spr.setTextDatum(BR_DATUM); // Jobb alsó sarokhoz igazítás - KULCS!

    // Inaktív számjegyek rajzolása (ha engedélyezve van) - SevenSegmentFreq stílusban
    if (config.data.tftDigitLigth) {
        spr.setTextColor(colors.inactive);
        spr.drawString(mask, contentWidth, FREQ_7SEGMENT_HEIGHT);
    }

    // Aktív frekvencia számok rajzolása - SevenSegmentFreq stílusban
    spr.setTextColor(colors.active);
    spr.drawString(freq_str, contentWidth, FREQ_7SEGMENT_HEIGHT);

    // Sprite kirajzolása a kijelzőre és memória felszabadítása
    spr.pushSprite(spritePushX, spritePushY);
    spr.deleteSprite();
}

/**
 * @brief Teljes frekvencia kijelző rajzolása sprite-tal és egységgel
 *
 * Ez a belső metódus rajzolja a 7-szegmenses frekvencia kijelzőt és
 * opcionálisan az egységet is (MHz, kHz, Hz). A teljes rajzolási ciklusban használt.
 *
 * @param freq_str A formázott frekvencia string
 * @param mask A 7-szegmenses maszk pattern
 * @param colors A színkonfiguráció
 * @param unit Az egység string (opcionális, lehet nullptr)
 */
void FreqDisplay::drawFrequencyInternal(const String &freq_str, const char *mask, const FreqSegmentColors &colors, const char *unit) {
    using namespace FreqDisplayConstants;

    // Font beállítása és sprite méret számítása (SevenSegmentFreq stílusban)
    spr.setFreeFont(&DSEG7_Classic_Mini_Regular_34);
    uint16_t contentWidth = spr.textWidth(mask);

    // Sprite pozícionálás
    uint32_t spriteRightEdgeX_relative = calcFreqSpriteXPosition();
    uint16_t spritePushX = bounds.x + spriteRightEdgeX_relative - contentWidth;
    uint16_t spritePushY = bounds.y + SpriteYOffset;

    // Sprite létrehozása és frekvencia rajzolása (SevenSegmentFreq stílusban)
    spr.createSprite(contentWidth, FREQ_7SEGMENT_HEIGHT);
    spr.fillSprite(this->colors.background);
    spr.setTextSize(1);
    spr.setTextPadding(0);
    spr.setFreeFont(&DSEG7_Classic_Mini_Regular_34);
    spr.setTextDatum(BR_DATUM); // Jobb alsó sarokhoz igazítás - KULCS!

    // Inaktív (háttér) számjegyek rajzolása (SevenSegmentFreq stílusban)
    if (config.data.tftDigitLigth) {
        spr.setTextColor(colors.inactive);
        spr.drawString(mask, contentWidth, FREQ_7SEGMENT_HEIGHT);
    }

    // Aktív frekvencia számok rajzolása (SevenSegmentFreq stílusban)
    spr.setTextColor(colors.active);
    spr.drawString(freq_str, contentWidth, FREQ_7SEGMENT_HEIGHT);

    // Sprite kirajzolása
    spr.pushSprite(spritePushX, spritePushY);
    spr.deleteSprite();

    // Egység kirajzolása a sprite mellé (ha meg van adva)
    if (unit != nullptr) {
        tft.setFreeFont(); // Alapértelmezett font az egységhez
        tft.setTextSize(2);
        tft.setTextDatum(BL_DATUM);                                  // Bal alsó sarok igazítás
        tft.setTextColor(colors.indicator, this->colors.background); // Háttérrel együtt rajzolás (felülírás)

        // Egység pozícionálása: sprite jobb széle + kis eltolás
        uint16_t unitX = spritePushX + contentWidth + UnitXOffset;
        uint16_t unitY = spritePushY + FREQ_7SEGMENT_HEIGHT;
        tft.drawString(unit, unitX, unitY);
    }
}

/**
 * @brief Rajzolja a frekvencia lépés aláhúzását
 *
 * Ez a metódus felelős a digitek alatti aláhúzás rajzolásáért, amely
 * jelzi az aktuálisan kiválasztott frekvencia lépés pozícióját.
 * BFO módban és letiltott állapotban az aláhúzás nem jelenik meg.
 *
 * @param colors A színkonfiguráció (az indikátor szín használatával)
 */
void FreqDisplay::drawStepUnderline(const FreqSegmentColors &colors) {
    using namespace FreqDisplayConstants;                                                                  // Az aláhúzás teljes területének számítása
    const int underlineAreaX_abs = bounds.x + FreqStepDigitXPositions[0];                                  // Bal oldali szél
    const int underlineAreaWidth = (FreqStepDigitXPositions[2] + DigitWidth) - FreqStepDigitXPositions[0]; // Teljes szélesség
    const int underlineAreaY_abs = bounds.y + UnderlineYOffset;                                            // Y pozíció

    // Ha a komponens le van tiltva, BFO mód aktív, vagy az aláhúzás el van rejtve, töröljük az aláhúzást
    if (isDisabled() || rtv::bfoOn || hideUnderline) {
        tft.fillRect(underlineAreaX_abs, underlineAreaY_abs, underlineAreaWidth, UnderlineHeight, this->colors.background);
        return;
    }

    // Először töröljük a teljes aláhúzási területet
    tft.fillRect(underlineAreaX_abs, underlineAreaY_abs, underlineAreaWidth, UnderlineHeight, this->colors.background); // Majd kirajzoljuk az aktív lépés aláhúzását
    const int activeUnderlineX = bounds.x + FreqStepDigitXPositions[rtv::freqstepnr];
    tft.fillRect(activeUnderlineX, underlineAreaY_abs, DigitWidth, UnderlineHeight, colors.indicator);
}

/**
 * @brief Visszaadja az aktuális színkonfigurációt a mód alapján
 *
 * @return Referencia a normalColors-ra vagy bfoColors-ra a BFO állapot szerint
 */
/**
 * @brief Visszaadja az aktuális színkonfigurációt a mód alapján
 *
 * @return Referencia a customColors-ra (ha useCustomColors), egyébként normalColors-ra vagy bfoColors-ra a BFO állapot szerint
 */
const FreqSegmentColors &FreqDisplay::getSegmentColors() const {
    if (useCustomColors) {
        return customColors;
    }
    return rtv::bfoOn ? bfoColors : normalColors;
}

/**
 * @brief SSB/CW frekvencia kijelzésének kezelése
 *
 * Ez a metódus koordinálja az SSB/CW módok frekvencia megjelenítését,
 * beleértve a BFO animáció kezelését, és a normál/BFO módok közötti váltást.
 *
 * @param currentFrequencyValue A megjelenítendő frekvencia értéke
 * @param colors A használandó színkonfiguráció
 */
void FreqDisplay::displaySsbCwFrequency(uint16_t currentFrequencyValue, const FreqSegmentColors &colors) {
    String formattedFreq = formatSsbCwFrequency(currentFrequencyValue);

    if (rtv::bfoTr) {
        handleBfoAnimation(formattedFreq);
    }

    if (!rtv::bfoOn) {
        drawNormalSsbCwMode(formattedFreq, colors);
    } else {
        drawBfoMode(formattedFreq, colors);
    }
}

/**
 * @brief Formázza az SSB/CW frekvenciát megjelenítésre
 *
 * Kiszámítja a BFO kompenzált frekvenciát és formázza kHz.hz formátumba.
 * A számítás: (frekvencia_kHz * 1000 - BFO_offset) / 1000 = kHz rész + hz tizesek
 *
 * @param currentFrequencyValue A nyers frekvencia érték
 * @return Formázott frekvencia string (pl. "14205.50")
 */
String FreqDisplay::formatSsbCwFrequency(uint16_t currentFrequencyValue) {
    // Az rtv::freqDec tartalmazza a finomhangolási eltolást Hz-ben
    // Ez az érték kerül levonásra a chip frekvenciától a helyes megjelenítéshez
    uint32_t displayFreqHz = (uint32_t)currentFrequencyValue * 1000 - rtv::freqDec;

    long khz_part = displayFreqHz / 1000;
    int hz_tens_part = abs((int)(displayFreqHz % 1000)) / 10;
    char buffer[16];
    // Formázás a "88 888.88" maszknak megfelelően: "x xxx.xx" vagy " xxxx.xx"
    long khz_thousands = khz_part / 1000;
    long khz_remainder = khz_part % 1000;

    if (khz_thousands > 0) {
        // Ha van ezres rész, akkor "x xxx.xx" formátum
        sprintf(buffer, "%ld %03ld.%02d", khz_thousands, khz_remainder, hz_tens_part);
    } else {
        // Ha nincs ezres rész, akkor " xxxx.xx" formátum (space a vezető null helyett)
        sprintf(buffer, " %ld.%02d", khz_remainder, hz_tens_part);
    }
    return String(buffer);
}

/**
 * @brief BFO mód váltás animációjának kezelése
 *
 * Amikor a BFO mód be vagy ki kapcsol, ez a metódus egy fokozatos
 * méretváltoztatási animációt jelenít meg a frekvencia értékkel.
 *
 * @param formattedFreq A formázott frekvencia string az animációhoz
 */
void FreqDisplay::handleBfoAnimation(const String &formattedFreq) {
    using namespace FreqDisplayConstants;

    rtv::bfoTr = false;
    tft.setFreeFont();
    tft.setTextDatum(BR_DATUM);
    tft.setTextColor(getSegmentColors().indicator, this->colors.background);

    for (uint8_t i = 4; i > 1; i--) {
        tft.setTextSize(rtv::bfoOn ? i : (6 - i));
        tft.drawString(formattedFreq, bounds.x + BfoMiniFreqX, bounds.y + BfoMiniFreqY);
        delay(50);
    }
}

/**
 * @brief Normál SSB/CW mód frekvencia kijelzésének rajzolása
 *
 * Megjeleníti a nagy formátumú frekvenciát és a "kHz" egységet
 * a normál (nem BFO) SSB/CW módban.
 *
 * @param formattedFreq A formázott frekvencia string
 * @param colors A használandó színkonfiguráció
 */
void FreqDisplay::drawNormalSsbCwMode(const String &formattedFreq, const FreqSegmentColors &colors) {
    using namespace FreqDisplayConstants; // Fő frekvencia kijelzése
    drawFrequencyInternal(formattedFreq, "88 888.88", colors, nullptr);

    // "kHz" egység kijelzése
    drawTextAtPosition("kHz", bounds.x + SsbCwUnitXOffset, bounds.y + SsbCwUnitYOffset, 2, BC_DATUM, colors.indicator);
}

/**
 * @brief BFO mód frekvencia kijelzésének rajzolása
 *
 * Komplex BFO mód megjelenítés, amely tartalmazza:
 * - Nagy BFO értéket Hz-ben
 * - "BFO" címkét
 * - Kis háttér frekvenciát kHz-ben
 *
 * @param formattedFreq A formázott háttér frekvencia string
 * @param colors A használandó színkonfiguráció
 */
void FreqDisplay::drawBfoMode(const String &formattedFreq, const FreqSegmentColors &colors) {
    using namespace FreqDisplayConstants; // BFO érték kijelzése
    drawFrequencyInternal(String(rtv::currentBFOmanu), "-888", colors, nullptr);

    // "Hz" egység
    drawTextAtPosition("Hz", bounds.x + BfoHzLabelXOffset, bounds.y + BfoHzLabelYOffset, 2, BL_DATUM, colors.indicator);

    // BFO címke
    drawBfoLabel(colors);

    // Kis frekvencia kijelzés
    drawMiniFrequency(formattedFreq, colors);
}

/**
 * @brief "BFO" címke rajzolása színes háttérrel
 *
 * Egy téglalap alakú háttérrel rendelkező "BFO" feliratot rajzol,
 * amely jelzi, hogy a BFO beállítási mód aktív.
 *
 * @param colors A színkonfiguráció (active szín használata)
 */
void FreqDisplay::drawBfoLabel(const FreqSegmentColors &colors) {
    using namespace FreqDisplayConstants;

    uint16_t rectX = bounds.x + BfoLabelRectXOffset;
    uint16_t rectY = bounds.y + BfoLabelRectYOffset;
    tft.fillRect(rectX, rectY, BfoLabelRectW, BfoLabelRectH, colors.active);
    tft.setTextColor(UIColorPalette::FREQ_BFO_LABEL_TEXT, colors.active);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("BFO", rectX + BfoLabelRectW / 2, rectY + BfoLabelRectH / 2);
}

/**
 * @brief Kis méretű háttér frekvencia rajzolása BFO módban
 *
 * A BFO mód során a háttérben kis méretben megjeleníti az eredeti
 * frekvenciát "kHz" egységgel együtt.
 *
 * @param formattedFreq A formázott frekvencia string
 * @param colors A használandó színkonfiguráció
 */
void FreqDisplay::drawMiniFrequency(const String &formattedFreq, const FreqSegmentColors &colors) {
    using namespace FreqDisplayConstants;

    // Kis frekvencia
    drawTextAtPosition(formattedFreq, bounds.x + BfoMiniFreqX, bounds.y + BfoMiniFreqY, 2, BR_DATUM, colors.indicator);

    // Kis "kHz" egység
    drawTextAtPosition("kHz", bounds.x + BfoMiniFreqX + BfoMiniUnitXOffset, bounds.y + BfoMiniFreqY, 1, BR_DATUM, colors.indicator);
}

/**
 * @brief Szöveg rajzolása megadott pozícióban és formátumban
 *
 * Általános segéd metódus szövegek rajzolásához specifikus formázással.
 * Automatikusan beállítja a háttérszínt a komponens háttérszínére.
 *
 * @param text A rajzolandó szöveg
 * @param x X pozíció
 * @param y Y pozíció
 * @param textSize Szöveg mérete
 * @param datum Igazítási mód (pl. BR_DATUM, BC_DATUM)
 * @param color Szöveg színe
 */
void FreqDisplay::drawTextAtPosition(const String &text, uint16_t x, uint16_t y, uint8_t textSize, uint8_t datum, uint16_t color) {
    tft.setFreeFont();
    tft.setTextSize(textSize);
    tft.setTextDatum(datum);
    tft.setTextColor(color, this->colors.background);
    tft.drawString(text, x, y);
}

/**
 * @brief FM/AM frekvencia kijelzésének kezelése
 *
 * Előkészíti és megjeleníti a frekvenciát FM/AM módokban.
 * A különböző sávtípusokhoz (MW/LW vs SW) eltérő formátumokat használ.
 *
 * @param currentFrequencyValue A megjelenítendő frekvencia értéke
 * @param colors A használandó színkonfiguráció
 */
void FreqDisplay::displayFmAmFrequency(uint16_t currentFrequencyValue, const FreqSegmentColors &colors) {
    FrequencyDisplayData displayData = prepareFrequencyDisplayData(currentFrequencyValue);
    drawFrequencyInternal(displayData.freqStr, displayData.mask, colors, displayData.unit);
}

/**
 * @brief Frekvencia megjelenítési adatok előkészítése FM/AM módokhoz
 *
 * A demodulációs mód és sávtípus alapján előkészíti a formázott
 * frekvencia stringet, maszkot és egységet.
 *
 * @param frequency A nyers frekvencia érték
 * @return FrequencyDisplayData struktúra az összes megjelenítési adattal
 */
FreqDisplay::FrequencyDisplayData FreqDisplay::prepareFrequencyDisplayData(uint16_t frequency) {
    FrequencyDisplayData data;
    uint8_t demodMode = pSi4735Manager->getCurrentBand().currMod;
    uint8_t bandType = pSi4735Manager->getCurrentBandType();

    if (demodMode == FM) {
        data = prepareFmDisplayData(frequency);
    } else { // AM modes
        data = prepareAmDisplayData(frequency, bandType);
    }

    return data;
}

/**
 * @brief FM mód megjelenítési adatainak előkészítése
 *
 * FM frekvenciákat MHz-ben jelenít meg 2 tizedesjegy pontossággal
 * (pl. 100.50 MHz).
 *
 * @param frequency A nyers frekvencia érték
 * @return FrequencyDisplayData struktúra FM formátummal
 */
FreqDisplay::FrequencyDisplayData FreqDisplay::prepareFmDisplayData(uint16_t frequency) {
    FrequencyDisplayData data;
    data.unit = "MHz";
    data.mask = "188.88";

    float displayFreqMHz = frequency / 100.0f;
    data.freqStr = String(displayFreqMHz, 2);

    return data;
}

/**
 * @brief AM mód megjelenítési adatainak előkészítése sávtípus szerint
 *
 * MW/LW sávok: egész kHz értékek (pl. 1440 kHz)
 * SW sávok: MHz formátum 3 tizedesjegy pontossággal (pl. 15.230 MHz)
 *
 * @param frequency A nyers frekvencia érték
 * @param bandType A sáv típusa (MW_BAND_TYPE, LW_BAND_TYPE, vagy SW)
 * @return FrequencyDisplayData struktúra AM formátummal
 */
FreqDisplay::FrequencyDisplayData FreqDisplay::prepareAmDisplayData(uint16_t frequency, uint8_t bandType) {
    FrequencyDisplayData data;
    if (bandType == MW_BAND_TYPE || bandType == LW_BAND_TYPE) {
        // MW/LW bands - display in kHz as integer
        data.unit = "kHz";
        data.mask = "8888";
        data.freqStr = String(frequency);
    } else {
        // SW AM - display in MHz with 3 decimal places
        data.unit = "MHz";
        data.mask = "88.888";
        data.freqStr = String(frequency / 1000.0f, 3);
    }

    return data;
}

/**
 * @brief Optimalizált rajzoláshoz frekvencia string és maszk meghatározása
 *
 * A demodulációs mód alapján meghatározza a formázott frekvencia stringet
 * és a megfelelő 7-szegmenses maszkot az optimalizált rajzoláshoz.
 *
 * @param frequency A nyers frekvencia érték
 * @param outFreqStr [kimenet] A formázott frekvencia string
 * @param outMask [kimenet] A 7-szegmenses maszk
 * @return true ha sikeresen meghatározta a formátumot, false egyébként
 */
bool FreqDisplay::determineFreqStrAndMaskForOptimizedDraw(uint16_t frequency, String &outFreqStr, const char *&outMask) {

    const uint8_t currDemod = pSi4735Manager->getCurrentBand().currMod;
    if (currDemod == LSB || currDemod == USB || currDemod == CW) {
        if (rtv::bfoOn) { // BFO érték kijelzése
            outFreqStr = String(rtv::currentBFOmanu);
            outMask = "-888";
        } else { // Normál SSB/CW frekvencia
            // Ugyanazt a formázást használjuk, mint a formatSsbCwFrequency függvény
            outFreqStr = formatSsbCwFrequency(frequency);
            outMask = "88 888.88";
        }
    } else if (currDemod == FM) {
        outMask = "188.88";
        outFreqStr = String(frequency / 100.0f, 2);
    } else if (currDemod == AM) {
        uint8_t currentBandType = pSi4735Manager->getCurrentBandType();
        if (currentBandType == MW_BAND_TYPE || currentBandType == LW_BAND_TYPE) {
            outMask = "8888";
            outFreqStr = String(frequency);
        } else { // SW AM
            outMask = "88.888";
            outFreqStr = String(frequency / 1000.0f, 3);
        }
    } else {
        // Ismeretlen demodulációs mód, nem tudjuk meghatározni a maszkot/stringet
        return false;
    }
    return true;
}

/**
 * @brief Fő rajzolási metódus - dönt az optimalizált vagy teljes rajzolásról
 *
 * Ez a metódus koordinálja a teljes rajzolási folyamatot. Először megvizsgálja,
 * hogy szükséges-e újrarajzolás, majd dönt az optimalizált vagy teljes
 * rajzolási mód között.
 */
void FreqDisplay::draw() {
    if (!shouldRedraw()) {
        return;
    }

    if (canUseOptimizedDraw()) {
        performOptimizedDraw();
    } else {
        performFullDraw();
    }

    // Debug: Piros keret rajzolás a komponens határainak vizualizálásához
    tft.drawRect(bounds.x, bounds.y, bounds.width, bounds.height, TFT_RED);
}

/**
 * @brief Megállapítja, hogy szükséges-e újrarajzolás
 *
 * Figyelembe veszi a BFO állapot változásokat és animációkat.
 * Ha a BFO állapot megváltozott, teljes újrarajzolást kényszerít ki.
 *
 * @return true ha újrarajzolás szükséges, false egyébként
 */
bool FreqDisplay::shouldRedraw() {
    // Ha a BFO állapot megváltozott, teljes újrarajzolás szükséges
    if (bfoModeActiveLastDraw != rtv::bfoOn || rtv::bfoTr) {
        redrawOnlyFrequencyDigits = false;
    }

    return rtv::bfoTr || needsRedraw;
}

/**
 * @brief Megállapítja, hogy használható-e az optimalizált rajzolás
 *
 * Az optimalizált rajzolás csak akkor lehetséges, ha:
 * - Csak a frekvencia számjegyek változtak (redrawOnlyFrequencyDigits = true)
 * - Nincs BFO animáció folyamatban
 *
 * @return true ha optimalizált rajzolás használható, false egyébként
 */
bool FreqDisplay::canUseOptimizedDraw() { return redrawOnlyFrequencyDigits && !rtv::bfoTr; }

/**
 * @brief Optimalizált rajzolás végrehajtása
 *
 * Csak a frekvencia számjegyeket rajzolja újra, a háttér és egyéb
 * UI elemek érintetlenül hagyásával. Teljesítmény optimalizáláshoz.
 */
void FreqDisplay::performOptimizedDraw() {
    String freqStr;
    const char *mask = nullptr;

    if (determineFreqStrAndMaskForOptimizedDraw(currentDisplayFrequency, freqStr, mask)) {
        const FreqSegmentColors &colors = getSegmentColors();
        drawFrequencySpriteOnly(freqStr, mask, colors);
        finishDraw();
    } else {
        // Fallback to full draw if optimization fails
        redrawOnlyFrequencyDigits = false;
        performFullDraw();
    }
}

/**
 * @brief Teljes rajzolás végrehajtása
 *
 * Teljesen újrarajzolja a komponenst: háttértörlés, frekvencia megjelenítés,
 * aláhúzás, és minden kiegészítő elem. Jelentős állapotváltozások után használt.
 */
void FreqDisplay::performFullDraw() {
    clearBackground();

    const FreqSegmentColors &colors = getSegmentColors();
    const uint8_t demodMode = pSi4735Manager->getCurrentBand().currMod;

    if (isSsbCwMode(demodMode)) {
        displaySsbCwFrequency(currentDisplayFrequency, colors);
    } else {
        displayFmAmFrequency(currentDisplayFrequency, colors);
    }

    drawStepUnderline(colors);

    bfoModeActiveLastDraw = rtv::bfoOn;
    restoreDefaultTextSettings();
    finishDraw();
}

/**
 * @brief Komponens háttérterületének törlése
 *
 * A teljes komponens területét kitölti a háttérszínnel.
 */
void FreqDisplay::clearBackground() { tft.fillRect(bounds.x, bounds.y, bounds.width, bounds.height, this->colors.background); }

/**
 * @brief Megállapítja, hogy a demodulációs mód SSB vagy CW-e
 *
 * @param demodMode A vizsgálandó demodulációs mód
 * @return true ha LSB, USB, vagy CW mód, false egyébként
 */
bool FreqDisplay::isSsbCwMode(uint8_t demodMode) { return demodMode == LSB || demodMode == USB || demodMode == CW; }

/**
 * @brief Visszaállítja az alapértelmezett szöveg beállításokat
 *
 * A rajzolás végén biztosítja, hogy a TFT szöveg beállításai
 * visszaálljanak az alapértelmezett állapotra.
 */
void FreqDisplay::restoreDefaultTextSettings() { tft.setTextDatum(BC_DATUM); }

/**
 * @brief Befejezi a rajzolási ciklust
 *
 * Visszaállítja a rajzolási flag-eket és jelzi, hogy a rajzolás befejezödött.
 */
void FreqDisplay::finishDraw() {
    needsRedraw = false;
    redrawOnlyFrequencyDigits = false;
}

/**
 * @brief Érintési esemény kezelésének fő metódusa
 *
 * Koordinálja az érintés feldolgozását: jogosultság ellenőrzés,
 * pozíció validálás, és digit kiválasztás kezelése.
 *
 * @param event Az érintési esemény adatai
 * @return true ha az érintés kezelve lett, false egyébként
 */
bool FreqDisplay::handleTouch(const TouchEvent &event) {
    if (!canHandleTouch(event)) {
        return false;
    }

    return processDigitTouch(event);
}

/**
 * @brief Ellenőrzi, hogy a komponens képes-e érintést kezelni és a pozíció érvényes-e
 *
 * Az érintéskezelés csak akkor engedélyezett, ha:
 * - A komponens nincs letiltva (isDisabled() == false)
 * - BFO mód nincs aktív (rtv::bfoOn == false)
 * - Az aláhúzás nincs elrejtve (hideUnderline == false)
 * - Az érintés a komponens határain belül van
 * - Az érintés a frekvencia digitek érintésre érzékeny területén belül található
 *
 * @param event Az érintési esemény adatai
 * @return true ha az érintés kezelhető, false egyébként
 */
bool FreqDisplay::canHandleTouch(const TouchEvent &event) {

    // Alapvető feltételek ellenőrzése
    if (isDisabled() || rtv::bfoOn || hideUnderline) {
        return false;
    }

    // Pozíció validálás
    if (!bounds.contains(event.x, event.y)) {
        return false;
    }

    using namespace FreqDisplayConstants;
    uint16_t relativeTouchY = event.y - bounds.y;
    return relativeTouchY >= DigitYStart && relativeTouchY <= DigitYStart + DigitHeight;
}

/**
 * @brief Feldolgozza a digit területen történt érintést
 *
 * Végigiterál a három frekvencia digit pozícióján és megállapítja,
 * hogy melyik digit területére érintett a felhasználó.
 *
 * @param event Az érintési esemény adatai
 * @return true ha egy digit sikeresen kiválasztásra került, false egyébként
 */
bool FreqDisplay::processDigitTouch(const TouchEvent &event) {
    using namespace FreqDisplayConstants;

    DEBUG("FreqDisplay::processDigitTouch: event.x=%d, event.y=%d", event.x, event.y);

    for (int digitIndex = 0; digitIndex <= 2; ++digitIndex) {
        if (isTouchOnDigit(event, digitIndex)) {
            return handleDigitSelection(digitIndex);
        }
    }

    return false;
}

/**
 * @brief Ellenőrzi, hogy az érintés egy adott digit területén van-e
 *
 * A digit területek X pozíciója FreqStepDigitXPositions[digitIndex] és szélessége DigitWidth.
 *
 * @param event Az érintési esemény adatai
 * @param digitIndex A vizsgálandó digit indexe (0, 1, vagy 2)
 * @return true ha az érintés a digit területén van, false egyébként
 */
bool FreqDisplay::isTouchOnDigit(const TouchEvent &event, int digitIndex) {
    using namespace FreqDisplayConstants;

    uint16_t digitStartX = bounds.x + FreqStepDigitXPositions[digitIndex];
    return event.x >= digitStartX && event.x < digitStartX + DigitWidth;
}

/**
 * @brief Kezeli egy digit kiválasztását
 *
 * Ha a kiválasztott digit különbözik a jelenlegi frekvencia lépéstől,
 * frissíti a frekvencia lépést és újrarajzolást kér.
 *
 * @param digitIndex A kiválasztott digit indexe (0=kHz, 1=100Hz, 2=10Hz)
 * @return true minden esetben (az érintés sikeresen kezelve)
 */
bool FreqDisplay::handleDigitSelection(int digitIndex) {
    if (rtv::freqstepnr == digitIndex) {
        return true; // Already selected, no change needed
    }

    updateFrequencyStep(digitIndex);
    markForRedraw();
    return true;
}

/**
 * @brief Frissíti a frekvencia lépés beállításokat
 *
 * A kiválasztott digit pozíció alapján beállítja a globális frekvencia
 * lépés változókat (rtv::freqstepnr és rtv::freqstep).
 *
 * Digit leképezés:
 * - 0: 1000 Hz (1 kHz) lépés
 * - 1: 100 Hz lépés
 * - 2: 10 Hz lépés
 *
 * @param digitIndex A kiválasztott digit indexe
 */
void FreqDisplay::updateFrequencyStep(int digitIndex) {
    rtv::freqstepnr = digitIndex;

    // Set frequency step based on digit position
    switch (digitIndex) {
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
}

/**
 * @brief Karakterenkénti frekvencia rajzolás space gap-ekkel
 *
 * Ez a függvény karakterenként rajzolja a frekvenciát és a maszkot,
 * a space karakterek helyén megfelelő gap-et hagyva.
 *
 * @param sprite A sprite objektum amire rajzolunk
 * @param freq_str A frekvencia string
 * @param mask A maszk string
 * @param colors A színkonfiguráció
 * @param totalWidth A sprite teljes szélessége
 */
void FreqDisplay::drawFrequencyWithSpaceGaps(TFT_eSprite &sprite, const String &freq_str, const char *mask, const FreqSegmentColors &colors, uint16_t totalWidth) {
    sprite.setFreeFont(&DSEG7_Classic_Mini_Regular_34);
    sprite.setTextSize(1);
    sprite.setTextDatum(TL_DATUM); // Bal felső sarokhoz igazítás karakterenkénti rajzoláshoz

    // Konstansok
    const int SPACE_GAP_WIDTH = 8; // Gap szélessége space karakterek helyén

    int maskLen = strlen(mask);
    int freqLen = freq_str.length();

    // Pozíciók számítása - a frekvencia stringet jobbra igazítjuk a maszkhoz
    // Visszafelé dolgozunk: maszk végétől visszafelé a frekvencia string végéig

    // Pozíciók tömb létrehozása minden maszk karakterhez
    int positions[32]; // Max 32 karakter
    int currentX = 0;

    // Első menet: pozíciók kiszámítása
    for (int i = 0; i < maskLen; i++) {
        positions[i] = currentX;
        if (mask[i] == ' ') {
            currentX += SPACE_GAP_WIDTH;
        } else {
            currentX += sprite.textWidth("8"); // Standard digit szélesség
        }
    }

    // Második menet: karakterek rajzolása jobbra igazítással
    int freqIndex = freqLen - 1; // Frekvencia string végétől indulunk

    for (int maskIndex = maskLen - 1; maskIndex >= 0 && freqIndex >= 0; maskIndex--) {
        char maskChar = mask[maskIndex];

        if (maskChar == ' ') {
            // Space karakternél nem rajzolunk semmit, csak továbbmegyünk
            continue;
        } else {
            char freqChar = freq_str.charAt(freqIndex);
            String singleChar = String(freqChar);

            // Inaktív karakter rajzolása (ha engedélyezve)
            if (config.data.tftDigitLigth) {
                sprite.setTextColor(colors.inactive);
                sprite.drawString(String(maskChar), positions[maskIndex], 0);
            }

            // Aktív karakter rajzolása
            sprite.setTextColor(colors.active);
            sprite.drawString(singleChar, positions[maskIndex], 0);

            freqIndex--; // Frekvencia stringben visszalépünk
        }
    }
}

/**
 * @brief Kiszámítja a frekvencia string szélességét space gap-ekkel együtt
 *
 * @param mask A maszk string, ami tartalmazza a space karaktereket
 * @return A teljes szélesség pixelben
 */
uint16_t FreqDisplay::calculateWidthWithSpaceGaps(const char *mask) {
    tft.setFreeFont(&DSEG7_Classic_Mini_Regular_34);
    tft.setTextSize(1);

    const int SPACE_GAP_WIDTH = 8; // Gap szélessége space karakterek helyén
    uint16_t totalWidth = 0;
    int maskLen = strlen(mask);

    for (int i = 0; i < maskLen; i++) {
        char maskChar = mask[i];

        if (maskChar == ' ') {
            // Space karakter - gap hozzáadása
            totalWidth += SPACE_GAP_WIDTH;
        } else {
            // Normál karakter szélessége
            totalWidth += tft.textWidth(String(maskChar));
        }
    }

    return totalWidth;
}
