#include "VirtualKeyboardDialog.h"
#include "UIColorPalette.h"
#include "UIComponent.h"
#include "utils.h"

VirtualKeyboardDialog::VirtualKeyboardDialog(UIScreen *parent, TFT_eSPI &tft, const String &title, const String &initialText, uint8_t maxLength, OnTextChangedCallback onChanged)
    : UIDialogBase(parent, tft, Rect(-1, -1, 350, 260), ""), currentText(initialText), dialogTitle(title), maxTextLength(maxLength), textChangedCallback(onChanged),
      lastCursorBlink(millis()) {

    DEBUG("VirtualKeyboardDialog constructor\n");

    // A dialógus címének beállítása a tárolt String objektumból
    this->title = dialogTitle.c_str();

    // Input mező pozíció számítása
    inputRect = Rect(bounds.x + INPUT_MARGIN, bounds.y + getHeaderHeight() + INPUT_MARGIN, bounds.width - (INPUT_MARGIN * 2), INPUT_HEIGHT);

    // Billentyűzet terület pozíció számítása
    keyboardRect = Rect(bounds.x + 5, inputRect.y + inputRect.height + 10, bounds.width - 10, bounds.height - getHeaderHeight() - INPUT_HEIGHT - 60);

    createKeyboard();
}

VirtualKeyboardDialog::~VirtualKeyboardDialog() {
    DEBUG("VirtualKeyboardDialog destructor\n");
    keyButtons.clear();
}

void VirtualKeyboardDialog::createKeyboard() {
    keyButtons.clear();
    keyLabelCount = 0;

    uint8_t buttonId = 100; // Kezdő ID

    // Karakteres gombok létrehozása
    uint16_t startY = keyboardRect.y;

    for (uint8_t row = 0; row < KEYBOARD_ROWS; ++row) {
        const char *rowKeys = keyboardLayout[row];
        uint8_t keysInRow = strlen(rowKeys);

        uint16_t rowWidth = keysInRow * KEY_WIDTH + (keysInRow - 1) * KEY_SPACING;
        uint16_t startX = keyboardRect.x + (keyboardRect.width - rowWidth) / 2;

        uint16_t currentX = startX;
        uint16_t currentY = startY + row * (KEY_HEIGHT + KEY_SPACING);
        for (uint8_t col = 0; col < keysInRow; ++col) {
            char keyChar = rowKeys[col];

            // Felirat tárolása a char tömbben
            if (keyLabelCount < 50) {
                keyLabelStorage[keyLabelCount][0] = keyChar;
                keyLabelStorage[keyLabelCount][1] = '\0';

                auto keyButton = std::make_shared<UIButton>(tft, buttonId++, Rect(currentX, currentY, KEY_WIDTH, KEY_HEIGHT), keyLabelStorage[keyLabelCount],
                                                            UIButton::ButtonType::Pushable, [this, keyChar](const UIButton::ButtonEvent &event) {
                                                                if (event.state == UIButton::EventButtonState::Clicked) {
                                                                    handleKeyPress(keyChar);
                                                                }
                                                            });

                keyButtons.push_back(keyButton);
                addChild(keyButton);
                keyLabelCount++;
            }

            currentX += KEY_WIDTH + KEY_SPACING;
        }
    }

    // Speciális gombok létrehozása az utolsó sor alatt
    uint16_t specialY = startY + KEYBOARD_ROWS * (KEY_HEIGHT + KEY_SPACING) + 5;
    uint16_t specialStartX = keyboardRect.x;

    // Shift gomb
    shiftButton = std::make_shared<UIButton>(tft, buttonId++, Rect(specialStartX, specialY, 45, KEY_HEIGHT), "Shift", UIButton::ButtonType::Toggleable,
                                             [this](const UIButton::ButtonEvent &event) {
                                                 if (event.state == UIButton::EventButtonState::Clicked) {
                                                     shiftActive = !shiftActive;
                                                     updateButtonLabels();
                                                 }
                                             });
    addChild(shiftButton);

    // Space gomb
    spaceButton = std::make_shared<UIButton>(tft, buttonId++, Rect(specialStartX + 50, specialY, 80, KEY_HEIGHT), "Space", UIButton::ButtonType::Pushable,
                                             [this](const UIButton::ButtonEvent &event) {
                                                 if (event.state == UIButton::EventButtonState::Clicked) {
                                                     handleKeyPress(' ');
                                                 }
                                             });
    addChild(spaceButton);

    // Backspace gomb
    backspaceButton = std::make_shared<UIButton>(tft, buttonId++, Rect(specialStartX + 135, specialY, 40, KEY_HEIGHT), "<--", UIButton::ButtonType::Pushable,
                                                 [this](const UIButton::ButtonEvent &event) {
                                                     if (event.state == UIButton::EventButtonState::Clicked) {
                                                         handleSpecialKey("Backspace");
                                                     }
                                                 });
    addChild(backspaceButton); // Clear gomb
    clearButton = std::make_shared<UIButton>(tft, buttonId++, Rect(specialStartX + 180, specialY, 40, KEY_HEIGHT), "Clr", UIButton::ButtonType::Pushable,
                                             [this](const UIButton::ButtonEvent &event) {
                                                 if (event.state == UIButton::EventButtonState::Clicked) {
                                                     handleSpecialKey("Clear");
                                                 }
                                             });
    addChild(clearButton);

    // OK és Cancel gombok a speciális sor alatt
    uint16_t okCancelY = specialY + KEY_HEIGHT + 8;
    uint16_t okCancelWidth = 60;
    uint16_t okCancelStartX = keyboardRect.x + (keyboardRect.width - 2 * okCancelWidth - 10) / 2;

    // OK gomb
    auto okButton = std::make_shared<UIButton>(tft, buttonId++, Rect(okCancelStartX, okCancelY, okCancelWidth, KEY_HEIGHT), "OK", UIButton::ButtonType::Pushable,
                                               [this](const UIButton::ButtonEvent &event) {
                                                   if (event.state == UIButton::EventButtonState::Clicked) {
                                                       close(UIDialogBase::DialogResult::Accepted);
                                                   }
                                               });
    addChild(okButton);

    // Cancel gomb
    auto cancelButton = std::make_shared<UIButton>(tft, buttonId++, Rect(okCancelStartX + okCancelWidth + 10, okCancelY, okCancelWidth, KEY_HEIGHT), "Cancel",
                                                   UIButton::ButtonType::Pushable, [this](const UIButton::ButtonEvent &event) {
                                                       if (event.state == UIButton::EventButtonState::Clicked) {
                                                           close(UIDialogBase::DialogResult::Rejected);
                                                       }
                                                   });
    addChild(cancelButton);
}

