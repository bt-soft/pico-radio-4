/**
 * @file FMScreen.h
 * @brief FM rádió vezérlő képernyő osztály definíció
 * @details Event-driven gombállapot kezeléssel és optimalizált teljesítménnyel
 *
 * Fő komponensek:
 * - FM frekvencia hangolás és megjelenítés
 * - S-Meter (jelerősség) valós idejű frissítés
 * - Függőleges gombsor (8 funkcionális gomb)
 * - Vízszintes gombsor (3 navigációs gomb)
 * - Event-driven architektúra (nincs folyamatos polling)
 *
 * @author Rádió projekt
 * @version 2.0 - Event-driven architecture
 */

#ifndef __FM_SCREEN_H
#define __FM_SCREEN_H
#include "UIButton.h"
#include "UIHorizontalButtonBar.h"
#include "UIScreen.h"
#include "UIVerticalButtonBar.h"

/**
 * @class FMScreen
 * @brief FM rádió vezérlő képernyő implementáció
 * @details Ez az osztály kezeli az FM rádió összes vezérlő funkcióját:
 *
 * **Event-driven architektúra:**
 * - Gombállapotok CSAK aktiváláskor szinkronizálódnak
 * - NINCS folyamatos polling a loop ciklusban
 * - Optimalizált teljesítmény és hatékonyság
 *
 * **UI komponensek:**
 * - Frekvencia kijelző (közép)
 * - S-Meter jelerősség mérő (alul)
 * - 8 funkcionális gomb (jobb oldal)
 * - 3 navigációs gomb (alsó sor)
 */
class FMScreen : public UIScreen {

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
     * @brief Függőleges gombsor létrehozása - Jobb oldali funkcionális gombok
     * @details 8 gomb elhelyezése függőleges elrendezésben:
     * Mute, Volume, AGC, Attenuator, Squelch, Frequency, Setup, Memory
     */
    void createVerticalButtonBar();

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
     * @brief Függőleges gombsor állapotainak szinkronizálása
     * @details CSAK aktiváláskor hívódik meg! Event-driven architektúra.
     *
     * Szinkronizált állapotok:
     * - Mute gomb ↔ rtv::muteStat
     * - AGC gomb ↔ Si4735 AGC állapot (TODO)
     * - Attenuator gomb ↔ Si4735 attenuator állapot (TODO)
     */
    void updateVerticalButtonStates();

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
     * @brief Függőleges gombsor komponens
     * @details Smart pointer a 8 funkcionális gombhoz (jobb oldal)
     * - Automatikus memória kezelés
     * - Event-driven eseménykezelés
     * - Mute, Volume, AGC, Attenuator, Squelch, Frequency, Setup, Memory
     */
    std::shared_ptr<UIVerticalButtonBar> verticalButtonBar;

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