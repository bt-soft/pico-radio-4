/**
 * @file SetupDecodersScreen.cpp
 * @brief Dekóderek beállítások képernyő implementációja
 *
 * Ez a fájl tartalmazza a SetupDecodersScreen osztály implementációját,
 * amely a különböző dekóderek (CW, RTTY, stb.) beállításait kezeli.
 *
 * @author [Fejlesztő neve]
 * @date 2025.06.10
 * @version 1.0
 */

#include "SetupDecodersScreen.h"
#include "MessageDialog.h"
#include "MultiButtonDialog.h"
#include "ValueChangeDialog.h"
#include "config.h"
#include "defines.h"

/**
 * @brief SetupDecodersScreen konstruktor
 *
 * @param tft TFT_eSPI referencia a kijelző kezeléséhez
 */
SetupDecodersScreen::SetupDecodersScreen(TFT_eSPI &tft) : SetupScreenBase(tft, "SETUP_DECODERS") { layoutComponents(); }

/**
 * @brief Képernyő címének visszaadása
 *
 * @return A képernyő címe
 */
const char *SetupDecodersScreen::getScreenTitle() const { return "Decoder Settings"; }

/**
 * @brief Menüpontok feltöltése dekóder specifikus beállításokkal
 *
 * Ez a metódus feltölti a menüpontokat a dekóderek aktuális
 * konfigurációs értékeivel.
 */
void SetupDecodersScreen::populateMenuItems() {
    // Korábbi menüpontok törlése
    settingItems.clear();

    // Dekóder specifikus beállítások hozzáadása
    settingItems.push_back(SettingItem("CW Receiver Offset", String(config.data.cwReceiverOffsetHz) + " Hz", static_cast<int>(DecoderItemAction::CW_RECEIVER_OFFSET)));

    settingItems.push_back(SettingItem("RTTY Frequencies", String(round(config.data.rttyMarkFrequencyHz)) + "/" + String(round(config.data.rttyShiftHz)) + " Hz",
                                       static_cast<int>(DecoderItemAction::RTTY_FREQUENCIES)));

    // Példa további dekóder beállításokra (ha léteznek a config-ban)
    // settingItems.push_back(SettingItem("CW Speed",
    //     String(config.data.cwSpeed) + " WPM",
    //     static_cast<int>(DecoderItemAction::CW_SPEED)));

    // settingItems.push_back(SettingItem("CW Tone Frequency",
    //     String(config.data.cwToneFreq) + " Hz",
    //     static_cast<int>(DecoderItemAction::CW_TONE_FREQ)));

    // settingItems.push_back(SettingItem("RTTY Baudrate",
    //     String(config.data.rttyBaudrate) + " Bd",
    //     static_cast<int>(DecoderItemAction::RTTY_BAUDRATE)));

    // Lista komponens újrarajzolásának kérése, ha létezik
    if (menuList) {
        menuList->markForRedraw();
    }
}

/**
 * @brief Menüpont akció kezelése
 *
 * Ez a metódus kezeli a dekóder specifikus menüpontok kattintásait.
 *
 * @param index A menüpont indexe
 * @param action Az akció azonosító
 */
void SetupDecodersScreen::handleItemAction(int index, int action) {
    DecoderItemAction decoderAction = static_cast<DecoderItemAction>(action);

    switch (decoderAction) {
        case DecoderItemAction::CW_RECEIVER_OFFSET:
            handleCWOffsetDialog(index);
            break;
        case DecoderItemAction::RTTY_FREQUENCIES:
            handleRTTYFrequenciesDialog(index);
            break;
        // case DecoderItemAction::CW_SPEED:
        //     handleCWSpeedDialog(index);
        //     break;
        // case DecoderItemAction::CW_TONE_FREQ:
        //     handleCWToneFreqDialog(index);
        //     break;
        // case DecoderItemAction::RTTY_BAUDRATE:
        //     handleRTTYBaudrateDialog(index);
        //     break;
        case DecoderItemAction::NONE:
        default:
            DEBUG("SetupDecodersScreen: Unknown action: %d\n", action);
            break;
    }
}

/**
 * @brief CW vevő frekvencia eltolásának beállítása dialógussal
 *
 * @param index A menüpont indexe a lista frissítéséhez
 */
