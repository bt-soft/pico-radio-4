/**
 * @file ValueChangeDialog.cpp
 * @brief Érték módosító dialógus osztály implementációja
 *
 * @version 1.0
 * @date 2025.06.01
 */

#include "ValueChangeDialog.h"
#include "UIColorPalette.h"
#include "UIScreen.h"
#include "defines.h"

/**
 * @brief Konstruktor integer értékhez
 * @param parentScreen Szülő képernyő referencia
 * @param tft TFT display referencia
 * @param title Dialógus címe
 * @param message Megjelenítendő üzenet
 * @param valuePtr Pointer az integer értékre
 * @param minValue Minimális érték
 * @param maxValue Maximális érték
 * @param stepValue Lépésköz
 * @param callback Callback függvény az érték változáskor
 * @param bounds Dialógus pozíciója és mérete
 * @param cs Színséma
 */
ValueChangeDialog::ValueChangeDialog(UIScreen *parentScreen, TFT_eSPI &tft, const char *title, const char *message, int *valuePtr, int minValue, int maxValue, int stepValue,
                                     ValueChangeCallback callback, const Rect &bounds, const ColorScheme &cs)
    : MessageDialog(parentScreen, tft, bounds, title, message, MessageDialog::ButtonsType::OkCancel, cs, true /*okClosesDialog*/), _valueType(ValueType::Integer),
      _intPtr(valuePtr), _minInt(minValue), _maxInt(maxValue), _stepInt(stepValue), _valueCallback(callback) { // Eredeti érték mentése
    if (_intPtr) {
        _originalIntValue = *_intPtr;
    }

    createDialogContent();
    layoutDialogContent();

    // Callback beállítása az OK/Cancel események kezelésére
    setDialogCallback([this](MessageDialog::DialogResult result) {
        if (result == MessageDialog::DialogResult::Accepted) {
            notifyValueChange();
        } else if (result == MessageDialog::DialogResult::Rejected) {
            restoreOriginalValue();
        }
        // A MessageDialog maga kezeli a close() hívást az _okClosesDialog alapján
    });

    // Kezdeti gombállapotok beállítása integer típushoz
    if (_decreaseButton)
        _decreaseButton->setEnabled(canDecrement());
    if (_increaseButton)
        _increaseButton->setEnabled(canIncrement());
}

/**
 * @brief Konstruktor float értékhez
 * @param parentScreen Szülő képernyő referencia
 * @param tft TFT display referencia
 * @param title Dialógus címe
 * @param message Megjelenítendő üzenet
 * @param valuePtr Pointer a float értékre
 * @param minValue Minimális érték
 * @param maxValue Maximális érték
 * @param stepValue Lépésköz
 * @param callback Callback függvény az érték változáskor
 * @param bounds Dialógus pozíciója és mérete
 * @param cs Színséma
 */
ValueChangeDialog::ValueChangeDialog(UIScreen *parentScreen, TFT_eSPI &tft, const char *title, const char *message, float *valuePtr, float minValue, float maxValue,
                                     float stepValue, ValueChangeCallback callback, const Rect &bounds, const ColorScheme &cs)
    : MessageDialog(parentScreen, tft, bounds, title, message, MessageDialog::ButtonsType::OkCancel, cs, true /*okClosesDialog*/), _valueType(ValueType::Float),
      _floatPtr(valuePtr), _minFloat(minValue), _maxFloat(maxValue), _stepFloat(stepValue), _valueCallback(callback) {

    // Eredeti érték mentése
    if (_floatPtr) {
        _originalFloatValue = *_floatPtr;
    }

    createDialogContent();
    layoutDialogContent();

    // Callback beállítása az OK/Cancel események kezelésére
    setDialogCallback([this](MessageDialog::DialogResult result) {
        if (result == MessageDialog::DialogResult::Accepted) {
            notifyValueChange();
        } else if (result == MessageDialog::DialogResult::Rejected) {
            restoreOriginalValue();
        }
    });

    // Kezdeti gombállapotok beállítása float típushoz
    if (_decreaseButton)
        _decreaseButton->setEnabled(canDecrement());
    if (_increaseButton)
        _increaseButton->setEnabled(canIncrement());
}

/**
 * @brief Konstruktor boolean értékhez
 * @param parentScreen Szülő képernyő referencia
 * @param tft TFT display referencia
 * @param title Dialógus címe
 * @param message Megjelenítendő üzenet
 * @param valuePtr Pointer a boolean értékre
 * @param callback Callback függvény az érték változáskor
 * @param bounds Dialógus pozíciója és mérete
 * @param cs Színséma
 */
