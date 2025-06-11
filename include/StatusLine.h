#ifndef __STATUSLINE_H
#define __STATUSLINE_H

#include "UIComponent.h"

/**
 * @file StatusLine.h
 * @brief StatusLine komponens - állapotsor megjelenítése a képernyő tetején
 * @details Ez a komponens egy állapotsor megjelenítésére szolgál, amely a képernyő tetején helyezkedik el.
 * A komponens nem reagál touch és rotary eseményekre, csak információ megjelenítésére szolgál.
 */
class StatusLine : public UIComponent {
  private:
    // Állandók a komponens méreteihez
    static constexpr uint16_t STATUS_LINE_WIDTH = 240; // Állapotsor szélessége pixelben
    static constexpr uint16_t STATUS_LINE_HEIGHT = 16; // Állapotsor magassága pixelben

  public:
    /**
     * @brief StatusLine konstruktor
     * @param tft TFT display referencia
     * @param x A komponens X koordinátája
     * @param y A komponens Y koordinátája
     * @param colors Színséma (opcionális, alapértelmezett színsémát használ ha nincs megadva)
     */
    StatusLine(TFT_eSPI &tft, int16_t x, int16_t y, const ColorScheme &colors = ColorScheme::defaultScheme());

    /**
     * @brief Virtuális destruktor
     */
    virtual ~StatusLine() = default;

    /**
     * @brief A komponens kirajzolása
     * @details Megrajzolja az állapotsor tartalmát
     */
    virtual void draw() override;

    /**
     * @brief Touch esemény kezelése
     * @param event Touch esemény
     * @return false - ez a komponens nem kezeli a touch eseményeket
     */
    virtual bool handleTouch(const TouchEvent &event) override { return false; };

    /**
     * @brief Rotary encoder esemény kezelése
     * @param event Rotary esemény
     * @return false - ez a komponens nem kezeli a rotary eseményeket
     */
    virtual bool handleRotary(const RotaryEvent &event) override { return false; };

    /**
     * @brief Komponens szélesség konstans getter
     * @return Az állapotsor szélessége pixelben
     */
    static constexpr uint16_t getStatusLineWidth() { return STATUS_LINE_WIDTH; }

    /**
     * @brief Komponens magasság konstans getter
     * @return Az állapotsor magassága pixelben
     */
    static constexpr uint16_t getStatusLineHeight() { return STATUS_LINE_HEIGHT; }

  protected:
    /**
     * @brief Vizuális lenyomott visszajelzés letiltása
     * @return false - ez a komponens nem ad vizuális visszajelzést lenyomásra
     */
    virtual bool allowsVisualPressedFeedback() const override { return false; }
};

#endif // __STATUSLINE_H
