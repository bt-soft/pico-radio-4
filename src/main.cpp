/**
 * @file main.cpp
 * @brief Core0/1 fő programfájl, amely az Arduino környezetet inicializálja és elindítja a fő ciklust.
 * @details Ez a fájl tartalmazza a szükséges könyvtárakat, a képernyőkezelést, a rotary enkódert és az SI4735 rádiómodult.
 */

#include <Arduino.h>

#include "PicoMemoryInfo.h"
#include "PicoSensorUtils.h"
#include "ScreenManager.h"
#include "SplashScreen.h"
#include "UIComponent.h"
#include "defines.h"
#include "pins.h"
#include "utils.h"

//-------------------- Config
#include "BandStore.h"
#include "Config.h"
#include "EepromLayout.h"
#include "StationStore.h"
#include "StoreEepromBase.h"
extern Config config;
extern FmStationStore fmStationStore;
extern AmStationStore amStationStore;
extern BandStore bandStore;

//------------------ TFT
#include <TFT_eSPI.h>
TFT_eSPI tft;

//-------------------- Screens
// Globális képernyőkezelő pointer - inicializálás a setup()-ban történik
ScreenManager *screenManager = nullptr;

//------------------ SI4735
#include "Si4735Manager.h"
Si4735Manager *si4735Manager = nullptr; // Si4735Manager: NEM lehet (hardware inicializálás miatt) statikus, mert HW inicializálások is vannak benne

//------------------- Rotary Encoder
#include <RPi_Pico_TimerInterrupt.h>
RPI_PICO_Timer rotaryTimer(0); // 0-ás timer használata
#include "RotaryEncoder.h"
RotaryEncoder rotaryEncoder = RotaryEncoder(PIN_ENCODER_CLK, PIN_ENCODER_DT, PIN_ENCODER_SW, ROTARY_ENCODER_STEPS_PER_NOTCH);
#define ROTARY_ENCODER_SERVICE_INTERVAL_IN_MSEC 1 // 1msec

/**
 * @brief  Hardware timer interrupt service routine a rotaryhoz
 */
bool rotaryTimerHardwareInterruptHandler(struct repeating_timer *t) {
    rotaryEncoder.service();
    return true;
}

/**
 * @brief Core0 Fő függvény, amely a program belépési pontja.
 * @details Ez a függvény inicializálja az Arduino környezetet és elindítja a fő
 * ciklust.
 */
void setup() {
#ifdef __DEBUG
    Serial.begin(115200);
#endif

    // PICO AD inicializálása
    PicoSensorUtils::init();

    // Beeper
    pinMode(PIN_BEEPER, OUTPUT);
    digitalWrite(PIN_BEEPER, LOW); // TFT LED háttérvilágítás kimenet
    pinMode(PIN_TFT_BACKGROUND_LED, OUTPUT);
    Utils::setTftBacklight(TFT_BACKGROUND_LED_MAX_BRIGHTNESS); // TFT inicializálása DC módban
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK); // Fekete háttér a splash screen-hez

    // UI komponensek számára képernyő méretek inicializálása
    UIComponent::initScreenDimensions(tft);

#ifdef DEBUG_WAIT_FOR_SERIAL
    Utils::debugWaitForSerial(tft);
