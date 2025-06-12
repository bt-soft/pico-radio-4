/**
 * @file FMScreen.h
 * @brief FM rádió vezérlő képernyő osztály definíció
 * @details Event-driven gombállapot kezeléssel és optimalizált teljesítménnyel
 */

#ifndef __FM_SCREEN_H
#define __FM_SCREEN_H
#include "CommonVerticalButtons.h"
#include "UIButton.h"
#include "UIHorizontalButtonBar.h"
#include "UIScreen.h"

class FMScreen : public UIScreen, public CommonVerticalButtons::Mixin<FMScreen> {

  public:
    // ===================================================================
    // Konstruktor és destruktor
    // ===================================================================

    /**
     * @brief FMScreen konstruktor - FM rádió képernyő inicializálás
     * @param tft TFT display referencia
     * @param si4735Manager Si4735 rádió chip kezelő referencia
     *
     * @details Automatikusan végrehajtja:
     * - Si4735 chip inicializálás
     * - UI komponensek layout létrehozás
     * - Event-driven gombkezelés beállítás
     */
    FMScreen(TFT_eSPI &tft, Si4735Manager &si4735Manager);

    /**
     * @brief Virtuális destruktor - Automatikus cleanup
     */
    virtual ~FMScreen() = default;

    // ===================================================================
    // UIScreen interface megvalósítás
    // ===================================================================

    /**
     * @brief Rotary encoder eseménykezelés - FM frekvencia hangolás
     * @param event Rotary encoder esemény (forgatás irány, érték, gombnyomás)
     * @return true ha sikeresen kezelte az eseményt, false egyébként
     *
     * @details Frekvencia hangolási logika:
     * - Rotary forgatás → frekvencia léptetés
     * - Automatikus Si4735 beállítás és band tábla mentés
     * - Frekvencia kijelző azonnali frissítése
     * - Dialógus aktív esetén esemény továbbítása
     */
    virtual bool handleRotary(const RotaryEvent &event) override;

    /**
     * @brief Folyamatos loop hívás - Optimalizált teljesítmény
     * @details Event-driven architektúra - NINCS gombállapot polling!
     *
     * Csak valóban szükséges frissítések:
     * - S-Meter (jelerősség) valós idejű frissítése
     *
     * Gombállapotok frissítése CSAK:
     * - Képernyő aktiválásakor (activate())
     * - Specifikus eseményekkor (eseménykezelőkben)
     */
    virtual void handleOwnLoop() override;

    /**
     * @brief Statikus képernyő tartalom kirajzolása
     * @details Csak a statikus UI elemeket rajzolja:
     * - S-Meter skála (vonalak, számok)
     *
     * A dinamikus tartalom (pl. S-Meter érték) a loop()-ban frissül.
     */
    virtual void drawContent() override;

    /**
     * @brief Képernyő aktiválása - Event-driven gombállapot szinkronizálás
     * @details Ez az EGYETLEN hely, ahol gombállapotokat szinkronizáljuk!
     *
     * Szinkronizálási pontok:
     * - Mute gomb ↔ rtv::muteStat állapot
     * - AM gomb ↔ aktuális band típus
     * - AGC/Attenuator gombok ↔ Si4735 állapotok (TODO)
     */
    virtual void activate() override;

    /**
     * @brief Dialógus bezárásának kezelése - Gombállapot szinkronizálás
     * @details Az utolsó dialógus bezárásakor frissíti a gombállapotokat
     *
     * Funkcionalitás:
     * - Alap UIScreen::onDialogClosed() hívása
     * - Ha ez volt az utolsó dialógus -> updateAllVerticalButtonStates() + updateHorizontalButtonStates()
     * - Biztosítja a konzisztens gombállapotokat dialógus bezárás után
     */
    virtual void onDialogClosed(UIDialogBase *closedDialog) override;

  private:
    // ===================================================================
    // UI komponensek layout és management
    // ===================================================================

    /**
     * @brief UI komponensek létrehozása és képernyőn való elhelyezése
     * @details Létrehozza és pozicionálja az összes UI elemet:
     * - Állapotsor (felül)
     * - Frekvencia kijelző (középen)
     * - S-Meter (jelerősség mérő)
     * - Függőleges gombsor (jobb oldal)
     * - Vízszintes gombsor (alul)
     */
    void layoutComponents();

    /**
     * @brief Vízszintes gombsor létrehozása - Alsó navigációs gombok
     * @details 3 navigációs gomb elhelyezése vízszintes elrendezésben:
     * AM, Test, Setup
     */
    void createHorizontalButtonBar();

    // ===================================================================
    // Event-driven gombállapot szinkronizálás
    // ===================================================================

    /**
     * @brief Vízszintes gombsor állapotainak szinkronizálása
     * @details CSAK aktiváláskor hívódik meg! Event-driven architektúra.
     *
     * Szinkronizált állapotok:
     * - AM gomb ↔ aktuális band típus (FM vs AM/MW/LW/SW)
     */
    void updateHorizontalButtonStates();

    // ===================================================================
    // Vízszintes gomb eseménykezelők
    // ===================================================================

    /**
     * @brief AM gomb eseménykezelő - AM családú képernyőre váltás
     * @param event Gomb esemény (Clicked)
     * @details Pushable gomb: AM/MW/LW/SW képernyőre navigálás
     */
    void handleAMButton(const UIButton::ButtonEvent &event);

    /**
     * @brief TEST gomb eseménykezelő - Teszt képernyőre váltás
     * @param event Gomb esemény (Clicked)
     * @details Pushable gomb: Test és diagnosztikai képernyőre navigálás
     */
    void handleTestButton(const UIButton::ButtonEvent &event);

    /**
     * @brief SETUP gomb eseménykezelő (vízszintes) - Beállítások képernyőre váltás
     * @param event Gomb esemény (Clicked)
     * @details Pushable gomb: Setup képernyőre navigálás (duplikáció a függőleges gombbal)
     */
    void handleSetupButtonHorizontal(const UIButton::ButtonEvent &event);

    // ===================================================================
    // UI komponens objektumok - Smart pointer kezelés
    // ===================================================================

    /**
     * @brief Vízszintes gombsor komponens
     * @details Smart pointer a 3 navigációs gombhoz (alsó sor)
     * - Automatikus memória kezelés
     * - Event-driven eseménykezelés
     * - AM, Test, Setup gombok
     */
    std::shared_ptr<UIHorizontalButtonBar> horizontalButtonBar;
};

#endif // __FM_SCREEN_H