/**
 * @file SetupScreenBase.cpp
 * @brief Setup képernyők alaposztályának implementációja
 *
 * Ez a fájl tartalmazza a SetupScreenBase osztály implementációját,
 * amely minden specifikus setup képernyő közös alapfunkcionalitását biztosítja.
 *
 * @author [Fejlesztő neve]
 * @date 2025.06.10
 * @version 1.0
 */

#include "SetupScreenBase.h"
#include "MessageDialog.h"
#include "MultiButtonDialog.h"
#include "SystemInfoDialog.h"
#include "ValueChangeDialog.h"
#include "config.h"
#include "defines.h"
#include "pins.h"

/**
 * @brief SetupScreenBase konstruktor
 *
 * Inicializálja a setup képernyő alapstruktúráját:
 * - Görgethető lista létrehozása
 * - Exit gomb létrehozása
 * - UI komponensek elhelyezése
 *
 * @param tft TFT_eSPI referencia a kijelző kezeléséhez
 * @param screenName A képernyő neve
 */
SetupScreenBase::SetupScreenBase(TFT_eSPI &tft, const char *screenName) : UIScreen(tft, screenName) {
    // A createCommonUI meghívása a layoutComponents()-ből történik,
    // miután a leszármazott osztály konstruktora lefutott
}

/**
 * @brief Közös UI komponensek létrehozása
 *
 * Ez a metódus létrehozza a minden setup képernyőn közös UI elemeket:
 * - Görgethető lista
 * - Exit gomb
 *
 * @param title A képernyő címe
 */
void SetupScreenBase::createCommonUI(const char *title) {
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
 * Ez a metódus akkor hívódik meg, amikor a setup képernyő aktívvá válik.
 * Frissíti a menüpontokat és megjelöli a képernyőt újrarajzolásra.
 */
void SetupScreenBase::activate() {
    DEBUG("SetupScreenBase (%s) activated.\n", getName());
    // Menüpontok újrafeltöltése az esetlegesen megváltozott értékekkel
    populateMenuItems();
    // Képernyő megjelölése újrarajzolásra
    markForRedraw();
}

/**
 * @brief Képernyő tartalmának kirajzolása
 *
 * Kirajzolja a képernyő címét a tetején középre igazítva.
 */
void SetupScreenBase::drawContent() {
    // Szöveg pozicionálása: középre igazítás, felső széle
    tft.setTextDatum(TC_DATUM);
    // Szövegszín beállítása: fehér előtér, háttérszín háttér
    tft.setTextColor(TFT_WHITE, TFT_COLOR_BACKGROUND);
    // Betűtípus és méret beállítása
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextSize(1);
    // Cím kirajzolása a képernyő tetején középen
    tft.drawString(getScreenTitle(), tft.width() / 2, 10);
}

/**
 * @brief Menüpontok számának lekérdezése (IScrollableListDataSource interfész)
 *
 * @return A beállítási menüpontok száma
 */
int SetupScreenBase::getItemCount() const { return settingItems.size(); }

/**
 * @brief Menüpont címkéjének lekérdezése index alapján (IScrollableListDataSource interfész)
 *
 * @param index A menüpont indexe (0-tól kezdődik)
 * @return A menüpont címkéje vagy üres string érvénytelen index esetén
 */
String SetupScreenBase::getItemLabelAt(int index) const {
    if (index >= 0 && index < settingItems.size()) {
        return String(settingItems[index].label);
    }
    return "";
}

/**
 * @brief Menüpont értékének lekérdezése index alapján (IScrollableListDataSource interfész)
 *
 * @param index A menüpont indexe (0-tól kezdődik)
 * @return A menüpont aktuális értéke vagy üres string érvénytelen index esetén
 */
String SetupScreenBase::getItemValueAt(int index) const {
    if (index >= 0 && index < settingItems.size()) {
        const SettingItem &item = settingItems[index];
        if (item.isSubmenu) {
            return ">"; // Almenü jelölése
        }
        return item.value;
    }
    return "";
}

/**
 * @brief Menüpont kattintás kezelése (IScrollableListDataSource interfész)
 *
 * Ez a metódus akkor hívódik meg, amikor a felhasználó rákattint egy menüpontra.
 * Almenü esetén navigál a megfelelő képernyőre, egyébként meghívja a leszármazott
 * osztály kezelő metódusát.
 *
 * @param index A kiválasztott menüpont indexe (0-tól kezdődik)
 * @return false (nem fogyasztja el az eseményt)
 */
bool SetupScreenBase::onItemClicked(int index) {
    // Index érvényességének ellenőrzése
    if (index < 0 || index >= settingItems.size())
        return false;

    const SettingItem &item = settingItems[index];

    // Almenü esetén navigáció
    if (item.isSubmenu && item.targetScreen) {
        DEBUG("SetupScreenBase: Navigating to submenu: %s\n", item.targetScreen);
        if (getManager()) {
            getManager()->switchToScreen(item.targetScreen);
        }
        return false;
    }

    // Normál menüpont esetén leszármazott osztály kezelése
    handleItemAction(index, item.action);
    return false;
}

/**
 * @brief Egy adott lista elem megjelenítésének frissítése
 *
 * Ez a metódus egy konkrét menüpont megjelenítését frissíti
 * anélkül, hogy az egész listát újra kellene rajzolni.
 *
 * @param index A frissítendő menüpont indexe (0-tól kezdődik)
 */
void SetupScreenBase::updateListItem(int index) {
    if (index >= 0 && index < settingItems.size() && menuList) {
        menuList->refreshItemDisplay(index);
    }
}

/**
 * @brief UI komponensek létrehozása és elhelyezése
 *
 * Ez a metódus hívja meg a createCommonUI-t a leszármazott konstruktor után,
 * hogy biztosítsa a getScreenTitle() virtuális metódus megfelelő működését.
 */
void SetupScreenBase::layoutComponents() {
    createCommonUI(getScreenTitle());
}
