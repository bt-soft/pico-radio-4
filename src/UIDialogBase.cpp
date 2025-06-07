#include "UIDialogBase.h"
#include "UIScreen.h" // Szükséges a parentScreen->onDialogClosed hívásához

/**
 * @brief UIDialogBase konstruktor
 * @param parentScreen A szülő UIScreen, amely megjeleníti ezt a dialógust
 * @param tft TFT_eSPI referencia a rajzoláshoz
 * @param initialBounds A dialógus kezdeti határai (pozíció és méret)
 * @param title A dialógus címe (nullptr, ha nincs cím)
 * @param cs A dialógus színpalettája (alapértelmezett ColorScheme)
 *
 */
UIDialogBase::UIDialogBase(UIScreen *parentScreen, TFT_eSPI &tft, const Rect &initialBounds, const char *title, const ColorScheme &cs)
    : UIContainerComponent(tft, initialBounds, cs), parentScreen(parentScreen), title(title) {

    Rect finalBounds = initialBounds;

    // Szélesség beállítása: ha 0, akkor alapértelmezett, egyébként a megadott
    if (finalBounds.width == 0) {
        finalBounds.width = tft.width() * 0.8f; // Alapértelmezett szélesség
    }
    // Magasság beállítása: ha 0, akkor alapértelmezett, egyébként a megadott
    if (finalBounds.height == 0) {
        finalBounds.height = tft.height() * 0.6f; // Alapértelmezett magasság (ezt a MessageDialog felülírhatja)
    }

    // Középre igazítás, ha x vagy y -1
    if (finalBounds.x == -1) {
        finalBounds.x = (tft.width() - finalBounds.width) / 2;
    }
    if (finalBounds.y == -1) {
        finalBounds.y = (tft.height() - finalBounds.height) / 2;
    }
    setBounds(finalBounds); // UIComponent::setBounds

    // Bezáró gomb létrehozása, ha van cím
    if (title != nullptr) {
        createCloseButton();
    }

    // A createDialogContent() és layoutDialogContent() a leszármazottban hívódik meg.
}

/**
 * @brief Megjeleníti a dialógust.
 * @details A dialógus megjelenítésekor a fátyol rajzolása csak egyszer történik meg,
 */
void UIDialogBase::show() {
    veilDrawn = false; // Reset a flag-et új megjelenéskor
    // A _parentScreen->markForRedraw() implicit módon megtörténik,
    // amikor a dialógus láthatóvá válik és a UIScreen::draw() lefut.
}

/**
 * @brief Bezárja a dialógust és meghívja a callback-et.
 * @param result A dialógus eredménye (Accepted, Rejected, Dismissed)
 */
void UIDialogBase::close(DialogResult result) {
    veilDrawn = false; // Reset bezáráskor

    // Callback meghívása ELŐBB - így a callback-ben lehet új dialógust megnyitni
    if (callback) {
        callback(result);
    }

    // Szülő képernyő értesítése UTÁNA - dialógus eltávolítása a stackből
    if (parentScreen) {
        parentScreen->onDialogClosed(this);
    }
}

/**
 * @brief Rajzolja a dialógust.
 * @details A dialógus hátterét és fejlécét rajzolja, valamint a gyerek komponenseket.
 */
void UIDialogBase::draw() {

    if (!veilDrawn) {
        drawVeilOptimized();
        veilDrawn = true;
    }

    // Dialógus keret és gyerekek rajzolása
    UIContainerComponent::draw();
}

/**
 * @brief Rajzolja a dialógus hátterét és fejlécét.
 * @details A fejléc kék háttérrel és egyenes sarkokkal rendelkezik.
 */
void UIDialogBase::drawSelf() {
    // Dialógus háttér és keret rajzolása - egyenes sarkokkal
    tft.fillRect(bounds.x, bounds.y, bounds.width, bounds.height, colors.background);
    tft.drawRect(bounds.x, bounds.y, bounds.width, bounds.height, colors.border); // Fejléc kék háttér - egyenes sarkokkal
    uint16_t headerColor = UIColorPalette::DIALOG_HEADER_BACKGROUND;
    tft.fillRect(bounds.x + 1, bounds.y + 1, bounds.width - 2, HEADER_HEIGHT, headerColor);

    // Fejléc elválasztó vonal
    tft.drawLine(bounds.x + 1, bounds.y + HEADER_HEIGHT, bounds.x + bounds.width - 2, bounds.y + HEADER_HEIGHT, colors.border);

    // Cím kirajzolása
    if (title != nullptr) {
        tft.setTextColor(UIColorPalette::DIALOG_HEADER_TEXT, headerColor);
        tft.setTextSize(2); // Nagyobb betűméret a címnek
        tft.setTextDatum(ML_DATUM);
        int16_t titleX = bounds.x + PADDING + 4;
        int16_t titleY = bounds.y + (HEADER_HEIGHT / 2);
        tft.drawString(title, titleX, titleY);
    }
}

