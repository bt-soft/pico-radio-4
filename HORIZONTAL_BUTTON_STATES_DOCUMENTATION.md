# UIHorizontalButtonBar Állapot Szinkronizáció

## Áttekintés

Az `updateHorizontalButtonStates()` metódus biztosítja, hogy a vízszintes gombsor gombjainak állapota mindig szinkronban legyen az aktuális rádió beállításokkal.

## Implementáció

### FMScreen.h
```cpp
// Gombállapot frissítő segédfunkciók
void updateVerticalButtonStates();
void updateHorizontalButtonStates();
```

### FMScreen.cpp
```cpp
void FMScreen::updateHorizontalButtonStates() {
    if (!horizontalButtonBar) {
        return;
    }

    // Aktuális sáv lekérdezése
    uint8_t currentBandType = pSi4735Manager->getCurrentBand().bandType;
    
    // AM gomb állapot szinkronizálása
    // AM gomb világít, ha nem FM módban vagyunk (tehát AM, MW, LW, SW módban)
    bool isAMMode = (currentBandType != FM_BAND_TYPE);
    horizontalButtonBar->setButtonState(FMScreenHorizontalButtonIDs::AM_BUTTON, 
                                      isAMMode ? UIButton::ButtonState::On : UIButton::ButtonState::Off);

    // Test és Setup gombok pushable típusúak, állapotuk nem változik
    // Ha szükséges, itt lehet további állapot szinkronizálást végezni
}
```

## Használat

### Automatikus Frissítés
A metódus automatikusan meghívásra kerül a `handleOwnLoop()` metódusban:

```cpp
void FMScreen::handleOwnLoop() {
    // S-Meter frissítése
    if (smeterComp) {
        SignalQualityData signalCache = pSi4735Manager->getSignalQuality();
        if (signalCache.isValid) {
            smeterComp->showRSSI(signalCache.rssi, signalCache.snr, true);
        }
    }

    // Függőleges gombállapotok frissítése
    updateVerticalButtonStates();
    
    // Vízszintes gombállapotok frissítése
    updateHorizontalButtonStates();
}
```

### Manuális Frissítés
Szükség esetén kézzel is meghívható:
```cpp
// Sáv váltás után
pSi4735Manager->switchToBand(newBandIndex);
updateHorizontalButtonStates();
```

## Gombállapot Logika

### AM Gomb
- **Világít (ON)**: Ha az aktuális sáv nem FM (MW, LW, SW)
- **Nem világít (OFF)**: Ha az aktuális sáv FM

### Test és Setup Gombok  
- **Pushable típusúak**: Állapotuk nem változik dinamikusan
- **Állandóan OFF állapotban**: Csak kattintáskor aktiválódnak

## Band Type Konstansok

A `Band.h` fájlból származó konstansok:
```cpp
#define FM_BAND_TYPE 0
#define MW_BAND_TYPE 1
#define SW_BAND_TYPE 2
#define LW_BAND_TYPE 3
```

## Kiterjesztés

További gombok állapot szinkronizálásához:

```cpp
void FMScreen::updateHorizontalButtonStates() {
    if (!horizontalButtonBar) {
        return;
    }

    uint8_t currentBandType = pSi4735Manager->getCurrentBand().bandType;
    
    // AM gomb
    bool isAMMode = (currentBandType != FM_BAND_TYPE);
    horizontalButtonBar->setButtonState(FMScreenHorizontalButtonIDs::AM_BUTTON, 
                                      isAMMode ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
    
    // Új gomb példa - Stereo indikátor
    if (currentBandType == FM_BAND_TYPE) {
        bool isStereo = pSi4735Manager->isStereo();
        horizontalButtonBar->setButtonState(FMScreenHorizontalButtonIDs::STEREO_BUTTON,
                                          isStereo ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
    }
}
```

## Kapcsolódó Komponensek

- **UIHorizontalButtonBar**: A vízszintes gombsor komponens
- **FMScreenHorizontalButtonIDs**: Gomb azonosítók namespace-e
- **Band.h**: Sáv típus konstansok
- **Si4735Manager**: Rádió állapot lekérdezés

## Teljesített Funkciók

✅ `updateHorizontalButtonStates()` metódus hozzáadva a header fájlhoz  
✅ Implementáció elkészítve a .cpp fájlban  
✅ Automatikus hívás a `handleOwnLoop()`-ban  
✅ AM gomb állapot szinkronizálás band type alapján  
✅ Fordítási hiba javítás (Band.h include hozzáadva)  
✅ Sikeres fordítás ellenőrzés  

## Következő Lépések

- További gombállapot szinkronizálások implementálása
- AMScreen hasonló funkcionalitás
- Egyéb képernyők (Setup, Test) gombállapot kezelése
