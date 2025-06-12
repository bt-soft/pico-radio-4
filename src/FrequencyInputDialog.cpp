/**
 * @file FrequencyInputDialog.cpp
 * @brief Frekvencia beviteli dialógus osztály implementációja
 */

#include "FrequencyInputDialog.h"
#include "Band.h"
#include "UIColorPalette.h"
#include "defines.h"
#include "utils.h"

/**
 * @brief Konstruktor
 */
FrequencyInputDialog::FrequencyInputDialog(UIScreen *parentScreen, TFT_eSPI &tft, const Rect &bounds, const char *title, const char *message, Si4735Manager *si4735Manager,
                                           FrequencyChangeCallback callback, const ColorScheme &cs)
    : MessageDialog(parentScreen, tft, bounds, title, message, ButtonsType::OkCancel, cs), _si4735Manager(si4735Manager), _frequencyCallback(callback), _isValid(false) {

    // Sáv paraméterek inicializálása
    initializeBandParameters();

    // Aktuális frekvencia betöltése
    uint16_t currentFreq = _si4735Manager->getSi4735().getFrequency();
    setCurrentFrequency(currentFreq);

    // Dialógus méret beállítása ha automatikus
    if (this->bounds.width <= 0 || this->bounds.height <= 0) {
        this->bounds.width = 320;
        this->bounds.height = 300;
    }
}

/**
 * @brief Sáv paraméterek inicializálása
 */
void FrequencyInputDialog::initializeBandParameters() {
    _currentBandType = _si4735Manager->getCurrentBandType();
    BandTable &currentBand = _si4735Manager->getCurrentBand();

    _minFreq = currentBand.minimumFreq;
    _maxFreq = currentBand.maximumFreq;

    // Sáv típus alapú formátum beállítás
    switch (_currentBandType) {
        case FM_BAND_TYPE:
            _unitString = "MHz";
            _maskString = "188.88";
            break;

        case MW_BAND_TYPE:
        case LW_BAND_TYPE:
            _unitString = "kHz";
            _maskString = "8888";
            break;

        case SW_BAND_TYPE:
        default:
            _unitString = "MHz";
            _maskString = "88.888";
            break;
    }

    DEBUG("FrequencyInputDialog: Band type %d, range %d-%d %s\n", _currentBandType, _minFreq, _maxFreq, _unitString.c_str());
}

/**
 * @brief Aktuális frekvencia beállítása
 */
void FrequencyInputDialog::setCurrentFrequency(uint16_t rawFrequency) {
    // Frekvencia konvertálása string formátumba
    switch (_currentBandType) {
        case FM_BAND_TYPE:
            // FM: 10800 -> "108.00"
            _inputString = String(rawFrequency / 100.0f, 2);
            break;

        case MW_BAND_TYPE:
        case LW_BAND_TYPE:
            // MW/LW: 1440 -> "1440"
            _inputString = String(rawFrequency);
            break;

        case SW_BAND_TYPE:
        default:
            // SW: 15230 -> "15.230"
            _inputString = String(rawFrequency / 1000.0f, 3);
            break;
    }

    // Validálás
    _isValid = validateAndParseFrequency();
    updateOkButtonState();
}

/**
 * @brief Dialógus tartalom létrehozása
 */
void FrequencyInputDialog::createDialogContent() {
    // Szülő metódus hívása (OK/Cancel gombok létrehozása)
    MessageDialog::createDialogContent();

    // Numerikus gombok létrehozása
    createNumericButtons();

    // Funkció gombok létrehozása
    createFunctionButtons();

    // OK gomb kezdeti állapot beállítása
    updateOkButtonState();
}

/**
 * @brief Numerikus gombok létrehozása
 */
void FrequencyInputDialog::createNumericButtons() {
    _digitButtons.clear();
    _digitButtons.reserve(10);

    // 0-9 gombok létrehozása
    for (uint8_t i = 0; i <= 9; i++) {
        String label = String(i);
        auto button =
            std::make_shared<UIButton>(tft, i, Rect(0, 0, NUMERIC_BUTTON_SIZE, NUMERIC_BUTTON_SIZE), label.c_str(), UIButton::ButtonType::Pushable, UIButton::ButtonState::Off);

        // Gomb esemény kezelő beállítása
        button->setEventCallback([this, i](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                handleDigitInput(i);
            }
        });

        _digitButtons.push_back(button);
        addChild(button);
    }
}

