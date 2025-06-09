// include/FreqDisplay.h
/**
 * @file FreqDisplay.h
 * @brief Frekvencia kijelző komponens - 7-szegmenses digitális kijelző különböző rádiómódokhoz
 *
 * Ez a komponens kezeli a frekvencia megjelenítését különböző demodulációs módokban:
 * - FM/AM: hagyományos frekvencia kijelzés MHz/kHz egységekkel
 * - SSB/CW: BFO kompenzált frekvencia kijelzés Hz pontossággal
 * - Optimalizált rajzolási lehetőségek a teljesítmény javításához
 * - Érintéses digit kiválasztás a frekvencia lépés beállításához
 */
#ifndef __FREQDISPLAY_H
#define __FREQDISPLAY_H

#include "Band.h"
#include "Config.h"
#include "UIColorPalette.h"
#include "UIComponent.h"
#include "rtVars.h"
#include <TFT_eSPI.h>

/**
 * @brief Színstruktúra a 7-szegmenses kijelző különböző elemeihez
 */
struct FreqSegmentColors {
    uint16_t active;    ///< Aktív számjegyek színe
    uint16_t inactive;  ///< Inaktív (háttér) számjegyek színe
    uint16_t indicator; ///< Indikátor elemek színe (egységek, aláhúzás)
};

/**
 * @brief Frekvencia kijelző komponens osztály
 *
 * A FreqDisplay komponens felelős a rádiós frekvencia megjelenítéséért
 * különböző módokban, optimalizált rajzolási algoritmusokkal és
 * érintéses interakciós lehetőségekkel.
 */
class FreqDisplay : public UIComponent {
  private:
    // === Referenciák és alapobjektumok ===
    Band &band;                     ///< Referencia a sávkezelő objektumra
    Config &config;                 ///< Referencia a konfigurációs objektumra
    TFT_eSprite spr;                ///< Sprite objektum a 7-szegmenses rajzolásához    // === Színkonfigurációk ===
    FreqSegmentColors normalColors; ///< Színek normál módban
    FreqSegmentColors bfoColors;    ///< Színek BFO módban
    FreqSegmentColors customColors; ///< Egyedi színkonfiguráció (pl. képernyővédő módhoz)
    bool useCustomColors;           ///< Ha true, akkor customColors-t használ normalColors helyett

    // === Állapotváltozók ===
    uint16_t currentDisplayFrequency; ///< Az aktuálisan kijelzendő frekvencia
    bool bfoModeActiveLastDraw;       ///< BFO mód állapota az utolsó rajzoláskor (változás detektáláshoz)
    bool redrawOnlyFrequencyDigits;   ///< Optimalizálási flag: csak számjegyek újrarajzolása
    bool hideUnderline;               ///< Ha true, az aláhúzás nem jelenik meg

    /**
     * @brief Frekvencia megjelenítési adatok struktúrája
     *
     * Ez a struktúra összefogja a frekvencia kijelzéshez szükséges
     * összes adatot: a formázott string-et, a maszkot és az egységet.
     */
    struct FrequencyDisplayData {
        String freqStr;                  ///< Formázott frekvencia string
        const __FlashStringHelper *mask; ///< 7-szegmenses maszk pattern
        const __FlashStringHelper *unit; ///< Mértékegység (MHz, kHz, Hz)
    };

    // === Rajzolás vezérlő metódusok ===
    /**
     * @brief Meghatározza, hogy szükséges-e újrarajzolás
     * @return true ha újrarajzolás szükséges
     */
    bool shouldRedraw();

    /**
     * @brief Ellenőrzi, hogy használható-e az optimalizált rajzolás
     * @return true ha optimalizált rajzolás lehetséges
     */
    bool canUseOptimizedDraw();

    /**
     * @brief Végzi az optimalizált újrarajzolást (csak számjegyek)
     */
    void performOptimizedDraw();

    /**
     * @brief Végzi a teljes újrarajzolást (teljes komponens)
     */
    void performFullDraw();

    /**
     * @brief Törli a komponens háttérterületét
     */
    void clearBackground();

    /**
     * @brief Visszaállítja az alapértelmezett szöveg beállításokat
     */
    void restoreDefaultTextSettings();

    /**
     * @brief Befejezi a rajzolási folyamatot (flag-ek törlése)
     */
    void finishDraw();

