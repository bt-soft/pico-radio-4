#ifndef __UI_COLOR_PALETTE_H
#define __UI_COLOR_PALETTE_H

#include <TFT_eSPI.h>

//--- TFT colors ---
#define TFT_COLOR(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))
#define TFT_COLOR_BACKGROUND TFT_BLACK

// Színsémák
struct ColorScheme {
    uint16_t background;
    uint16_t foreground;
    uint16_t border;
    uint16_t pressedBackground;
    uint16_t pressedForeground;
    uint16_t pressedBorder;
    uint16_t disabledBackground;
    uint16_t disabledForeground;
    uint16_t disabledBorder;
    uint16_t activeBackground;
    uint16_t activeForeground;
    uint16_t activeBorder;
    uint16_t ledOnColor;
    uint16_t ledOffColor;

    static ColorScheme defaultScheme() {
        return {
            TFT_DARKGREY,  // background
            TFT_WHITE,     // foreground
            TFT_LIGHTGREY, // border
            TFT_BLUE,      // pressedBackground
            TFT_WHITE,     // pressedForeground
            TFT_WHITE,     // pressedBorder
            TFT_BLACK,     // disabledBackground
            TFT_DARKGREY,  // disabledForeground
            TFT_DARKGREY,  // disabledBorder
            TFT_GREEN,     // activeBackground (pl. gomb ON állapota)
            TFT_WHITE,     // activeForeground
            TFT_GREEN,     // activeBorder
            TFT_GREEN,     // ledOnColor
            TFT_DARKGREEN  // ledOffColor
        };
    }
};

/**
 * Központi színpaletta a UI komponensekhez
 * Itt egy helyen lehet módosítani az alkalmazás színeit
 */
class UIColorPalette {
  public:
    // === DIALÓGUS SZÍNEK ===

    // Dialógus fejléc színek
    static constexpr uint16_t DIALOG_HEADER_BACKGROUND = TFT_NAVY; // Fejléc háttér
    static constexpr uint16_t DIALOG_HEADER_TEXT = TFT_WHITE;      // Fejléc szöveg

    // Dialógus bezáró gomb színek
    static constexpr uint16_t DIALOG_CLOSE_BUTTON_BACKGROUND = TFT_NAVY;  // Bezáró gomb háttér (fejléccel egyező)
    static constexpr uint16_t DIALOG_CLOSE_BUTTON_BORDER = TFT_NAVY;      // Bezáró gomb keret
    static constexpr uint16_t DIALOG_CLOSE_BUTTON_TEXT = TFT_WHITE;       // Bezáró gomb szöveg
    static constexpr uint16_t DIALOG_CLOSE_BUTTON_PRESSED = TFT_DARKGREY; // Bezáró gomb lenyomott állapot

    // Dialógus veil (fátyol) színek
    static constexpr uint16_t DIALOG_VEIL_COLOR = TFT_COLOR(190, 190, 190); // Fátyol szín

    // === GOMB SZÍNEK ===

    // Alapértelmezett gomb színek
    static constexpr uint16_t BUTTON_DEFAULT_BACKGROUND = TFT_COLOR(65, 65, 114); // Kék-szürke
    static constexpr uint16_t BUTTON_DEFAULT_TEXT = TFT_WHITE;
    static constexpr uint16_t BUTTON_DEFAULT_BORDER = TFT_WHITE;
    static constexpr uint16_t BUTTON_DEFAULT_PRESSED = TFT_BLUE;
    static constexpr uint16_t BUTTON_DEFAULT_PRESSED_BORDER = TFT_WHITE;

    // Speciális gomb színek
    static constexpr uint16_t BUTTON_OK_BACKGROUND = TFT_DARKGREEN;
    static constexpr uint16_t BUTTON_OK_TEXT = TFT_WHITE;
    static constexpr uint16_t BUTTON_OK_BORDER = TFT_DARKGREEN;
    static constexpr uint16_t BUTTON_CANCEL_BACKGROUND = TFT_MAROON;
    static constexpr uint16_t BUTTON_CANCEL_TEXT = TFT_WHITE;
    static constexpr uint16_t BUTTON_CANCEL_BORDER = TFT_MAROON;   // Disabled gomb színek - subtle but visible
    static constexpr uint16_t BUTTON_DISABLED_BACKGROUND = 0x2945; // Dark gray (RGB: 40, 40, 40)
    static constexpr uint16_t BUTTON_DISABLED_TEXT = 0x8410;       // Medium gray (RGB: 128, 128, 128)
    static constexpr uint16_t BUTTON_DISABLED_BORDER = 0x4208;     // Darker gray (RGB: 64, 64, 64)

