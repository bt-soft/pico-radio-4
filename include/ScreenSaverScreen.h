#ifndef __SCREEN_SAVER_SCREEN_H
#define __SCREEN_SAVER_SCREEN_H

#include "Config.h"
#include "FreqDisplay.h"
#include "IScreenManager.h"
#include "Si4735Manager.h"
#include "UIScreen.h"
#include "defines.h"
#include "rtVars.h"

/**
 * @brief Képernyővédő konstansok namespace
 * @details Animáció és UI elem pozicionálási konstansok
 */
namespace ScreenSaverConstants {
// Animáció alapvető paraméterei
constexpr int SAVER_ANIMATION_STEPS = 500;         // Animációs lépések száma
constexpr int SAVER_ANIMATION_LINE_LENGTH = 63;    // Animációs vonal hossza
constexpr int SAVER_LINE_CENTER = 31;              // Vonal középpontja
constexpr int SAVER_NEW_POS_INTERVAL_MSEC = 15000; // Új pozíció intervalluma (ms)
constexpr int SAVER_COLOR_FACTOR = 64;             // Szín változtatási faktor
constexpr int SAVER_ANIMATION_STEP_JUMP = 3;       // Animációs lépés ugrás

// Animált keret mérete és UI elemek relatív pozíciói a keret bal felső sarkához képest
// Különböző szélességek a rádió módok szerint
constexpr int ANIMATION_BORDER_WIDTH_FM = 210;      // FM mód: széles keret
constexpr int ANIMATION_BORDER_WIDTH_AM = 180;      // AM mód: közepes keret
constexpr int ANIMATION_BORDER_WIDTH_SSB = 150;     // SSB módok (LSB/USB): keskeny keret
constexpr int ANIMATION_BORDER_WIDTH_CW = 120;      // CW mód: legkeskenyebb keret
constexpr int ANIMATION_BORDER_WIDTH_DEFAULT = 210; // Alapértelmezett szélesség
constexpr int ANIMATION_BORDER_HEIGHT = 45;         // Animált keret magassága (kompaktabb)

// FreqDisplay pozíció - jobb felső sarokhoz igazítva (negatív offset = jobb oldaltól számítva)
constexpr int FREQ_DISPLAY_X_OFFSET_FROM_RIGHT = -1 * FreqDisplay::FREQDISPLAY_WIDTH + 35;
constexpr int FREQ_DISPLAY_Y_OFFSET_FROM_TOP = 0;
// Konvertált pozíciók (bal felső sarokhoz képest) - ezeket használja a kód
// Megjegyzés: ezek dinamikusan kerülnek kiszámításra a getCurrentBorderWidth() alapján
constexpr int FREQ_DISPLAY_Y_OFFSET = FREQ_DISPLAY_Y_OFFSET_FROM_TOP;

// Akkumulátor szimbólum pozíció - jobb felső sarokhoz igazítva
constexpr int BATTERY_X_OFFSET_FROM_RIGHT = -43; // Akkumulátor jobb szélétől a keret jobb széléig
constexpr int BATTERY_Y_OFFSET_FROM_TOP = 3;     // Akkumulátor Y pozíció a keret tetejétől
// Megjegyzés: BATTERY_BASE_X_OFFSET dinamikusan kerül kiszámításra a getCurrentBorderWidth() alapján
constexpr int BATTERY_BASE_Y_OFFSET = BATTERY_Y_OFFSET_FROM_TOP;
constexpr uint8_t BATTERY_RECT_W = 38; // Akkumulátor téglalap szélessége
constexpr uint8_t BATTERY_RECT_H = 18; // Akkumulátor téglalap magassága
constexpr uint8_t BATTERY_NUB_W = 2;   // Akkumulátor "Dudor" (+ érintkező) szélessége
constexpr uint8_t BATTERY_NUB_H = 10;  // Akkumulátor "Dudor" (+ érintkező) magassága
} // namespace ScreenSaverConstants

/**
 * @file ScreenSaverScreen.h
 * @brief Képernyővédő osztály implementációja
 * @details Animált kerettel és frekvencia kijelzéssel rendelkező képernyővédő
 */
class ScreenSaverScreen : public UIScreen {
  private:
    // Időzítés változók
    uint32_t activationTime;          // Képernyővédő aktiválásának időpontja
    uint32_t lastAnimationUpdateTime; // Utolsó animáció frissítés időpontja

    /**
     * @brief Képernyővédő aktiválása
     * @details Privát metódus, konstruktorból hívva
     */
    virtual void activate() override; // Animáció állapot változók
    uint16_t animationBorderX;        // Animált keret bal felső sarkának X koordinátája
    uint16_t animationBorderY;        // Animált keret bal felső sarkának Y koordinátája
    uint16_t currentFrequencyValue;   // Aktuális frekvencia érték

    uint16_t posSaver;                                                          // Animáció pozíció számláló
    uint8_t saverLineColors[ScreenSaverConstants::SAVER_ANIMATION_LINE_LENGTH]; // Animációs vonal színei

    uint32_t lastFullUpdateSaverTime; // Utolsó teljes frissítés időpontja

    /**
     * @brief Animált keret rajzolása
     * @details Frekvencia kijelző körül mozgó téglalap keret rajzolása
     */
    void drawAnimatedBorder();

    /**
     * @brief Akkumulátor információ rajzolása
     * @details Akkumulátor szimbólum és töltöttségi szöveg kirajzolása
     * Az animált keret pozíciójához relatívan pozícionálva
     */
    void drawBatteryInfo();

    /**
     * @brief Frekvencia és akkumulátor kijelző frissítése
     * @details 15 másodpercenként frissíti a pozíciót és a kijelzett információkat
     */
    void updateFrequencyAndBatteryDisplay();

    /**
     * @brief Aktuális rádió módhoz tartozó keret szélesség meghatározása
     * @return A keret szélessége pixelben
     * @details FM: 210px, AM: 180px, SSB: 150px, CW: 120px
     */
    uint16_t getCurrentBorderWidth() const;

  public:
    /**
     * @brief Konstruktor
     * @param tft TFT kijelző referencia
     * @param band_ref Sáv objektum referencia
     * @param config_ref Konfiguráció objektum referencia
     */
    ScreenSaverScreen(TFT_eSPI &tft, Si4735Manager &si4735Manager);

    /**
     * @brief Destruktor
     */
    virtual ~ScreenSaverScreen() = default;

    /**
     * @brief Képernyővédő deaktiválása
     */
    virtual void deactivate() override;

    /**
     * @brief Tartalom rajzolása
     * @details Minden animációs frame-ben meghívódik
     */
    virtual void drawContent() override;

    /**
     * @brief Saját loop kezelése
     * @details Animáció és időzítés logika
     */
    virtual void handleOwnLoop() override;

    /**
     * @brief Érintés esemény kezelése
     * @param event Érintés esemény
     * @return true ha kezelte az eseményt, false egyébként
     */
    virtual bool handleTouch(const TouchEvent &event) override;

    /**
     * @brief Forgó encoder esemény kezelése
     * @param event Forgó encoder esemény
     * @return true ha kezelte az eseményt, false egyébként
     */
    virtual bool handleRotary(const RotaryEvent &event) override;
};

#endif // __SCREEN_SAVER_SCREEN_H