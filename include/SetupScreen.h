#ifndef __SETUP_SCREEN_H
#define __SETUP_SCREEN_H

#include "SetupScreenBase.h"

// Forward deklarációk
class SystemInfoDialog;
class MessageDialog;

/**
 * @brief Főbeállítások képernyő.
 *
 * Ez a képernyő a fő setup menüt jeleníti meg, amely almenükre vezet:
 * - Display Settings (kijelző beállítások)
 * - Si4735 Settings (rádió chip beállítások)
 * - System Information
 * - Factory Reset
 */
class SetupScreen : public SetupScreenBase {
  private:
    /**
     * @brief Főmenü specifikus menüpont akciók
     */
    enum class MainItemAction {
        NONE = 0,
        DISPLAY_SETTINGS = 400, // Almenü: Display beállítások
        SI4735_SETTINGS = 401,  // Almenü: Si4735 beállítások
        DECODER_SETTINGS = 402, // Almenü: Dekóder beállítások
        INFO = 403,             // System Information dialógus
        FACTORY_RESET = 404     // Factory Reset dialógus
    };

    // Dialógus kezelő függvények
    void handleSystemInfoDialog();
    void handleFactoryResetDialog();

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
    SetupScreen(TFT_eSPI &tft);
    virtual ~SetupScreen() = default;
};

#endif // __SETUP_SCREEN_H