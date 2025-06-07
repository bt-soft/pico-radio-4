#include "MessageDialog.h"

/**
 * @brief MessageDialog konstruktor
 * @param parentScreen A szülő UIScreen, amely megjeleníti ezt a dialógust
 * @param tft TFT_eSPI referencia a rajzoláshoz
 * @param bounds A dialógus kezdeti határai (pozíció és méret)
 * @param title A dialógus címe (nullptr, ha nincs cím)
 * @param message Az üzenet szövege, amely megjelenik a dialógusban
 * @param buttonsType A gombok típusa (Ok, OkCancel, YesNo, YesNoCancel)
 * @param cs A dialógus színpalettája (alapértelmezett ColorScheme)
 * @details A dialógus automatikusan méreteződik, ha a bounds.height 0.
 *          A gombok típusa meghatározza, hogy milyen gombok jelennek meg a dialógusban.
 */
MessageDialog::MessageDialog(UIScreen *parentScreen, TFT_eSPI &tft, const Rect &bounds, const char *title, const char *message, ButtonsType buttonsType, const ColorScheme &cs)
    : UIDialogBase(parentScreen, tft, bounds, title, cs), message(message), buttonsType(buttonsType) {

    setDialogCallback(callback);

    // Dialógus tartalmának létrehozása és elrendezése
    createDialogContent();
    layoutDialogContent();

    // Ha a dialógus magassága 0 volt (automatikus méretezés kérése),
    // akkor a layoutDialogContent után frissíteni kellhet a bounds.height értékét.
    // Ez egy egyszerűsített példa, egy robusztusabb layout manager bonyolultabb lenne.
    // Most, hogy nincs _messageLabel, a magasság becslése nehezebb előre.
    // A layoutDialogContent-ben megpróbáljuk megbecsülni a szöveg magasságát.
    if (bounds.height == 0) {
        tft.setTextSize(1);                        // Üzenet szövegmérete
        int16_t textHeight = tft.fontHeight() * 2; // Durva becslés 2 sorra (állítható)
        // TODO: Pontosabb szövegmagasság számítás a layoutDialogContent-ben
        uint16_t requiredHeight = getHeaderHeight() + PADDING + textHeight + PADDING + UIButton::DEFAULT_BUTTON_HEIGHT + PADDING; // Gombmagasság konstanssal
        if (this->bounds.height < requiredHeight) {                                                                               // Csak akkor növeljük, ha tényleg kisebb
            Rect newBounds = this->bounds;
            newBounds.height = requiredHeight;
            // Ha középre volt igazítva, újra kell pozícionálni vertikálisan
            if (bounds.y == -1) {
                newBounds.y = (tft.height() - newBounds.height) / 2;
            }
            setBounds(newBounds);
            // Újra kell rendezni az elemeket az új méretekhez
            layoutDialogContent();
        }
    }

    markForRedraw();
}

/**
 * @brief Létrehozza a dialógus tartalmát, beleértve a gombokat.
 * @details A gombok típusa alapján hozza létre a megfelelő gombokat és azok eseménykezelőit.
 */
