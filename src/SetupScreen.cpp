/**
 * @file SetupScreen.cpp
 * @brief Raspberry Pi Pico rádió projekt beállítások képernyő implementációja
 *
 * Ez a fájl tartalmazza a SetupScreen osztály teljes implementációját,
 * amely a rádió különböző beállításainak konfigurálásához szükséges
 * felhasználói felületet biztosítja.
 *
 * A SetupScreen a következő funkciókat támogatja:
 * - TFT háttérvilágítás fényességének beállítása valós idejű előnézettel
 * - Zajzár (squelch) alapjának kiválasztása (RSSI/SNR)
 * - Képernyővédő időtúllépésének beállítása
 * - Boolean beállítások váltása (inaktív számjegyek, hangjelzések)
 * - FFT konfigurációk AM és FM módokhoz (letiltva/auto/manuális)
 * - CW vevő frekvencia eltolásának beállítása
 * - RTTY frekvenciák konfigurációja (placeholder)
 * - Rendszer információk megjelenítése
 * - Gyári beállítások visszaállítása megerősítéssel
 *
 * A képernyő egy görgethető listát használ a menüpontok megjelenítésére,
 * és különböző típusú dialógusokat (ValueChangeDialog, MultiButtonDialog,
 * MessageDialog) a beállítások módosításához.
 *
 * @author [Fejlesztő neve]
 * @date 2025.06.09
 * @version 1.0
 */

#include "SetupScreen.h"
#include "MessageDialog.h"
#include "MultiButtonDialog.h"
#include "SystemInfoDialog.h"
#include "ValueChangeDialog.h"
#include "config.h"
#include "defines.h"
#include "pins.h"

/**
 * @brief SetupScreen konstruktor
 *
 * Inicializálja a beállítások képernyőt a következő komponensekkel:
 * - Görgethető lista a beállítási menüpontokhoz
 * - Exit gomb a képernyő elhagyásához
 * - Menüpontok feltöltése az aktuális konfigurációs értékekkel
 *
 * @param tft TFT_eSPI referencia a kijelző kezeléséhez
 */
SetupScreen::SetupScreen(TFT_eSPI &tft) : UIScreen(tft, SCREEN_NAME_SETUP) {
    // Menüpontok feltöltése az aktuális konfigurációs értékekkel
    populateMenuItems();

    // Képernyő dimenzióinak és margóinak meghatározása
    const int16_t screenW = tft.width();
    const int16_t screenH = tft.height();
    const int16_t margin = 5;
    const int16_t buttonHeight = UIButton::DEFAULT_BUTTON_HEIGHT;
    const int16_t listTopMargin = 30;                            // Hely a címnek
    const int16_t listBottomPadding = buttonHeight + margin * 2; // Hely az Exit gombnak

    // Görgethető lista komponens létrehozása és hozzáadása a gyermek komponensekhez
    Rect listBounds(margin, listTopMargin, screenW - (2 * margin), screenH - listTopMargin - listBottomPadding);
    menuList = std::make_shared<UIScrollableListComponent>(tft, listBounds, this);
    addChild(menuList);

    // Exit gomb létrehozása a képernyő jobb alsó sarkában
    constexpr int8_t exitButtonWidth = UIButton::DEFAULT_BUTTON_WIDTH;
    Rect exitButtonBounds(screenW - exitButtonWidth - margin, screenH - buttonHeight - margin, exitButtonWidth, buttonHeight);
    exitButton =
        std::make_shared<UIButton>(tft, 0, exitButtonBounds, "Exit", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) {
            // Lambda callback: Exit gomb megnyomásakor visszatérés az előző képernyőre
            if (event.state == UIButton::EventButtonState::Clicked && getManager()) {
                getManager()->goBack();
            }
        });
    addChild(exitButton);
}

/**
 * @brief Képernyő aktiválása
 *
 * Ez a metódus akkor hívódik meg, amikor a SetupScreen aktívvá válik.
 * Frissíti a menüpontokat az aktuális konfigurációs értékekkel és
 * megjelöli a képernyőt újrarajzolásra.
 */
void SetupScreen::activate() {
    DEBUG("SetupScreen activated.\n");
    // Menüpontok újrafeltöltése az esetlegesen megváltozott konfigurációs értékekkel
    populateMenuItems();
    // Képernyő megjelölése újrarajzolásra
    markForRedraw();
}