ValueChangeDialog::ValueChangeDialog(UIScreen *parentScreen, TFT_eSPI &tft, const char *title, const char *message, bool *valuePtr, ValueChangeCallback callback,
                                     const Rect &bounds, const ColorScheme &cs)
    : MessageDialog(parentScreen, tft, bounds, title, message, MessageDialog::ButtonsType::OkCancel, cs, true /*okClosesDialog*/), _valueType(ValueType::Boolean),
      _boolPtr(valuePtr), _valueCallback(callback) {

    // Eredeti érték mentése
    if (_boolPtr) {
        _originalBoolValue = *_boolPtr;
    }

    createDialogContent();
    layoutDialogContent();

    // Callback beállítása az OK/Cancel események kezelésére
    setDialogCallback([this](MessageDialog::DialogResult result) {
        if (result == MessageDialog::DialogResult::Accepted) {
            notifyValueChange();
        } else if (result == MessageDialog::DialogResult::Rejected) {
            restoreOriginalValue();
        }
    });

    // Boolean gombok kezdeti állapotának beállítása
    updateBooleanButtonStates();
}

/**
 * @brief Érték módosító gombok létrehozása
 * Létrehozza a +/- gombokat integer és float típusokhoz, illetve TRUE/FALSE gombokat boolean típushoz.
 * A gombok eseménykezelői frissítik az értéket és újrarajzolják az érték területet.
 * A boolean típus esetén a gombok TRUE/FALSE értékeket állítanak be, és frissítik az állapotukat.
 */
void ValueChangeDialog::createDialogContent() {
    // Az OK és Cancel gombokat a MessageDialog ősosztály hozza létre.
    // Itt csak az érték-specifikus gombokat kell létrehozni.

    // Érték módosító gombok (csak integer és float esetén)
    if (_valueType != ValueType::Boolean) {
        // Csökkentő gomb (-)
        _decreaseButton =
            std::make_shared<UIButton>(tft, 3, Rect(0, 0, SMALL_BUTTON_WIDTH, BUTTON_HEIGHT), "-", UIButton::ButtonType::Pushable, [this](const UIButton::ButtonEvent &event) {
                if (event.state == UIButton::EventButtonState::Clicked && canDecrement()) {
                    decrementValue();
                    redrawValueArea(); // Csak az érték területet rajzoljuk újra
                }
            });
        _decreaseButton->setUseMiniFont(true);
        addChild(_decreaseButton);

        // Növelő gomb (+)
        _increaseButton =
            std::make_shared<UIButton>(tft, 4, Rect(0, 0, SMALL_BUTTON_WIDTH, BUTTON_HEIGHT), "+", UIButton::ButtonType::Pushable, [this](const UIButton::ButtonEvent &event) {
                if (event.state == UIButton::EventButtonState::Clicked && canIncrement()) {
                    incrementValue();
                    redrawValueArea(); // Csak az érték területet rajzoljuk újra
                }
            });
        _increaseButton->setUseMiniFont(true);
        addChild(_increaseButton);

    } else {
        // Boolean esetén FALSE/TRUE gombok létrehozása
        _decreaseButton = std::make_shared<UIButton>(tft, 3, Rect(0, 0, SMALL_BUTTON_WIDTH + 10, BUTTON_HEIGHT), "FALSE", UIButton::ButtonType::Pushable,
                                                     [this](const UIButton::ButtonEvent &event) {
                                                         if (event.state == UIButton::EventButtonState::Clicked && _boolPtr && *_boolPtr) {
                                                             *_boolPtr = false;           // FALSE-ra állítás
                                                             updateBooleanButtonStates(); // Gombok állapotának frissítése
                                                             redrawValueTextOnly();       // Csak az érték szöveg frissítése
                                                         }
                                                     });
        _decreaseButton->setUseMiniFont(true);
        addChild(_decreaseButton);

        _increaseButton = std::make_shared<UIButton>(tft, 4, Rect(0, 0, SMALL_BUTTON_WIDTH + 10, BUTTON_HEIGHT), "TRUE", UIButton::ButtonType::Pushable,
                                                     [this](const UIButton::ButtonEvent &event) {
                                                         if (event.state == UIButton::EventButtonState::Clicked && _boolPtr && !*_boolPtr) {
                                                             *_boolPtr = true;            // TRUE-ra állítás
                                                             updateBooleanButtonStates(); // Gombok állapotának frissítése
                                                             redrawValueTextOnly();       // Csak az érték szöveg frissítése
                                                         }
                                                     });
        _increaseButton->setUseMiniFont(true);
        addChild(_increaseButton);
    }
}