    /**
     * @brief Ellenőrzi, hogy SSB/CW mód van-e aktív
     * @param demodMode A demodulációs mód azonosítója
     * @return true ha SSB/CW mód
     */
    bool isSsbCwMode(uint8_t demodMode);

    // === SSB/CW specifikus metódusok ===
    /**
     * @brief Formázza az SSB/CW frekvenciát BFO kompenzációval
     * @param currentFrequencyValue A nyers frekvencia érték
     * @return Formázott frekvencia string
     */
    String formatSsbCwFrequency(uint16_t currentFrequencyValue);

    /**
     * @brief Kezeli a BFO átváltási animációt
     * @param formattedFreq A formázott frekvencia string
     */
    void handleBfoAnimation(const String &formattedFreq);

    /**
     * @brief Rajzolja a normál SSB/CW módot (nem BFO)
     * @param formattedFreq A formázott frekvencia
     * @param colors A használandó színek
     */
    void drawNormalSsbCwMode(const String &formattedFreq, const FreqSegmentColors &colors);

    /**
     * @brief Rajzolja a BFO módot
     * @param formattedFreq A formázott frekvencia
     * @param colors A használandó színek
     */
    void drawBfoMode(const String &formattedFreq, const FreqSegmentColors &colors);

    /**
     * @brief Rajzolja a "BFO" címkét
     * @param colors A használandó színek
     */
    void drawBfoLabel(const FreqSegmentColors &colors);

    /**
     * @brief Rajzolja a kis méretű frekvencia kijelzést BFO módban
     * @param formattedFreq A formázott frekvencia
     * @param colors A használandó színek
     */
    void drawMiniFrequency(const String &formattedFreq, const FreqSegmentColors &colors);

    // === FM/AM specifikus metódusok ===
    /**
     * @brief Előkészíti a frekvencia megjelenítési adatokat FM/AM módokhoz
     * @param frequency A frekvencia érték
     * @return Megjelenítési adatok struktúra
     */
    FrequencyDisplayData prepareFrequencyDisplayData(uint16_t frequency);

    /**
     * @brief Előkészíti az FM megjelenítési adatokat
     * @param frequency A frekvencia érték
     * @return FM megjelenítési adatok
     */
    FrequencyDisplayData prepareFmDisplayData(uint16_t frequency);

    /**
     * @brief Előkészíti az AM megjelenítési adatokat
     * @param frequency A frekvencia érték
     * @param bandType A sáv típusa (MW, LW, SW)
     * @return AM megjelenítési adatok
     */
    FrequencyDisplayData prepareAmDisplayData(uint16_t frequency, uint8_t bandType);

    // === Segédmetódusok ===
    /**
     * @brief Univerzális szöveg rajzoló metódus megadott pozícióra
     * @param text A rajzolandó szöveg
     * @param x X koordináta
     * @param y Y koordináta
     * @param textSize Szöveg méret
     * @param datum Szöveg igazítási pont
     * @param color Szöveg színe
     */
    void drawTextAtPosition(const String &text, uint16_t x, uint16_t y, uint8_t textSize, uint8_t datum, uint16_t color);

    // === Érintés kezelő metódusok ===
    /**
     * @brief Ellenőrzi, hogy a komponens kezelheti-e az érintést
     * @return true ha kezelhető az érintés
     */
    bool canHandleTouch();

    /**
     * @brief Validálja az érintés pozícióját
     * @param event Az érintési esemény
     * @return true ha érvényes pozíció
     */
    bool isValidTouchPosition(const TouchEvent &event);

    /**
     * @brief Feldolgozza a digit érintést
     * @param event Az érintési esemény
     * @return true ha sikeresen kezelve
     */
    bool processDigitTouch(const TouchEvent &event);

    /**
     * @brief Ellenőrzi, hogy az érintés egy adott digit területén van-e
     * @param event Az érintési esemény
     * @param digitIndex A digit indexe (0-2)
     * @return true ha a digit területén van az érintés
     */
    bool isTouchOnDigit(const TouchEvent &event, int digitIndex);

    /**
     * @brief Kezeli a digit kiválasztását
     * @param digitIndex A kiválasztott digit indexe
     * @return true ha sikeresen kezelve
     */
    bool handleDigitSelection(int digitIndex);

    /**
     * @brief Frissíti a frekvencia lépés beállítást
     * @param digitIndex A digit index alapján
     */
    void updateFrequencyStep(int digitIndex);

