/**
 * @file RadioScreen.h
 * @brief Rádió vezérlő képernyő alaposztály definíció
 * @details Absztrakciós réteg az UIScreen és a rádiós képernyők között
 *
 * Fő funkciók:
 * - Seek (automatikus állomáskeresés) infrastruktúra
 * - Frekvencia kezelés és mentés
 * - Rádió-specifikus UI komponensek kezelése
 * - Közös callback rendszer a valós idejű seek frissítéshez
 *
 * @author Rádió projekt
 * @version 1.0 - Refaktorált absztrakció
 */

#ifndef __RADIO_SCREEN_H
#define __RADIO_SCREEN_H

#include "RDSComponent.h"
#include "UIHorizontalButtonBar.h"
#include "UIScreen.h"

// Forward deklaráció a seek callback függvényhez
void radioSeekProgressCallback(uint16_t frequency);

/**
 * @brief Közös vízszintes gombsor gomb azonosítók
 * @details Minden RadioScreen alapú képernyő közös gombjai
 */
namespace CommonHorizontalButtonIDs {
static constexpr uint8_t HAM_BUTTON = 50;  ///< Ham rádió funkcionalitás
static constexpr uint8_t BAND_BUTTON = 51; ///< Band (sáv) kezelés
static constexpr uint8_t SCAN_BUTTON = 52; ///< Scan (folyamatos keresés)
} // namespace CommonHorizontalButtonIDs

/**
 * @class RadioScreen
 * @brief Rádió vezérlő képernyők közös alaposztálya
 * @details Ez az absztrakciós réteg az UIScreen és a konkrét rádiós képernyők között.
 *
 * **Fő funkciók:**
 * - Seek (automatikus állomáskeresés) valós idejű frissítéssel
 * - Frekvencia és band kezelés
 * - RDS komponens támogatás
 * - SI4735 chip vezérlési logika
 *
 * **Örökölhető osztályok:**
 * - FMScreen - FM rádió vezérlés
 * - AMScreen - AM/MW/LW/SW rádió vezérlés
 *
 * **Nem rádiós képernyők:**
 * - SetupScreen, MemoryScreen, TestScreen stb. - közvetlenül UIScreen-ből származnak
 */
class RadioScreen : public UIScreen {

    // Friend deklaráció a seek callback számára
    friend void radioSeekProgressCallback(uint16_t frequency);

  public:
    // ===================================================================
    // Konstruktor és destruktor
    // ===================================================================

    /**
     * @brief RadioScreen konstruktor - Rádió képernyő alaposztály inicializálás
     * @param tft TFT display referencia
     * @param name Képernyő egyedi neve
     * @param si4735Manager Si4735 rádió chip kezelő referencia
     */
    RadioScreen(TFT_eSPI &tft, const char *name, Si4735Manager *si4735Manager);

    /**
     * @brief Virtuális destruktor - Automatikus cleanup
     */
    virtual ~RadioScreen() = default;

  protected:
    // ===================================================================
    // Közös vízszintes gombsor kezelés
    // ===================================================================    /// Közös vízszintes gombsor komponens (alsó navigációs gombok)
    std::shared_ptr<UIHorizontalButtonBar> horizontalButtonBar;

    /**
     * @brief Közös vízszintes gombsor létrehozása és inicializálása
     * @details Létrehozza a közös gombokat, amiket minden RadioScreen használ
     * A leszármazott osztályok ezt kiterjeszthetik saját specifikus gombokkal
     */
    virtual void createCommonHorizontalButtons();

    /**
     * @brief Közös vízszintes gombsor állapotainak szinkronizálása
     * @details Csak aktiváláskor hívódik meg! Event-driven architektúra.
     * A leszármazott osztályok felülírhatják saját specifikus állapotokkal
     */
    virtual void updateCommonHorizontalButtonStates();

    /**
     * @brief Lehetőség a leszármazott osztályoknak további gombok hozzáadására
     * @param buttonConfigs A már meglévő gomb konfigurációk vektora
     * @details A leszármazott osztályok felülírhatják ezt a metódust, hogy
     * hozzáadhassanak specifikus gombokat a közös gombokhoz
     */
    virtual void addSpecificHorizontalButtons(std::vector<UIHorizontalButtonBar::ButtonConfig> &buttonConfigs) {}

    // ===================================================================
    // Közös gomb eseménykezelők
    // ===================================================================