/**
 * @brief Dialógus tartalom elrendezése
 * Elrendezi a gombokat és az érték megjelenítését a dialógusban
 * A gombok a dialógus alján, az érték pedig a dialógus közepén jelenik meg.
 * A gombok elrendezése a MessageDialog ősosztály által kezelt ButtonsGroupManager segítségével történik.
 */
void ValueChangeDialog::layoutDialogContent() {
    const Rect contentBounds = bounds;
    const int16_t centerX = contentBounds.x + contentBounds.width / 2;

    // Az OK és Cancel gombokat a MessageDialog ősosztály rendezi el a ButtonsGroupManager segítségével, a dialógus aljára.
    // Itt csak az érték-specifikus gombokat kell elrendezni.

    const int16_t headerHeight = getHeaderHeight();
    const int16_t valueAreaY = contentBounds.y + headerHeight + PADDING + VERTICAL_OFFSET_FOR_VALUE_AREA;

    if (_valueType != ValueType::Boolean) {
        // +/- gombok az érték körül - UGYANAZON A VONALON
        const int16_t valueBoxWidth = 100;
        const int16_t valueButtonSpacing = 10;
        const int16_t totalWidth = 2 * SMALL_BUTTON_WIDTH + 2 * valueButtonSpacing + valueBoxWidth;
        const int16_t startX2 = centerX - totalWidth / 2;

        _decreaseButton->setBounds(Rect(startX2, valueAreaY, SMALL_BUTTON_WIDTH, BUTTON_HEIGHT));
        _increaseButton->setBounds(Rect(startX2 + SMALL_BUTTON_WIDTH + 2 * valueButtonSpacing + valueBoxWidth, valueAreaY, SMALL_BUTTON_WIDTH, BUTTON_HEIGHT));
    } else {
        // Boolean TRUE/FALSE gombok az érték körül
        const int16_t valueBoxWidth = 100;
        const int16_t valueButtonSpacing = 10;
        const int16_t boolButtonWidth = SMALL_BUTTON_WIDTH + 10;
        const int16_t totalWidth = 2 * boolButtonWidth + 2 * valueButtonSpacing + valueBoxWidth;
        const int16_t startX2 = centerX - totalWidth / 2;

        _decreaseButton->setBounds(Rect(startX2, valueAreaY, boolButtonWidth, BUTTON_HEIGHT));                                                            // FALSE
        _increaseButton->setBounds(Rect(startX2 + boolButtonWidth + 2 * valueButtonSpacing + valueBoxWidth, valueAreaY, boolButtonWidth, BUTTON_HEIGHT)); // TRUE
    }
}

/**
 * @brief Dialógus teljes kirajzolása
 * Kirajzolja a dialógus keretét, üzenetet és az aktuális értéket
 */
void ValueChangeDialog::drawSelf() {
    // 1. Az UIDialogBase kirajzolja a keretet és a fejlécet.
    UIDialogBase::drawSelf();

    const Rect contentBounds = bounds;
    const int16_t centerX = contentBounds.x + contentBounds.width / 2;
    const int16_t headerHeight = getHeaderHeight();

    // 2. Az üzenet szövegének kirajzolása (a MessageDialog-tól örökölt 'message' tagváltozó alapján)
    // Az üzenetet a fejléc alá, középre igazítva rajzoljuk.
    if (this->message) {                                                        // Ellenőrizzük, hogy van-e üzenet
        const int16_t messageY = contentBounds.y + headerHeight + PADDING + 10; // Üzenet teteje 10 pixellel a fejléc+padding alatt
        tft.setFreeFont(&FreeSansBold9pt7b);
        tft.setTextSize(1);
        tft.setTextColor(colors.foreground, colors.background); // A dialógus általános színeit használjuk
        tft.setTextDatum(TC_DATUM);                             // Top-Center igazítás
        // Szélesség korlátozása a szövegnek, hogy ne lógjon ki
        // uint16_t messageMaxWidth = contentBounds.width - (2 * (PADDING + 5)); // 5px extra margó mindkét oldalon
        // TODO: Szükség esetén szövegtördelés implementálása, ha a szöveg túl hosszú
        tft.drawString(this->message, centerX, messageY); // A font már be van állítva a setFreeFont hívással
    }

    // Aktuális érték megjelenítése - gombok szintjében
    const int16_t valueAreaY = contentBounds.y + headerHeight + PADDING + VERTICAL_OFFSET_FOR_VALUE_AREA; // Ugyanaz mint layoutban
    const int16_t valueY = valueAreaY + BUTTON_HEIGHT / 2;                                                // Gomb közepén

    // Érték háttér nélküli megjelenítés - csak nagy szöveg
    String valueStr = getCurrentValueAsString(); // Színválasztás: teal ha eredeti érték, különben fehér
    uint16_t textColor = UIColorPalette::SCREEN_TEXT;
    if (isCurrentValueOriginal()) {
        textColor = TFT_CYAN; // Teal színű az eredeti érték
    }

    tft.setFreeFont(&FreeSansBold9pt7b); // Biztosítjuk a helyes betűtípust az értékhez
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(textColor, colors.background); // A dialógus hátterét használjuk
    tft.setTextSize(VALUE_TEXT_FONT_SIZE);
    tft.drawString(valueStr, centerX, valueY);
}