void MessageDialog::createDialogContent() {
    _buttonDefs.clear();
    uint8_t buttonIdCounter = 1;

    switch (buttonsType) {
    case ButtonsType::Ok:
        _buttonDefs.push_back({buttonIdCounter++, "OK", UIButton::ButtonType::Pushable,
                               [this](const UIButton::ButtonEvent &event) {
                                   if (event.state == UIButton::EventButtonState::Clicked)
                                       close(DialogResult::Accepted);
                               },
                               UIButton::ButtonState::Off, 0, UIButton::DEFAULT_BUTTON_HEIGHT});
        break;
    case ButtonsType::OkCancel:
        _buttonDefs.push_back({buttonIdCounter++, "OK", UIButton::ButtonType::Pushable,
                               [this](const UIButton::ButtonEvent &event) {
                                   if (event.state == UIButton::EventButtonState::Clicked)
                                       close(DialogResult::Accepted);
                               },
                               UIButton::ButtonState::Off, 0, UIButton::DEFAULT_BUTTON_HEIGHT});
        _buttonDefs.push_back({buttonIdCounter++, "Cancel", UIButton::ButtonType::Pushable,
                               [this](const UIButton::ButtonEvent &event) {
                                   if (event.state == UIButton::EventButtonState::Clicked)
                                       close(DialogResult::Rejected);
                               },
                               UIButton::ButtonState::Off, 0, UIButton::DEFAULT_BUTTON_HEIGHT});
        break;
    case ButtonsType::YesNo:
        _buttonDefs.push_back({buttonIdCounter++, "Yes", UIButton::ButtonType::Pushable,
                               [this](const UIButton::ButtonEvent &event) {
                                   if (event.state == UIButton::EventButtonState::Clicked)
                                       close(DialogResult::Accepted);
                               },
                               UIButton::ButtonState::Off, 0, UIButton::DEFAULT_BUTTON_HEIGHT});
        _buttonDefs.push_back({buttonIdCounter++, "No", UIButton::ButtonType::Pushable,
                               [this](const UIButton::ButtonEvent &event) {
                                   if (event.state == UIButton::EventButtonState::Clicked)
                                       close(DialogResult::Rejected);
                               },
                               UIButton::ButtonState::Off, 0, UIButton::DEFAULT_BUTTON_HEIGHT});
        break;
    case ButtonsType::YesNoCancel:
        _buttonDefs.push_back({buttonIdCounter++, "Yes", UIButton::ButtonType::Pushable,
                               [this](const UIButton::ButtonEvent &event) {
                                   if (event.state == UIButton::EventButtonState::Clicked)
                                       close(DialogResult::Accepted);
                               },
                               UIButton::ButtonState::Off, 0, UIButton::DEFAULT_BUTTON_HEIGHT});
        _buttonDefs.push_back({buttonIdCounter++, "No", UIButton::ButtonType::Pushable,
                               [this](const UIButton::ButtonEvent &event) {
                                   if (event.state == UIButton::EventButtonState::Clicked)
                                       close(DialogResult::Rejected);
                               },
                               UIButton::ButtonState::Off, 0, UIButton::DEFAULT_BUTTON_HEIGHT});
        _buttonDefs.push_back({buttonIdCounter++, "Cancel", UIButton::ButtonType::Pushable,
                               [this](const UIButton::ButtonEvent &event) {
                                   if (event.state == UIButton::EventButtonState::Clicked)
                                       close(DialogResult::Dismissed);
                               },
                               UIButton::ButtonState::Off, 0, UIButton::DEFAULT_BUTTON_HEIGHT});
        break;
    }
}

/**
 * @brief Elrendezi a dialógus gombjait a megadott elrendezési szabályok szerint.
 */
