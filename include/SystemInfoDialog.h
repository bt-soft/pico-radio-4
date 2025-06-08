#ifndef __SYSTEM_INFO_DIALOG_H
#define __SYSTEM_INFO_DIALOG_H

#include "PicoMemoryInfo.h"
#include "StationData.h"
#include "UIButton.h"
#include "UIDialogBase.h"
#include "defines.h"

/**
 * @brief Rendszer információ dialógus
 *
 * Átfogó rendszer információkat jelenít meg angolul:
 * - Program információk (név, verzió, szerző, fordítás dátuma)
 * - Memória használat (Flash, RAM, EEPROM)
 * - Hardver információk
 * - Si4735 rádió modul információk (helykitöltő)
 */
class SystemInfoDialog : public UIDialogBase {
  public: /**
           * @brief Konstruktor
           * @param parentScreen Szülő képernyő, amely ezt a dialógust birtokolja
           * @param tft TFT kijelző referencia
           * @param bounds Dialógus határok (használd Rect(-1, -1, width, 0) auto-magassághoz)
           */
    SystemInfoDialog(UIScreen *parentScreen, TFT_eSPI &tft, const Rect &bounds);
    virtual ~SystemInfoDialog() = default;

  protected:
    std::shared_ptr<UIButton> okButton;
    String infoContent; // Adatok összegyűjtési módszerek
    void collectSystemInfo();
    String formatProgramInfo();
    String formatMemoryInfo();
    String formatHardwareInfo();
    String formatSi4735Info();

    // UI módszerek
    virtual void createDialogContent() override;
    virtual void layoutDialogContent() override;
    virtual void drawSelf() override;

    // Eseménykezelés
    void onOkButtonClicked();
};

#endif // __SYSTEM_INFO_DIALOG_H