/**
 * @brief Forgójeladó események kezelése
 * Kezeli a forgatást (érték növelés/csökkentés) és a gombnyomást (OK művelet)
 * @param event A forgójeladó esemény
 * @return true ha az eseményt kezelte, false egyébként
 */
bool ValueChangeDialog::handleRotary(const RotaryEvent &event) {
    // Érték változtatás kezelése görgetéssel
    if (event.direction == RotaryEvent::Direction::Up) {
        if (canIncrement()) {
            incrementValue();
            redrawValueArea(); // Csak az érték területet rajzoljuk újra
        }
        return true;
    } else if (event.direction == RotaryEvent::Direction::Down) {
        if (canDecrement()) {
            decrementValue();
            redrawValueArea(); // Csak az érték területet rajzoljuk újra
        }
        return true;
    }

    // Ha a forgatógombot megnyomták (Clicked), azt a MessageDialog (UIDialogBase) kezeli
    // "OK"-ként, ami meghívja a setDialogCallback-ben beállított logikánkat.
    // Ezért itt csak a görgetést kezeljük, a többit az ősosztályra bízzuk.
    return MessageDialog::handleRotary(event);
}

/**
 * @brief Aktuális érték szöveges formában
 * @return Az aktuális érték string reprezentációja
 */
String ValueChangeDialog::getCurrentValueAsString() const {
    switch (_valueType) {
    case ValueType::Integer:
        return _intPtr ? String(*_intPtr) : "N/A";
    case ValueType::Float:
        return _floatPtr ? String(*_floatPtr, 2) : "N/A";
    case ValueType::Boolean:
        return _boolPtr ? (*_boolPtr ? "True" : "False") : "N/A";
    default:
        return "Error";
    }
}

/**
 * @brief Érték növelése a lépésközzel
 * Növeli az értéket a megadott lépésközzel, ha nem lépi túl a maximumot
 */
void ValueChangeDialog::incrementValue() {
    switch (_valueType) {
    case ValueType::Integer:
        if (_intPtr && (*_intPtr + _stepInt) <= _maxInt) {
            *_intPtr += _stepInt;
        }
        break;
    case ValueType::Float:
        if (_floatPtr && (*_floatPtr + _stepFloat) <= _maxFloat) {
            *_floatPtr += _stepFloat;
        }
        break;
    case ValueType::Boolean:
        if (_boolPtr && !(*_boolPtr)) {
            *_boolPtr = true;
        }
        break;
    }
}

/**
 * @brief Érték csökkentése a lépésközzel
 * Csökkenti az értéket a megadott lépésközzel, ha nem megy a minimum alá
 */
void ValueChangeDialog::decrementValue() {
    switch (_valueType) {
    case ValueType::Integer:
        if (_intPtr && (*_intPtr - _stepInt) >= _minInt) {
            *_intPtr -= _stepInt;
        }
        break;
    case ValueType::Float:
        if (_floatPtr && (*_floatPtr - _stepFloat) >= _minFloat) {
            *_floatPtr -= _stepFloat;
        }
        break;
    case ValueType::Boolean:
        if (_boolPtr && (*_boolPtr)) {
            *_boolPtr = false;
        }
        break;
    }
}

