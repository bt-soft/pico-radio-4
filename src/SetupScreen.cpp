#include "SetupScreen.h"
#include "MultiButtonDialog.h"
#include "ValueChangeDialog.h"
#include "SystemInfoDialog.h"
#include "MessageDialog.h"
#include "config.h"
#include "defines.h"
#include "pins.h"

SetupScreen::SetupScreen(TFT_eSPI &tft) : UIScreen(tft, SCREEN_NAME_SETUP) {
    populateMenuItems();

    const int16_t screenW = tft.width();
    const int16_t screenH = tft.height();
    const int16_t margin = 5;
    const int16_t buttonHeight = UIButton::DEFAULT_BUTTON_HEIGHT;
    const int16_t listTopMargin = 30;
    const int16_t listBottomPadding = buttonHeight + margin * 2;

    // Lista komponens létrehozása és hozzáadása
    Rect listBounds(margin, listTopMargin, screenW - (2 * margin), screenH - listTopMargin - listBottomPadding);
    menuList = std::make_shared<UIScrollableListComponent>(tft, listBounds, this);
    addChild(menuList);

    // Exit gomb létrehozása
    constexpr int8_t exitButtonWidth = UIButton::DEFAULT_BUTTON_WIDTH;
    Rect exitButtonBounds(screenW - exitButtonWidth - margin, screenH - buttonHeight - margin, exitButtonWidth, buttonHeight);
    exitButton = std::make_shared<UIButton>(tft, 0, exitButtonBounds, "Exit", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, 
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked && getManager()) {
                getManager()->goBack();
            }
        });
    addChild(exitButton);
}

void SetupScreen::activate() {
    DEBUG("SetupScreen activated.\n");
    populateMenuItems();
    markForRedraw();
}

void SetupScreen::drawContent() {
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_COLOR_BACKGROUND);
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextSize(1);
    tft.drawString("Setup Menu", tft.width() / 2, 10);
}

int SetupScreen::getItemCount() const {
    return settingItems.size();
}

String SetupScreen::getItemLabelAt(int index) const {
    if (index >= 0 && index < settingItems.size()) {
        return String(settingItems[index].label);
    }
    return "";
}

String SetupScreen::getItemValueAt(int index) const {
    if (index >= 0 && index < settingItems.size()) {
        return settingItems[index].value;
    }
    return "";
}

void SetupScreen::populateMenuItems() {
    settingItems.clear();

    // Beállítások hozzáadása a menühöz
    settingItems.push_back({"Brightness", String(config.data.tftBackgroundBrightness), ItemAction::BRIGHTNESS});
    settingItems.push_back({"Squelch Basis", String(config.data.squelchUsesRSSI ? "RSSI" : "SNR"), ItemAction::SQUELCH_BASIS});
    settingItems.push_back({"Screen Saver", String(config.data.screenSaverTimeoutMinutes) + " min", ItemAction::SAVER_TIMEOUT});
    settingItems.push_back({"Inactive Digit Light", String(config.data.tftDigitLigth ? "ON" : "OFF"), ItemAction::INACTIVE_DIGIT_LIGHT});
    settingItems.push_back({"Beeper", String(config.data.beeperEnabled ? "ON" : "OFF"), ItemAction::BEEPER_ENABLED});
    settingItems.push_back({"FFT Config AM", decodeFFTConfig(config.data.miniAudioFftConfigAm), ItemAction::FFT_CONFIG_AM});
    settingItems.push_back({"FFT Config FM", decodeFFTConfig(config.data.miniAudioFftConfigFm), ItemAction::FFT_CONFIG_FM});
    settingItems.push_back({"CW Receiver Offset", String(config.data.cwReceiverOffsetHz) + " Hz", ItemAction::CW_RECEIVER_OFFSET});
    settingItems.push_back({"RTTY Frequencies", String(round(config.data.rttyMarkFrequencyHz)) + "/" + String(round(config.data.rttyShiftHz)) + " Hz", ItemAction::RTTY_FREQUENCIES});
    settingItems.push_back({"System Information", "", ItemAction::INFO});
    settingItems.push_back({"Factory Reset", "", ItemAction::FACTORY_RESET});

    if (menuList) {
        menuList->markForRedraw();
    }
}

void SetupScreen::updateListItem(int index) {
    if (index >= 0 && index < settingItems.size() && menuList) {
        menuList->refreshItemDisplay(index);
    }
}

String SetupScreen::decodeFFTConfig(float value) {
    if (value == -1.0f)
        return "Disabled";
    else if (value == 0.0f)
        return "Auto Gain";
    else
        return "Manual: " + String(value, 1) + "x";
}

