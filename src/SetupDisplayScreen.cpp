/**
 * @file SetupDisplayScreen.cpp
 * @brief Kijelző beállítások képernyő implementációja
 *
 * Ez a fájl tartalmazza a SetupDisplayScreen osztály implementációját,
 * amely a kijelző és felhasználói felület beállításait kezeli.
 *
 * @author [Fejlesztő neve]
 * @date 2025.06.10
 * @version 1.0
 */

#include "SetupDisplayScreen.h"
#include "MessageDialog.h"
#include "MultiButtonDialog.h"
#include "ValueChangeDialog.h"
#include "config.h"
#include "defines.h"
#include "pins.h"

/**
 * @brief SetupDisplayScreen konstruktor
 *
 * @param tft TFT_eSPI referencia a kijelző kezeléséhez
 */
SetupDisplayScreen::SetupDisplayScreen(TFT_eSPI &tft) : SetupScreenBase(tft, "SETUP_DISPLAY") { layoutComponents(); }

/**
 * @brief Képernyő címének visszaadása
 *
 * @return A képernyő címe
 */
const char *SetupDisplayScreen::getScreenTitle() const { return "Display Settings"; }

/**
 * @brief Menüpontok feltöltése kijelző specifikus beállításokkal
 *
 * Ez a metódus feltölti a menüpontokat a kijelző aktuális
 * konfigurációs értékeivel.
 */
void SetupDisplayScreen::populateMenuItems() {
    // Korábbi menüpontok törlése
    settingItems.clear();

    // Kijelző specifikus beállítások hozzáadása
    settingItems.push_back(SettingItem("Brightness", String(config.data.tftBackgroundBrightness), static_cast<int>(DisplayItemAction::BRIGHTNESS)));

    settingItems.push_back(SettingItem("Screen Saver", String(config.data.screenSaverTimeoutMinutes) + " min", static_cast<int>(DisplayItemAction::SAVER_TIMEOUT)));

    settingItems.push_back(SettingItem("Inactive Digit Light", String(config.data.tftDigitLigth ? "ON" : "OFF"), static_cast<int>(DisplayItemAction::INACTIVE_DIGIT_LIGHT)));

    settingItems.push_back(SettingItem("Beeper", String(config.data.beeperEnabled ? "ON" : "OFF"), static_cast<int>(DisplayItemAction::BEEPER_ENABLED)));

    // Példa további kijelző beállításokra (ha léteznek a config-ban)
    // settingItems.push_back(SettingItem("Contrast",
    //     String(config.data.contrast),
    //     static_cast<int>(DisplayItemAction::CONTRAST)));

    // settingItems.push_back(SettingItem("Color Scheme",
    //     String(config.data.colorScheme),
    //     static_cast<int>(DisplayItemAction::COLOR_SCHEME)));

    // Lista komponens újrarajzolásának kérése, ha létezik
    if (menuList) {
        menuList->markForRedraw();
    }
}

/**
 * @brief Menüpont akció kezelése
 *
 * Ez a metódus kezeli a kijelző specifikus menüpontok kattintásait.
 *
 * @param index A menüpont indexe
 * @param action Az akció azonosító
 */
void SetupDisplayScreen::handleItemAction(int index, int action) {
    DisplayItemAction displayAction = static_cast<DisplayItemAction>(action);

    switch (displayAction) {
        case DisplayItemAction::BRIGHTNESS:
            handleBrightnessDialog(index);
            break;
        case DisplayItemAction::SAVER_TIMEOUT:
            handleSaverTimeoutDialog(index);
            break;
        case DisplayItemAction::INACTIVE_DIGIT_LIGHT:
            handleToggleItem(index, config.data.tftDigitLigth);
            break;
        case DisplayItemAction::BEEPER_ENABLED:
            handleToggleItem(index, config.data.beeperEnabled);
            break;
        // case DisplayItemAction::CONTRAST:
        //     handleContrastDialog(index);
        //     break;
        // case DisplayItemAction::COLOR_SCHEME:
        //     handleColorSchemeDialog(index);
        //     break;
        case DisplayItemAction::NONE:
        default:
            DEBUG("SetupDisplayScreen: Unknown action: %d\n", action);
            break;
    }
}

/**
 * @brief TFT háttérvilágítás fényességének beállítása dialógussal
 *
 * @param index A menüpont indexe a lista frissítéséhez
 */
void SetupDisplayScreen::handleBrightnessDialog(int index) {
    auto brightnessDialog = std::make_shared<ValueChangeDialog>(
        this, this->tft, "Brightness", "Adjust TFT Backlight:", &config.data.tftBackgroundBrightness, static_cast<uint8_t>(TFT_BACKGROUND_LED_MIN_BRIGHTNESS),
        static_cast<uint8_t>(TFT_BACKGROUND_LED_MAX_BRIGHTNESS), static_cast<uint8_t>(10),
        [this, index](const std::variant<int, float, bool> &liveNewValue) {
            if (std::holds_alternative<int>(liveNewValue)) {
                int currentDialogVal = std::get<int>(liveNewValue);
                config.data.tftBackgroundBrightness = static_cast<uint8_t>(currentDialogVal);
                analogWrite(PIN_TFT_BACKGROUND_LED, config.data.tftBackgroundBrightness);
                DEBUG("SetupDisplayScreen: Live brightness preview: %u\n", config.data.tftBackgroundBrightness);
            }
        },
        [this, index](UIDialogBase *sender, MessageDialog::DialogResult dialogResult) {
            if (dialogResult == MessageDialog::DialogResult::Accepted) {
                settingItems[index].value = String(config.data.tftBackgroundBrightness);
                updateListItem(index);
            }
        },
        Rect(-1, -1, 280, 0));
    this->showDialog(brightnessDialog);
}

/**
 * @brief Képernyővédő időtúllépésének beállítása dialógussal
 *
 * @param index A menüpont indexe a lista frissítéséhez
 */
void SetupDisplayScreen::handleSaverTimeoutDialog(int index) {
    auto saverDialog = std::make_shared<ValueChangeDialog>(
        this, this->tft, "Screen Saver", "Timeout (minutes):", &config.data.screenSaverTimeoutMinutes, SCREEN_SAVER_TIMEOUT_MIN, SCREEN_SAVER_TIMEOUT_MAX, 1,
        [this, index](const std::variant<int, float, bool> &liveNewValue) {
            if (std::holds_alternative<int>(liveNewValue)) {
                int currentDialogVal = std::get<int>(liveNewValue);
                config.data.screenSaverTimeoutMinutes = static_cast<uint8_t>(currentDialogVal);
                config.checkSave();
            }
        },
        [this, index](UIDialogBase *sender, MessageDialog::DialogResult dialogResult) {
            if (dialogResult == MessageDialog::DialogResult::Accepted) {
                settingItems[index].value = String(config.data.screenSaverTimeoutMinutes) + " min";
                updateListItem(index);
            }
        },
        Rect(-1, -1, 280, 0));
    this->showDialog(saverDialog);
}

/**
 * @brief Boolean beállítások váltása
 *
 * @param index A menüpont indexe
 * @param configValue Referencia a módosítandó boolean értékre
 */
void SetupDisplayScreen::handleToggleItem(int index, bool &configValue) {
    configValue = !configValue;
    config.checkSave();

    if (index >= 0 && index < settingItems.size()) {
        settingItems[index].value = String(configValue ? "ON" : "OFF");
        updateListItem(index);
    }
}
