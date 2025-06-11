# UIVerticalButtonBar - Függőleges Gombsor Komponens

## Áttekintés

A `UIVerticalButtonBar` egy újrafelhasználható UI komponens, amely automatikusan elrendezi a gombokat függőlegesen. Speciálisan rádió képernyőkhöz tervezve, ahol gyakran használt funkció gombok vannak.

## Pozicionálás

### Alapértelmezett elhelyezés:
- **Függőleges gombok**: Jobb felső sarok (teljes képernyő széléhez illesztve)
- **Vízszintes gombok**: Bal alsó sarok (teljes képernyő széléhez illesztve)

### Pozicionálási koordináták:
```cpp
// Függőleges gombsor - jobb felső sarok
Rect(tft.width() - buttonBarWidth, 0, buttonBarWidth, tft.height())

// Vízszintes gombsor - bal alsó sarok  
Rect(0, tft.height() - buttonBarHeight, buttonBarWidth, buttonBarHeight)
```

## Implementáció

### 1. UIVerticalButtonBar Komponens

#### Fájlok:
- `include/UIVerticalButtonBar.h` - Header fájl
- `src/UIVerticalButtonBar.cpp` - Implementáció

#### Főbb tulajdonságok:
- Automatikus függőleges gomb elrendezés
- Konfigurálható gomb méretek és távolságok
- Egyszerű gomb konfiguráció struktúra
- ID alapú gomb állapot kezelés

#### Használat:

```cpp
// Gomb konfiguráció
std::vector<UIVerticalButtonBar::ButtonConfig> buttonConfigs = {
    {10, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, callback},
    {11, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, callback},
    // ... további gombok
};

// Gombsor létrehozása
auto buttonBar = std::make_shared<UIVerticalButtonBar>(
    tft, bounds, buttonConfigs, 60, 32, 4  // width, height, gap
);
```

### 2. FMScreen Integráció

#### Fájlok módosítva:
- `include/FMScreen.h` - Header kiegészítése
- `src/FMScreen.cpp` - Implementáció kiegészítése

#### Újlétrehozott gombok:
1. **Mute** - Toggleable, hang ki/bekapcsolása
2. **Vol** - Pushable, hangerő dialógus (TODO)
3. **AGC** - Toggleable, AGC ki/bekapcsolása (TODO)  
4. **Att** - Toggleable, attenuátor ki/bekapcsolása (TODO)
5. **Sql** - Pushable, squelch beállítás dialógus (TODO)
6. **Freq** - Pushable, frekvencia input dialógus (TODO)
7. **Setup** - Pushable, setup képernyőre váltás
8. **Memo** - Pushable, memória funkciók (TODO)

#### Gomb pozíció:
- Jobb oldali margó: 5px
- Gomb szélesség: 60px
- Gomb magasság: 32px
- Gombok közötti távolság: 4px
- Y pozíció: 80px (StatusLine és FreqDisplay után)

## Implementált Funkciók

### ✅ Kész funkciók:
- [x] UIVerticalButtonBar komponens
- [x] FMScreen integráció
- [x] Mute gomb működése (`rtv::muteStat` + `getSi4735().setAudioMute()`)
- [x] Setup gomb működése (képernyőváltás)
- [x] Gombállapot szinkronizálás (mute gomb)
- [x] Projekt lefordítás

### 🔄 TODO funkciók:
- [ ] Volume dialógus implementálása
- [ ] AGC ki/bekapcsolás implementálása  
- [ ] Attenuátor ki/bekapcsolás implementálása
- [ ] Squelch beállító dialógus
- [ ] Frekvencia input dialógus
- [ ] Memória funkciók dialógus
- [ ] További gombállapot szinkronizálások

## Gomb ID Konstansok

```cpp
namespace FMScreenButtonIDs {
    static constexpr uint8_t MUTE = 10;
    static constexpr uint8_t VOLUME = 11;
    static constexpr uint8_t AGC = 12;
    static constexpr uint8_t ATT = 13;
    static constexpr uint8_t SQUELCH = 14;
    static constexpr uint8_t FREQ = 15;
    static constexpr uint8_t SETUP = 16;
    static constexpr uint8_t MEMO = 17;
}
```

## API Használat

### Gombállapot beállítása:
```cpp
verticalButtonBar->setButtonState(FMScreenButtonIDs::MUTE, UIButton::ButtonState::On);
```

### Gombállapot lekérdezése:
```cpp
UIButton::ButtonState state = verticalButtonBar->getButtonState(FMScreenButtonIDs::MUTE);
```

### Gomb referencia megszerzése:
```cpp
auto button = verticalButtonBar->getButton(FMScreenButtonIDs::MUTE);
```

## Újrafelhasználhatóság

A `UIVerticalButtonBar` komponens könnyen használható más képernyőkön is:

### AMScreen-ben:
```cpp
// Hasonló implementáció, más gomb konfigurációval
createVerticalButtonBar(); // AMScreen specifikus gombok
```

### Vízszintes elrendezés:
A koncepció alapján könnyen létrehozható egy `UIHorizontalButtonBar` is.

## Következő Lépések

1. **Volume Dialog** implementálása `ValueChangeDialog` használatával
2. **AGC/Attenuator** funkciók implementálása a Si4735Manager-ben
3. **Squelch Dialog** implementálása
4. **Frequency Input Dialog** implementálása
5. **Memory Dialog** implementálása
6. **AMScreen** hasonló implementációja
7. **UIHorizontalButtonBar** komponens létrehozása (opcionális)

## Tanulságok

- A `UIVerticalButtonBar` jól elkülöníti a gomb layout logikát
- A gomb ID alapú kezelés egyszerűsíti az állapot szinkronizálást  
- A callback függvények rugalmas eseménykezelést biztosítanak
- A `getSi4735()` metódus használata szükséges a protected si4735 tag eléréséhez
- Az `rtv::muteStat` globális változó szinkronizálása fontos a helyes működéshez
