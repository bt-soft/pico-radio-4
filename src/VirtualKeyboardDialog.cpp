#include "VirtualKeyboardDialog.h"
#include "UIColorPalette.h"
#include "UIComponent.h"
#include "utils.h"

VirtualKeyboardDialog::VirtualKeyboardDialog(UIScreen *parent, TFT_eSPI &tft, const char *title, const String &initialText, uint8_t maxLength, OnTextChangedCallback onChanged)
    : UIDialogBase(parent, tft, Rect(-1, -1, 350, 260), title), currentText(initialText), maxTextLength(maxLength), textChangedCallback(onChanged), lastCursorBlink(millis()) {

    DEBUG("VirtualKeyboardDialog: Dialog bounds: x=%d, y=%d, w=%d, h=%d\n", bounds.x, bounds.y, bounds.width, bounds.height);
    DEBUG("VirtualKeyboardDialog: Screen size: %dx%d\n", UIComponent::SCREEN_W, UIComponent::SCREEN_H);

    // Input mező pozíció számítása
    inputRect = Rect(bounds.x + INPUT_MARGIN, bounds.y + getHeaderHeight() + INPUT_MARGIN, bounds.width - (INPUT_MARGIN * 2), INPUT_HEIGHT);
    DEBUG("VirtualKeyboardDialog: Input rect: x=%d, y=%d, w=%d, h=%d\n", inputRect.x, inputRect.y, inputRect.width, inputRect.height);

    // Billentyűzet terület pozíció számítása
    keyboardRect = Rect(bounds.x + 5, inputRect.y + inputRect.height + 10, bounds.width - 10, bounds.height - getHeaderHeight() - INPUT_HEIGHT - 60);
    DEBUG("VirtualKeyboardDialog: Keyboard rect: x=%d, y=%d, w=%d, h=%d\n", keyboardRect.x, keyboardRect.y, keyboardRect.width, keyboardRect.height);

    createKeyboard();
}