    // === Eredeti segédmetódusok ===
    /**
     * @brief Belső frekvencia rajzolási metódus sprite-tal és egységgel
     * @param freq A frekvencia string
     * @param mask A 7-szegmenses maszk
     * @param colors A színkonfiguráció
     * @param unit Az egység (opcionális)
     */
    void drawFrequencyInternal(const String &freq, const __FlashStringHelper *mask, const FreqSegmentColors &colors, const __FlashStringHelper *unit = nullptr);

    /**
     * @brief Visszaadja az aktuális szín konfigurációt (normál/BFO)
     * @return Aktuális színkonfiguráció referencia
     */
    const FreqSegmentColors &getSegmentColors() const;

    /**
     * @brief Főkoordinátor metódus SSB/CW frekvencia megjelenítéshez
     * @param currentFrequencyValue A frekvencia érték
     * @param colors A színkonfiguráció
     */
    void displaySsbCwFrequency(uint16_t currentFrequencyValue, const FreqSegmentColors &colors);

    /**
     * @brief Főkoordinátor metódus FM/AM frekvencia megjelenítéshez
     * @param currentFrequencyValue A frekvencia érték
     * @param colors A színkonfiguráció
     */
    void displayFmAmFrequency(uint16_t currentFrequencyValue, const FreqSegmentColors &colors);

    /**
     * @brief Optimalizált rajzoláshoz - csak sprite újrarajzolása
     * @param freq_str A frekvencia string
     * @param mask A maszk pattern
     * @param colors A színkonfiguráció
     */
    void drawFrequencySpriteOnly(const String &freq_str, const __FlashStringHelper *mask, const FreqSegmentColors &colors);

    /**
     * @brief Meghatározza a frekvencia stringet és maszkot optimalizált rajzoláshoz
     * @param frequency A frekvencia érték
     * @param outFreqStr [out] A frekvencia string
     * @param outMask [out] A maszk pattern
     * @return true ha sikeresen meghatározva
     */
    bool determineFreqStrAndMaskForOptimizedDraw(uint16_t frequency, String &outFreqStr, const __FlashStringHelper *&outMask);

    /**
     * @brief Rajzolja a frekvencia lépés aláhúzását
     * @param colors A színkonfiguráció
     */
    void drawStepUnderline(const FreqSegmentColors &colors);

    /**
     * @brief Kiszámítja a frekvencia sprite X pozícióját a mód alapján
     * @return A sprite X pozíciója
     */
    uint32_t calcFreqSpriteXPosition() const;

  public:
    /**
     * @brief Konstruktor - inicializálja a frekvencia kijelző komponenst
     * @param tft A TFT kijelző referencia
     * @param bounds A komponens területe
     * @param band_ref A sávkezelő referencia
     * @param config_ref A konfiguráció referencia
     */
    FreqDisplay(TFT_eSPI &tft, const Rect &bounds, Band &band_ref, Config &config_ref);

    /**
     * @brief Virtuális destruktor
     */
    virtual ~FreqDisplay() = default; /**
                                       * @brief Beállítja a megjelenítendő frekvenciát
                                       * @param freq Az új frekvencia érték
                                       */
    void setFrequency(uint16_t freq);

    /**
     * @brief Beállítja az egyedi színkonfigurációt (pl. képernyővédő módhoz)
     * @param colors Az új színkonfiguráció
     */
    void setCustomColors(const FreqSegmentColors &colors);

    /**
     * @brief Visszaállítja az alapértelmezett színkonfigurációt
     */
    void resetToDefaultColors();

    /**
     * @brief Beállítja, hogy megjelenjen-e az aláhúzás
     * @param hide Ha true, az aláhúzás elrejtve
     */
    void setHideUnderline(bool hide);

    // === UIComponent felülírt metódusok ===
    /**
     * @brief Rajzolja a komponenst (UIComponent override)
     */
    virtual void draw() override;

    /**
     * @brief Kezeli az érintési eseményeket (UIComponent override)
     * @param event Az érintési esemény
     * @return true ha az esemény kezelve lett
     */
    virtual bool handleTouch(const TouchEvent &event) override; // Megjegyzés: A handleRotary-t a szülő képernyő (FMScreen) kezeli,
    // és az hívja meg a setFrequency-t.
};

#endif // __FREQDISPLAY_H