    // === KÉPERNYŐ SZÍNEK ===

    // Alapértelmezett képernyő színek
    static constexpr uint16_t SCREEN_BACKGROUND = TFT_BLACK;
    static constexpr uint16_t SCREEN_TEXT = TFT_WHITE;
    static constexpr uint16_t SCREEN_BORDER = TFT_WHITE;

    // === Batterry szimbólum színek ===
    static constexpr uint16_t TFT_COLOR_DRAINED_BATTERY = TFT_COLOR(248, 252, 0);
    static constexpr uint16_t TFT_COLOR_SUBMERSIBLE_BATTERY = TFT_ORANGE;

    // === SEGÉD METÓDUSOK ===

    /**
     * Dialógus bezáró gomb ColorScheme létrehozása
     */
    static ColorScheme createDialogCloseButtonScheme() {
        ColorScheme colors;
        colors.background = DIALOG_CLOSE_BUTTON_BACKGROUND;
        colors.foreground = DIALOG_CLOSE_BUTTON_TEXT;
        colors.border = DIALOG_CLOSE_BUTTON_BORDER;
        colors.pressedForeground = DIALOG_CLOSE_BUTTON_TEXT;
        colors.pressedBackground = DIALOG_CLOSE_BUTTON_PRESSED;
        return colors;
    }

    /**
     * Alapértelmezett gomb ColorScheme létrehozása
     */
    static ColorScheme createDefaultButtonScheme() {
        ColorScheme colors = ColorScheme::defaultScheme();

        colors.background = BUTTON_DEFAULT_BACKGROUND;
        colors.foreground = BUTTON_DEFAULT_TEXT;
        colors.border = BUTTON_DEFAULT_BORDER;
        colors.pressedBackground = BUTTON_DEFAULT_PRESSED;
        colors.pressedForeground = BUTTON_DEFAULT_TEXT;
        colors.pressedBorder = BUTTON_DEFAULT_PRESSED_BORDER; // A lenyomott állapot szegélye
        colors.disabledBackground = BUTTON_DISABLED_BACKGROUND;
        colors.disabledForeground = BUTTON_DISABLED_TEXT;
        colors.disabledBorder = BUTTON_DISABLED_BORDER;
        return colors;
    }

    /**
     * OK gomb ColorScheme létrehozása
     */
    static ColorScheme createOkButtonScheme() {
        ColorScheme colors;
        colors.background = BUTTON_OK_BACKGROUND;
        colors.foreground = BUTTON_OK_TEXT;
        colors.border = BUTTON_OK_BORDER;
        colors.pressedBackground = BUTTON_DEFAULT_PRESSED_BORDER; // A lenyomott állapot szegélye
        colors.pressedForeground = BUTTON_OK_TEXT;
        return colors;
    }
    /**
     * Cancel gomb ColorScheme létrehozása
     */
    static ColorScheme createCancelButtonScheme() {
        ColorScheme colors;
        colors.background = BUTTON_CANCEL_BACKGROUND;
        colors.foreground = BUTTON_CANCEL_TEXT;
        colors.border = BUTTON_CANCEL_BORDER;
        colors.pressedBackground = TFT_RED;
        colors.pressedForeground = BUTTON_CANCEL_TEXT;
        return colors;
    }

    /**
     * Default choice gomb ColorScheme létrehozása (MultiButtonDialog-ban használatos)
     */
    static ColorScheme createDefaultChoiceButtonScheme() {
        ColorScheme colors = createDefaultButtonScheme();
        colors.background = TFT_DARKGREEN; //  háttér
        colors.foreground = TFT_NAVY;      //  szöveg
        colors.border = TFT_DARKGREEN;     //  szegély
        //---Letiltott default állapot
        colors.disabledBackground = TFT_DARKGREEN; // Sötétzöld háttér a tiltott gombokhoz
        colors.disabledForeground = TFT_BROWN;     // Barna szöveg a tiltott gombokhoz
        colors.disabledBorder = TFT_GREENYELLOW;   // Világos sárga szegély a tiltott gombokhoz
        return colors;
    }
};

#endif // __UI_COLOR_PALETTE_H