void VirtualKeyboardDialog::drawSelf() {
    // Szülő osztály rajzolása (keret, cím, háttér, 'X' gomb)
    UIDialogBase::drawSelf();

    // Input mező rajzolása
    drawInputField();

    // Minden gomb már hozzá van adva a gyerekekhez,
    // az UIDialogBase::draw() automatikusan kirajzolja őket
}

void VirtualKeyboardDialog::drawInputField() {
    // Input mező háttér
    tft.fillRect(inputRect.x, inputRect.y, inputRect.width, inputRect.height, TFT_BLACK);
    tft.drawRect(inputRect.x, inputRect.y, inputRect.width, inputRect.height, TFT_WHITE);

    // Szöveg kirajzolása
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(ML_DATUM);
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextSize(1);

    String displayText = currentText;
    if (displayText.length() > 18) { // Ha túl hosszú, csak a végét mutatjuk
        displayText = "..." + displayText.substring(displayText.length() - 15);
    }

    tft.drawString(displayText, inputRect.x + 5, inputRect.y + inputRect.height / 2);

    // Kurzor rajzolása
    if (cursorVisible) {
        drawCursor();
    }
}

void VirtualKeyboardDialog::drawCursor() {
    // Kurzor pozíció számítása
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextSize(1);

    String displayText = currentText;
    if (displayText.length() > 18) {
        displayText = "..." + displayText.substring(displayText.length() - 15);
    }

    uint16_t textWidth = tft.textWidth(displayText);
    uint16_t cursorX = inputRect.x + 5 + textWidth;
    uint16_t cursorY = inputRect.y + 3;
    uint16_t cursorHeight = inputRect.height - 6;

    if (cursorX < inputRect.x + inputRect.width - 3) {
        tft.drawFastVLine(cursorX, cursorY, cursorHeight, TFT_WHITE);
    }
}