void MessageDialog::layoutDialogContent() {
    // Korábbi gombok eltávolítása
    for (const auto &btn : _buttonsList) {
        removeChild(btn);
    }
    _buttonsList.clear();

    if (_buttonDefs.empty()) {
        markForRedraw();
        return;
    }

    uint16_t numButtons = _buttonDefs.size();
    uint16_t buttonHeight = UIButton::DEFAULT_BUTTON_HEIGHT; // Gomb magassága

    // Gombok szélességének kiszámítása, hogy kitöltsék a helyet
    uint16_t contentWidthForButtons = bounds.width - (2 * UIDialogBase::PADDING);
    if (bounds.width <= (2 * UIDialogBase::PADDING))
        contentWidthForButtons = 0;

    uint16_t singleButtonWidth = 0;
    if (numButtons > 0) {
        if (numButtons == 1) {
            singleButtonWidth = contentWidthForButtons;
        } else {
            uint16_t totalInterButtonPadding = (numButtons - 1) * UIDialogBase::PADDING;
            if (contentWidthForButtons > totalInterButtonPadding) {
                singleButtonWidth = (contentWidthForButtons - totalInterButtonPadding) / numButtons;
            } else {
                singleButtonWidth = 10; // Minimális szélesség, ha nincs elég hely
            }
        }
    }
    if (singleButtonWidth < 10 && numButtons > 0)
        singleButtonWidth = 10; // Minimális szélesség

    // Frissítjük a _buttonDefs-ben a szélességeket
    for (auto &def : _buttonDefs) {
        def.width = singleButtonWidth;
        def.height = buttonHeight; // Biztosítjuk a magasságot is
    }

    // Margók kiszámítása a ButtonsGroupManager számára (képernyő-relatív)
    // A gombok a dialógus alján lesznek.
    int16_t manager_marginLeft = bounds.x + UIDialogBase::PADDING;
    int16_t manager_marginRight = tft.width() - (bounds.x + bounds.width - UIDialogBase::PADDING);
    // A marginBottom azt jelenti, hogy a gombok alja milyen messze van a képernyő aljától.
    int16_t manager_marginBottom = tft.height() - (bounds.y + bounds.height - UIDialogBase::PADDING);

    // Gombok elrendezése a ButtonsGroupManager segítségével
    layoutHorizontalButtonGroup(_buttonDefs, &_buttonsList, manager_marginLeft, manager_marginRight, manager_marginBottom,
                                UIButton::DEFAULT_BUTTON_WIDTH, // defaultButtonWidthRef (ha def.width=0 lenne)
                                buttonHeight,                   // defaultButtonHeightRef
                                UIDialogBase::PADDING,          // rowGap (egy sor esetén nem releváns)
                                UIDialogBase::PADDING           // buttonGap (gombok közötti rés)
    );

    markForRedraw();
}

/**
 * @brief Rajzolja a dialógus hátterét, fejlécét és az üzenetet.
 * @details A dialógus háttere és kerete rajzolódik, majd az üzenet szövege jelenik meg a középső területen.
 */
void MessageDialog::drawSelf() {
    UIDialogBase::drawSelf(); // Alap dialógus keret és fejléc rajzolása

    if (message) {
        tft.setTextSize(1);                                     // Vagy UI_DEFAULT_TEXT_SIZE
        tft.setTextColor(colors.foreground, colors.background); // Dialógus színeit használva

        uint16_t headerH = getHeaderHeight();
        // Hozzávetőleges magasság a gomb területének
        uint16_t buttonAreaH = UIButton::DEFAULT_BUTTON_HEIGHT + UIDialogBase::PADDING;

        Rect textArea;
        textArea.x = bounds.x + UIDialogBase::PADDING + 2; // Kis extra margó
        textArea.y = bounds.y + headerH + UIDialogBase::PADDING;
        textArea.width = bounds.width - (2 * UIDialogBase::PADDING) - 4;
        textArea.height = bounds.height - headerH - buttonAreaH - (2 * UIDialogBase::PADDING);

        if (textArea.width > 0 && textArea.height > 0) {
            // Szöveg tördelése és rajzolása (egyszerűsített, egy UILabel komponens jobb lenne)
            tft.setTextDatum(TC_DATUM); // Top-Center
            // TODO: Implement proper text wrapping if message is long
            // For now, drawString might clip or overflow.
            // A simple approach for multi-line if message has '\n':
            // Or use a UILabel component if available.
            // Centering the text in the available text area:
            int16_t textDrawY = textArea.y + textArea.height / 2;
            if (tft.fontHeight() > textArea.height) {          // Ha a szöveg magasabb, mint a terület
                textDrawY = textArea.y + tft.fontHeight() / 2; // Próbáljuk középre igazítani a szöveg tetejét
            }
            tft.drawString(message, textArea.x + textArea.width / 2, textDrawY);
        }
    }
}