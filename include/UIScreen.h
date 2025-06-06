#ifndef __UI_SCREEN_H
#define __UI_SCREEN_H

#include "IScreenManager.h"
#include "UIContainerComponent.h"

class UIScreen : public UIContainerComponent {

  private:
    /** @brief A képernyő egyedi neve */
    const char *name;

    /** @brief ScreenManager referencia a képernyő váltásokhoz */
    IScreenManager *manager = nullptr;

  public:
    /**
     * @brief Konstruktor képernyő névvel
     * @param tft TFT display referencia
     * @param name A képernyő egyedi neve
     *
     * A képernyő teljes display méretet használja automatikusan.
     */
    UIScreen(TFT_eSPI &tft, const char *name);

    /**
     * @brief Virtuális destruktor
     *
     * Automatikusan felszabadítja az összes erőforrást.
     */
    virtual ~UIScreen() = default;

    /**
     * @brief Képernyő egyedi nevének elkérése
     * @return A képernyő neve
     */
    const char *getName() const { return name; }

    /**
     * @brief ScreenManager beállítása
     * @param mgr ScreenManager referencia
     */
    void setManager(IScreenManager *mgr) { manager = mgr; }

    /**
     * @brief Paraméterek beállítása
     * @param params Paraméter pointer (képernyő specifikus típus)
     *
     * Képernyők közötti adatátadáshoz használható.
     * Felülírható a specifikus paraméter kezelés implementálásához.
     */
    virtual void setParameters(void *params) {}

    // ================================
    // Screen Lifecycle Management
    // ================================
    /**
     * @brief Képernyő aktiválása
     *
     * Meghívódik amikor a képernyő aktívvá válik.
     * Felülírható a specifikus aktiválási logika implementálásához.
     */
    virtual void activate() {}

    /**
     * @brief Képernyő deaktiválása
     *
     * Meghívódik amikor a képernyő inaktívvá válik.
     * Felülírható a specifikus deaktiválási logika implementálásához.
     */
    virtual void deactivate() {}

    // ================================
    // UIComponent Override Methods - Event Handling és Rendering
    // ================================

    /**
     * @brief Újrarajzolás szükségességének ellenőrzése
     * @return true ha újrarajzolás szükséges, false egyébként
     *
     * Ellenőrzi mind a képernyő, mind az aktív dialógusok újrarajzolási igényét.
     * A szülő UICompositeComponent logikát kiegészíti dialógus ellenőrzéssel.
     */
    virtual bool isRedrawNeeded() const override;

    /**
     * @brief Képernyő és dialógusok kirajzolása
     *
     * Rajzolási sorrend:
     * 1. Alap képernyő komponensek (UICompositeComponent::draw())
     * 2. Összes aktív dialógus a stack sorrendjében (alulról felfelé)
     *
     * A layered dialog rendszer magja - minden látható dialógust kirajzol
     * a megfelelő rétegzési sorrendben.
     */
    virtual void draw() override;

    /**
     * @brief Touch esemény kezelése
     * @param event Touch esemény adatok
     * @return true ha az esemény kezelve lett, false egyébként
     *
     * Event routing:
     * 1. Ha van aktív dialógus → dialógusnak továbbítja
     * 2. Egyébként → alap képernyő komponenseknek (UICompositeComponent)
     *
     * Ez biztosítja, hogy a felső dialógus mindig megkapja az eseményeket.
     */
    virtual bool handleTouch(const TouchEvent &event) override;

    /**
     * @brief Rotary encoder esemény kezelése
     * @param event Rotary esemény adatok (forgatás/gombnyomás)
     * @return true ha az esemény kezelve lett, false egyébként
     *
     * Event routing logika megegyezik a handleTouch-sal:
     * aktív dialógus -> alap képernyő komponensek
     */
    virtual bool handleRotary(const RotaryEvent &event) override;

    /**
     * @brief Folyamatos loop hívás kezelése
     *
     * Loop routing:
     * - Ha van aktív dialógus → csak a dialógus loop-ja fut
     * - Egyébként → alap képernyő komponensek loop-ja (UICompositeComponent)
     *
     * Ez optimalizálja a teljesítményt azáltal, hogy csak a szükséges
     * komponensek loop-ja fut.
     */
    virtual void loop() override;
};

#endif //__UI_SCREEN_H