/**
 * @brief Funkció gombok létrehozása
 */
void FrequencyInputDialog::createFunctionButtons() {
    // Tizedes pont gomb (csak FM és SW esetén)
    if (_currentBandType == FM_BAND_TYPE || _currentBandType == SW_BAND_TYPE) {
        _dotButton = std::make_shared<UIButton>(tft, 10, Rect(0, 0, NUMERIC_BUTTON_SIZE, NUMERIC_BUTTON_SIZE), ".", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off);

        _dotButton->setEventCallback([this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                handleDotInput();
            }
        });

        addChild(_dotButton);
    }

    // Backspace gomb
    _clearButton = std::make_shared<UIButton>(tft, 11, Rect(0, 0, FUNCTION_BUTTON_WIDTH, FUNCTION_BUTTON_HEIGHT), "←", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off);

    _clearButton->setEventCallback([this](const UIButton::ButtonEvent &event) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            handleClearDigit();
        }
    });

    addChild(_clearButton);

    // Clear All gomb
    _clearAllButton =
        std::make_shared<UIButton>(tft, 12, Rect(0, 0, FUNCTION_BUTTON_WIDTH, FUNCTION_BUTTON_HEIGHT), "C", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off);

    _clearAllButton->setEventCallback([this](const UIButton::ButtonEvent &event) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            handleClearAll();
        }
    });

    addChild(_clearAllButton);
}

/**
 * @brief Dialógus tartalom elrendezése
 */
void FrequencyInputDialog::layoutDialogContent() {
    // Szülő layout hívása
    MessageDialog::layoutDialogContent();                                               // Frekvencia kijelző terület a cím alatt
    uint16_t displayY = bounds.y + UIDialogBase::HEADER_HEIGHT + UIDialogBase::PADDING; // Numerikus billentyűzet elrendezése (3x4 grid + funkció gombok)
    uint16_t keypadStartY = displayY + DISPLAY_AREA_HEIGHT + UIDialogBase::PADDING;
    uint16_t keypadStartX = bounds.x + UIDialogBase::PADDING;
    uint16_t buttonSpacing = NUMERIC_BUTTON_SIZE + BUTTON_SPACING;

    // Számok elrendezése (1-9, majd 0)
    // Sor 1: 1, 2, 3
    for (int i = 1; i <= 3; i++) {
        auto &button = _digitButtons[i];
        button->setBounds(Rect(keypadStartX + (i - 1) * buttonSpacing, keypadStartY, NUMERIC_BUTTON_SIZE, NUMERIC_BUTTON_SIZE));
    }

    // Sor 2: 4, 5, 6
    for (int i = 4; i <= 6; i++) {
        auto &button = _digitButtons[i];
        button->setBounds(Rect(keypadStartX + (i - 4) * buttonSpacing, keypadStartY + buttonSpacing, NUMERIC_BUTTON_SIZE, NUMERIC_BUTTON_SIZE));
    }

    // Sor 3: 7, 8, 9
    for (int i = 7; i <= 9; i++) {
        auto &button = _digitButtons[i];
        button->setBounds(Rect(keypadStartX + (i - 7) * buttonSpacing, keypadStartY + 2 * buttonSpacing, NUMERIC_BUTTON_SIZE, NUMERIC_BUTTON_SIZE));
    }

    // Sor 4: 0 (középen), és speciális gombok
    _digitButtons[0]->setBounds(Rect(keypadStartX + buttonSpacing, keypadStartY + 3 * buttonSpacing, NUMERIC_BUTTON_SIZE, NUMERIC_BUTTON_SIZE));

    // Tizedes pont (ha van)
    if (_dotButton) {
        _dotButton->setBounds(Rect(keypadStartX, keypadStartY + 3 * buttonSpacing, NUMERIC_BUTTON_SIZE, NUMERIC_BUTTON_SIZE));
    }

    // Funkció gombok jobb oldalon
    uint16_t funcButtonX = keypadStartX + 3 * buttonSpacing + BUTTON_SPACING;

    _clearButton->setBounds(Rect(funcButtonX, keypadStartY, FUNCTION_BUTTON_WIDTH, FUNCTION_BUTTON_HEIGHT));

    _clearAllButton->setBounds(Rect(funcButtonX, keypadStartY + FUNCTION_BUTTON_HEIGHT + BUTTON_SPACING, FUNCTION_BUTTON_WIDTH, FUNCTION_BUTTON_HEIGHT));
}

