#include "UIScreen.h"

// ================================
// Konstruktorok és inicializálás
// ================================
/**
 * @brief UIScreen konstruktor névvel
 * @param tft TFT display referencia
 * @param name Képernyő egyedi neve
 *
 * Automatikusan teljes képernyő méretet használ (0,0 - tft.width(), tft.height()).
 * Az UIContainerComponent konstruktor hívása után a név beállítása történik.
 */
UIScreen::UIScreen(TFT_eSPI &tft, const char *name) : UIContainerComponent(tft, {0, 0, static_cast<uint16_t>(tft.width()), static_cast<uint16_t>(tft.height())}), name(name) {}

// ================================
// UIComponent Override Methods - Event Handling és Rendering
// ================================

/**
 * @brief Újrarajzolás szükségességének ellenőrzése
 * @return true ha újrarajzolás szükséges, false egyébként
 *
 * A metódus kompozit ellenőrzést végez:
 * 1. Szülő UIContainerComponent újrarajzolási igényének ellenőrzése
 * 2. Aktív dialógus újrarajzolási igényének ellenőrzése (ha van)
 *
 * Ez biztosítja, hogy mind az alapképernyő, mind a dialógusok
 * változásai megfelelően detektálódjanak.
 */
bool UIScreen::isRedrawNeeded() const {

    // Alapképernyő újrarajzolási igény ellenőrzése
    if (UIContainerComponent::isRedrawNeeded()) {
        return true;
    }

    // // Aktív dialógus újrarajzolási igény ellenőrzése
    // if (isDialogActive()) {
    //     auto topDialog = _dialogStack.back().lock();
    //     if (topDialog && topDialog->isRedrawNeeded()) {
    //         return true;
    //     }
    // }

    // Ha egyik sem igényel újrarajzolást, akkor false-t adunk vissza
    return false;
}

/**
 * @brief Képernyő és rétegzett dialógusok kirajzolása
 *
 * A draw metódus implementálja a layered dialog rendszer vizuális megjelenítését.
 *
 * Rajzolási sorrend (alulról felfelé):
 * 1. **Alapképernyő komponensek**: UIContainerComponent::draw() - gombok, szövegek, stb.
 * 2. **Rétegzett dialógusok**: Összes aktív dialógus a stack sorrendjében
 *
 * A rétegzési logika:
 * - A stack első eleme (index 0) = legalsó réteg
 * - A stack utolsó eleme (back()) = legfelső réteg
 * - Minden látható dialógus kirajzolódik, lehetővé téve az átláthatóságot
 *
 * @note Csak látható (_visible == true) komponensek rajzolódnak ki
 */
void UIScreen::draw() {

    // ===============================
    // 1. Alapképernyő komponensek rajzolása (alsó réteg)
    // ===============================
    UIContainerComponent::draw(); // Gombok, szövegek, egyéb UI elemek

    // // ===============================
    // // 2. Rétegzett dialógusok rajzolása (felső rétegek)
    // // ===============================
    // if (!_dialogStack.empty()) {

    //     // Összes látható dialógus kirajzolása stack sorrendjében (alulról felfelé)
    //     int dialogCount = 0;
    //     for (auto &weakDialog : _dialogStack) {
    //         auto dialog = weakDialog.lock();
    //         if (dialog && dialog->getVisible()) {
    //             dialog->draw();
    //             dialogCount++;
    //         }
    //     }
    // }
}

/**
 * @brief Touch esemény kezelése és routing
 * @param event Touch esemény adatok (pozíció, típus, stb.)
 * @return true ha az esemény kezelve lett, false egyébként
 *
 * Event routing stratégia:
 * 1. **Dialog Priority**: Ha van aktív dialógus, az kapja meg az eseményt
 * 2. **Screen Fallback**: Ha nincs dialógus, az alapképernyő komponensek kapják
 *
 * Ez biztosítja a helyes event handling hierarchiát:
 * - A felső réteg (dialógus) mindig prioritást élvez
 * - Az alsó réteg (képernyő) csak akkor kap eseményt, ha a felső nem kezeli
 *
 * @note A visszatérési érték jelzi, hogy az esemény feldolgozásra került-e
 */
bool UIScreen::handleTouch(const TouchEvent &event) {
    // if (isDialogActive()) {
    //     auto topDialog = _dialogStack.back().lock();
    //     if (topDialog) {
    //         return topDialog->handleTouch(event);
    //     }
    // }
    return UIContainerComponent::handleTouch(event);
}

/**
 * @brief Rotary encoder esemény kezelése és routing
 * @param event Rotary esemény adatok (forgatás irány, gombnyomás)
 * @return true ha az esemény kezelve lett, false egyébként
 *
 * A rotary event routing ugyanazt a hierarchikus logikát követi,
 * mint a touch events:
 * - Aktív dialógus → első prioritás
 * - Alapképernyő komponensek → fallback
 *
 * Ez különösen fontos a navigációs dialógusoknál, ahol a rotary
 * encoder segítségével lehet OK/Cancel között váltani.
 */
bool UIScreen::handleRotary(const RotaryEvent &event) {
    // if (isDialogActive()) {
    //     auto topDialog = _dialogStack.back().lock();
    //     if (topDialog) {
    //         return topDialog->handleRotary(event);
    //     }
    // }
    return UIContainerComponent::handleRotary(event);
}

/**
 * @brief Folyamatos loop kezelése és optimalizáció
 *
 * Loop routing stratégia:
 * - **Dialog Mode**: Ha van aktív dialógus, csak a dialógus loop-ja fut
 * - **Screen Mode**: Ha nincs dialógus, az alapképernyő komponensek loop-ja fut
 *
 * Teljesítmény optimalizáció:
 * Ez a megközelítés csökkenti a CPU terhelést azáltal, hogy egyszerre
 * csak a szükséges komponensek loop metódusai futnak.
 *
 * Használat:
 * - Animációk frissítése
 * - Időzített események kezelése
 * - Státusz frissítések
 * - Szenzorok olvasása
 */
void UIScreen::loop() {
    // if (isDialogActive()) {
    //     auto topDialog = _dialogStack.back().lock();
    //     if (topDialog) {
    //         topDialog->loop();
    //     }

    //     // Ha van aktív dialógus, csak annak loop-ja fut
    //     return;
    // }

    // Ha nincs aktív dialógus, akkor az alap képernyő komponensek loop-ja fut
    UIContainerComponent::loop();
}