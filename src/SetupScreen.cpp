/**
 * @file SetupScreen.cpp
 * @brief Főbeállítások képernyő implementációja
 *
 * Ez a fájl tartalmazza a SetupScreen osztály implementációját,
 * amely a fő setup menüt kezeli és almenükre navigál.
 *
 * A SetupScreen a következő almenüket támogatja:
 * - Display Settings: kijelző és UI beállítások
 * - Si4735 Settings: rádió chip beállítások
 * - Decoder Settings: dekóderek beállításai
 * - System Information: rendszer információk megjelenítése
 * - Factory Reset: gyári beállítások visszaállítása
 *
 * @author [Fejlesztő neve]
 * @date 2025.06.10
 * @version 2.0 (Többszintű setup rendszer)
 */

#include "SetupScreen.h"
#include "MessageDialog.h"
#include "SystemInfoDialog.h"
#include "config.h"
#include "defines.h"

/**
 * @brief SetupScreen konstruktor
 *
 * @param tft TFT_eSPI referencia a kijelző kezeléséhez
 */
SetupScreen::SetupScreen(TFT_eSPI &tft) : SetupScreenBase(tft, SCREEN_NAME_SETUP) { layoutComponents(); }

/**
 * @brief Képernyő címének visszaadása
 *
 * @return A képernyő címe
 */
const char *SetupScreen::getScreenTitle() const { return "Setup Menu"; }

/**
 * @brief Menüpontok feltöltése fő setup menü elemeivel
 *
 * Ez a metódus feltölti a menüpontokat almenük hivatkozásaival
 * és egyéb fő setup funkciókkal.
 */
void SetupScreen::populateMenuItems() {
    // Korábbi menüpontok törlése
    settingItems.clear();

    // Fő setup menü elemek hozzáadása
    settingItems.push_back(SettingItem("System Settings", "", static_cast<int>(MainItemAction::DISPLAY_SETTINGS), true, "SETUP_SYSTEM"));

    settingItems.push_back(SettingItem("Si4735 Settings", "", static_cast<int>(MainItemAction::SI4735_SETTINGS), true, "SETUP_SI4735"));

    settingItems.push_back(SettingItem("Decoder Settings", "", static_cast<int>(MainItemAction::DECODER_SETTINGS), true, "SETUP_DECODERS"));

    settingItems.push_back(SettingItem("System Information", "", static_cast<int>(MainItemAction::INFO)));

    settingItems.push_back(SettingItem("Factory Reset", "", static_cast<int>(MainItemAction::FACTORY_RESET)));

    // Lista komponens újrarajzolásának kérése, ha létezik
    if (menuList) {
        menuList->markForRedraw();
    }
}

/**
 * @brief Menüpont akció kezelése
 *
 * Ez a metódus kezeli a fő setup menü kattintásait.
 * Almenük esetén a navigáció a SetupScreenBase-ben történik.
 *
 * @param index A menüpont indexe
 * @param action Az akció azonosító
 */
void SetupScreen::handleItemAction(int index, int action) {
    MainItemAction mainAction = static_cast<MainItemAction>(action);

    switch (mainAction) {
        case MainItemAction::DISPLAY_SETTINGS:
        case MainItemAction::SI4735_SETTINGS:
        case MainItemAction::DECODER_SETTINGS:
            // Ezeket a SetupScreenBase::onItemClicked kezeli (almenü navigáció)
            break;
        case MainItemAction::INFO:
            handleSystemInfoDialog();
            break;
        case MainItemAction::FACTORY_RESET:
            handleFactoryResetDialog();
            break;
        case MainItemAction::NONE:
        default:
            DEBUG("SetupScreen: Unknown action: %d\n", action);
            break;
    }
}

/**
 * @brief Rendszer információ dialógus megjelenítése
 */
void SetupScreen::handleSystemInfoDialog() {
    auto systemInfoDialog = std::make_shared<SystemInfoDialog>(                    //
        this,                                                                      //
        this->tft,                                                                 //
        Rect(-1, -1, UIComponent::SCREEN_W * 3 / 4, UIComponent::SCREEN_H * 3 / 4) //
    );
    this->showDialog(systemInfoDialog);
}

/**
 * @brief Gyári beállítások visszaállítása dialógussal
 */
void SetupScreen::handleFactoryResetDialog() {
    auto confirmDialog = std::make_shared<MessageDialog>(            //
        this,                                                        //
        this->tft,                                                   //
        Rect(-1, -1, UIComponent::SCREEN_W * 3 / 4, 0),              //
        "Factory Reset",                                             //
        "Reset all settings to defaults?\n\nThis cannot be undone!", //
        MessageDialog::ButtonsType::YesNo);

    confirmDialog->setDialogCallback([this](UIDialogBase *sender, MessageDialog::DialogResult result) {
        if (result == MessageDialog::DialogResult::Accepted) {
            config.loadDefaults();
            config.forceSave();
            populateMenuItems(); // Frissítés nem szükséges itt, mert ez a főmenü
        }
    });

    this->showDialog(confirmDialog);
}