/**
 * @brief Eredeti érték visszaállítása (Cancel esetén)
 * Visszaállítja az értéket a dialógus megnyitásakori eredeti értékre
 */
void ValueChangeDialog::restoreOriginalValue() {
    switch (_valueType) {
    case ValueType::Integer:
        if (_intPtr) {
            *_intPtr = _originalIntValue;
        }
        break;
    case ValueType::Float:
        if (_floatPtr) {
            *_floatPtr = _originalFloatValue;
        }
        break;
    case ValueType::Boolean:
        if (_boolPtr) {
            *_boolPtr = _originalBoolValue;
        }
        break;
    }
}

/**
 * @brief Érték validálása és határok közé szorítása
 * Ellenőrzi és korrigálja az értéket, ha kívül esik a megengedett tartományon
 */
void ValueChangeDialog::validateAndClampValue() {
    switch (_valueType) {
    case ValueType::Integer:
        if (_intPtr) {
            if (*_intPtr < _minInt)
                *_intPtr = _minInt;
            if (*_intPtr > _maxInt)
                *_intPtr = _maxInt;
        }
        break;
    case ValueType::Float:
        if (_floatPtr) {
            if (*_floatPtr < _minFloat)
                *_floatPtr = _minFloat;
            if (*_floatPtr > _maxFloat)
                *_floatPtr = _maxFloat;
        }
        break;
    case ValueType::Boolean:
        // Boolean esetén nincs validáció szükséges
        break;
    }
}

/**
 * @brief Értesítés küldése az érték változásáról
 * Meghívja a callback függvényt az aktuális értékkel
 */
void ValueChangeDialog::notifyValueChange() {
    if (_valueCallback) {
        switch (_valueType) {
        case ValueType::Integer:
            if (_intPtr) {
                _valueCallback(std::variant<int, float, bool>(*_intPtr));
            }
            break;
        case ValueType::Float:
            if (_floatPtr) {
                _valueCallback(std::variant<int, float, bool>(*_floatPtr));
            }
            break;
        case ValueType::Boolean:
            if (_boolPtr) {
                _valueCallback(std::variant<int, float, bool>(*_boolPtr));
            }
            break;
        }
    }
}

/**
 * @brief Érték terület újrarajzolása
 * Újrarajzolja az érték szöveget és frissíti a gombok állapotát
 */
void ValueChangeDialog::redrawValueArea() {
    const Rect contentBounds = bounds;
    const int16_t centerX = contentBounds.x + contentBounds.width / 2;

    // Értékterület koordinátái
    const int16_t headerHeight = getHeaderHeight();
    const int16_t valueAreaY = contentBounds.y + headerHeight + PADDING + VERTICAL_OFFSET_FOR_VALUE_AREA;
    const int16_t valueY = valueAreaY + BUTTON_HEIGHT / 2; // Háttér törölése a régi érték helyén (csak az érték szöveg területe)
    const int16_t clearWidth = 120;                        // Kisebb, csak az érték szöveghez
    const int16_t clearHeight = 30;                        // Kisebb magasság
    const int16_t clearX = centerX - clearWidth / 2;
    const int16_t clearY = valueY - clearHeight / 2;

    tft.fillRect(clearX, clearY, clearWidth, clearHeight, colors.background);

    // Érték szöveg - nagyobb fonttal, színkóddal
    String valueStr = getCurrentValueAsString();

    // Színválasztás: teal ha eredeti érték, különben fehér
    uint16_t textColor = UIColorPalette::SCREEN_TEXT;
    tft.setFreeFont(&FreeSansBold9pt7b); // Biztosítjuk a helyes betűtípust
    if (isCurrentValueOriginal()) {
        textColor = TFT_CYAN; // Teal színű az eredeti érték
    }
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(textColor, colors.background);
    tft.setTextSize(VALUE_TEXT_FONT_SIZE);
    tft.drawString(valueStr, centerX, valueY);

    // Gombok állapotának frissítése típus alapján
    if (_valueType == ValueType::Boolean) {
        updateBooleanButtonStates();
    } else {
        // Frissítjük a +/- UIButton-ok enabled állapotát
        if (_decreaseButton)
            _decreaseButton->setEnabled(canDecrement());
        if (_increaseButton)
            _increaseButton->setEnabled(canIncrement());
    }
}

