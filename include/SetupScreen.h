#ifndef __SETUP_SCREEN_H
#define __SETUP_SCREEN_H

#include "IScrollableListDataSource.h"
#include "UIScreen.h"
#include "UIScrollableListComponent.h"
#include "config.h"
#include "defines.h"

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
        INFO,
        SQUELCH_BASIS,
        SAVER_TIMEOUT,
        INACTIVE_DIGIT_LIGHT,
        BEEPER_ENABLED,
        FFT_CONFIG_AM,      // Új, összevont
        FFT_CONFIG_FM,      // Új, összevont
        CW_RECEIVER_OFFSET, // CW vételi eltolás
        RTTY_FREQUENCIES,   // RTTY Mark és Shift frekvenciák
        FACTORY_RESET,
        NONE
    };
    struct SettingItem {
        const char *label; // Const char*-ra a statikus részhez
        String value;      // Dinamikus érték Stringként
        ItemAction action;
    };

    std::shared_ptr<UIScrollableListComponent> menuList;
    std::vector<SettingItem> settingItems; // String helyett SettingItem
    std::shared_ptr<UIButton> exitButton;

    /**
     * @brief Menüelemek feltöltése.
     *
     * Frissíti a menüelemek listáját a jelenlegi beállításokkal.
     */
    void populateMenuItems() {

        // Mindent törlünk, hogy újra feltölthessük
        settingItems.clear();

        // Segédfüggvény a String konverzióhoz, ha szükséges
        auto valToString = [](auto v) { return String(v); };

        // Segédfüggvény a MiniAudioFft konfiguráció dekódolásához
        auto decodeMiniFftConfigLambda = [](float value) -> String {
            if (value == -1.0f)
                return "Disabled";
            else if (value == 0.0f)
                return "Auto Gain";
            else
                return "Manual: " + String(value, 1) + "x";
        };

        // Beállítások hozzáadása a menühöz
        settingItems.push_back({"Brightness", valToString(config.data.tftBackgroundBrightness), ItemAction::BRIGHTNESS});
        settingItems.push_back({"Squelch Basis", String(config.data.squelchUsesRSSI ? "RSSI" : "SNR"), ItemAction::SQUELCH_BASIS});
        settingItems.push_back({"Screen Saver", String(config.data.screenSaverTimeoutMinutes) + " min", ItemAction::SAVER_TIMEOUT});
        settingItems.push_back({"Inactive Digit Light", String(config.data.tftDigitLigth ? "ON" : "OFF"), ItemAction::INACTIVE_DIGIT_LIGHT});
        settingItems.push_back({"Beeper", String(config.data.beeperEnabled ? "ON" : "OFF"), ItemAction::BEEPER_ENABLED});

        settingItems.push_back({"FFT Config AM", decodeMiniFftConfigLambda(config.data.miniAudioFftConfigAm), ItemAction::FFT_CONFIG_AM});
        settingItems.push_back({"FFT Config FM", decodeMiniFftConfigLambda(config.data.miniAudioFftConfigFm), ItemAction::FFT_CONFIG_FM});

        settingItems.push_back({"CW Receiver Offset", String(config.data.cwReceiverOffsetHz) + " Hz", ItemAction::CW_RECEIVER_OFFSET});
        settingItems.push_back(
            {"RTTY Frequencies", String(round(config.data.rttyMarkFrequencyHz)) + "/" + String(round(config.data.rttyShiftHz)) + " Hz", ItemAction::RTTY_FREQUENCIES});

        settingItems.push_back({"Factory Reset", "", ItemAction::FACTORY_RESET}); // Nincs érték, vagy "Execute"

        if (menuList) {
            menuList->markForRedraw(); // Frissítjük a listát, ha már létezik
        }
    }

  public:
    /**
     * @brief Konstruktor.
     * @param tft TFT_eSPI referencia.
     */
    SetupScreen(TFT_eSPI &tft) : UIScreen(tft, SCREEN_NAME_SETUP) {
        populateMenuItems();

        const int16_t screenW = tft.width();
        const int16_t screenH = tft.height();
        const int16_t margin = 5;
        const int16_t buttonHeight = UIButton::DEFAULT_BUTTON_HEIGHT;
        const int16_t listTopMargin = 30;                            // Hely a képernyő címének
        const int16_t listBottomPadding = buttonHeight + margin * 2; // Hely az Exit gombnak és alatta/felette lévő résnek

        // Lista komponens létrehozása és hozzáadása
        Rect listBounds(margin, listTopMargin, screenW - (2 * margin), screenH - listTopMargin - listBottomPadding);
        menuList = std::make_shared<UIScrollableListComponent>(tft, listBounds, this);
        addChild(menuList);

        // Exit gomb létrehozása
        constexpr int8_t exitButtonWidth = UIButton::DEFAULT_BUTTON_WIDTH;
        Rect exitButtonBounds(screenW - exitButtonWidth - margin, screenH - buttonHeight - margin, exitButtonWidth, buttonHeight);
        exitButton =
            std::make_shared<UIButton>(tft, 0, exitButtonBounds, "Exit", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) {
                if (event.state == UIButton::EventButtonState::Clicked && getManager()) {
                    getManager()->goBack();
                }
            });
        addChild(exitButton);
    }

    virtual ~SetupScreen() = default;

    /**
     * @brief Aktiválja a képernyőt.
     * Ez a metódus frissíti a menüelemeket és jelzi, hogy a képernyő újrarajzolást igényel.
     * Meghívódik, amikor a képernyő aktívvá válik.
     */
    virtual void activate() override {
        DEBUG("SetupScreen activated.\n");
        populateMenuItems(); // Frissítjük a menüelemeket aktiváláskor
        if (menuList)
            menuList->markForRedraw();
        markForRedraw();
    }

    /**
     * @brief Kirajzolja a képernyőt.
     * Ez a metódus felelős a képernyő címének kirajzolásáért.
     */
    virtual void drawContent() override {
        // Képernyő címének kirajzolása
        tft.setTextDatum(TC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_COLOR_BACKGROUND);
        tft.setFreeFont(&FreeSansBold9pt7b); // Vagy a kívánt font
        tft.setTextSize(1);
        tft.drawString("Setup Menu", tft.width() / 2, 10);
    }

    /**
     * @brief Lekéri a menüelemek számát.
     * @return A menüelemek száma.
     */
    virtual int getItemCount() const override { return settingItems.size(); }

    /**
     * @brief Lekéri az adott indexű menüelem címkéjét.
     * @param index Az elem indexe.
     */
    virtual String getItemLabelAt(int index) const override {
        if (index >= 0 && index < settingItems.size()) {
            return String(settingItems[index].label);
        }
        return "";
    }

    /**
     * @brief Lekéri az adott indexű menüelem értékét.
     * @param index Az elem indexe.
     */
    virtual String getItemValueAt(int index) const override {
        if (index >= 0 && index < settingItems.size()) {
            return settingItems[index].value;
        }
        return "";
    }

    /**
     * @brief Esemény kezelése, amikor egy menüelemre kattintanak.
     * @param index Az elem indexe, amelyre kattintottak.
     */
    virtual void onItemClicked(int index) override {
        if (index < 0 || index >= settingItems.size())
            return;

        const SettingItem &item = settingItems[index];
        DEBUG("SetupScreen: Item %d ('%s':'%s') clicked, action: %d\n", index, item.label, item.value.c_str(), static_cast<int>(item.action));

        switch (item.action) {
        case ItemAction::BRIGHTNESS:
            // TODO: Fényerő beállító dialógus/logika
            break;
        case ItemAction::INFO:
            // TODO: Információs képernyő/dialógus
            break;
        // ... többi eset ...
        case ItemAction::NONE:
        default:
            // Nincs teendő, vagy hibaüzenet
            break;
        }
        // A beállítás módosítása után frissíteni kellhet a menüelem szövegét
        populateMenuItems(); // Újraépíti a menüt a frissített értékekkel
    }
};

#endif // __SETUP_SCREEN_H