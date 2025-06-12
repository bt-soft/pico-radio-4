/**
 * @file ExampleScreenWithRefactoredButtons.h
 * @brief Példa screen osztály a refaktorált CommonVerticalButtons használatára
 * @details Demonstrálja, hogyan kell használni a ButtonsGroupManager alapú
 *          CommonVerticalButtons implementációt
 */

#ifndef __EXAMPLE_SCREEN_WITH_REFACTORED_BUTTONS_H
#define __EXAMPLE_SCREEN_WITH_REFACTORED_BUTTONS_H

#include "CommonVerticalButtons_Refactored.h"
#include "UIScreen.h"

/**
 * @brief Példa screen osztály ButtonsGroupManager integrációval
 * @details A CommonVerticalButtons::Mixin használatával egyszerűen
 *          integrálja a függőleges gombokat
 */
class ExampleScreenWithRefactoredButtons : public UIScreen, public CommonVerticalButtons::Mixin<ExampleScreenWithRefactoredButtons> {

  private:
    Si4735Manager *pSi4735Manager;
    IScreenManager *pScreenManager;

  public:
    ExampleScreenWithRefactoredButtons(TFT_eSPI &tft, Si4735Manager *si4735Manager, IScreenManager *screenManager)
        : UIScreen(tft, "ExampleScreen"), pSi4735Manager(si4735Manager), pScreenManager(screenManager) {}

  protected:
    void initializeComponents() override {
        // Függőleges gombok létrehozása ButtonsGroupManager segítségével
        createCommonVerticalButtons(pSi4735Manager, pScreenManager);

        // További UI komponensek inicializálása...
        // pl. frekvencia kijelző, spektrum analizátor, stb.
    }

    void activate() override {
        UIScreen::activate();

        // Gombállapotok szinkronizálása aktiváláskor
        updateAllVerticalButtonStates(pSi4735Manager);
    }

    void handleEvent(UIComponent *source, const UIButton::ButtonEvent &event) override {
        // Az eseménykezelés automatikusan működik a callback-ek révén
        // További custom eseménykezelés itt...

        UIScreen::handleEvent(source, event);
    }

    // EREDETI HASZNÁLAT ÖSSZEHASONLÍTÁSKÉNT:
    /*
    // RÉGI módszer UIVerticalButtonBar-ral:
    void initializeComponents_OLD() {
        verticalButtonBar = CommonVerticalButtons::createVerticalButtonBar(
            tft, this, pSi4735Manager, pScreenManager
        );
        addChild(verticalButtonBar);
    }
    */

    // ÚJ módszer ButtonsGroupManager-rel:
    // - Automatikus gombhozzáadás az addChild-on keresztül
    // - Nincs külön verticalButtonBar változó szükséglet
    // - Közvetlen gomb referenciák elérhetők getVerticalButtons()-on keresztül
    // - Ugyanazok a callback-ek és funkciók
};

/**
 * @brief Használati példa factory metódussal
 */
std::shared_ptr<ExampleScreenWithRefactoredButtons> createExampleScreen(TFT_eSPI &tft, Si4735Manager *si4735Manager, IScreenManager *screenManager) {

    return std::make_shared<ExampleScreenWithRefactoredButtons>(tft, si4735Manager, screenManager);
}

#endif // __EXAMPLE_SCREEN_WITH_REFACTORED_BUTTONS_H