bool VirtualKeyboardDialog::handleTouch(const TouchEvent &event) {
    // Először próbáljuk a szülő osztály touch kezelését (pl. close gomb)
    if (UIDialogBase::handleTouch(event)) {
        return true;
    }

    uint16_t x = event.x;
    uint16_t y = event.y; // Karakteres gombok ellenőrzése
    for (auto &keyButton : keyButtons) {
        if (keyButton->getBounds().contains(x, y)) {
            char keyChar = keyButton->getLabel()[0];
            handleKeyPress(keyChar);
            return true;
        }
    }

    // Speciális gombok ellenőrzése
    if (shiftButton->getBounds().contains(x, y)) {
        shiftActive = !shiftActive;
        shiftButton->setButtonState(shiftActive ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
        updateButtonLabels();
        return true;
    }

    if (spaceButton->getBounds().contains(x, y)) {
        handleKeyPress(' ');
        return true;
    }

    if (backspaceButton->getBounds().contains(x, y)) {
        handleSpecialKey("backspace");
        return true;
    }

    if (clearButton->getBounds().contains(x, y)) {
        handleSpecialKey("clear");
        return true;
    }

    return false;
}

void VirtualKeyboardDialog::handleKeyPress(char key) {
    if (currentText.length() >= maxTextLength) {
        return; // Max hossz elérve
    }

    char actualKey = getKeyChar(key, shiftActive);
    currentText += actualKey; // Shift automatikus kikapcsolása betű után
    if (shiftActive && isalpha(key)) {
        shiftActive = false;
        shiftButton->setButtonState(UIButton::ButtonState::Off);
        updateButtonLabels();
    }

    notifyTextChanged();
    markForRedraw();
}

void VirtualKeyboardDialog::handleSpecialKey(const String &keyType) {
    if (keyType == "backspace") {
        if (currentText.length() > 0) {
            currentText.remove(currentText.length() - 1);
            notifyTextChanged();
            markForRedraw();
        }
    } else if (keyType == "clear") {
        if (currentText.length() > 0) {
            currentText = "";
            notifyTextChanged();
            markForRedraw();
        }
    }
}

void VirtualKeyboardDialog::updateButtonLabels() {
    for (size_t i = 0; i < keyButtons.size() && i < keyLabelCount; i++) {
        const char *currentLabel = keyButtons[i]->getLabel();
        if (strlen(currentLabel) == 1) {
            char baseChar = currentLabel[0];
            char newChar = getKeyChar(baseChar, shiftActive);
            keyLabelStorage[i][0] = newChar;
            keyLabelStorage[i][1] = '\0';
            keyButtons[i]->setLabel(keyLabelStorage[i]);
        }
    }
}

char VirtualKeyboardDialog::getKeyChar(char baseChar, bool shifted) {
    if (isalpha(baseChar)) {
        return shifted ? toupper(baseChar) : tolower(baseChar);
    }

    // Speciális karakterek shift módban
    if (shifted) {
        switch (baseChar) {
            case '1':
                return '!';
            case '2':
                return '@';
            case '3':
                return '#';
            case '4':
                return '$';
            case '5':
                return '%';
            case '6':
                return '^';
            case '7':
                return '&';
            case '8':
                return '*';
            case '9':
                return '(';
            case '0':
                return ')';
            case '-':
                return '_';
            case '.':
                return ':';
            default:
                return baseChar;
        }
    }

    return baseChar;
}

void VirtualKeyboardDialog::setText(const String &text) {
    currentText = text;
    if (currentText.length() > maxTextLength) {
        currentText = currentText.substring(0, maxTextLength);
    }
    notifyTextChanged();
    markForRedraw();
}

void VirtualKeyboardDialog::notifyTextChanged() {
    if (textChangedCallback) {
        textChangedCallback(currentText);
    }
}

void VirtualKeyboardDialog::handleOwnLoop() {
    // Kurzor villogtatás
    unsigned long now = millis();
    if (now - lastCursorBlink >= CURSOR_BLINK_INTERVAL) {
        cursorVisible = !cursorVisible;
        lastCursorBlink = now;
        markForRedraw();
    }
}
