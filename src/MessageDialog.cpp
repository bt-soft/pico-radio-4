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
MessageDialog::MessageDialog(UIScreen *parentScreen, TFT_eSPI &tft, const Rect &initialInputBounds, const char *title, const char *message, ButtonsType buttonsType,
                             const ColorScheme &cs, bool okClosesDialog)
    : UIDialogBase(parentScreen, tft, initialInputBounds, title, cs), message(message), buttonsType(buttonsType), _okClosesDialog(okClosesDialog) {

    // setDialogCallback(nullptr); // UIDialogBase már nullptr-re inicializálja

    // Dialógus tartalmának létrehozása és elrendezése
    createDialogContent(); // Előkészíti a _buttonDefs-et

    Rect currentBoundsAfterBase = this->bounds; // A bounds, amit az UIDialogBase konstruktora beállított
    Rect finalDialogBounds = currentBoundsAfterBase;
    bool boundsHaveChanged = false;

    // Automatikus magasság számítása, ha bounds.height == 0 volt az inputon
    // A UIDialogBase konstruktora már adhatott neki egy alapértelmezett magasságot.
    // Itt finomítjuk az üzenet és a gombok alapján.
    if (initialInputBounds.height == 0) {          // Az eredeti kérés volt auto-magasság
        tft.setTextSize(2);                        // Üzenet szövegmérete a magasság kalkulációhoz
        int16_t textHeight = tft.fontHeight() * 2; // Durva becslés 2 sorra (állítható)
        // TODO: Pontosabb szövegmagasság számítás a layoutDialogContent-ben
        uint16_t requiredHeight = getHeaderHeight() + PADDING + textHeight + PADDING + UIButton::DEFAULT_BUTTON_HEIGHT + (2 * PADDING); // Megduplázott alsó PADDING
        if (finalDialogBounds.height < requiredHeight) {
            finalDialogBounds.height = requiredHeight;
            boundsHaveChanged = true;
        }
    }

    // Középre igazítás, ha x vagy y -1 volt az inputon
    if (initialInputBounds.x == -1) {
        finalDialogBounds.x = (tft.width() - finalDialogBounds.width) / 2;
        boundsHaveChanged = true;
    }
    if (initialInputBounds.y == -1) {
        finalDialogBounds.y = (tft.height() - finalDialogBounds.height) / 2;
        boundsHaveChanged = true;
    }

    if (boundsHaveChanged) {
        // UIComponent::setBounds-et hívjuk, hogy a this->bounds frissüljön, és a dialógus újrarajzolásra legyen jelölve.
        // Ez nem hívja meg automatikusan a layoutDialogContent-et.
        UIComponent::setBounds(finalDialogBounds);
    }

    // A dialógus tartalmát (gombokat) mindig elrendezzük a véglegesített határok alapján.
    layoutDialogContent(); // Ez létrehozza és hozzáadja a gombokat, és markForRedraw-t hív.
    // Az UIComponent::setBounds már beállította a markForRedraw-t a dialógusra, ha a boundsHaveChanged igaz volt.
    // Ha nem, akkor a layoutDialogContent() hívja. Dupla hívás nem probléma.
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
                                   if (event.state == UIButton::EventButtonState::Clicked) {
                                       if (_okClosesDialog) {
                                           close(DialogResult::Accepted);
                                       } else {
                                           if (this->callback) { // UIDialogBase::callback
                                               this->callback(DialogResult::Accepted);
                                           }
                                       }
                                   }
                               },
                               UIButton::ButtonState::Off, 0, UIButton::DEFAULT_BUTTON_HEIGHT});
        break;
    case ButtonsType::OkCancel:
        _buttonDefs.push_back({buttonIdCounter++, "OK", UIButton::ButtonType::Pushable,
                               [this](const UIButton::ButtonEvent &event) {
                                   if (event.state == UIButton::EventButtonState::Clicked) {
                                       if (_okClosesDialog) {
                                           close(DialogResult::Accepted);
                                       } else {
                                           if (this->callback) {
                                               this->callback(DialogResult::Accepted);
                                           }
                                       }
                                   }
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
                                   if (event.state == UIButton::EventButtonState::Clicked) {
                                       if (_okClosesDialog) {
                                           close(DialogResult::Accepted);
                                       } else {
                                           if (this->callback) {
                                               this->callback(DialogResult::Accepted);
                                           }
                                       }
                                   }
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
                                   if (event.state == UIButton::EventButtonState::Clicked) {
                                       if (_okClosesDialog) {
                                           close(DialogResult::Accepted);
                                       } else {
                                           if (this->callback) {
                                               this->callback(DialogResult::Accepted);
                                           }
                                       }
                                   }
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

    // uint16_t numButtons = _buttonDefs.size(); // Erre már nincs szükség
    uint16_t buttonHeight = UIButton::DEFAULT_BUTTON_HEIGHT; // Gomb magassága

    // Frissítjük a _buttonDefs-ben a szélességeket
    for (auto &def : _buttonDefs) {
        // def.width = 0; // A createDialogContent-ben már 0-ra van állítva,
        // jelezve az auto-méretezést a ButtonsGroupManager számára.
        def.height = buttonHeight; // Biztosítjuk a magasságot is
    }

    // Margók kiszámítása a ButtonsGroupManager számára (képernyő-relatív)
    // A gombok a dialógus alján lesznek.
    int16_t manager_marginLeft = bounds.x + UIDialogBase::PADDING;
    int16_t manager_marginRight = tft.width() - (bounds.x + bounds.width - UIDialogBase::PADDING);
    // A marginBottom azt jelenti, hogy a gombok alja milyen messze van a képernyő aljától.
    int16_t manager_marginBottom = tft.height() - (bounds.y + bounds.height - (2 * UIDialogBase::PADDING)); // Megduplázott PADDING itt is

    // Gombok elrendezése a ButtonsGroupManager segítségével
    layoutHorizontalButtonGroup(_buttonDefs, &_buttonsList, manager_marginLeft, manager_marginRight, manager_marginBottom,
                                UIButton::DEFAULT_BUTTON_WIDTH, // defaultButtonWidthRef (ha def.width=0 lenne)
                                buttonHeight,                   // defaultButtonHeightRef
                                UIDialogBase::PADDING,          // rowGap (egy sor esetén nem releváns)
                                UIDialogBase::PADDING,          // buttonGap (gombok közötti rés)
                                true                            // centerHorizontally = true
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
        tft.setTextSize(2);                                     // Nagyobb betűméret az üzenetnek
        tft.setTextColor(colors.foreground, colors.background); // Dialógus színeit használva

        uint16_t headerH = getHeaderHeight();
        // Hozzávetőleges magasság a gomb területének
        Rect textArea;
        textArea.x = bounds.x + UIDialogBase::PADDING + 2; // Kis extra margó
        textArea.y = bounds.y + headerH + UIDialogBase::PADDING;
        textArea.width = bounds.width - (2 * (UIDialogBase::PADDING + 2)); // Szélességben is figyelembe vesszük a 2px extra margót
        textArea.height =
            bounds.height - headerH - UIButton::DEFAULT_BUTTON_HEIGHT - (4 * UIDialogBase::PADDING); // Header, PADDING_alatta, text, PADDING_alatta, gombok, 2*PADDING_alatta

        if (textArea.width > 0 && textArea.height > 0) {
            tft.setTextDatum(TC_DATUM); // Top-Center
            int16_t textDrawY = textArea.y + textArea.height / 2;
            if (tft.fontHeight() > textArea.height) {          // Ha a szöveg magasabb, mint a terület
                textDrawY = textArea.y + tft.fontHeight() / 2; // Próbáljuk középre igazítani a szöveg tetejét
            }
            tft.drawString(message, textArea.x + textArea.width / 2, textDrawY);
        }
    }
}