#endif

    // Csak az általános információkat jelenítjük meg először (SI4735 nélkül)
    // Program cím és build info megjelenítése
    tft.setFreeFont();
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(PROGRAM_NAME, tft.width() / 2, 20);

    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("Version " + String(PROGRAM_VERSION), tft.width() / 2, 50);
    tft.drawString(PROGRAM_AUTHOR, tft.width() / 2, 70);

    // Build info
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("Build: " + String(__DATE__) + " " + String(__TIME__), tft.width() / 2, 100);

    // Inicializálási progress
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("Initializing...", tft.width() / 2, 140);

    // EEPROM inicializálása (A fordítónak muszáj megadni egy típust, itt most egy Config_t-t használunk, igaziból mindegy)
    tft.drawString("Loading EEPROM...", tft.width() / 2, 160);
    StoreEepromBase<Config_t>::init(); // Meghívjuk a statikus init metódust

    // Ha a bekapcsolás alatt nyomva tartjuk a rotary gombját, akkor töröljük a konfigot
    if (digitalRead(PIN_ENCODER_SW) == LOW) {
        DEBUG("Encoder button pressed during startup, restoring defaults...\n");
        Utils::beepTick();
        delay(1500);
        if (digitalRead(PIN_ENCODER_SW) == LOW) { // Ha még mindig nyomják

            DEBUG("Restoring default settings...\n");
            Utils::beepTick();
            config.loadDefaults();
            fmStationStore.loadDefaults();
            amStationStore.loadDefaults();
            bandStore.loadDefaults();

            DEBUG("Save default settings...\n");
            Utils::beepTick();
            config.checkSave();
            bandStore.checkSave(); // Band adatok mentése
            fmStationStore.checkSave();
            amStationStore.checkSave();

            Utils::beepTick();
            DEBUG("Default settings resored!\n");
        }
    } else {
        // konfig betöltése
        tft.drawString("Loading config...", tft.width() / 2, 180);
        config.load();
    }

    // Rotary Encoder beállítása
    rotaryEncoder.setDoubleClickEnabled(true);                                  // Dupla kattintás engedélyezése
    rotaryEncoder.setAccelerationEnabled(config.data.rotaryAcceleratonEnabled); // Gyorsítás engedélyezése a rotary enkóderhez
    // Pico HW Timer1 beállítása a rotaryhoz
    rotaryTimer.attachInterruptInterval(ROTARY_ENCODER_SERVICE_INTERVAL_IN_MSEC * 1000, rotaryTimerHardwareInterruptHandler);

    // Kell kalibrálni a TFT Touch-t?
    if (Utils::isZeroArray(config.data.tftCalibrateData)) {
        Utils::beepError();
        Utils::tftTouchCalibrate(tft, config.data.tftCalibrateData);
    }

    // Beállítjuk a touch scren-t
    tft.setTouch(config.data.tftCalibrateData); // Állomáslisták és band adatok betöltése az EEPROM-ból (a config után!)
    tft.drawString("Loading stations & bands...", tft.width() / 2, 200);
    bandStore.load(); // Band adatok betöltése
    fmStationStore.load();
    amStationStore.load();

    // Splash screen megjelenítése inicializálás közben
    // Most átváltunk a teljes splash screen-re az SI4735 infókkal
    SplashScreen splash(tft);
    splash.show(true, 6);

    // Splash screen megjelenítése progress bar-ral    // Lépés 1: I2C inicializálás
    splash.updateProgress(1, 6, "Initializing I2C...");

    // Az si473x (Nem a default I2C lábakon [4,5] van!!!)
    Wire.setSDA(PIN_SI4735_I2C_SDA); // I2C for SI4735 SDA
    Wire.setSCL(PIN_SI4735_I2C_SCL); // I2C for SI4735 SCL
    Wire.begin();
    delay(300); // Si4735Manager inicializálása itt
    splash.updateProgress(2, 6, "Initializing SI4735 Manager...");
    if (si4735Manager == nullptr) {
        si4735Manager = new Si4735Manager();
        // BandStore beállítása a Si4735Manager-ben
        si4735Manager->setBandStore(&bandStore);
    }

    // KRITIKUS: Band tábla dinamikus adatainak EGYSZERI inicializálása RÖGTÖN a Si4735Manager létrehozása után!
    si4735Manager->initializeBandTableData(true); // forceReinit = true az első inicializálásnál

    // Si4735 inicializálása
    splash.updateProgress(3, 6, "Detecting SI4735...");
    int16_t si4735Addr = si4735Manager->getDeviceI2CAddress();
    if (si4735Addr == 0) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.setTextSize(2);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("SI4735 NOT DETECTED!", tft.width() / 2, tft.height() / 2);
        DEBUG("Si4735 not detected");
        Utils::beepError();
        while (true) // nem megyünk tovább
            ;
    } // Lépés 4: SI4735 konfigurálás
    splash.updateProgress(4, 6, "Configuring SI4735...");
    si4735Manager->setDeviceI2CAddress(si4735Addr == 0x11 ? 0 : 1); // Sets the I2C Bus Address, erre is szükség van...    splash.drawSI4735Info(si4735Manager->getSi4735());

    delay(300);
    //--------------------------------------------------------------------

    // Lépés 5: Frekvencia beállítások
    splash.updateProgress(5, 6, "Setting up radio...");
    si4735Manager->init(true);
    si4735Manager->getSi4735().setVolume(config.data.currVolume); // Hangerő visszaállítása

    delay(100);

    // Kezdő képernyőtípus beállítása
    splash.updateProgress(6, 6, "Preparing display...");
    const char *startScreeName = si4735Manager->getCurrentBandType() == FM_BAND_TYPE ? SCREEN_NAME_FM : SCREEN_NAME_AM;
    delay(100);

    //--------------------------------------------------------------------
    // Lépés 7: Finalizálás
    splash.updateProgress(7, 7, "Starting up...");

    Serial.println("Creating ScreenManager...");
    Serial.flush();
    // ScreenManager inicializálása itt, amikor minden más már kész
    if (screenManager == nullptr) {
        screenManager = new ScreenManager(tft);
    }
    Serial.println("ScreenManager created successfully");
    Serial.flush();

    Serial.println("Switching to start screen...");
    Serial.flush();
    screenManager->switchToScreen(startScreeName); // A kezdő képernyő
    Serial.println("Screen switch completed");
    Serial.flush();

    delay(100); // Rövidebb delay

    // Splash screen eltűntetése
    splash.hide();

    //--------------------------------------------------------------------

    // Csippantunk egyet
    Utils::beepTick();
}

