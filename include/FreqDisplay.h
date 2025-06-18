// include/FreqDisplay.h
/**
 * @file FreqDisplay.h
 * @brief Újratervezett frekvencia kijelző komponens - egyszerűsített és optimalizált
 *
 * Módok:
 * 1) FM/LW/AM (nem SSB vagy CW): Mértékegység jobbra igazítva, maszk tőle balra
 * 2) SSB/CW: Maszk jobbra igazítva, finomhangolás jel az utolsó 3 digit alatt, mértékegység a finomhangolás alatt
 * 3) Képernyővédő: AM/FM esetén maszk + mértékegység, SSB/CW esetén maszk + mértékegység (nincs finomhangolás)
 */
#ifndef __FREQDISPLAY_H
#define __FREQDISPLAY_H

#include <TFT_eSPI.h>

#include "Config.h"
#include "Si4735Manager.h"
#include "UIColorPalette.h"
#include "UIComponent.h"
#include "rtVars.h"

/**
 * @brief Színstruktúra a 7-szegmenses kijelző különböző elemeihez
 */
struct FreqSegmentColors {
    uint16_t active;    ///< Aktív számjegyek színe
    uint16_t inactive;  ///< Inaktív (háttér) számjegyek színe
    uint16_t indicator; ///< Indikátor elemek színe (egységek, aláhúzás)
};

/**
 * @brief Frekvencia kijelző komponens osztály - újratervezett és egyszerűsített
 */
class FreqDisplay : public UIComponent {

  public:
    constexpr static uint16_t FREQDISPLAY_HEIGHT = 60;
    constexpr static uint16_t FREQDISPLAY_WIDTH = 270;

  private:
    // === Referenciák és alapobjektumok ===
    Si4735Manager *pSi4735Manager;  ///< Hivatkozás a Si4735Manager objektumra
    TFT_eSprite spr;                ///< Sprite objektum a 7-szegmenses rajzolásához    // === Színkonfigurációk ===
    FreqSegmentColors normalColors; ///< Színek normál módban
    FreqSegmentColors bfoColors;    ///< Színek BFO módban
    FreqSegmentColors customColors; ///< Egyedi színkonfiguráció (pl. képernyővédő módhoz)
    bool useCustomColors;           ///< Ha true, akkor customColors-t használ normalColors helyett

    // === Állapotváltozók ===
    uint16_t currentDisplayFrequency; ///< Az aktuálisan kijelzendő frekvencia
    bool hideUnderline;               ///< Ha true, az aláhúzás nem jelenik meg (képernyővédő mód)
    unsigned long lastUpdateTime;     ///< Utolsó frissítés ideje (villogás optimalizáláshoz)
    bool needsFullClear;              ///< Ha true, teljes háttér törlése szükséges

    /**
     * @brief Frekvencia megjelenítési adatok struktúrája
     */
    struct FrequencyDisplayData {
        String freqStr;   ///< Formázott frekvencia string
        const char *mask; ///< 7-szegmenses maszk pattern
        const char *unit; ///< Mértékegység (MHz, kHz, Hz)
    };

    // === Pozicionálási konstansok ===
    static constexpr int UNIT_TEXT_SIZE = 2;        ///< Mértékegység szöveg mérete
    static constexpr int FREQ_7SEGMENT_HEIGHT = 38; ///< 7-szegmenses font magassága
    static constexpr int DIGIT_WIDTH = 22;          ///< Egy digit becsült szélessége érintéshez
    static constexpr int UNDERLINE_HEIGHT = 4;      ///< Aláhúzás magassága
    static constexpr int UNDERLINE_Y_OFFSET = 5;    ///< Aláhúzás távolsága a frekvenciától
    static constexpr int UNIT_Y_OFFSET_SSB_CW = 0;  ///< Mértékegység Y eltolása SSB/CW módban (aláhúzás alatt)

    // === Fő rajzolási metódusok ===
    /**
     * @brief Meghatározza a frekvencia formátumot és adatokat a mód alapján
     */
    FrequencyDisplayData getFrequencyDisplayData(uint16_t frequency);