/**
 * @brief Ellenőrzi, hogy az aktuális érték megegyezik-e az eredetivel
 * @return true ha az aktuális érték megegyezik az eredetivel, false egyébként
 */
bool ValueChangeDialog::isCurrentValueOriginal() const {
    switch (_valueType) {
    case ValueType::Integer:
        return _intPtr && (*_intPtr == _originalIntValue);
    case ValueType::Float:
        return _floatPtr && (abs(*_floatPtr - _originalFloatValue) < 0.001f); // Float összehasonlítás toleranciával
    case ValueType::Boolean:
        return _boolPtr && (*_boolPtr == _originalBoolValue);
    default:
        return false;
    }
}

/**
 * @brief Ellenőrzi, hogy növelhető-e az érték
 * @return true ha az érték növelhető (nem éri el a maximumot), false egyébként
 */
bool ValueChangeDialog::canIncrement() const {
    switch (_valueType) {
    case ValueType::Integer:
        if (_intPtr) {
            bool result = (*_intPtr + _stepInt) <= _maxInt;
            return result;
        }
        return false;
    case ValueType::Float:
        if (_floatPtr) {
            bool result = (*_floatPtr + _stepFloat) <= _maxFloat;
            return result;
        }
        return false;
    case ValueType::Boolean:
        return _boolPtr && !(*_boolPtr); // Csak akkor növelhető (TRUE-ra), ha jelenleg FALSE
    default:
        return false;
    }
}

/**
 * @brief Ellenőrzi, hogy csökkenthető-e az érték
 * @return true ha az érték csökkenthető (nem éri el a minimumot), false egyébként
 */
bool ValueChangeDialog::canDecrement() const {
    switch (_valueType) {
    case ValueType::Integer:
        if (_intPtr) {
            bool result = (*_intPtr - _stepInt) >= _minInt;
            return result;
        }
        return false;
    case ValueType::Float:
        if (_floatPtr) {
            bool result = (*_floatPtr - _stepFloat) >= _minFloat;
            return result;
        }
        return false;
    case ValueType::Boolean:
        return _boolPtr && (*_boolPtr); // Csak akkor csökkenthető (FALSE-ra), ha jelenleg TRUE
    default:
        return false;
    }
}

/**
 * @brief Boolean gombok állapotának frissítése
 * Beállítja a TRUE/FALSE gombok engedélyezett/letiltott állapotát az aktuális érték alapján
 */
void ValueChangeDialog::updateBooleanButtonStates() {
    if (_valueType != ValueType::Boolean || !_boolPtr) {
        return;
    }

    // FALSE gomb állapota: engedélyezett ha jelenleg TRUE
    bool falseEnabled = *_boolPtr;
    if (_decreaseButton) {
        _decreaseButton->setEnabled(falseEnabled);
    }

    // TRUE gomb állapota: engedélyezett ha jelenleg FALSE
    bool trueEnabled = !*_boolPtr;
    if (_increaseButton) {
        _increaseButton->setEnabled(trueEnabled);
    }
}

/**
 * @brief Csak az érték szöveg újrarajzolása (optimalizált)
 * Újrarajzolja csak az érték szöveget anélkül, hogy a gombokat is frissítené
 */
void ValueChangeDialog::redrawValueTextOnly() {
    const Rect contentBounds = bounds;
    const int16_t centerX = contentBounds.x + contentBounds.width / 2;
    const int16_t headerHeight = getHeaderHeight();
    const int16_t valueAreaY = contentBounds.y + headerHeight + PADDING + VERTICAL_OFFSET_FOR_VALUE_AREA;
    const int16_t valueY = valueAreaY + BUTTON_HEIGHT / 2;

    // Csak az érték szöveg területének törlése és újrarajzolása
    const int16_t clearWidth = 120;
    const int16_t clearHeight = 30;
    const int16_t clearX = centerX - clearWidth / 2;
    const int16_t clearY = valueY - clearHeight / 2;
    tft.fillRect(clearX, clearY, clearWidth, clearHeight, colors.background);
    String valueStr = getCurrentValueAsString();
    uint16_t textColor = isCurrentValueOriginal() ? TFT_CYAN : UIColorPalette::SCREEN_TEXT;
    tft.setFreeFont(&FreeSansBold9pt7b); // Biztosítjuk a helyes betűtípust
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(textColor, colors.background);
    tft.setTextSize(VALUE_TEXT_FONT_SIZE);
    tft.drawString(valueStr, centerX, valueY);
}
