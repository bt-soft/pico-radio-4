#ifndef __SETUP_DISPLAY_SCREEN_H
#define __SETUP_DISPLAY_SCREEN_H

#include "SetupScreenBase.h"

/**
 * @brief Kijelző beállítások képernyő.
 *
 * Ez a képernyő a kijelző és felhasználói felület beállításait kezeli:
 * - TFT háttérvilágítás fényességének beállítása
 * - Képernyővédő időtúllépésének beállítása
 * - Inaktív számjegyek világítása
 * - Hangjelzések engedélyezése
 */
class SetupDisplayScreen : public SetupScreenBase {
  private:
    /**
     * @brief Kijelző specifikus menüpont akciók
     */
    enum class DisplayItemAction {
        NONE = 0,
        BRIGHTNESS = 300,
        SAVER_TIMEOUT = 301,
        INACTIVE_DIGIT_LIGHT = 302,
        BEEPER_ENABLED = 303,
        CONTRAST = 304,
        COLOR_SCHEME = 305,
        FONT_SIZE = 306
    };

    // Kijelző specifikus dialógus kezelő függvények
    void handleBrightnessDialog(int index);
    void handleSaverTimeoutDialog(int index);
    void handleToggleItem(int index, bool &configValue);
    void handleContrastDialog(int index);
    void handleColorSchemeDialog(int index);
    void handleFontSizeDialog(int index);

  protected:
    // SetupScreenBase virtuális metódusok implementációja
    virtual void populateMenuItems() override;
    virtual void handleItemAction(int index, int action) override;
    virtual const char *getScreenTitle() const override;

  public:
    /**
     * @brief Konstruktor.
     * @param tft TFT_eSPI referencia.
     */
    SetupDisplayScreen(TFT_eSPI &tft);
    virtual ~SetupDisplayScreen() = default;
};

#endif // __SETUP_DISPLAY_SCREEN_H