    /**
     * @brief Rajzolja a frekvencia kijelzőt a megadott mód szerint
     */
    void drawFrequencyDisplay(const FrequencyDisplayData &data);

    /**
     * @brief Rajzolja FM/AM/LW stílusú frekvencia kijelzőt (mértékegység jobbra)
     */
    void drawFmAmLwStyle(const FrequencyDisplayData &data);

    /**
     * @brief Rajzolja SSB/CW stílusú frekvencia kijelzőt (maszk jobbra, finomhangolás, mértékegység alul)
     */
    void drawSsbCwStyle(const FrequencyDisplayData &data);

    /**
     * @brief Rajzolja a BFO módot (BFO érték nagyban, fő frekvencia kicsiben)
     */
    void drawBfoStyle(const FrequencyDisplayData &data);

    /**
     * @brief Kezeli a BFO be/kikapcsolási animációt
     */
    void handleBfoAnimation();

    /**
     * @brief Rajzolja a finomhangolás aláhúzást SSB/CW módban
     */
    void drawFineTuningUnderline(int freqSpriteX, int freqSpriteWidth);

    /**
     * @brief Kiszámítja az SSB/CW frekvencia érintési területeket
     */
    void calculateSsbCwTouchAreas(int freqSpriteX, int freqSpriteWidth);

    /**
     * @brief Visszaadja egy karakter szélességét konstansok alapján (optimalizált)
     */
    static int getCharacterWidth(char c);

    /**
     * @brief Visszaadja az aktuális színkonfigurációt
     */
    const FreqSegmentColors &getSegmentColors() const; /**
                                                        * @brief Segédmetódus szöveg rajzolásához
                                                        */
    void drawText(const String &text, int x, int y, int textSize, uint8_t datum, uint16_t color);

    /**
     * @brief Optimalizált segédmetódus a BFO frekvencia számításához
     */
    void calculateBfoFrequency(char *buffer, size_t bufferSize);

    /**
     * @brief Megbízható sprite szélesség számítás konstansokkal (textWidth() helyett)
     */
    int calculateFixedSpriteWidth(const String &mask);

    /**
     * @brief Kiszámítja a sprite szélességét space karakterekkel együtt
     */
    int calculateSpriteWidthWithSpaces(const char *mask);

    /**
     * @brief Rajzolja a frekvencia sprite-ot space karakterekkel
     */
    void drawFrequencySpriteWithSpaces(const FrequencyDisplayData &data, int x, int y, int width);

    // === Érintéskezelés ===
    bool isInSsbCwMode() const;
    int ssbCwTouchDigitAreas[3][2]; ///< Érintési területek: [digitIndex][0=x_start, 1=x_end]

  public:
    /**
     * @brief Konstruktor
     */
    FreqDisplay(TFT_eSPI &tft, const Rect &bounds, Si4735Manager *pSi4735Manager);

    /**
     * @brief Virtuális destruktor
     */
    virtual ~FreqDisplay() = default;

    /**
     * @brief Beállítja a megjelenítendő frekvenciát
     */
    void setFrequency(uint16_t freq, bool forceRedraw = false);

    /**
     * @brief Beállítja a megjelenítendő frekvenciát teljes újrarajzolással
     * @param freq Az új frekvencia érték
     * @param hideUnderline Ha true, az aláhúzás elrejtve lesz
     */
    void setFrequencyWithFullDraw(uint16_t freq, bool hideUnderline = false);

    /**
     * @brief Beállítja az egyedi színkonfigurációt (pl. képernyővédő módhoz)
     */
    void setCustomColors(const FreqSegmentColors &colors);

    /**
     * @brief Visszaállítja az alapértelmezett színkonfigurációt
     */
    void resetToDefaultColors();

    /**
     * @brief Beállítja, hogy megjelenjen-e a finomhangolás aláhúzás (képernyővédő mód)
     */
    void setHideUnderline(bool hide);

    /**
     * @brief Kényszeríti a teljes újrarajzolást (BFO módváltáskor)
     */
    void forceFullRedraw();

    // === UIComponent felülírt metódusok ===
    virtual void draw() override;
    virtual bool handleTouch(const TouchEvent &event) override;
};

#endif // __FREQDISPLAY_H
