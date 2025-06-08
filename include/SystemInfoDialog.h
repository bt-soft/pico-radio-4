#ifndef __SYSTEM_INFO_DIALOG_H
#define __SYSTEM_INFO_DIALOG_H

#include "MessageDialog.h"
#include "PicoMemoryInfo.h"
#include "StationData.h"
#include "defines.h"

/**
 * @brief Rendszer információ dialógus - MessageDialog alapú implementáció
 *
 * Átfogó rendszer információkat jelenít meg angolul:
 * - Program információk (név, verzió, szerző, fordítás dátuma)
 * - Memória használat (Flash, RAM, EEPROM)
 * - Hardver információk
 * - Si4735 rádió modul információk (helykitöltő)
 *
 * A MessageDialog-ból örökli az OK gomb automatikus kezelését.
 */
class SystemInfoDialog : public MessageDialog {
  public:
    /**
     * @brief Konstruktor
     * @param parentScreen Szülő képernyő, amely ezt a dialógust birtokolja
     * @param tft TFT kijelző referencia
     * @param bounds Dialógus határok
     * @param cs Színséma (opcionális)
     */
    SystemInfoDialog(UIScreen *parentScreen, TFT_eSPI &tft, const Rect &bounds, const ColorScheme &cs = ColorScheme::defaultScheme());

    virtual ~SystemInfoDialog() = default;

  protected:
    // Felülírjuk a drawSelf-et a custom szöveg megjelenítéshez
    virtual void drawSelf() override;

    // Felülírjuk a layoutDialogContent-et a navigációs gombok helyes pozicionálásához
    virtual void layoutDialogContent() override;

  private:
    // Lapozás állapot
    int currentPage;

    // Navigációs gombok
    std::shared_ptr<UIButton> prevButton;
    std::shared_ptr<UIButton> nextButton;

    // Adatok összegyűjtési módszerek
    String getCurrentPageContent();
    String formatProgramInfo();
    String formatMemoryInfo();
    String formatHardwareInfo();
    String formatSi4735Info(); // Segéd módszerek
    void updateNavigationButtons();

    // Konstansok
    static const int TOTAL_PAGES = 4; // Program, Memory, Hardware, Radio
};

#endif // __SYSTEM_INFO_DIALOG_H