    /**
     * @brief HAM gomb eseménykezelő - Ham rádió funkcionalitás
     * @param event Gomb esemény (Clicked)
     * @details Virtuális függvény - leszármazott osztályok felülírhatják
     */
    virtual void handleHamButton(const UIButton::ButtonEvent &event);

    /**
     * @brief BAND gomb eseménykezelő - Sáv (Band) kezelés
     * @param event Gomb esemény (Clicked)
     * @details Virtuális függvény - leszármazott osztályok felülírhatják
     */
    virtual void handleBandButton(const UIButton::ButtonEvent &event);

    /**
     * @brief Közös BAND gomb eseménykezelő - Sáv (Band) kezelés
     * @param isHamBand Igaz, ha a Ham sávot kell kezelni, hamis, ha más sáv
     * @details Ez a metódus a közös BAND gomb eseménykezelés logikáját tartalmazza.
     */
    void processBandButton(bool isHamBand);

    /**
     * @brief SCAN gomb eseménykezelő - Folyamatos keresés
     * @param event Gomb esemény (Clicked)
     * @details Virtuális függvény - leszármazott osztályok felülírhatják
     */
    virtual void handleScanButton(const UIButton::ButtonEvent &event);

    // ===================================================================
    // RDS komponens kezelés
    // ===================================================================

    /// RDS (Radio Data System) komponens - FM rádió adatok megjelenítése
    std::shared_ptr<RDSComponent> rdsComponent;

    /**
     * @brief Létrehozza az RDS komponenst
     * @param rdsBounds Az RDS komponens határai (opcionális, most már nem szükséges)
     */
    inline void createRDSComponent(const Rect &rdsBounds = Rect(0, 0, 0, 0)) {
        rdsComponent = std::make_shared<RDSComponent>(tft, *pSi4735Manager, rdsBounds);
        addChild(rdsComponent);
    }

    // ===================================================================
    // Seek (automatikus állomáskeresés) infrastruktúra
    // ===================================================================

    /**
     * @brief Seek keresés indítása lefelé valós idejű frekvencia frissítéssel
     * @details Beállítja a callback infrastruktúrát és indítja a seek-et
     *
     * Művelet:
     * 1. Callback infrastruktúra beállítása
     * 2. SI4735 seekStationProgress hívás SEEK_DOWN irányban
     * 3. Valós idejű frekvencia frissítés a callback-en keresztül
     * 4. Konfiguráció és band tábla frissítése
     */
    void seekStationDown();

    /**
     * @brief Seek keresés indítása felfelé valós idejű frekvencia frissítéssel
     * @details Beállítja a callback infrastruktúrát és indítja a seek-et
     *
     * Művelet:
     * 1. Callback infrastruktúra beállítása
     * 2. SI4735 seekStationProgress hívás SEEK_UP irányban
     * 3. Valós idejű frekvencia frissítés a callback-en keresztül
     * 4. Konfiguráció és band tábla frissítése
     */
    void seekStationUp();

    // ===================================================================
    // Rádió-specifikus utility metódusok
    // ===================================================================

    /**
     * @brief RDS cache törlése frekvencia változáskor
     * @details Biztonságos RDS cache törlés null pointer ellenőrzéssel
     */
    inline void clearRDSCache() {
        if (rdsComponent) {
            rdsComponent->clearRdsOnFrequencyChange();
        }
    }

    /**
     * @brief Frekvencia mentése a konfigurációba és band táblába
     * @details Szinkronizálja az aktuális frekvenciát minden szükséges helyre
     */
    void saveCurrentFrequency();

    /**
     * @brief Ellenőrzi, hogy az aktuális frekvencia benne van-e a memóriában
     * @return true ha a frekvencia elmentett állomás, false egyébként
     */
    bool checkCurrentFrequencyInMemory() const;

    /**
     * @brief Ellenőrzi, hogy az aktuális frekvencia benne van-e a memóriában
     * @return true ha a frekvencia elmentett állomás, false egyébként
     *
     * @details Ellenőrzi az aktuális frekvenciát a StationStore memóriában.
     * Ha talál egyezést, frissíti a StatusLine státuszát is.
     *
     * Használat frekvencia változáskor (rotary encoder, seek):
     * - Meghívja a StationStore keresést
     * - Frissíti a StatusLine::updateStationInMemory státuszt
     */
    bool checkAndUpdateMemoryStatus();
};

#endif //__RADIO_SCREEN_H