/**
 * @brief Saját tartalom rajzolása
 */
void FrequencyInputDialog::drawSelf() {
    // Szülő rajzolás (háttér, cím, üzenet, gombok)
    MessageDialog::drawSelf();

    // Frekvencia kijelző rajzolása
    drawFrequencyDisplay();
}

/**
 * @brief Frekvencia kijelző rajzolása 7-szegmenses fonttal
 */
void FrequencyInputDialog::drawFrequencyDisplay() { // Kijelző terület
    uint16_t displayY = bounds.y + UIDialogBase::HEADER_HEIGHT + UIDialogBase::PADDING;
    uint16_t displayCenterX = bounds.x + bounds.width / 2;

    // 7-szegmenses font beállítása
    tft.setFreeFont(&DSEG7_Classic_Mini_Regular_34);
    tft.setTextSize(1);
    tft.setTextDatum(BC_DATUM);

    // Inaktív háttér számjegyek (ha engedélyezve van)
    if (config.data.tftDigitLigth) {
        tft.setTextColor(colors.foreground, colors.background);
        tft.drawString(_maskString, displayCenterX, displayY + 40);
    }

    // Aktív frekvencia
    uint16_t textColor = _isValid ? colors.foreground : TFT_RED;
    tft.setTextColor(textColor, colors.background);
    String displayText = _inputString.isEmpty() ? "0" : _inputString;
    tft.drawString(displayText, displayCenterX, displayY + 40);

    // Egység kijelzése
    tft.setFreeFont();
    tft.setTextSize(2);
    tft.setTextDatum(BL_DATUM);
    tft.setTextColor(colors.foreground, colors.background);

    uint16_t unitX = displayCenterX + tft.textWidth(displayText) / 2 + 5;
    tft.drawString(_unitString, unitX, displayY + 40);
}

/**
 * @brief Numerikus gomb megnyomás kezelése
 */
void FrequencyInputDialog::handleDigitInput(uint8_t digit) {
    // Maximum hossz ellenőrzése
    uint8_t maxLength = (_currentBandType == SW_BAND_TYPE) ? 6 : (_currentBandType == FM_BAND_TYPE) ? 6 : 4;

    if (_inputString.length() >= maxLength) {
        Utils::beepError();
        return;
    }

    // Digit hozzáadása
    _inputString += String(digit);

    // Validálás és kijelző frissítése
    _isValid = validateAndParseFrequency();
    updateOkButtonState();
    updateFrequencyDisplay();
}

/**
 * @brief Tizedes pont bevitel kezelése
 */
void FrequencyInputDialog::handleDotInput() {
    // Ellenőrizzük, hogy már van-e pont
    if (_inputString.indexOf('.') != -1) {
        Utils::beepError();
        return;
    }

    // Ha üres, akkor "0." -ot kezdünk
    if (_inputString.isEmpty()) {
        _inputString = "0.";
    } else {
        _inputString += ".";
    }

    updateFrequencyDisplay();
}

/**
 * @brief Egy digit törlése
 */
void FrequencyInputDialog::handleClearDigit() {
    if (_inputString.length() > 0) {
        _inputString = _inputString.substring(0, _inputString.length() - 1);
        _isValid = validateAndParseFrequency();
        updateOkButtonState();
        updateFrequencyDisplay();
    }
}

/**
 * @brief Minden digit törlése
 */
void FrequencyInputDialog::handleClearAll() {
    _inputString = "";
    _isValid = false;
    updateOkButtonState();
    updateFrequencyDisplay();
}

/**
 * @brief Frekvencia string validálása és parsing
 */
