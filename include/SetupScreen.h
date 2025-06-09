#ifndef __SETUP_SCREEN_H
#define __SETUP_SCREEN_H

#include "IScrollableListDataSource.h"
#include "UIButton.h"
#include "UIScreen.h"
#include "UIScrollableListComponent.h"
#include <functional>
#include <vector>

// Forward deklarációk
class MultiButtonDialog;
class ValueChangeDialog;
class SystemInfoDialog;
class MessageDialog;
class UIDialogBase;

/**
 * @brief Beállítások képernyő.
 *
 * Ez a képernyő egy görgethető listát használ a különböző beállítási
 * menüpontok megjelenítésére.
 */
class SetupScreen : public UIScreen, public IScrollableListDataSource {
  private:
    enum class ItemAction {
        BRIGHTNESS,
        SQUELCH_BASIS,
        SAVER_TIMEOUT,
        INACTIVE_DIGIT_LIGHT,
        BEEPER_ENABLED,
        FFT_CONFIG_AM,
        FFT_CONFIG_FM,
        CW_RECEIVER_OFFSET,
        RTTY_FREQUENCIES,
        INFO,
        FACTORY_RESET,
        NONE
    };

    struct SettingItem {
        const char *label;
        String value;
        ItemAction action;
    };

    std::shared_ptr<UIScrollableListComponent> menuList;
    std::vector<SettingItem> settingItems;
    std::shared_ptr<UIButton> exitButton;

    // Segédfüggvények
    void populateMenuItems();
    void updateListItem(int index);
    String decodeFFTConfig(float value);

    // Dialógus kezelő függvények
    void handleBrightnessDialog(int index);
    void handleSquelchBasisDialog(int index);
    void handleSaverTimeoutDialog(int index);
    void handleToggleItem(int index, bool &configValue);
    void handleFFTConfigDialog(int index, bool isAM);
    void handleCWOffsetDialog(int index);
    void handleRTTYFrequenciesDialog(int index);
    void handleSystemInfoDialog();
    void handleFactoryResetDialog();

  public:
    /**
     * @brief Konstruktor.
     * @param tft TFT_eSPI referencia.
     */
    SetupScreen(TFT_eSPI &tft);
    virtual ~SetupScreen() = default;

    // UIScreen interface
    virtual void activate() override;
    virtual void drawContent() override;

    // IScrollableListDataSource interface
    virtual int getItemCount() const override;
    virtual String getItemLabelAt(int index) const override;
    virtual String getItemValueAt(int index) const override;
    virtual bool onItemClicked(int index) override;
};

#endif // __SETUP_SCREEN_H