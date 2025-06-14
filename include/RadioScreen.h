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
#include "UIScreen.h"

// Forward deklaráció a seek callback függvényhez
void radioSeekProgressCallback(uint16_t frequency);

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
    // RDS komponens kezelés
    // ===================================================================

    /// RDS (Radio Data System) komponens - FM rádió adatok megjelenítése
    std::shared_ptr<RDSComponent> rdsComponent;

    /**
     * @brief Létrehozza az RDS komponenst
     * @param rdsBounds Az RDS komponens határai (Rect)
     */
    inline void createRDSComponent(Rect rdsBounds) {
        rdsComponent = std::make_shared<RDSComponent>(tft, rdsBounds, *pSi4735Manager);
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
    } /**
       * @brief Frekvencia mentése a konfigurációba és band táblába
       * @details Szinkronizálja az aktuális frekvenciát minden szükséges helyre
       */
    void saveCurrentFrequency();

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