bool SetupScreen::onItemClicked(int index) {
    if (index < 0 || index >= settingItems.size())
        return false;

    const SettingItem &item = settingItems[index];
    DEBUG("SetupScreen: Item %d ('%s':'%s') clicked, action: %d\n", index, item.label, item.value.c_str(), static_cast<int>(item.action));

    switch (item.action) {
        case ItemAction::BRIGHTNESS:
            handleBrightnessDialog(index);
            break;
        case ItemAction::SQUELCH_BASIS:
            handleSquelchBasisDialog(index);
            break;
        case ItemAction::SAVER_TIMEOUT:
            handleSaverTimeoutDialog(index);
            break;
        case ItemAction::INACTIVE_DIGIT_LIGHT:
            handleToggleItem(index, config.data.tftDigitLigth);
            break;
        case ItemAction::BEEPER_ENABLED:
            handleToggleItem(index, config.data.beeperEnabled);
            break;
        case ItemAction::FFT_CONFIG_AM:
            handleFFTConfigDialog(index, true);
            break;
        case ItemAction::FFT_CONFIG_FM:
            handleFFTConfigDialog(index, false);
            break;
        case ItemAction::CW_RECEIVER_OFFSET:
            handleCWOffsetDialog(index);
            break;
        case ItemAction::RTTY_FREQUENCIES:
            handleRTTYFrequenciesDialog(index);
            break;
        case ItemAction::INFO:
            handleSystemInfoDialog();
            break;
        case ItemAction::FACTORY_RESET:
            handleFactoryResetDialog();
            break;
        case ItemAction::NONE:
        default:
            break;
    }
    return false;
}

void SetupScreen::handleBrightnessDialog(int index) {
    auto brightnessDialog = std::make_shared<ValueChangeDialog>(
        this, this->tft,
        "Brightness",
        "Adjust TFT Backlight:",
        &config.data.tftBackgroundBrightness,
        (uint8_t)TFT_BACKGROUND_LED_MIN_BRIGHTNESS,
        (uint8_t)TFT_BACKGROUND_LED_MAX_BRIGHTNESS,
        (uint8_t)10,
        [this, index](const std::variant<int, float, bool> &liveNewValue) {
            if (std::holds_alternative<int>(liveNewValue)) {
                int currentDialogVal = std::get<int>(liveNewValue);
                config.data.tftBackgroundBrightness = static_cast<uint8_t>(currentDialogVal);
                analogWrite(PIN_TFT_BACKGROUND_LED, config.data.tftBackgroundBrightness);
                DEBUG("SetupScreen: Live brightness preview: %u\n", config.data.tftBackgroundBrightness);
            }
        },
        [this, index](UIDialogBase *sender, MessageDialog::DialogResult dialogResult) {
            if (dialogResult == MessageDialog::DialogResult::Accepted) {
                settingItems[index].value = String(config.data.tftBackgroundBrightness);
                updateListItem(index);
            }
        },
        Rect(-1, -1, 280, 0)
    );
    this->showDialog(brightnessDialog);
}

void SetupScreen::handleSquelchBasisDialog(int index) {
    const char *options[] = {"RSSI", "SNR"};
    int currentSelection = config.data.squelchUsesRSSI ? 0 : 1;
    
    auto basisDialog = std::make_shared<MultiButtonDialog>(
        this, this->tft,
        "Squelch Basis",
        "Select squelch basis:",
        options, ARRAY_ITEM_COUNT(options),
        [this, index](int buttonIndex, const char *buttonLabel, MultiButtonDialog *dialog) {
            bool newSquelchUsesRSSI = (buttonIndex == 0);
            if (config.data.squelchUsesRSSI != newSquelchUsesRSSI) {
                config.data.squelchUsesRSSI = newSquelchUsesRSSI;
                config.checkSave();
            }
            settingItems[index].value = String(config.data.squelchUsesRSSI ? "RSSI" : "SNR");
            updateListItem(index);
        },
        true,
        currentSelection,
        true,
        Rect(-1, -1, 250, 120)
    );
    this->showDialog(basisDialog);
}

void SetupScreen::handleSaverTimeoutDialog(int index) {
    auto saverDialog = std::make_shared<ValueChangeDialog>(
        this, this->tft,
        "Screen Saver",
        "Timeout (minutes):",
        &config.data.screenSaverTimeoutMinutes,
        SCREEN_SAVER_TIMEOUT_MIN, SCREEN_SAVER_TIMEOUT_MAX, 1,
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
        Rect(-1, -1, 280, 0)
    );
    this->showDialog(saverDialog);
}

void SetupScreen::handleToggleItem(int index, bool &configValue) {
    configValue = !configValue;
    config.checkSave();
    
    // Update UI value based on the specific setting
    if (index >= 0 && index < settingItems.size()) {
        settingItems[index].value = String(configValue ? "ON" : "OFF");
        updateListItem(index);
    }
}

