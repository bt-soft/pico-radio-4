/**
 * @file AMScreen.h
 * @brief AM rádió vezérlő képernyő osztály definíció
 * @details Event-driven gombállapot kezeléssel és optimalizált teljesítménnyel
 *
 * Fő komponensek:
 * - AM/MW/LW/SW frekvencia hangolás és megjelenítés
 * - S-Meter (jelerősség) valós idejű frissítés
 * - Közös függőleges gombsor (8 funkcionális gomb) - FMScreen-nel megegyező
 * - Vízszintes gombsor (3 navigációs gomb) - FM gombbal
 * - Event-driven architektúra (nincs folyamatos polling)
 *
 * @author Rádió projekt
 * @version 2.0 - Event-driven architecture
 */

#ifndef __AM_SCREEN_H
#define __AM_SCREEN_H
#include "CommonVerticalButtons.h"
#include "UIButton.h"
#include "UIHorizontalButtonBar.h"
#include "UIScreen.h"

/**
 * @class AMScreen
 * @brief AM rádió vezérlő képernyő implementáció
 * @details Ez az osztály kezeli az AM családú rádiók összes vezérlő funkcióját:
 *
 * **Event-driven architektúra:**
 * - Gombállapotok CSAK aktiváláskor szinkronizálódnak
 * - NINCS folyamatos polling a loop ciklusban
 * - Optimalizált teljesítmény és hatékonyság
 *
 * **UI komponensek:**
 * - Frekvencia kijelző (közép)
 * - S-Meter jelerősség mérő (alul)
 * - 8 funkcionális gomb (jobb oldal) - Közös FMScreen-nel
 * - 3 navigációs gomb (alsó sor) - FM gombot tartalmaz
 *
 * **Támogatott band-ek:**
 * - MW (Medium Wave) - 520-1710 kHz
 * - LW (Long Wave) - 150-519 kHz
 * - SW (Short Wave) - 2.3-26.1 MHz
 */
class AMScreen : public UIScreen, public CommonVerticalButtons::Mixin<AMScreen> {

  public:
    // ===================================================================
    // Konstruktor és destruktor
    // ===================================================================

    /**
     * @brief AMScreen konstruktor - AM rádió képernyő inicializálás
     * @param tft TFT display referencia
     * @param si4735Manager Si4735 rádió chip kezelő referencia
     *
     * @details Automatikusan végrehajtja:
     * - Si4735 chip inicializálás AM módba
     * - UI komponensek layout létrehozás
     * - Event-driven gombkezelés beállítás
     * - MW band aktiválás alapértelmezettként
     */
    AMScreen(TFT_eSPI &tft, Si4735Manager &si4735Manager);

    /**
     * @brief Virtuális destruktor - Automatikus cleanup
     */
    virtual ~AMScreen() = default;

    // ===================================================================
    // UIScreen interface megvalósítás
    // ===================================================================

    /**
     * @brief Rotary encoder eseménykezelés - AM frekvencia hangolás
     * @param event Rotary encoder esemény (forgatás irány, érték, gombnyomás)
     * @return true ha sikeresen kezelte az eseményt, false egyébként
     *
     * @details AM frekvencia hangolási logika:
     * - Rotary forgatás → frekvencia léptetés (AM band-nek megfelelően)
     * - Automatikus Si4735 beállítás és band tábla mentés
     * - Frekvencia kijelző azonnali frissítése
     * - Dialógus aktív esetén esemény továbbítása
     * - MW: 9/10 kHz lépések, LW: 1 kHz, SW: 5 kHz lépések
     */
    virtual bool handleRotary(const RotaryEvent &event) override;

    /**
     * @brief Folyamatos loop hívás - Optimalizált teljesítmény
     * @details Event-driven architektúra - NINCS gombállapot polling!
     *
     * Csak valóban szükséges frissítések:
     * - S-Meter (jelerősség) valós idejű frissítése AM módban
     *
     * Gombállapotok frissítése CSAK:
     * - Képernyő aktiválásakor (activate())
     * - Specifikus eseményekkor (eseménykezelőkben)
     */
    virtual void handleOwnLoop() override;

    /**
     * @brief Statikus képernyő tartalom kirajzolása
     * @details Csak a statikus UI elemeket rajzolja:
     * - S-Meter skála (vonalak, számok) AM módban
     *
     * A dinamikus tartalom (pl. S-Meter érték) a loop()-ban frissül.
     */
    virtual void drawContent() override;

    /**
     * @brief Képernyő aktiválása - Event-driven gombállapot szinkronizálás
     * @details Ez az EGYETLEN hely, ahol gombállapotokat szinkronizálunk!
     *
     * Szinkronizálási pontok:
     * - Mute gomb ↔ rtv::muteStat állapot
     * - FM gomb ↔ aktuális band típus (AM vs FM)
     * - AGC/Attenuator gombok ↔ Si4735 állapotok (TODO)
     * - Bandwidth gomb ↔ AM szűrő beállítások
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
     * - Függőleges gombsor (jobb oldal) - Közös FMScreen-nel
     * - Vízszintes gombsor (alul) - FM gombbal
     */
    void layoutComponents(); /**
                              * @brief Vízszintes gombsor létrehozása - Alsó navigációs gombok
                              * @details 3 navigációs gomb elhelyezése vízszintes elrendezésben:
                              * FM, Test, Setup (FM gomb az FMScreen-re navigál)
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
     * - FM gomb ↔ aktuális band típus (AM vs FM)
     */
    void updateHorizontalButtonStates();

    // ===================================================================
    // Vízszintes gomb eseménykezelők
    // ===================================================================

    /**
     * @brief FM gomb eseménykezelő - FM képernyőre váltás
     * @param event Gomb esemény (Clicked)
     * @details Pushable gomb: FMScreen-re navigálás
     * Ellentéte az FMScreen AM gombjának
     */
    void handleFMButton(const UIButton::ButtonEvent &event);

    /**
     * @brief TEST gomb eseménykezelő - Teszt képernyőre váltás
     * @param event Gomb esemény (Clicked)
     * @details Pushable gomb: Test és diagnosztikai képernyőre navigálás
     * Azonos logika, mint FMScreen-ben
     */
    void handleTestButton(const UIButton::ButtonEvent &event);

    // ===================================================================
    // UI komponens objektumok - Smart pointer kezelés
    // ===================================================================

    /**
     * @brief Vízszintes gombsor komponens
     * @details Smart pointer a 3 navigációs gombhoz (alsó sor)
     * - Automatikus memória kezelés
     * - Event-driven eseménykezelés
     * - FM, Test, Setup gombok
     */
    std::shared_ptr<UIHorizontalButtonBar> horizontalButtonBar;
};

#endif // __AM_SCREEN_H