bool FrequencyInputDialog::validateAndParseFrequency() {
    if (_inputString.isEmpty()) {
        return false;
    }

    // String-ből float konvertálás
    float freqValue = _inputString.toFloat();

    // Alapvető tartomány ellenőrzés
    switch (_currentBandType) {
        case FM_BAND_TYPE:
            // FM: 64.0 - 108.0 MHz
            if (freqValue < 64.0f || freqValue > 108.0f)
                return false;
            break;

        case MW_BAND_TYPE:
            // MW: 520 - 1710 kHz (tipikus)
            if (freqValue < 520.0f || freqValue > 1710.0f)
                return false;
            break;

        case LW_BAND_TYPE:
            // LW: 150 - 450 kHz (tipikus)
            if (freqValue < 150.0f || freqValue > 450.0f)
                return false;
            break;

        case SW_BAND_TYPE:
        default:
            // SW: 1.8 - 30.0 MHz (tipikus)
            if (freqValue < 1.8f || freqValue > 30.0f)
                return false;
            break;
    }

    // Sávhatárokon belüli ellenőrzés
    uint16_t rawFreq = calculateRawFrequency();
    return isFrequencyInBounds(rawFreq);
}

/**
 * @brief Frekvencia string-ből nyers érték kiszámolása
 */
uint16_t FrequencyInputDialog::calculateRawFrequency() const {
    float freqValue = _inputString.toFloat();

    switch (_currentBandType) {
        case FM_BAND_TYPE:
            // FM: MHz -> x100 (pl. 100.5 -> 10050)
            return static_cast<uint16_t>(freqValue * 100);

        case MW_BAND_TYPE:
        case LW_BAND_TYPE:
            // MW/LW: kHz -> x1 (pl. 1440 -> 1440)
            return static_cast<uint16_t>(freqValue);

        case SW_BAND_TYPE:
        default:
            // SW: MHz -> x1000 (pl. 15.230 -> 15230)
            return static_cast<uint16_t>(freqValue * 1000);
    }
}

/**
 * @brief Sáv határokon belül van-e a frekvencia
 */
bool FrequencyInputDialog::isFrequencyInBounds(uint16_t rawFreq) const { return (rawFreq >= _minFreq && rawFreq <= _maxFreq); }

/**
 * @brief OK gomb állapot frissítése
 */
void FrequencyInputDialog::updateOkButtonState() {
    auto okButton = getOkButton();
    if (okButton) {
        okButton->setEnabled(_isValid);
    }
}

/**
 * @brief Frekvencia kijelző terület frissítése
 */
void FrequencyInputDialog::updateFrequencyDisplay() {
    // Csak a frekvencia kijelző területet frissítjük
    uint16_t displayY = bounds.y + UIDialogBase::HEADER_HEIGHT + UIDialogBase::PADDING;

    // Kijelző terület törlése
    tft.fillRect(bounds.x + UIDialogBase::PADDING, displayY, bounds.width - 2 * UIDialogBase::PADDING, DISPLAY_AREA_HEIGHT, colors.background);

    // Frekvencia újrarajzolása
    drawFrequencyDisplay();
}

/**
 * @brief Rotary encoder kezelés
 */
bool FrequencyInputDialog::handleRotary(const RotaryEvent &event) {
    // Rotary encoder-rel végigmehetünk a gomboko és kattintással aktiválhatjuk őket
    // Egyelőre nem implementáljuk, a szülő osztály kezeli
    return MessageDialog::handleRotary(event);
}

/**
 * @brief OK gomb megnyomás kezelése
 */
void FrequencyInputDialog::onOkClicked() {
    if (_isValid && _frequencyCallback) {
        uint16_t rawFreq = calculateRawFrequency();
        DEBUG("FrequencyInputDialog: Setting frequency to %d\n", rawFreq);
        _frequencyCallback(rawFreq);
    }

    // Dialógus bezárása
    close(DialogResult::Accepted);
}

/**
 * @brief Cancel gomb megnyomás kezelése
 */
void FrequencyInputDialog::onCancelClicked() {
    DEBUG("FrequencyInputDialog: Cancelled\n");

    // Dialógus bezárása
    close(DialogResult::Rejected);
}
