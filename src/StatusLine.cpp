#include "StatusLine.h"

/**
 * @file StatusLine.cpp
 * @brief StatusLine komponens - állapotsor megjelenítése a képernyő tetején
 */
StatusLine::StatusLine(TFT_eSPI &tft, int16_t x, int16_t y, const ColorScheme &colors) : UIComponent(tft, Rect(x, y, STATUS_LINE_WIDTH, STATUS_LINE_HEIGHT), colors) {
    // Konstruktor inicializálás - a komponens készen áll a használatra
}

/**
 * @brief A komponens kirajzolása
 * @details Megrajzolja az állapotsor tartalmát
 */
void StatusLine::draw() {
    // Ha nincs szükség újrarajzolásra, akkor nem csinálunk semmit
    if (!needsRedraw) {
        return;
    }

    // Állapotsor háttérszínének beállítása
    tft.fillRect(bounds.x, bounds.y, bounds.width, bounds.height, colors.background);

    // Keret rajzolása az állapotsor körül
    tft.drawRect(bounds.x, bounds.y, bounds.width, bounds.height, colors.border);
    tft.setTextColor(colors.foreground, colors.background);
    tft.setTextDatum(ML_DATUM); // Középre igazított bal oldali szöveg
    tft.drawString("Status: Ready", bounds.x + 5, bounds.y + bounds.height / 2);

    // Jobb oldalon esetleg időpontot vagy egyéb információt lehet megjeleníteni
    tft.setTextDatum(MR_DATUM); // Középre igazított jobb oldali szöveg
    tft.drawString("12:34", bounds.x + bounds.width - 5, bounds.y + bounds.height / 2);

    // Újrarajzolás jelző visszaállítása
    needsRedraw = false;
}