/**
 * @brief Core0 loop függvény, amely a fő ciklust kezeli.
 * @details Ez a függvény folyamatosan fut, és kezeli a program fő logikáját.
 */
void loop() {
    //------------------- EEPROM mentés figyelése
#define EEPROM_SAVE_CHECK_INTERVAL 1000 * 60 * 5 // 5 perc
    static uint32_t lastEepromSaveCheck = 0;
    if (millis() - lastEepromSaveCheck >= EEPROM_SAVE_CHECK_INTERVAL) {
        config.checkSave();
        bandStore.checkSave(); // Band adatok mentése
        fmStationStore.checkSave();
        amStationStore.checkSave();
        lastEepromSaveCheck = millis();
    }
//------------------- Memória információk megjelenítése
#ifdef SHOW_MEMORY_INFO
    static uint32_t lasDebugMemoryInfo = 0;
    if (millis() - lasDebugMemoryInfo >= MEMORY_INFO_INTERVAL) {
        PicoMemoryInfo::debugMemoryInfo();
        lasDebugMemoryInfo = millis();
    }
#endif

    //------------------- Touch esemény kezelése
    uint16_t touchX, touchY;
    bool touchedRaw = tft.getTouch(&touchX, &touchY);
    bool validCoordinates = true;
    if (touchedRaw) {
        if (touchX > tft.width() || touchY > tft.height()) {
            validCoordinates = false;
        }
    }

    static bool lastTouchState = false;
    static uint16_t lastTouchX = 0, lastTouchY = 0;
    bool touched = touchedRaw && validCoordinates;

    // Touch press event (immediate response)
    if (touched && !lastTouchState) {
        TouchEvent touchEvent(touchX, touchY, true);
        if (screenManager)
            screenManager->handleTouch(touchEvent);
        lastTouchX = touchX;
        lastTouchY = touchY;
    }
    // Touch release event (immediate response)
    else if (!touched && lastTouchState) {
        TouchEvent touchEvent(lastTouchX, lastTouchY, false);
        if (screenManager)
            screenManager->handleTouch(touchEvent);
    }

    lastTouchState = touched;

    //------------------- Rotary Encoder esemény kezelése

    // Rotary Encoder olvasása
    RotaryEncoder::EncoderState encoderState = rotaryEncoder.read();

    // Rotary encoder eseményeinek továbbítása a ScreenManager-nek
    if (encoderState.direction != RotaryEncoder::Direction::None || encoderState.buttonState != RotaryEncoder::ButtonState::Open) {

        // RotaryEvent létrehozása a ScreenManager típusaival
        RotaryEvent::Direction direction = RotaryEvent::Direction::None;
        if (encoderState.direction == RotaryEncoder::Direction::Up) {
            direction = RotaryEvent::Direction::Up;
        } else if (encoderState.direction == RotaryEncoder::Direction::Down) {
            direction = RotaryEvent::Direction::Down;
        }

        RotaryEvent::ButtonState buttonState = RotaryEvent::ButtonState::NotPressed;
        if (encoderState.buttonState == RotaryEncoder::ButtonState::Clicked) {
            buttonState = RotaryEvent::ButtonState::Clicked;
        } else if (encoderState.buttonState == RotaryEncoder::ButtonState::DoubleClicked) {
            buttonState = RotaryEvent::ButtonState::DoubleClicked;
        }

        // Esemény továbbítása a ScreenManager-nek
        RotaryEvent rotaryEvent(direction, buttonState, encoderState.value);
        if (screenManager)
            screenManager->handleRotary(rotaryEvent);
        // bool handled = screenManager->handleRotary(rotaryEvent);
        // DEBUG("Rotary event handled by screen: %s\n", handled ? "YES" : "NO");
    }

    // Deferred actions feldolgozása - biztonságos képernyőváltások végrehajtása
    if (screenManager) {
        screenManager->processDeferredActions();
    }

    // Képernyőkezelő loop hívása
    if (screenManager) {
        screenManager->loop();
    }

    // Képernyő rajzolása (csak szükség esetén, korlátozott gyakorisággal)
    static uint32_t lastDrawTime = 0;
    const uint32_t DRAW_INTERVAL = 16; // ~60 FPS (16ms között rajzolás) - UI gyorsabb frissítés
    if (millis() - lastDrawTime >= DRAW_INTERVAL) {
        if (screenManager) {
            screenManager->draw();
        }
        lastDrawTime = millis();
    }

    // SI4735 loop hívása, squelch és hardver némítás kezelése
    if (si4735Manager) {
        si4735Manager->loop();
    }
}
