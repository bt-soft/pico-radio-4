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
        // if (menuList) // Ez redundáns, a populateMenuItems már tartalmazza
        //     menuList->markForRedraw();
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
    virtual bool onItemClicked(int index) override {

        if (index < 0 || index >= settingItems.size())
            return false; // Ha az index érvénytelen, adjunk vissza false-t

        const SettingItem &item = settingItems[index];
        DEBUG("SetupScreen: Item %d ('%s':'%s') clicked, action: %d\n", index, item.label, item.value.c_str(), static_cast<int>(item.action));

        switch (item.action) {
            case ItemAction::BRIGHTNESS: // Fényerő beállító dialógus/logika
            {
                // Fényerő beállító dialógus létrehozása
                auto brightnessDialog = std::make_shared<ValueChangeDialog>(            //
                    this,                                                               // parentScreen
                    this->tft,                                                          // tft
                    "Brightness",                                                       // title
                    "Adjust TFT Backlight:",                                            // message (EZ HIÁNYZOTT)
                    &config.data.tftBackgroundBrightness,                               // valuePtr (most már uint8_t*)
                    (uint8_t)TFT_BACKGROUND_LED_MIN_BRIGHTNESS,                         // minValue
                    (uint8_t)TFT_BACKGROUND_LED_MAX_BRIGHTNESS,                         // maxValue
                    (uint8_t)10,                                                        // stepValue
                    [this, index](const std::variant<int, float, bool> &liveNewValue) { // callback (ValueChangeCallback) - int-ként kapja az értéket
                        // Ez az "élő" callback minden értékváltozáskor lefut a dialógusban.
                        if (std::holds_alternative<int>(liveNewValue)) {
                            int currentDialogVal = std::get<int>(liveNewValue);
                            // Közvetlenül frissítjük a config.data értékét és a hardvert.
                            config.data.tftBackgroundBrightness = static_cast<uint8_t>(currentDialogVal);
                            analogWrite(PIN_TFT_BACKGROUND_LED, config.data.tftBackgroundBrightness);
                            DEBUG("SetupScreen: Live brightness preview: %u (config updated)\n", config.data.tftBackgroundBrightness);
                        }
                    },
                    [this, index](MessageDialog::DialogResult dialogResult) {
                        // A dialog bezárásakor hívódik meg ez a callback.
                        // A dialogResult lehet Accepted vagy Rejected
                        // Nem figyeljük a dialogResult-ot, minden esetben frissítjük a lista UI elemét a végleges (elfogadott vagy visszaállított) értékkel
                        if (index >= 0 && index < settingItems.size()) {
                            settingItems[index].value = String(config.data.tftBackgroundBrightness);
                            if (menuList) {
                                menuList->refreshItemDisplay(index);
                            }
                        }
                    },
                    Rect(-1, -1, 280, 0) // Auto-magasság
                );

                this->showDialog(brightnessDialog);
            } break;

            case ItemAction::SQUELCH_BASIS: {
                // Direkt váltás, nincs szükség megerősítő dialógusra
                config.data.squelchUsesRSSI = !config.data.squelchUsesRSSI;
                DEBUG("SetupScreen: Squelch basis changed to %s\n", config.data.squelchUsesRSSI ? "RSSI" : "SNR");
                config.checkSave();
                // Frissítjük csak az érintett menüelemet
                if (index >= 0 && index < settingItems.size()) {
                    settingItems[index].value = String(config.data.squelchUsesRSSI ? "RSSI" : "SNR");
                    // Frissítjük csak az érintett menüelemet
                    if (index >= 0 && index < settingItems.size()) {
                        settingItems[index].value = String(config.data.squelchUsesRSSI ? "RSSI" : "SNR");
                        if (menuList) {
                            menuList->refreshItemDisplay(index);
                        }
                    }
                }
            } break;

            case ItemAction::SAVER_TIMEOUT: {
                Rect dlgBounds(-1, -1, 280, 0);
                static int tempSaverTimeout;
                tempSaverTimeout = config.data.screenSaverTimeoutMinutes;

                auto saverDialog = std::make_shared<ValueChangeDialog>(    //
                    this, this->tft,                                       //
                    "Screen Saver",                                        // title
                    "Timeout (minutes):",                                  // message
                    &tempSaverTimeout,                                     // valuePtr
                    SCREEN_SAVER_TIMEOUT_MIN, SCREEN_SAVER_TIMEOUT_MAX, 1, // min, max, step
                    nullptr,                                               // Nincs szükség live callback-re itt
                    [this, index](MessageDialog::DialogResult result) {
                        if (result == MessageDialog::DialogResult::Accepted) {
                            config.data.screenSaverTimeoutMinutes = static_cast<uint8_t>(tempSaverTimeout);
                            DEBUG("SetupScreen: Screen saver timeout set to: %u min\n", config.data.screenSaverTimeoutMinutes);
                            config.checkSave();
                            // Frissítjük csak az érintett menüelemet
                            if (index >= 0 && index < settingItems.size()) {
                                settingItems[index].value = String(config.data.screenSaverTimeoutMinutes) + " min";
                                if (menuList) {
                                    menuList->refreshItemDisplay(index);
                                }
                            }
                        }
                        // populateMenuItems(); // Eltávolítva
                    },
                    dlgBounds);

                this->showDialog(saverDialog);
            } break;

            case ItemAction::INACTIVE_DIGIT_LIGHT: {
                config.data.tftDigitLigth = !config.data.tftDigitLigth;
                DEBUG("SetupScreen: Inactive digit light toggled to %s\n", config.data.tftDigitLigth ? "ON" : "OFF");
                config.checkSave();
                // Frissítjük csak az érintett menüelemet
                if (index >= 0 && index < settingItems.size()) {
                    settingItems[index].value = String(config.data.tftDigitLigth ? "ON" : "OFF");
                    if (menuList)
                        menuList->refreshItemDisplay(index);
                }
            } break;

            case ItemAction::BEEPER_ENABLED: {
                config.data.beeperEnabled = !config.data.beeperEnabled;
                DEBUG("SetupScreen: Beeper toggled to %s\n", config.data.beeperEnabled ? "ON" : "OFF");
                config.checkSave();
                if (index >= 0 && index < settingItems.size()) {
                    settingItems[index].value = String(config.data.beeperEnabled ? "ON" : "OFF");
                    if (menuList)
                        menuList->refreshItemDisplay(index);
                }
            } break;

                // TODO: Implement other actions (FFT_CONFIG_AM, FFT_CONFIG_FM, CW_RECEIVER_OFFSET, RTTY_FREQUENCIES, FACTORY_RESET)
                // using ValueChangeDialog or MessageDialog as appropriate.

            case ItemAction::INFO: // Példa: Információs dialógus
            {
                // TODO: Valós információk megjelenítése
                String infoMsg = "Firmware Version: " PROGRAM_VERSION "\nAuthor: " PROGRAM_AUTHOR;
                auto infoDialog = std::make_shared<MessageDialog>(this, this->tft, Rect(-1, -1, 300, 0), "Information", infoMsg.c_str(), MessageDialog::ButtonsType::Ok);
                this->showDialog(infoDialog);
            } break;

            case ItemAction::FACTORY_RESET: {
                auto confirmDialog =
                    std::make_shared<MessageDialog>(this, this->tft, Rect(-1, -1, 300, 0), "Factory Reset", "Are you sure you want to reset all settings to default?",
                                                    MessageDialog::ButtonsType::YesNo, ColorScheme::defaultScheme(), true);
                confirmDialog->setDialogCallback([this, index](MessageDialog::DialogResult result) { // index itt lehet, hogy nem is kell, de a konzisztencia miatt maradhat
                    if (result == MessageDialog::DialogResult::Accepted) {                           // Yes
                        DEBUG("SetupScreen: Performing factory reset.\n");
                        config.loadDefaults(); // Betölti az alapértelmezett értékeket
                        config.forceSave();    // Kimenti az EEPROM-ba

                        // Itt további teendők lehetnek, pl. más store-ok resetelése
                        // fmStationStore.loadDefaults(); fmStationStore.forceSave();
                        // amStationStore.loadDefaults(); amStationStore.forceSave();
                        // TODO: Szükség esetén újraindítás vagy a felhasználó tájékoztatása
                        populateMenuItems(); // Itt helyénvaló a teljes frissítés
                    }
                });
                this->showDialog(confirmDialog);
            } break;

            // ... többi eset ...
            case ItemAction::NONE:
            default:
                // Nincs teendő, vagy hibaüzenet
                break;
        }
        // Alapértelmezés szerint false-t adunk vissza, mert vagy
        // - egyedi sort frissítettünk, vagy
        // - dialógust nyitottunk, ami később kezeli a frissítést.
        // Ha egy eset explicit teljes újra-rajzolást igényelne a UIScrollableListComponent-től,
        // akkor az a case adna vissza true-t.
        return false;
    } // End of onItemClicked
};

#endif // __SETUP_SCREEN_H