void SetupScreen::handleFFTConfigDialog(int index, bool isAM) {
    float &currentConfig = isAM ? config.data.miniAudioFftConfigAm : config.data.miniAudioFftConfigFm;
    const char *title = isAM ? "FFT Config AM" : "FFT Config FM";

    // Determine current setting for default button highlighting
    int defaultSelection = 0; // Disabled
    if (currentConfig == 0.0f) {
        defaultSelection = 1; // Auto G
    } else if (currentConfig > 0.0f) {
        defaultSelection = 2; // Manual G
    }
    
    const char *options[] = {"Disabled", "Auto G", "Manual G"};

    auto fftDialog = std::make_shared<MultiButtonDialog>(
        this, this->tft,
        title,
        "Select FFT gain mode:",
        options, ARRAY_ITEM_COUNT(options),
        [this, index, isAM, &currentConfig, title](int buttonIndex, const char *buttonLabel, MultiButtonDialog *dialog) {
            DEBUG("SetupScreen: FFT Config %s button %d ('%s') clicked\n", isAM ? "AM" : "FM", buttonIndex, buttonLabel);
            switch (buttonIndex) {
                case 0: // Disabled
                    currentConfig = -1.0f;
                    config.checkSave();
                    settingItems[index].value = "Disabled";
                    updateListItem(index);
                    dialog->close(UIDialogBase::DialogResult::Accepted);
                    break;

                case 1: // Auto G
                    currentConfig = 0.0f;
                    config.checkSave();
                    settingItems[index].value = "Auto Gain";
                    updateListItem(index);
                    dialog->close(UIDialogBase::DialogResult::Accepted);
                    break;
                    
                case 2: // Manual G - open nested ValueChangeDialog
                {
                    dialog->close(UIDialogBase::DialogResult::Accepted);

                    auto tempGainValuePtr = std::make_shared<float>((currentConfig > 0.0f) ? currentConfig : 1.0f);
                    DEBUG("SetupScreen: Opening manual gain dialog with initial value: %.1f\n", *tempGainValuePtr);

                    auto gainDialog = std::make_shared<ValueChangeDialog>(
                        this, this->tft,
                        (String(title) + " - Manual Gain").c_str(),
                        "Set gain factor (0.1 - 10.0):",
                        tempGainValuePtr.get(),
                        0.1f, 10.0f, 0.1f,
                        [this, tempGainValuePtr](const std::variant<int, float, bool> &liveNewValue) {
                            if (std::holds_alternative<float>(liveNewValue)) {
                                float currentDialogVal = std::get<float>(liveNewValue);
                                DEBUG("SetupScreen: Live gain preview: %.1f\n", currentDialogVal);
                            }
                        },
                        [this, index, &currentConfig, tempGainValuePtr](UIDialogBase *sender, MessageDialog::DialogResult result) {
                            if (result == MessageDialog::DialogResult::Accepted) {
                                currentConfig = *tempGainValuePtr;
                                config.checkSave();
                                DEBUG("SetupScreen: Manual gain set to %.1f\n", *tempGainValuePtr);
                                populateMenuItems();
                            }
                        },
                        Rect(-1, -1, 300, 0)
                    );
                    this->showDialog(gainDialog);
                } break;
            }
        },
        false,
        defaultSelection,
        false,
        Rect(-1, -1, 340, 120)
    );
    this->showDialog(fftDialog);
}

void SetupScreen::handleCWOffsetDialog(int index) {
    auto cwOffsetDialog = std::make_shared<ValueChangeDialog>(
        this, this->tft,
        "CW Receiver Offset",
        "Set CW receiver offset (Hz):",
        reinterpret_cast<int *>(&config.data.cwReceiverOffsetHz),
        CW_DECODER_MIN_FREQUENCY, CW_DECODER_MAX_FREQUENCY, 10,
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
        Rect(-1, -1, 280, 0)
    );
    this->showDialog(cwOffsetDialog);
}

void SetupScreen::handleRTTYFrequenciesDialog(int index) {
    // RTTY frequencies - ezt még nem implementáltuk a részletekben
    // Egyelőre csak egy placeholder MessageDialog
    auto rttyDialog = std::make_shared<MessageDialog>(
        this, this->tft,
        Rect(-1, -1, 300, 150),
        "RTTY Frequencies",
        "RTTY frequency configuration\nnot yet implemented.",
        MessageDialog::ButtonsType::Ok
    );
    this->showDialog(rttyDialog);
}

void SetupScreen::handleSystemInfoDialog() {
    auto systemInfoDialog = std::make_shared<SystemInfoDialog>(this, this->tft, Rect(-1, -1, 320, 240));
    this->showDialog(systemInfoDialog);
}

void SetupScreen::handleFactoryResetDialog() {
    auto confirmDialog = std::make_shared<MessageDialog>(
        this, this->tft,
        Rect(-1, -1, 300, 0),
        "Factory Reset",
        "Are you sure you want to reset all settings to default?",
        MessageDialog::ButtonsType::YesNo,
        ColorScheme::defaultScheme(),
        true
    );
    
    confirmDialog->setDialogCallback([this](UIDialogBase *sender, MessageDialog::DialogResult result) {
        if (result == MessageDialog::DialogResult::Accepted) {
            DEBUG("SetupScreen: Performing factory reset.\n");
            config.loadDefaults();
            config.forceSave();
            populateMenuItems();
        }
    });
    
    this->showDialog(confirmDialog);
}