/**
 * @brief Képernyő tartalmának kirajzolása
 *
 * Kirajzolja a "Setup Menu" címet a képernyő tetején középre igazítva.
 * A szöveg formázása: fehér szín, FreeSansBold9pt7b betűtípus.
 */
void SetupScreen::drawContent() {
    // Szöveg pozicionálása: középre igazítás, felső széle
    tft.setTextDatum(TC_DATUM);
    // Szövegszín beállítása: fehér előtér, háttérszín háttér
    tft.setTextColor(TFT_WHITE, TFT_COLOR_BACKGROUND);
    // Betűtípus és méret beállítása
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextSize(1);
    // "Setup Menu" cím kirajzolása a képernyő tetején középen
    tft.drawString("Setup Menu", tft.width() / 2, 10);
}

/**
 * @brief Menüpontok számának lekérdezése (IScrollableListDataSource interfész)
 *
 * @return A beállítási menüpontok száma
 */
int SetupScreen::getItemCount() const { return settingItems.size(); }

/**
 * @brief Menüpont címkéjének lekérdezése index alapján (IScrollableListDataSource interfész)
 *
 * @param index A menüpont indexe (0-tól kezdődik)
 * @return A menüpont címkéje (pl. "Brightness", "Volume", stb.) vagy üres string érvénytelen index esetén
 */
String SetupScreen::getItemLabelAt(int index) const {
    if (index >= 0 && index < settingItems.size()) {
        return String(settingItems[index].label);
    }
    return "";
}

/**
 * @brief Menüpont értékének lekérdezése index alapján (IScrollableListDataSource interfész)
 *
 * @param index A menüpont indexe (0-tól kezdődik)
 * @return A menüpont aktuális értéke (pl. "255", "ON", "50 Hz", stb.) vagy üres string érvénytelen index esetén
 */
String SetupScreen::getItemValueAt(int index) const {
    if (index >= 0 && index < settingItems.size()) {
        return settingItems[index].value;
    }
    return "";
}

/**
 * @brief Menüpontok feltöltése az aktuális konfigurációs értékekkel
 *
 * Ez a metódus törli a korábbi menüpontokat és újra feltölti őket
 * az aktuális konfigurációs beállításokkal. Minden menüponthoz
 * társítva van egy ItemAction, amely meghatározza, mi történjen
 * a menüpont kiválasztásakor.
 *
 * Beállítások:
 * - Brightness: TFT háttérvilágítás fényessége (0-255)
 * - Squelch Basis: Zajzár alapja (RSSI vagy SNR)
 * - Screen Saver: Képernyővédő időtúllépése percekben
 * - Inactive Digit Light: Inaktív számjegyek világítása (BE/KI)
 * - Beeper: Hangjelzések engedélyezése (BE/KI)
 * - FFT Config AM/FM: FFT konfiguráció AM és FM módokhoz
 * - CW Receiver Offset: CW vevő frekvencia eltolása Hz-ben
 * - RTTY Frequencies: RTTY frekvenciák beállítása
 * - System Information: Rendszer információk megjelenítése
 * - Factory Reset: Gyári beállítások visszaállítása
 */
void SetupScreen::populateMenuItems() {
    // Korábbi menüpontok törlése
    settingItems.clear();

    // Beállítások hozzáadása a menühöz az aktuális konfigurációs értékekkel
    settingItems.push_back({"Brightness", String(config.data.tftBackgroundBrightness), ItemAction::BRIGHTNESS});
    settingItems.push_back({"Squelch Basis", String(config.data.squelchUsesRSSI ? "RSSI" : "SNR"), ItemAction::SQUELCH_BASIS});
    settingItems.push_back({"Screen Saver", String(config.data.screenSaverTimeoutMinutes) + " min", ItemAction::SAVER_TIMEOUT});
    settingItems.push_back({"Inactive Digit Light", String(config.data.tftDigitLigth ? "ON" : "OFF"), ItemAction::INACTIVE_DIGIT_LIGHT});
    settingItems.push_back({"Beeper", String(config.data.beeperEnabled ? "ON" : "OFF"), ItemAction::BEEPER_ENABLED});
    settingItems.push_back({"FFT Config AM", decodeFFTConfig(config.data.miniAudioFftConfigAm), ItemAction::FFT_CONFIG_AM});
    settingItems.push_back({"FFT Config FM", decodeFFTConfig(config.data.miniAudioFftConfigFm), ItemAction::FFT_CONFIG_FM});
    settingItems.push_back({"CW Receiver Offset", String(config.data.cwReceiverOffsetHz) + " Hz", ItemAction::CW_RECEIVER_OFFSET});
    settingItems.push_back(
        {"RTTY Frequencies", String(round(config.data.rttyMarkFrequencyHz)) + "/" + String(round(config.data.rttyShiftHz)) + " Hz", ItemAction::RTTY_FREQUENCIES});
    settingItems.push_back({"System Information", "", ItemAction::INFO});
    settingItems.push_back({"Factory Reset", "", ItemAction::FACTORY_RESET});

    // Lista komponens újrarajzolásának kérése, ha létezik
    if (menuList) {
        menuList->markForRedraw();
    }
}