VirtualKeyboardDialog::~VirtualKeyboardDialog() { keyButtons.clear(); }

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
            char keyChar = rowKeys[col]; // Felirat tárolása a char tömbben
            if (keyLabelCount < 50) {
                keyLabelStorage[keyLabelCount][0] = keyChar;
                keyLabelStorage[keyLabelCount][1] = '\0';
                DEBUG("VirtualKeyboardDialog: Creating button '%c' at (%d,%d) size %dx%d\n", keyChar, currentX, currentY, KEY_WIDTH, KEY_HEIGHT);

                auto keyButton = std::make_shared<UIButton>(         //
                    tft,                                             //
                    buttonId++,                                      //
                    Rect(currentX, currentY, KEY_WIDTH, KEY_HEIGHT), //
                    keyLabelStorage[keyLabelCount],                  //
                    UIButton::ButtonType::Pushable,                  //
                    [this, keyChar](const UIButton::ButtonEvent &event) {
                        DEBUG("VirtualKeyboardDialog: Button callback called for key: %c, state: %d\n", keyChar, (int)event.state);
                        if (event.state == UIButton::EventButtonState::Clicked) {
                            DEBUG("VirtualKeyboardDialog: Calling handleKeyPress for key: %c\n", keyChar);
                            handleKeyPress(keyChar);
                        }
                    });

                keyButtons.push_back(keyButton);
                addChild(keyButton);
                keyLabelCount++;
            }

            currentX += KEY_WIDTH + KEY_SPACING;
        }
    } // Speciális gombok létrehozása az utolsó sor alatt
    uint16_t specialY = startY + KEYBOARD_ROWS * (KEY_HEIGHT + KEY_SPACING) + 5;

    // Speciális gombok méretei
    uint16_t shiftWidth = 45;
    uint16_t spaceWidth = 80;
    uint16_t backspaceWidth = 40;
    uint16_t clearWidth = 40;
    uint16_t specialSpacing = 5;

    // Teljes sor szélessége
    uint16_t specialRowWidth = shiftWidth + spaceWidth + backspaceWidth + clearWidth + (3 * specialSpacing); // Középre igazítás
    uint16_t specialStartX = keyboardRect.x + (keyboardRect.width - specialRowWidth) / 2;

    DEBUG("VirtualKeyboardDialog: Special row - specialStartX=%d, specialY=%d\n", specialStartX, specialY);
    DEBUG("VirtualKeyboardDialog: Shift button bounds: x=%d, y=%d, w=%d, h=%d\n", specialStartX, specialY, shiftWidth, KEY_HEIGHT);

    // Shift gomb
    shiftButton = std::make_shared<UIButton>(                  //
        tft,                                                   //
        buttonId++,                                            //
        Rect(specialStartX, specialY, shiftWidth, KEY_HEIGHT), //
        "Shift",                                               //
        UIButton::ButtonType::Toggleable,                      //
        UIButton::ButtonState::Off,                            //
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::On || event.state == UIButton::EventButtonState::Off) {
                shiftActive = !shiftActive;
                DEBUG("VirtualKeyboardDialog: Shift button clicked, shiftActive now: %s\n", shiftActive ? "true" : "false");

                // Gomb állapot beállítása
                shiftButton->setButtonState(shiftActive ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
                DEBUG("VirtualKeyboardDialog: Shift button state set to: %s\n", shiftActive ? "On" : "Off");

                updateButtonLabels();
                markForRedraw(); // Dialógus újrarajzolása
            }
        });
    addChild(shiftButton);

    // Space gomb
    uint16_t spaceX = specialStartX + shiftWidth + specialSpacing;
    DEBUG("VirtualKeyboardDialog: Space button bounds: x=%d, y=%d, w=%d, h=%d\n", spaceX, specialY, spaceWidth, KEY_HEIGHT);

    spaceButton = std::make_shared<UIButton>(           //
        tft,                                            //
        buttonId++,                                     //
        Rect(spaceX, specialY, spaceWidth, KEY_HEIGHT), //
        "Space",                                        //
        UIButton::ButtonType::Pushable, [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                handleKeyPress(' ');
            }
        });
    addChild(spaceButton);

    // Backspace gomb
    uint16_t backspaceX = specialStartX + shiftWidth + spaceWidth + (2 * specialSpacing);
    DEBUG("VirtualKeyboardDialog: Backspace button bounds: x=%d, y=%d, w=%d, h=%d\n", backspaceX, specialY, backspaceWidth, KEY_HEIGHT);

    backspaceButton = std::make_shared<UIButton>(               //
        tft,                                                    //
        buttonId++,                                             //
        Rect(backspaceX, specialY, backspaceWidth, KEY_HEIGHT), //
        "<--",                                                  //
        UIButton::ButtonType::Pushable,                         //
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                handleSpecialKey("backspace");
            }
        });
    addChild(backspaceButton);

    // Clear gomb
    uint16_t clearX = specialStartX + shiftWidth + spaceWidth + backspaceWidth + (3 * specialSpacing);
    DEBUG("VirtualKeyboardDialog: Clear button bounds: x=%d, y=%d, w=%d, h=%d\n", clearX, specialY, clearWidth, KEY_HEIGHT);

    clearButton = std::make_shared<UIButton>( //
        tft,                                  //
        buttonId++,                           //
        Rect(clearX, specialY, clearWidth, KEY_HEIGHT),
        "Clr",                          //
        UIButton::ButtonType::Pushable, //
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                handleSpecialKey("clear");
            }
        });
    addChild(clearButton);

    // OK és Cancel gombok a speciális sor alatt
    uint16_t okCancelY = specialY + KEY_HEIGHT + 8;
    uint16_t okCancelWidth = 60;
    uint16_t okCancelStartX = keyboardRect.x + (keyboardRect.width - 2 * okCancelWidth - 10) / 2;

    // OK gomb
    auto okButton = std::make_shared<UIButton>(                     //
        tft,                                                        //
        buttonId++,                                                 //
        Rect(okCancelStartX, okCancelY, okCancelWidth, KEY_HEIGHT), //
        "OK",                                                       //
        UIButton::ButtonType::Pushable,                             //
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                close(UIDialogBase::DialogResult::Accepted);
            }
        });
    addChild(okButton);

    // Cancel gomb
    auto cancelButton = std::make_shared<UIButton>(                                      //
        tft,                                                                             //
        buttonId++,                                                                      //
        Rect(okCancelStartX + okCancelWidth + 10, okCancelY, okCancelWidth, KEY_HEIGHT), //
        "Cancel",                                                                        //
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
    DEBUG("VirtualKeyboardDialog::drawInputField - drawing text: '%s'\n", currentText.c_str());
    DEBUG("VirtualKeyboardDialog::drawInputField - input rect: x=%d, y=%d, w=%d, h=%d\n", inputRect.x, inputRect.y, inputRect.width, inputRect.height);

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

// Touch kezelés debug verzió
bool VirtualKeyboardDialog::handleTouch(const TouchEvent &event) {
    DEBUG("VirtualKeyboardDialog::handleTouch called at (%d,%d) pressed=%d\n", event.x, event.y, event.pressed);

    // Először próbáljuk a szülő osztály touch kezelését (pl. close gomb, gyerek komponensek)
    bool handled = UIDialogBase::handleTouch(event);
    DEBUG("VirtualKeyboardDialog::handleTouch - UIDialogBase handled: %d\n", handled);

    return handled;
}

void VirtualKeyboardDialog::handleKeyPress(char key) {
    DEBUG("VirtualKeyboardDialog::handleKeyPress called with key: %c\n", key);

    if (currentText.length() >= maxTextLength) {
        DEBUG("VirtualKeyboardDialog::handleKeyPress - max length reached\n");
        return; // Max hossz elérve
    }

    char actualKey = getKeyChar(key, shiftActive);
    currentText += actualKey;
    DEBUG("VirtualKeyboardDialog::handleKeyPress - text now: %s\n", currentText.c_str());

    // Shift automatikus kikapcsolása betű után
    if (shiftActive && isalpha(key)) {
        shiftActive = false;
        shiftButton->setButtonState(UIButton::ButtonState::Off);
        updateButtonLabels();
    }

    notifyTextChanged();
    markForRedraw();

    // Azonnali újrarajzolás az input mezőnek
    DEBUG("VirtualKeyboardDialog::handleKeyPress - forcing input field redraw\n");
    drawInputField();
}

void VirtualKeyboardDialog::handleSpecialKey(const String &keyType) {
    if (keyType == "backspace") {
        if (currentText.length() > 0) {
            currentText.remove(currentText.length() - 1);
            notifyTextChanged();
            markForRedraw();

            // Azonnali újrarajzolás az input mezőnek
            DEBUG("VirtualKeyboardDialog::handleSpecialKey(backspace) - forcing input field redraw\n");
            drawInputField();
        }
    } else if (keyType == "clear") {
        if (currentText.length() > 0) {
            currentText = "";
            notifyTextChanged();
            markForRedraw();

            // Azonnali újrarajzolás az input mezőnek
            DEBUG("VirtualKeyboardDialog::handleSpecialKey(clear) - forcing input field redraw\n");
            drawInputField();
        }
    }
}

void VirtualKeyboardDialog::updateButtonLabels() {
    DEBUG("VirtualKeyboardDialog::updateButtonLabels - shiftActive: %s\n", shiftActive ? "true" : "false");
    for (size_t i = 0; i < keyButtons.size() && i < keyLabelCount; i++) {
        const char *currentLabel = keyButtons[i]->getLabel();
        if (strlen(currentLabel) == 1) {
            char baseChar = currentLabel[0];
            char newChar = getKeyChar(baseChar, shiftActive);
            keyLabelStorage[i][0] = newChar;
            keyLabelStorage[i][1] = '\0';
            keyButtons[i]->setLabel(keyLabelStorage[i]);
            DEBUG("VirtualKeyboardDialog::updateButtonLabels - button %zu: '%c' -> '%c'\n", i, baseChar, newChar);
        }
    }
}

char VirtualKeyboardDialog::getKeyChar(char baseChar, bool shifted) {
    DEBUG("VirtualKeyboardDialog::getKeyChar - baseChar: '%c', shifted: %s\n", baseChar, shifted ? "true" : "false");

    if (isalpha(baseChar)) {
        char result = shifted ? toupper(baseChar) : tolower(baseChar);
        DEBUG("VirtualKeyboardDialog::getKeyChar - alpha result: '%c'\n", result);
        return result;
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

    // Azonnali újrarajzolás az input mezőnek
    DEBUG("VirtualKeyboardDialog::setText - forcing input field redraw\n");
    drawInputField();
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
