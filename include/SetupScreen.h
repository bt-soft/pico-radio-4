#ifndef __SETUP_SCREEN_H
#define __SETUP_SCREEN_H

#include "IScrollableListDataSource.h"
#include "MultiButtonDialog.h"
#include "PicoMemoryInfo.h"
#include "StationData.h"
#include "SystemInfoDialog.h"
#include "UIScreen.h"
#include "UIScrollableListComponent.h"
#include "ValueChangeDialog.h"
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
        SQUELCH_BASIS,
        SAVER_TIMEOUT,
        INACTIVE_DIGIT_LIGHT,
        BEEPER_ENABLED,
        FFT_CONFIG_AM,
        FFT_CONFIG_FM,
        CW_RECEIVER_OFFSET, // CW vételi eltolás
        RTTY_FREQUENCIES,   // RTTY Mark és Shift frekvenciák
        INFO,
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
     * @brief Általános DialogCallback segédfüggvény lista elem frissítéshez.
     * @param index A frissítendő elem indexe
     * @param valueGetter Függvény, ami visszaadja az új értéket String-ként
     * @return DialogCallback, amit dialógusokhoz lehet használni
     */
    std::function<void(UIDialogBase *, MessageDialog::DialogResult)> createListUpdateCallback(int index, std::function<String()> valueGetter) {
        return [this, index, valueGetter](UIDialogBase *sender, MessageDialog::DialogResult dialogResult) {
            if (dialogResult == MessageDialog::DialogResult::Accepted) {
                if (index >= 0 && index < settingItems.size()) {
                    settingItems[index].value = valueGetter();
                    if (menuList) {
                        menuList->refreshItemDisplay(index);
                    }
                }
            }
        };
    }

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

        settingItems.push_back({"System Information", "", ItemAction::INFO});     // "Show Info"
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
                    }, // "Élő" ValueChangeCallback vége
                    // DialogCallback paraméter - egyszerűsítve a segédfüggvénnyel
                    createListUpdateCallback(index, []() { return String(config.data.tftBackgroundBrightness); }), //
                    Rect(-1, -1, 280, 0)                                                                           // Auto-magasság
                );

                this->showDialog(brightnessDialog);
            } break;
            case ItemAction::SQUELCH_BASIS: {
                const char *options[] = {"RSSI", "SNR"};
                int currentSelection = config.data.squelchUsesRSSI ? 0 : 1; // Current squelch basis as default
                auto basisDialog = std::make_shared<MultiButtonDialog>(
                    this, this->tft,                                                                     // parentScreen, tft
                    "Squelch Basis",                                                                     // title
                    "Select squelch basis:",                                                             // message
                    options, ARRAY_ITEM_COUNT(options),                                                  // gombok feliratai és számossága
                    [this, index](int buttonIndex, const char *buttonLabel, MultiButtonDialog *dialog) { // ButtonClickCallback
                        // Beállítjuk az új értéket
                        bool newSquelchUsesRSSI = (buttonIndex == 0); // 0 for RSSI, 1 for SNR
                        if (config.data.squelchUsesRSSI != newSquelchUsesRSSI) {
                            config.data.squelchUsesRSSI = newSquelchUsesRSSI;
                            config.checkSave();
                        }
                        // Frissítjük a lista UI elemét
                        settingItems[index].value = String(config.data.squelchUsesRSSI ? "RSSI" : "SNR");
                        if (menuList) {
                            menuList->refreshItemDisplay(index);
                        }
                    },                     // ButtonClickCallback vége
                    true,                  // autoClose
                    currentSelection,      // defaultButtonIndex - current setting highlighted
                    true,                  // disableDefaultButton - keep default behavior (disabled)
                    Rect(-1, -1, 250, 120) // bounds
                );
                this->showDialog(basisDialog);
            } break;

            case ItemAction::SAVER_TIMEOUT: {
                auto saverDialog = std::make_shared<ValueChangeDialog>(                 //
                    this, this->tft,                                                    //
                    "Screen Saver",                                                     // title
                    "Timeout (minutes):",                                               // message
                    &config.data.screenSaverTimeoutMinutes,                             // valuePtr
                    SCREEN_SAVER_TIMEOUT_MIN, SCREEN_SAVER_TIMEOUT_MAX, 1,              // min, max, step
                    [this, index](const std::variant<int, float, bool> &liveNewValue) { // "Élő" ValueChangeCallback
                        if (std::holds_alternative<int>(liveNewValue)) {
                            int currentDialogVal = std::get<int>(liveNewValue);
                            config.data.screenSaverTimeoutMinutes = static_cast<uint8_t>(currentDialogVal);
                            config.checkSave();
                        }
                    },
                    // DialogCallback paraméter - egyszerűsítve a segédfüggvénnyel
                    createListUpdateCallback(index, []() { return String(config.data.screenSaverTimeoutMinutes) + " min"; }), //
                    Rect(-1, -1, 280, 0));

                this->showDialog(saverDialog);
            } break;

            case ItemAction::INACTIVE_DIGIT_LIGHT: {
                config.data.tftDigitLigth = !config.data.tftDigitLigth;
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

            case ItemAction::FFT_CONFIG_AM:
            case ItemAction::FFT_CONFIG_FM: {
                bool isAM = (item.action == ItemAction::FFT_CONFIG_AM);
                float &currentConfig = isAM ? config.data.miniAudioFftConfigAm : config.data.miniAudioFftConfigFm;
                const char *title = isAM ? "FFT Config AM" : "FFT Config FM";

                // Determine current setting for default button highlighting
                int defaultSelection = 0; // Disabled
                if (currentConfig == 0.0f) {
                    defaultSelection = 1; // Auto G
                } else if (currentConfig > 0.0f) {
                    defaultSelection = 2; // Manu G
                }
                const char *options[] = {"Disabled", "Auto G", "Manu G"};

                // Create fftDialog shared_ptr that will be captured in lambda
                std::shared_ptr<MultiButtonDialog> fftDialog;

                fftDialog = std::make_shared<MultiButtonDialog>(
                    this, this->tft,                                                                                                             // parentScreen, tft
                    title,                                                                                                                       // title
                    "Select FFT gain mode:",                                                                                                     // message
                    options, ARRAY_ITEM_COUNT(options),                                                                                          // buttons
                    [this, index, isAM, &currentConfig, title, fftDialog](int buttonIndex, const char *buttonLabel, MultiButtonDialog *dialog) { // ButtonClickCallback
                        DEBUG("SetupScreen: FFT Config %s button %d ('%s') clicked\n", isAM ? "AM" : "FM", buttonIndex, buttonLabel);
                        switch (buttonIndex) {
                            case 0: // Disabled
                                currentConfig = -1.0f;
                                config.checkSave();
                                // Update list item and close dialog
                                settingItems[index].value = "Disabled";
                                if (menuList) {
                                    menuList->refreshItemDisplay(index);
                                }
                                // Manual close for non-nested case
                                dialog->close(UIDialogBase::DialogResult::Accepted);
                                break;

                            case 1: // Auto G
                                currentConfig = 0.0f;
                                config.checkSave();
                                // Update list item and close dialog
                                settingItems[index].value = "Auto Gain";
                                if (menuList) {
                                    menuList->refreshItemDisplay(index);
                                }
                                // Manual close for non-nested case
                                dialog->close(UIDialogBase::DialogResult::Accepted);
                                break;
                            case 2: // Manu G - open nested ValueChangeDialog
                            {
                                // Close the MultiButtonDialog immediately to avoid conflicts
                                dialog->close(UIDialogBase::DialogResult::Accepted);

                                // Shared pointer-rel kezeljük a tempGainValue-t, hogy biztosan életben maradjon
                                auto tempGainValuePtr = std::make_shared<float>((currentConfig > 0.0f) ? currentConfig : 1.0f); // Default to 1.0 if not already manual

                                DEBUG("SetupScreen: Opening manual gain dialog with initial value: %.1f\n", *tempGainValuePtr);

                                auto gainDialog = std::make_shared<ValueChangeDialog>(
                                    this, this->tft,                                                               // parentScreen, tft
                                    (String(title) + " - Manual Gain").c_str(),                                    // title
                                    "Set gain factor (0.1 - 10.0):",                                               // message
                                    tempGainValuePtr.get(),                                                        // valuePtr - raw pointer a shared_ptr-ből
                                    0.1f, 10.0f, 0.1f,                                                             // min, max, step
                                    [this, tempGainValuePtr](const std::variant<int, float, bool> &liveNewValue) { // ValueChangeCallback for live preview
                                        if (std::holds_alternative<float>(liveNewValue)) {
                                            float currentDialogVal = std::get<float>(liveNewValue);
                                            DEBUG("SetupScreen: Live gain preview: %.1f\n", currentDialogVal);
                                            // Optional: Apply live changes if needed
                                        }
                                    },
                                    [this, index, &currentConfig, tempGainValuePtr](UIDialogBase *sender, MessageDialog::DialogResult result) { // DialogCallback
                                        if (result == MessageDialog::DialogResult::Accepted) {
                                            // Use the tempGainValue which was modified by the ValueChangeDialog
                                            currentConfig = *tempGainValuePtr;
                                            config.checkSave();

                                            DEBUG("SetupScreen: Manual gain set to %.1f\n", *tempGainValuePtr);

                                            // Update UI in a simpler way - just mark for refresh
                                            populateMenuItems();
                                        }
                                        // ValueChangeDialog will close automatically on Accept/Cancel
                                    },
                                    Rect(-1, -1, 300, 0) // bounds - centered, auto height
                                );
                                this->showDialog(gainDialog);
                            } break;
                        }
                    },
                    false,                 // autoClose -  false, mert a gombok kezelése manuális
                    defaultSelection,      // defaultButtonIndex - aktuális beállítás kiemelve
                    false,                 // disableDefaultButton - maradjon aktív a default gomb
                    Rect(-1, -1, 340, 120) // bounds
                );

                // Note: Dialog callback is handled by individual button callbacks
                // No need for additional setDialogCallback as it conflicts with ValueChangeDialog's callback

                this->showDialog(fftDialog);
            } break;
            case ItemAction::CW_RECEIVER_OFFSET: {
                auto cwOffsetDialog = std::make_shared<ValueChangeDialog>(              //
                    this, this->tft,                                                    //
                    "CW Receiver Offset",                                               // title
                    "Set CW receiver offset (Hz):",                                     // message
                    reinterpret_cast<int *>(&config.data.cwReceiverOffsetHz),           // valuePtr (cast uint16_t* to int*)
                    CW_DECODER_MIN_FREQUENCY, CW_DECODER_MAX_FREQUENCY, 10,             // min, max, step
                    [this, index](const std::variant<int, float, bool> &liveNewValue) { // "Élő" ValueChangeCallback
                        if (std::holds_alternative<int>(liveNewValue)) {
                            int currentDialogVal = std::get<int>(liveNewValue);
                            config.data.cwReceiverOffsetHz = static_cast<uint16_t>(currentDialogVal);
                            config.checkSave();
                        }
                    },
                    // DialogCallback paraméter - egyszerűsítve a segédfüggvénnyel
                    createListUpdateCallback(index, []() { return String(config.data.cwReceiverOffsetHz) + " Hz"; }), //
                    Rect(-1, -1, 280, 0));

                this->showDialog(cwOffsetDialog);
            } break;
            case ItemAction::INFO: // Rendszer információ dialógus
            {
                auto systemInfoDialog = std::make_shared<SystemInfoDialog>(this, this->tft, Rect(-1, -1, 320, 240));
                this->showDialog(systemInfoDialog);
            } break;

            case ItemAction::FACTORY_RESET: {
                auto confirmDialog =
                    std::make_shared<MessageDialog>(this, this->tft, Rect(-1, -1, 300, 0), "Factory Reset", "Are you sure you want to reset all settings to default?",
                                                    MessageDialog::ButtonsType::YesNo, ColorScheme::defaultScheme(), true);
                confirmDialog->setDialogCallback([this, index](UIDialogBase *sender, MessageDialog::DialogResult result) {
                    if (result == MessageDialog::DialogResult::Accepted) { // Yes
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

            case ItemAction::NONE:
            default:
                // Nincs teendő, vagy hibaüzenet
                break;
        }
        // Alapértelmezés szerint false-t adunk vissza, mert vagy
        // - egyedi sort frissítettünk, vagy
        // - dialógust nyitottunk, ami később kezeli a frissítést.
        // Ha egy eset explicit teljes újra-rajzolást igényelne a UIScrollableListComponent-től,        // akkor az a case adna vissza true-t.
        return false;
    } // End of onItemClicked
};

#endif // __SETUP_SCREEN_H