/**
 * @brief Egy adott lista elem megjelenítésének frissítése
 *
 * Ez a metódus egy konkrét menüpont megjelenítését frissíti
 * anélkül, hogy az egész listát újra kellene rajzolni.
 * Hasznos, ha csak egy beállítás értéke változott.
 *
 * @param index A frissítendő menüpont indexe (0-tól kezdődik)
 */
void SetupScreen::updateListItem(int index) {
    if (index >= 0 && index < settingItems.size() && menuList) {
        menuList->refreshItemDisplay(index);
    }
}

/**
 * @brief FFT konfiguráció érték dekódolása olvasható szöveggé
 *
 * Az FFT konfigurációs float értékét alakítja át ember által
 * olvasható string formátumra:
 * - -1.0f → "Disabled" (letiltva)
 * - 0.0f → "Auto Gain" (automatikus erősítés)
 * - pozitív érték → "Manual: X.Xx" (manuális erősítés)
 *
 * @param value Az FFT konfigurációs érték
 * @return Olvasható string reprezentáció
 */
String SetupScreen::decodeFFTConfig(float value) {
    if (value == -1.0f)
        return "Disabled";
    else if (value == 0.0f)
        return "Auto Gain";
    else
        return "Manual: " + String(value, 1) + "x";
}

/**
 * @brief Menüpont kattintás kezelése (IScrollableListDataSource interfész)
 *
 * Ez a metódus akkor hívódik meg, amikor a felhasználó rákattint
 * egy menüpontra. Az ItemAction enum alapján határozza meg,
 * hogy melyik dialógust kell megnyitni vagy milyen műveletet kell végrehajtani.
 *
 * @param index A kiválasztott menüpont indexe (0-tól kezdődik)
 * @return false (nem fogyasztja el az eseményt, továbbadja)
 */
bool SetupScreen::onItemClicked(int index) {
    // Index érvényességének ellenőrzése
    if (index < 0 || index >= settingItems.size())
        return false;

    const SettingItem &item = settingItems[index];

    // Megfelelő kezelő metódus meghívása az ItemAction alapján
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
            // Nincs művelet definiálva
            break;
    }
    return false;
}

/**
 * @brief TFT háttérvilágítás fényességének beállítása dialógussal
 *
 * Megnyitja a ValueChangeDialog dialógust a TFT háttérvilágítás fényességének
 * beállításához. A dialógus lehetővé teszi a valós idejű előnézetet,
 * ahol a fényesség azonnal alkalmazásra kerül a módosítás során.
 *
 * Beállítási tartomány: TFT_BACKGROUND_LED_MIN_BRIGHTNESS - TFT_BACKGROUND_LED_MAX_BRIGHTNESS
 * Lépésköz: 10
 *
 * @param index A menüpont indexe a lista frissítéséhez
 */
