#ifndef __SCREEN_SAVER_SCREEN_H
#define __SCREEN_SAVER_SCREEN_H

#include "Band.h"
#include "Config.h"
#include "FreqDisplay.h"
#include "IScreenManager.h"
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
constexpr int ANIMATION_BORDER_WIDTH = 210; // Animált keret szélessége (még kompaktabb)
constexpr int ANIMATION_BORDER_HEIGHT = 45; // Animált keret magassága (kompaktabb)

// FreqDisplay pozíció - jobb felső sarokhoz igazítva (negatív offset = jobb oldaltól számítva)
constexpr int FREQ_DISPLAY_X_OFFSET_FROM_RIGHT = -250; // FreqDisplay jobb szélétől a keret jobb széléig (-180 - 5px gap)
constexpr int FREQ_DISPLAY_Y_OFFSET_FROM_TOP = -18;    // FreqDisplay Y eltolás a keret tetejétől (kompenzálja a belső SpriteYOffset = 20-at)
constexpr int FREQ_DISPLAY_WIDTH = 180;                // FreqDisplay szélessége
constexpr int FREQ_DISPLAY_HEIGHT = 30;                // FreqDisplay magassága

// Akkumulátor szimbólum pozíció - jobb felső sarokhoz igazítva
constexpr int BATTERY_X_OFFSET_FROM_RIGHT = -43; // Akkumulátor jobb szélétől a keret jobb széléig (-38 - 5px gap)
constexpr int BATTERY_Y_OFFSET_FROM_TOP = 3;     // Akkumulátor Y pozíció a keret tetejétől

// Konvertált pozíciók (bal felső sarokhoz képest) - ezeket használja a kód
constexpr int FREQ_DISPLAY_X_OFFSET = ANIMATION_BORDER_WIDTH + FREQ_DISPLAY_X_OFFSET_FROM_RIGHT; // 210 + (-185) = 25
constexpr int FREQ_DISPLAY_Y_OFFSET = FREQ_DISPLAY_Y_OFFSET_FROM_TOP;                            // -18
constexpr int BATTERY_BASE_X_OFFSET = ANIMATION_BORDER_WIDTH + BATTERY_X_OFFSET_FROM_RIGHT;      // 210 + (-43) = 167
constexpr int BATTERY_BASE_Y_OFFSET = BATTERY_Y_OFFSET_FROM_TOP;                                 // 3
constexpr int BATTERY_RECT_W = 38;                                                               // Akkumulátor téglalap szélessége
constexpr int BATTERY_RECT_H = 18;                                                               // Akkumulátor téglalap magassága
constexpr int BATTERY_NUB_W = 2;                                                                 // Akkumulátor "Dudor" (+ érintkező) szélessége
constexpr int BATTERY_NUB_H = 10;                                                                // Akkumulátor "Dudor" (+ érintkező) magassága
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

    // UI komponensek és referenciák
    std::shared_ptr<FreqDisplay> freqDisplayComp; // Frekvencia kijelző komponens
    Band &band;                                   // Sáv referencia
    Config &config;                               // Konfiguráció referencia

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

  public:
    /**
     * @brief Konstruktor
     * @param tft TFT kijelző referencia
     * @param band_ref Sáv objektum referencia
     * @param config_ref Konfiguráció objektum referencia
     */
    ScreenSaverScreen(TFT_eSPI &tft, Band &band_ref, Config &config_ref);

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