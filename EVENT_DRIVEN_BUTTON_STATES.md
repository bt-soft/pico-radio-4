# Event-Driven Button State Management

## Probléma leírása

Az eredeti implementációban a gombállapotok minden loop ciklusban frissültek:

```cpp
void FMScreen::handleOwnLoop() {
    // ... S-Meter frissítése ...
    
    // ❌ ROSSZ: Folyamatos pollozás minden ciklusban
    updateVerticalButtonStates();
    updateHorizontalButtonStates();
}
```

Ez nem optimális működési modell, még akkor sem, ha a `UIButton::setButtonState()` optimalizálja a redundáns hívásokat.

## Új megoldás: Event-driven megközelítés

### 1. Aktiválás alapú szinkronizálás

Gombállapotok **csak akkor** frissülnek, amikor:
- A képernyő aktiválódik (`activate()` metódus)
- Specifikus események történnek (pl. mute állapot változás)

```cpp
void FMScreen::activate() {
    DEBUG("FMScreen activated - syncing button states\n");
    
    // Szülő activate() hívása
    UIScreen::activate();
    
    // Gombállapotok szinkronizálása CSAK az aktiváláskor
    updateVerticalButtonStates();
    updateHorizontalButtonStates();
}
```

### 2. Eseménykezelő alapú frissítés

Az eseménykezelőkben **nem** állítjuk be manuálisan a gomb állapotát:

```cpp
void FMScreen::handleMuteButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::On) {
        DEBUG("FMScreen: Mute ON\n");
        rtv::muteStat = true;
        pSi4735Manager->getSi4735().setAudioMute(true);
    } else if (event.state == UIButton::EventButtonState::Off) {
        DEBUG("FMScreen: Mute OFF\n");
        rtv::muteStat = false;
        pSi4735Manager->getSi4735().setAudioMute(false);
    }
    // ✅ A gomb állapotát már a UIButton automatikusan beállítja
    // Nincs szükség manuális setButtonState() hívásra
}
```

### 3. Tiszta loop logika

```cpp
void FMScreen::handleOwnLoop() {
    // S-Meter frissítése
    if (smeterComp) {
        SignalQualityData signalCache = pSi4735Manager->getSignalQuality();
        if (signalCache.isValid) {
            smeterComp->showRSSI(signalCache.rssi, signalCache.snr, true);
        }
    }

    // ✅ Gombállapotok már az eseménykezelőkben és activate()-ben frissülnek
    // Nincs szükség folyamatos pollozásra
}
```

## Előnyök

### 1. **Jobb teljesítmény**
- Nincs felesleges pollozás minden loop ciklusban
- CPU terhelés csökkentése

### 2. **Tisztább architektúra**
- Event-driven megközelítés
- Állapotváltozások csak akkor, amikor szükséges

### 3. **Karbantarthatóság**
- Egyértelműbb kód struktúra
- Könnyebb hibakeresés

## Használat más képernyőkön

Ez a minta minden képernyőn alkalmazható:

```cpp
class AMScreen : public UIScreen {
public:
    virtual void activate() override {
        UIScreen::activate();
        updateButtonStates(); // Csak aktiváláskor
    }
    
    virtual void handleOwnLoop() override {
        // Csak a valóban szükséges loop logika
        updateSignalMeter();
        // NINCS button state update!
    }
    
private:
    void handleMuteButton(const UIButton::ButtonEvent &event) {
        // Állapotváltoztatás
        if (event.state == UIButton::EventButtonState::On) {
            rtv::muteStat = true;
            // ...
        }
        // UIButton automatikusan kezeli a vizuális állapotot
    }
};
```

## Összefoglalás

✅ **ÚJ**: Event-driven, activate() alapú szinkronizálás
❌ **RÉGI**: Folyamatos loop-based pollozás

Ez egy sokkal elegánsabb és hatékonyabb megoldás a gombállapot kezelésre.