void SetupScreen::handleBrightnessDialog(int index) {
    auto brightnessDialog = std::make_shared<ValueChangeDialog>(
        this, this->tft, "Brightness", "Adjust TFT Backlight:", &config.data.tftBackgroundBrightness, (uint8_t)TFT_BACKGROUND_LED_MIN_BRIGHTNESS,
        (uint8_t)TFT_BACKGROUND_LED_MAX_BRIGHTNESS, (uint8_t)10,
        // Valós idejű előnézet callback - azonnal alkalmazza a fényességet
        [this, index](const std::variant<int, float, bool> &liveNewValue) {
            if (std::holds_alternative<int>(liveNewValue)) {
                int currentDialogVal = std::get<int>(liveNewValue);
                config.data.tftBackgroundBrightness = static_cast<uint8_t>(currentDialogVal);
                analogWrite(PIN_TFT_BACKGROUND_LED, config.data.tftBackgroundBrightness);
                DEBUG("SetupScreen: Live brightness preview: %u\n", config.data.tftBackgroundBrightness);
            }
        },
        // Dialógus bezárás callback - lista elem frissítése elfogadás esetén
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
 * @brief Zajzár alapjának kiválasztása dialógussal
 *
 * Megnyitja a MultiButtonDialog dialógust, amely lehetővé teszi
 * a zajzár alapjának kiválasztását RSSI vagy SNR között.
 * A jelenlegi beállítás alapértelmezetten ki van választva.
 *
 * Opciók:
 * - RSSI: Received Signal Strength Indicator alapú zajzár
 * - SNR: Signal-to-Noise Ratio alapú zajzár
 *
 * @param index A menüpont indexe a lista frissítéséhez
 */
void SetupScreen::handleSquelchBasisDialog(int index) {
    const char *options[] = {"RSSI", "SNR"};
    // Jelenlegi beállítás meghatározása az alapértelmezett kiválasztáshoz
    int currentSelection = config.data.squelchUsesRSSI ? 0 : 1;

    auto basisDialog = std::make_shared<MultiButtonDialog>(
        this, this->tft, "Squelch Basis", "Select squelch basis:", options, ARRAY_ITEM_COUNT(options),
        // Gomb kattintás callback
        [this, index](int buttonIndex, const char *buttonLabel, MultiButtonDialog *dialog) {
            bool newSquelchUsesRSSI = (buttonIndex == 0);
            // Csak akkor mentjük, ha változott az érték
            if (config.data.squelchUsesRSSI != newSquelchUsesRSSI) {
                config.data.squelchUsesRSSI = newSquelchUsesRSSI;
                config.checkSave();
            }
            // Lista elem értékének frissítése
            settingItems[index].value = String(config.data.squelchUsesRSSI ? "RSSI" : "SNR");
            updateListItem(index);
        },
        true,             // Automatikus bezárás gomb kattintásra
        currentSelection, // Alapértelmezett kiválasztás
        true,             // Highlight engedélyezett
        Rect(-1, -1, 250, 120));
    this->showDialog(basisDialog);
}

/**
 * @brief Képernyővédő időtúllépésének beállítása dialógussal
 *
 * Megnyitja a ValueChangeDialog dialógust a képernyővédő aktiválódási
 * idejének percekben történő beállításához. A valós idejű előnézet
 * azonnal menti a konfigurációt.
 *
 * Beállítási tartomány: SCREEN_SAVER_TIMEOUT_MIN - SCREEN_SAVER_TIMEOUT_MAX perc
 * Lépésköz: 1 perc
 *
 * @param index A menüpont indexe a lista frissítéséhez
 */
void SetupScreen::handleSaverTimeoutDialog(int index) {
    auto saverDialog = std::make_shared<ValueChangeDialog>(
        this, this->tft, "Screen Saver", "Timeout (minutes):", &config.data.screenSaverTimeoutMinutes, SCREEN_SAVER_TIMEOUT_MIN, SCREEN_SAVER_TIMEOUT_MAX, 1,
        // Valós idejű előnézet callback - azonnal menti a konfigurációt
        [this, index](const std::variant<int, float, bool> &liveNewValue) {
            if (std::holds_alternative<int>(liveNewValue)) {
                int currentDialogVal = std::get<int>(liveNewValue);
                config.data.screenSaverTimeoutMinutes = static_cast<uint8_t>(currentDialogVal);
                config.checkSave();
            }
        },
        // Dialógus bezárás callback - lista elem frissítése elfogadás esetén
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
 * @brief Boolean (BE/KI) beállítások váltása
 *
 * Ez egy általános segédfüggvény a boolean típusú beállítások
 * egyszerű váltásához. Megfordítja az érték állapotát (true↔false),
 * menti a konfigurációt és frissíti a lista megjelenítését.
 *
 * Használható beállítások:
 * - Inactive Digit Light (inaktív számjegyek világítása)
 * - Beeper (hangjelzések engedélyezése)
 * - És bármely más boolean konfigurációs érték
 *
 * @param index A menüpont indexe a lista frissítéséhez
 * @param configValue Referencia a módosítandó boolean konfigurációs értékre
 */
void SetupScreen::handleToggleItem(int index, bool &configValue) {
    // Boolean érték megfordítása
    configValue = !configValue;
    // Konfiguráció mentése
    config.checkSave();

    // UI érték frissítése a specifikus beállítás alapján
    if (index >= 0 && index < settingItems.size()) {
        settingItems[index].value = String(configValue ? "ON" : "OFF");
        updateListItem(index);
    }
}

/**
 * @brief FFT konfigurációs dialógus kezelése AM vagy FM módhoz
 *
 * Megnyitja a MultiButtonDialog dialógust az FFT (Fast Fourier Transform)
 * erősítési módjának beállításához. Három opció közül lehet választani:
 *
 * 1. Disabled (-1.0f): FFT teljesen letiltva
 * 2. Auto Gain (0.0f): Automatikus erősítés vezérlés
 * 3. Manual Gain (>0.0f): Manuális erősítés beállítása (0.1x - 10.0x)
 *
 * A manuális mód kiválasztásakor egy nested ValueChangeDialog nyílik meg
 * a pontos erősítési tényező beállításához.
 *
 * @param index A menüpont indexe a lista frissítéséhez
 * @param isAM true = AM mód konfigurációja, false = FM mód konfigurációja
 */
void SetupScreen::handleFFTConfigDialog(int index, bool isAM) {
    // Megfelelő konfigurációs változó kiválasztása (AM vagy FM)
    float &currentConfig = isAM ? config.data.miniAudioFftConfigAm : config.data.miniAudioFftConfigFm;
    const char *title = isAM ? "FFT Config AM" : "FFT Config FM";

    // Jelenlegi beállítás meghatározása az alapértelmezett gomb kiemeléséhez
    int defaultSelection = 0; // Disabled
    if (currentConfig == 0.0f) {
        defaultSelection = 1; // Auto G
    } else if (currentConfig > 0.0f) {
        defaultSelection = 2; // Manual G
    }

    const char *options[] = {"Disabled", "Auto G", "Manual G"};

    auto fftDialog = std::make_shared<MultiButtonDialog>(
        this, this->tft, title, "Select FFT gain mode:", options, ARRAY_ITEM_COUNT(options),
        // Gomb kattintás callback
        [this, index, isAM, &currentConfig, title](int buttonIndex, const char *buttonLabel, MultiButtonDialog *dialog) {
            switch (buttonIndex) {
                case 0: // Disabled - FFT letiltása
                    currentConfig = -1.0f;
                    config.checkSave();
                    settingItems[index].value = "Disabled";
                    updateListItem(index);
                    dialog->close(UIDialogBase::DialogResult::Accepted);
                    break;

                case 1: // Auto Gain - automatikus erősítés
                    currentConfig = 0.0f;
                    config.checkSave();
                    settingItems[index].value = "Auto Gain";
                    updateListItem(index);
                    dialog->close(UIDialogBase::DialogResult::Accepted);
                    break;

                case 2: // Manual Gain - manuális erősítés beállítása nested dialógussal
                {
                    dialog->close(UIDialogBase::DialogResult::Accepted);

                    // Ideiglenes float változó a manuális erősítés értékének tárolásához
                    auto tempGainValuePtr = std::make_shared<float>((currentConfig > 0.0f) ? currentConfig : 1.0f);

                    // Nested ValueChangeDialog a pontos erősítési tényező beállításához
                    auto gainDialog = std::make_shared<ValueChangeDialog>(
                        this,                                       // Képernyő referencia
                        this->tft,                                  // TFT_eSPI referencia
                        (String(title) + " - Manual Gain").c_str(), // Cím
                        "Set gain factor (0.1 - 10.0):",            // Leírás
                        tempGainValuePtr.get(), 0.1f, 10.0f, 0.1f,  // Érték, tartomány és lépésköz
                        nullptr,                                    // Valós idejű előnézet callback
                        // Nested dialógus bezárás callback
                        [this, index, &currentConfig, tempGainValuePtr](UIDialogBase *sender, MessageDialog::DialogResult result) {
                            if (result == MessageDialog::DialogResult::Accepted) {
                                currentConfig = *tempGainValuePtr;
                                config.checkSave();
                                // Teljes menü újrafeltöltése a helyes érték megjelenítéséhez
                                populateMenuItems();
                            }
                        },
                        Rect(-1, -1, 300, 0) // Dialógus mérete (szélesség, magasság)
                    );
                    this->showDialog(gainDialog);
                } break;
            }
        },
        false,            // Nincs automatikus bezárás
        defaultSelection, // Alapértelmezett kiválasztás
        false,            // Nincs highlight
        Rect(-1, -1, 340, 120));
    this->showDialog(fftDialog);
}

/**
 * @brief CW (Continuous Wave) vevő frekvencia eltolásának beállítása dialógussal
 *
 * Megnyitja a ValueChangeDialog dialógust a CW vevő frekvencia eltolásának
 * beállításához Hz-ben. Ez a beállítás határozza meg, hogy a CW dekóder
 * milyen frekvencia eltolással dolgozzon.
 *
 * Beállítási tartomány: CW_DECODER_MIN_FREQUENCY - CW_DECODER_MAX_FREQUENCY Hz
 * Lépésköz: 10 Hz
 *
 * @param index A menüpont indexe a lista frissítéséhez
 */
void SetupScreen::handleCWOffsetDialog(int index) {
    auto cwOffsetDialog = std::make_shared<ValueChangeDialog>(
        this, this->tft, "CW Receiver Offset", "Set CW receiver offset (Hz):", reinterpret_cast<int *>(&config.data.cwReceiverOffsetHz), CW_DECODER_MIN_FREQUENCY,
        CW_DECODER_MAX_FREQUENCY, 10,
        // Valós idejű előnézet callback - azonnal menti a konfigurációt
        [this, index](const std::variant<int, float, bool> &liveNewValue) {
            if (std::holds_alternative<int>(liveNewValue)) {
                int currentDialogVal = std::get<int>(liveNewValue);
                config.data.cwReceiverOffsetHz = static_cast<uint16_t>(currentDialogVal);
                config.checkSave();
            }
        },
        // Dialógus bezárás callback - lista elem frissítése elfogadás esetén
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
 * Megnyitja egy MultiButtonDialog-ot, amely lehetővé teszi az RTTY
 * (Radio Teletype) frekvencia paramétereinek beállítását:
 * - "Mark" gomb: RTTY mark frekvencia beállítása (1000-3000 Hz, 5 Hz lépésekkel)
 * - "Shift" gomb: RTTY shift érték beállítása (50-1000 Hz, 5 Hz lépésekkel)
 *
 * Mindkét opció ValueChangeDialog-ot nyit meg a pontos érték megadásához.
 * A konfiguráció változtatása után a lista automatikusan frissül.
 *
 * @param index A menüpont indexe a listában a frissítéshez
 */
void SetupScreen::handleRTTYFrequenciesDialog(int index) {
    // RTTY frekvenciák beállítása MultiButtonDialog segítségével
    const char *options[] = {"Mark", "Shift"};
    auto rttyDialog = std::make_shared<MultiButtonDialog>(
        this, this->tft,                    //
        "RTTY Frequencies",                 //
        "Select frequency to configure:",   //
        options, ARRAY_ITEM_COUNT(options), //
        [this, index](int buttonIndex, const char *buttonLabel, MultiButtonDialog *dialog) {
            if (strcmp(buttonLabel, "Mark") == 0) {

                // RTTY Mark frekvencia beállítása (1000-3000 Hz)
                auto markDialog = std::make_shared<ValueChangeDialog>(
                    this, this->tft,                                                    //
                    "RTTY Mark Frequency",                                              //
                    "Hz (1000-3000):",                                                  //
                    &config.data.rttyMarkFrequencyHz, 1000.0f, 3000.0f, 5.0f,           //
                    [this, index](const std::variant<int, float, bool> &liveNewValue) { // Valós idejű előnézet callback
                        if (std::holds_alternative<float>(liveNewValue)) {
                            float currentDialogVal = std::get<float>(liveNewValue);
                            config.data.rttyMarkFrequencyHz = currentDialogVal;
                            config.checkSave();
                        }
                    },
                    [this, index](UIDialogBase *sender, MessageDialog::DialogResult dialogResult) { // Dialógus bezárás callback
                        if (dialogResult == MessageDialog::DialogResult::Accepted) {
                            settingItems[index].value = String(round(config.data.rttyMarkFrequencyHz)) + "/" + String(round(config.data.rttyShiftHz)) + " Hz";
                            updateListItem(index);
                        }
                    },
                    Rect(-1, -1, 270, 0) // Dialógus mérete (szélesség, magasság)
                );
                this->showDialog(markDialog);

            } else if (strcmp(buttonLabel, "Shift") == 0) {

                // RTTY Shift frekvencia beállítása (50-1000 Hz)
                auto shiftDialog = std::make_shared<ValueChangeDialog>(
                    this, this->tft,                                                    //
                    "RTTY Shift",                                                       //
                    "Hz (50-1000):",                                                    //
                    &config.data.rttyShiftHz, 50.0f, 1000.0f, 5.0f,                     //
                    [this, index](const std::variant<int, float, bool> &liveNewValue) { // Valós idejű előnézet callback
                        if (std::holds_alternative<float>(liveNewValue)) {
                            float currentDialogVal = std::get<float>(liveNewValue);
                            config.data.rttyShiftHz = currentDialogVal;
                            config.checkSave();
                        }
                    },
                    [this, index](UIDialogBase *sender, MessageDialog::DialogResult dialogResult) { // Dialógus bezárás callback
                        if (dialogResult == MessageDialog::DialogResult::Accepted) {
                            settingItems[index].value = String(round(config.data.rttyMarkFrequencyHz)) + "/" + String(round(config.data.rttyShiftHz)) + " Hz";
                            updateListItem(index);
                        }
                    },
                    Rect(-1, -1, 270, 0) // Dialógus mérete (szélesség, magasság)
                );
                this->showDialog(shiftDialog);
            }
        },
        true,                  // Automatikus bezárás
        -1,                    // Nincs alapértelmezett kiválasztás
        true,                  // Highlight engedélyezett
        Rect(-1, -1, 300, 120) // Dialógus mérete (szélesség, magasság)
    );
    this->showDialog(rttyDialog);
}

/**
 * @brief Rendszer információ dialógus megjelenítése
 *
 * Megnyitja a SystemInfoDialog dialógust, amely részletes információkat
 * jelenít meg a rendszer állapotáról, beleértve:
 * - Firmware verzió
 * - Memória használat
 * - CPU információk
 * - Különböző rendszer paraméterek
 *
 * Ez a dialógus hasznos a hibakereséshez és a rendszer állapotának
 * monitorozásához.
 */
void SetupScreen::handleSystemInfoDialog() {
    auto systemInfoDialog = std::make_shared<SystemInfoDialog>(this, this->tft, Rect(-1, -1, 320, 240));
    this->showDialog(systemInfoDialog);
}

/**
 * @brief Gyári beállítások visszaállítása dialógussal
 *
 * Megnyitja egy megerősítő MessageDialog dialógust, amely rákérdez,
 * hogy a felhasználó valóban vissza kívánja-e állítani az összes
 * beállítást a gyári alapértékekre.
 *
 * Megerősítés esetén a következő műveletek hajtódnak végre:
 * 1. config.loadDefaults() - gyári értékek betöltése
 * 2. config.forceSave() - azonnal mentés a flash memóriába
 * 3. populateMenuItems() - menüpontok frissítése az új értékekkel
 *
 * FIGYELEM: Ez a művelet visszafordíthatatlan!
 */
void SetupScreen::handleFactoryResetDialog() {
    // Megerősítő dialógus létrehozása Igen/Nem gombokkal
    auto confirmDialog = std::make_shared<MessageDialog>(          //
        this, this->tft,                                           //
        Rect(-1, -1, 300, 0),                                      // Dialógus mérete
        "Factory Reset",                                           // Cím
        "Are you sure you want to reset all settings to default?", // Üzenet
        MessageDialog::ButtonsType::YesNo                          // Gombok típusa
    );

    // Dialógus callback beállítása
    confirmDialog->setDialogCallback([this](UIDialogBase *sender, MessageDialog::DialogResult result) {
        // Csak elfogadás (Igen) esetén hajtjuk végre a reset műveletet
        if (result == MessageDialog::DialogResult::Accepted) {
            // Gyári alapértékek betöltése
            config.loadDefaults();
            // Azonnali mentés a flash memóriába
            config.forceSave();
            // Menüpontok újrafeltöltése az új értékekkel
            populateMenuItems();
        }
    });

    // Megerősítő dialógus megjelenítése
    this->showDialog(confirmDialog);
}