void SetupDecodersScreen::handleCWOffsetDialog(int index) {
    auto cwOffsetDialog = std::make_shared<ValueChangeDialog>(
        this, this->tft, "CW Receiver Offset", "Set CW receiver offset (Hz):", reinterpret_cast<int *>(&config.data.cwReceiverOffsetHz), CW_DECODER_MIN_FREQUENCY,
        CW_DECODER_MAX_FREQUENCY, 10,
        [this, index](const std::variant<int, float, bool> &liveNewValue) {
            if (std::holds_alternative<int>(liveNewValue)) {
                int currentDialogVal = std::get<int>(liveNewValue);
                config.data.cwReceiverOffsetHz = static_cast<uint16_t>(currentDialogVal);
                config.checkSave();
            }
        },
        [this, index](UIDialogBase *sender, MessageDialog::DialogResult dialogResult) {
            if (dialogResult == MessageDialog::DialogResult::Accepted) {
                settingItems[index].value = String(config.data.cwReceiverOffsetHz) + " Hz";
                updateListItem(index);
            }
        },
        Rect(-1, -1, 280, 0));
    this->showDialog(cwOffsetDialog);
}

/**
 * @brief RTTY frekvenciák beállítása dialógussal
 *
 * @param index A menüpont indexe a lista frissítéséhez
 */
void SetupDecodersScreen::handleRTTYFrequenciesDialog(int index) {
    const char *options[] = {"Mark", "Shift"};
    auto rttyDialog = std::make_shared<MultiButtonDialog>(
        this, this->tft, "RTTY Frequencies", "Select frequency to configure:", options, ARRAY_ITEM_COUNT(options),
        [this, index](int buttonIndex, const char *buttonLabel, MultiButtonDialog *dialog) {
            if (strcmp(buttonLabel, "Mark") == 0) {
                // RTTY Mark frekvencia beállítása
                auto markDialog = std::make_shared<ValueChangeDialog>(
                    this, this->tft, "RTTY Mark Frequency", "Hz (1000-3000):", &config.data.rttyMarkFrequencyHz, 1000.0f, 3000.0f, 5.0f,
                    [this, index](const std::variant<int, float, bool> &liveNewValue) {
                        if (std::holds_alternative<float>(liveNewValue)) {
                            float currentDialogVal = std::get<float>(liveNewValue);
                            config.data.rttyMarkFrequencyHz = currentDialogVal;
                            config.checkSave();
                        }
                    },
                    [this, index](UIDialogBase *sender, MessageDialog::DialogResult dialogResult) {
                        if (dialogResult == MessageDialog::DialogResult::Accepted) {
                            settingItems[index].value = String(round(config.data.rttyMarkFrequencyHz)) + "/" + String(round(config.data.rttyShiftHz)) + " Hz";
                            updateListItem(index);
                        }
                    },
                    Rect(-1, -1, 270, 0));
                this->showDialog(markDialog);

            } else if (strcmp(buttonLabel, "Shift") == 0) {
                // RTTY Shift frekvencia beállítása
                auto shiftDialog = std::make_shared<ValueChangeDialog>(
                    this, this->tft, "RTTY Shift", "Hz (50-1000):", &config.data.rttyShiftHz, 50.0f, 1000.0f, 5.0f,
                    [this, index](const std::variant<int, float, bool> &liveNewValue) {
                        if (std::holds_alternative<float>(liveNewValue)) {
                            float currentDialogVal = std::get<float>(liveNewValue);
                            config.data.rttyShiftHz = currentDialogVal;
                            config.checkSave();
                        }
                    },
                    [this, index](UIDialogBase *sender, MessageDialog::DialogResult dialogResult) {
                        if (dialogResult == MessageDialog::DialogResult::Accepted) {
                            settingItems[index].value = String(round(config.data.rttyMarkFrequencyHz)) + "/" + String(round(config.data.rttyShiftHz)) + " Hz";
                            updateListItem(index);
                        }
                    },
                    Rect(-1, -1, 270, 0));
                this->showDialog(shiftDialog);
            }
        },
        true, -1, true, Rect(-1, -1, 300, 120));
    this->showDialog(rttyDialog);
}
