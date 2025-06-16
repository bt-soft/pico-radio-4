/**
 * @file SetupSystemScreen.cpp
 * @brief Rendszer beállítások képernyő implementációja
 *
 * Ez a fájl tartalmazza a SetupSystemScreen osztály implementációját,
 * amely a rendszer és felhasználói felület beállításait kezeli.
 *
 * @author [Fejlesztő neve]
 * @date 2025.06.10
 * @version 1.0
 */

#include "SetupSystemScreen.h"
#include "MessageDialog.h"
#include "MultiButtonDialog.h"
#include "ValueChangeDialog.h"
#include "config.h"
#include "defines.h"
#include "pins.h"
#include "utils.h" // Utils::setTftBacklight függvényhez

/**
 * @brief SetupSystemScreen konstruktor
 *
 * @param tft TFT_eSPI referencia a kijelző kezeléséhez
 */
SetupSystemScreen::SetupSystemScreen(TFT_eSPI &tft) : SetupScreenBase(tft, "SETUP_SYSTEM") { layoutComponents(); }

/**
 * @brief Képernyő címének visszaadása
 *
 * @return A képernyő címe
 */
const char *SetupSystemScreen::getScreenTitle() const { return "System Settings"; }

/**
 * @brief Menüpontok feltöltése kijelző specifikus beállításokkal
 *
 * Ez a metódus feltölti a menüpontokat a kijelző aktuális
 * konfigurációs értékeivel.
 */
void SetupSystemScreen::populateMenuItems() {
    // Korábbi menüpontok törlése
    settingItems.clear();

    settingItems.push_back(SettingItem("Brightness", String(config.data.tftBackgroundBrightness), static_cast<int>(SystemItemAction::BRIGHTNESS)));
    settingItems.push_back(SettingItem("Screen Saver", String(config.data.screenSaverTimeoutMinutes) + " min", static_cast<int>(SystemItemAction::SAVER_TIMEOUT)));
    settingItems.push_back(SettingItem("Inactive Digit Light", String(config.data.tftDigitLigth ? "ON" : "OFF"), static_cast<int>(SystemItemAction::INACTIVE_DIGIT_LIGHT)));
    settingItems.push_back(SettingItem("Beeper", String(config.data.beeperEnabled ? "ON" : "OFF"), static_cast<int>(SystemItemAction::BEEPER_ENABLED)));
    settingItems.push_back(
        SettingItem("Rotary Acceleration", String(config.data.rotaryAcceleratonEnabled ? "ON" : "OFF"), static_cast<int>(SystemItemAction::ROTARY_ACCDELERATION)));

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
void SetupSystemScreen::handleItemAction(int index, int action) {
    SystemItemAction systemAction = static_cast<SystemItemAction>(action);

    switch (systemAction) {
        case SystemItemAction::BRIGHTNESS:
            handleBrightnessDialog(index);
            break;
        case SystemItemAction::SAVER_TIMEOUT:
            handleSaverTimeoutDialog(index);
            break;
        case SystemItemAction::INACTIVE_DIGIT_LIGHT:
            handleToggleItem(index, config.data.tftDigitLigth);
            break;
        case SystemItemAction::BEEPER_ENABLED:
            handleToggleItem(index, config.data.beeperEnabled);
            break;
        case SystemItemAction::ROTARY_ACCDELERATION:
            handleToggleItem(index, config.data.rotaryAcceleratonEnabled);
            break;
        case SystemItemAction::NONE:
        default:
            DEBUG("SetupSystemScreen: Unknown action: %d\n", action);
            break;
    }
}

/**
 * @brief TFT háttérvilágítás fényességének beállítása dialógussal
 *
 * @param index A menüpont indexe a lista frissítéséhez
 */
void SetupSystemScreen::handleBrightnessDialog(int index) {
    auto brightnessDialog = std::make_shared<ValueChangeDialog>(
        this, this->tft, "Brightness", "Adjust TFT Backlight:", &config.data.tftBackgroundBrightness, static_cast<uint8_t>(TFT_BACKGROUND_LED_MIN_BRIGHTNESS),
        static_cast<uint8_t>(TFT_BACKGROUND_LED_MAX_BRIGHTNESS), static_cast<uint8_t>(10),
        [this, index](const std::variant<int, float, bool> &liveNewValue) {
            if (std::holds_alternative<int>(liveNewValue)) {
                int currentDialogVal = std::get<int>(liveNewValue);
                config.data.tftBackgroundBrightness = static_cast<uint8_t>(currentDialogVal);
                Utils::setTftBacklight(config.data.tftBackgroundBrightness);
                DEBUG("SetupSystemScreen: Live brightness preview: %u\n", config.data.tftBackgroundBrightness);
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
void SetupSystemScreen::handleSaverTimeoutDialog(int index) {
    auto saverDialog = std::make_shared<ValueChangeDialog>(
        this, this->tft, "Screen Saver", "Timeout (minutes):", &config.data.screenSaverTimeoutMinutes, SCREEN_SAVER_TIMEOUT_MIN, SCREEN_SAVER_TIMEOUT_MAX, 1,
        [this, index](const std::variant<int, float, bool> &liveNewValue) {
            if (std::holds_alternative<int>(liveNewValue)) {
                int currentDialogVal = std::get<int>(liveNewValue);
                config.data.screenSaverTimeoutMinutes = static_cast<uint8_t>(currentDialogVal);
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
void SetupSystemScreen::handleToggleItem(int index, bool &configValue) {
    configValue = !configValue;

    if (index >= 0 && index < settingItems.size()) {
        settingItems[index].value = String(configValue ? "ON" : "OFF");
        updateListItem(index);
    }
}
