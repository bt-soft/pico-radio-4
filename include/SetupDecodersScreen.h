#ifndef __SETUP_DECODERS_SCREEN_H
#define __SETUP_DECODERS_SCREEN_H

#include "SetupScreenBase.h"

/**
 * @brief Dekóderek beállítások képernyő.
 *
 * Ez a képernyő a különböző dekóderek (CW, RTTY, stb.) beállításait kezeli:
 * - CW vevő frekvencia eltolásának beállítása
 * - RTTY frekvenciák konfigurációja
 * - Egyéb dekóder specifikus paraméterek
 */
class SetupDecodersScreen : public SetupScreenBase {
  private:
    /**
     * @brief Dekóder specifikus menüpont akciók
     */
    enum class DecoderItemAction { NONE = 0, CW_RECEIVER_OFFSET = 200, RTTY_FREQUENCIES = 201, CW_SPEED = 202, CW_TONE_FREQ = 203, RTTY_BAUDRATE = 204, RTTY_STOP_BITS = 205 };

    // Dekóder specifikus dialógus kezelő függvények
    void handleCWOffsetDialog(int index);
    void handleRTTYFrequenciesDialog(int index);
    void handleCWSpeedDialog(int index);
    void handleCWToneFreqDialog(int index);
    void handleRTTYBaudrateDialog(int index);
    void handleRTTYStopBitsDialog(int index);

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
    SetupDecodersScreen(TFT_eSPI &tft);
    virtual ~SetupDecodersScreen() = default;
};

#endif // __SETUP_DECODERS_SCREEN_H
