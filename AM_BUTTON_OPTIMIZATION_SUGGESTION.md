# AM Screen Gombsor Optimalizálási Javaslat

## Jelenlegi Helyzet - Kiváló Alapok
A jelenlegi AM és FM képernyők 87.5%-ban (7/8 gomb) azonos funkcionalitással rendelkeznek, ami **kiváló konzisztenciát** biztosít.

## Opcionális Optimalizálás - AM Specifikus Testreszabás

### Squelch Gomb Alternatívák AM Módban

```cpp
// Jelenlegi (universal approach)
{AMScreenButtonIDs::SQUELCH, "Sql", UIButton::ButtonType::Pushable, 
 UIButton::ButtonState::Off, [this](auto &e) { handleSquelchButton(e); }}

// Alternatíva 1: AM Bandwidth Control
{AMScreenButtonIDs::BANDWIDTH, "BW", UIButton::ButtonType::Pushable, 
 UIButton::ButtonState::Off, [this](auto &e) { handleBandwidthButton(e); }}

// Alternatíva 2: AM Noise Filter
{AMScreenButtonIDs::FILTER, "Filter", UIButton::ButtonType::Toggleable, 
 UIButton::ButtonState::Off, [this](auto &e) { handleNoiseFilterButton(e); }}

// Alternatíva 3: AM Step Size
{AMScreenButtonIDs::STEP, "Step", UIButton::ButtonType::Pushable, 
 UIButton::ButtonState::Off, [this](auto &e) { handleStepSizeButton(e); }}
```

## Implementációs Opciók

### Opció 1: Megtartani a Jelenlegi Universal Megközelítést ✅ AJÁNLOTT
**Előnyök:**
- Felhasználó nem kell újratanuljon gombokat
- Egyszerű karbantartás  
- Konzisztens UX FM és AM között
- Squelch AM-ben is használható (RSSI alapú)

### Opció 2: AM Specifikus Optimalizálás
**Előnyök:**
- AM specifikus funkciók könnyebb elérése
- Optimális AM hangolási élmény

**Hátrányok:**
- Felhasználóknak két különböző gombkiosztást kell megjegyezni
- Komplexebb kódkarbantartás

## Ajánlás: Megtartani a Jelenlegi Megközelítést

A jelenlegi implementáció **kiváló egyensúlyt** teremt:
1. 87.5% kompatibilitás FM és AM között
2. Squelch AM-ben is értelmes (RSSI küszöb)
3. Egyszerű karbantarthatóság
4. Konzisztens felhasználói élmény

### Ha Mégis Optimalizálni Szeretnéd

Használj **conditional gomb konfigurációt**:

```cpp
void AMScreen::createVerticalButtonBar() {
    std::vector<UIVerticalButtonBar::ButtonConfig> configs = {
        {MUTE, "Mute", Toggleable, Off, handleMuteButton},
        {VOLUME, "Vol", Pushable, Off, handleVolumeButton},
        {AGC, "AGC", Toggleable, Off, handleAGCButton},
        {ATT, "Att", Toggleable, Off, handleAttButton},
        
        // Conditional: AM specifikus vs Universal
        #ifdef AM_SPECIFIC_BUTTONS
            {BANDWIDTH, "BW", Pushable, Off, handleBandwidthButton},
        #else
            {SQUELCH, "Sql", Pushable, Off, handleSquelchButton},
        #endif
        
        {FREQ, "Freq", Pushable, Off, handleFreqButton},
        {SETUP, "Setup", Pushable, Off, handleSetupButton},
        {MEMO, "Memo", Pushable, Off, handleMemoButton}
    };
}
```

## Összefoglalás

A **jelenlegi implementáció kiváló** és érdemes megtartani. Ha optimalizálás szükséges, akkor kondicionális megközelítést javasolok a kompatibilitás megőrzése érdekében.
