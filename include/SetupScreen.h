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
        String label; // Módosítva String-re
        ItemAction action;
    };

    std::shared_ptr<UIScrollableListComponent> menuList;
    std::vector<SettingItem> settingItems; // String helyett SettingItem

    /**
     * @brief MiniAudioFft konfiguráció dekódolása.
     *
     * Dekódolja a MiniAudioFft konfigurációs értéket emberi olvasható formára.
     * -1.0f: Disabled, 0.0f: Auto, >0.0f: Manual Gain Factor
     *
     * @param value A MiniAudioFft konfigurációs érték.
     * @return String Az emberi olvasható formátum.
     */
    String decodeMiniFftConfig(float value) const {
        if (value == -1.0f)
            return "Disabled";
        else if (value == 0.0f)
            return "Auto Gain";
        else
            return "Manual: " + String(value, 1) + "x";
    }

    /**
     * @brief Menüelemek feltöltése.
     *
     * Frissíti a menüelemek listáját a jelenlegi beállításokkal.
     */
    void populateMenuItems() {

        // Mindent törlünk, hogy újra feltölthessük
        settingItems.clear();

        // Beállítások hozzáadása a menühöz
        settingItems.push_back({String("Brightness: ") + String(config.data.tftBackgroundBrightness), ItemAction::BRIGHTNESS});
        settingItems.push_back({String("Squelch Basis: ") + config.data.squelchUsesRSSI ? "RSSI" : "SNR", ItemAction::SQUELCH_BASIS});
        settingItems.push_back({String("Screen Saver: ") + String(config.data.screenSaverTimeoutMinutes) + " min", ItemAction::SAVER_TIMEOUT});
        settingItems.push_back({String("Inactive Digit Light: ") + (config.data.tftDigitLigth ? "ON" : "OFF"), ItemAction::INACTIVE_DIGIT_LIGHT});
        settingItems.push_back({String("Beeper: ") + (config.data.beeperEnabled ? "ON" : "OFF"), ItemAction::BEEPER_ENABLED});

        settingItems.push_back({String("FFT Config AM: ") + decodeMiniFftConfig(config.data.miniAudioFftConfigAm), ItemAction::FFT_CONFIG_AM});
        settingItems.push_back({String("FFT Config FM: ") + decodeMiniFftConfig(config.data.miniAudioFftConfigFm), ItemAction::FFT_CONFIG_FM});

        settingItems.push_back({String("CW Receiver Offset: ") + String(config.data.cwReceiverOffsetHz) + " Hz", ItemAction::CW_RECEIVER_OFFSET});
        settingItems.push_back({String("RTTY Frequencies: ") + String(round(config.data.rttyMarkFrequencyHz)) + " Hz / " + String(round(config.data.rttyShiftHz)) + " Hz",
                                ItemAction::RTTY_FREQUENCIES});

        settingItems.push_back({"Factory Reset", ItemAction::FACTORY_RESET}); // Ennek nincs dinamikus értéke a labelben
        settingItems.push_back({"Exit Setup", ItemAction::NONE});

        if (menuList) {
            menuList->markForRedraw(); // Frissítjük a listát, ha már létezik
        }
    }

  public:
    SetupScreen(TFT_eSPI &tft) : UIScreen(tft, "SetupScreen") {
        populateMenuItems();

        // Lista komponens létrehozása és hozzáadása
        // A bounds-ot úgy állítsd be, hogy a képernyő nagy részét elfoglalja,
        // esetleg hagyva helyet egy fejlécnek vagy láblécnek.
        Rect listBounds(10, 30, tft.width() - 20, tft.height() - 40);
        menuList = std::make_shared<UIScrollableListComponent>(tft, listBounds, this);
        addChild(menuList);
    }

    virtual ~SetupScreen() = default;

    virtual void activate() override {
        DEBUG("SetupScreen activated.\n");
        populateMenuItems(); // Frissítjük a menüelemeket aktiváláskor
        if (menuList)
            menuList->markForRedraw();
        markForRedraw();
    }

    virtual void drawSelf() override {
        // Képernyő címének kirajzolása
        tft.setTextDatum(TC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_COLOR_BACKGROUND);
        tft.setFreeFont(&FreeSansBold9pt7b); // Vagy a kívánt font
        tft.setTextSize(1);
        tft.drawString("Setup Menu", tft.width() / 2, 10);
    }

    // IScrollableListDataSource implementáció
    virtual int getItemCount() const override { return settingItems.size(); }

    virtual String getItemAt(int index) const override {
        if (index >= 0 && index < settingItems.size()) {
            // Most már a teljes, formázott label-t adjuk vissza
            return settingItems[index].label;
        }
        return "";
    }

    virtual void onItemClicked(int index) override {
        if (index < 0 || index >= settingItems.size())
            return;

        const SettingItem &item = settingItems[index];
        DEBUG("SetupScreen: Item %d ('%s') clicked, action: %d\n", index, item.label, static_cast<int>(item.action));

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