/**
 * @brief Kezeli a rotary eseményeket, amelyek a dialógusra vonatkoznak.
 * @param event A rotary esemény, amely tartalmazza az irányt és a gomb állapotát
 * @return true, ha a rotary eseményt a dialógus kezelte, false egyébként
 * @details A handleRotary és loop alapértelmezetten a UIContainerComponent-től öröklődik,
 *  ami továbbítja a gyerekeknek. Ha a dialógusnak magának kellene kezelnie, felülírható.
 */
bool UIDialogBase::handleRotary(const RotaryEvent &event) {

    // Csak akkor kezeljük, ha a toppon van és nem tiltott a dialógus
    if (!topDialog || disabled) {
        return false;
    }

    // 1. Először a gyerek komponensek próbálják kezelni (fordított sorrendben, a legfelső először)
    // Nem hívjuk a UICompositeComponent::handleRotary-t, hogy elkerüljük
    // a dialógusra (mint UIComponent) történő felesleges továbbhívást,
    // mert azt itt specifikusan kezeljük.
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        if ((*it)->handleRotary(event)) {
            return true; // Egy gyerek feldolgozta
        }
    }

    // 2. Ha egyik gyerek sem kezelte, akkor a dialógus maga kezeli a rotary gombnyomást (click)
    if (event.buttonState == RotaryEvent::ButtonState::Clicked) {
        // A dialógus maga kezeli a klikket, az OK-nak/elfogadásnak feleltetjük meg
        if (autoClose) {
            close(DialogResult::Accepted);
        } else if (callback) {
            callback(DialogResult::Accepted);
        }
        return true;
    }

    return false; // A dialógus sem dolgozta fel az eseményt
}

/**
 * @brief Kezeli a touch eseményeket a dialóguson belül.
 * @param event A touch esemény, amely tartalmazza a koordinátákat és a lenyomás állapotát
 */
bool UIDialogBase::handleTouch(const TouchEvent &event) {

    // Csak akkor kezeljük, ha a toppon van és nem tiltott a dialógus
    if (!topDialog || disabled) {
        return false;
    }

    // Gyerek komponensek kezelik az eseményt (beleértve a bezáró gombot is)
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        if ((*it)->handleTouch(event)) {
            return true; // Egy gyerek komponens kezelte az eseményt
        }
    }

    // Ha a dialógus területén belül történt az érintés, elnyeljük
    if (bounds.contains(event.x, event.y)) {
        return true; // Az eseményt a dialógus kezelte (elnyelés)
    }

    // Ha kívül történt, nem kezeljük
    return false;
}

/**
 * @brief Jelzi, hogy a dialógust újra kell rajzolni.
 * @details Felülírja az UIComponent::markForRedraw metódusát, hogy biztosítsa
 * a dialógus-specifikus újrarajzolási igények kezelését.
 */
void UIDialogBase::markForRedraw() {
    UIContainerComponent::markForRedraw(); // Meghívja az ősosztály implementációját
}

void UIDialogBase::drawVeilOptimized() {
    // CSAK a dialógus területén KÍVÜL rajzoljuk a fátyolt!
    constexpr uint8_t VEIL_PIXEL_SIZE = 4; // Fátyol pixel mérete
    for (int16_t y = 0; y < tft.height(); y += VEIL_PIXEL_SIZE) {
        for (int16_t x = (y % VEIL_PIXEL_SIZE); x < tft.width(); x += VEIL_PIXEL_SIZE) { // Ne rajzoljunk fátyolt a dialógus területére!
            if (!bounds.contains(x, y)) {
                tft.drawPixel(x, y, UIColorPalette::DIALOG_VEIL_COLOR);
            }
        }
    }
}

void UIDialogBase::createCloseButton() {
    // Bezáró gomb mérete és pozíciója
    constexpr int16_t CLOSE_BTN_SIZE = 20;
    constexpr int16_t CLOSE_BTN_MARGIN = 4;

    int16_t closeBtnX = bounds.x + bounds.width - CLOSE_BTN_SIZE - CLOSE_BTN_MARGIN;
    int16_t closeBtnY = bounds.y + CLOSE_BTN_MARGIN;
    Rect closeBtnBounds(closeBtnX, closeBtnY, CLOSE_BTN_SIZE, CLOSE_BTN_SIZE); // Bezáró gomb létrehozása központi színpalettával

    ColorScheme closeButtonColors;
    closeButtonColors.background = UIColorPalette::DIALOG_CLOSE_BUTTON_BACKGROUND;
    closeButtonColors.foreground = UIColorPalette::DIALOG_CLOSE_BUTTON_TEXT;
    closeButtonColors.border = UIColorPalette::DIALOG_CLOSE_BUTTON_BACKGROUND;
    closeButtonColors.pressedBackground = TFT_RED;
    closeButtonColors.pressedForeground = UIColorPalette::DIALOG_CLOSE_BUTTON_TEXT;

    closeButton = std::make_shared<UIButton>(tft, DIALOG_DEFAULT_CLOSE_BUTTON_ID, closeBtnBounds, "X", //
                                             [this](const UIButton::ButtonEvent &event) {
                                                 if (event.state == UIButton::EventButtonState::Clicked) {
                                                     this->close(DialogResult::Dismissed);
                                                 }
                                             });

    addChild(closeButton);
}