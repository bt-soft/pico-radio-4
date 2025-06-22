/**
 * @file ScanScreen.cpp
 * @brief Spektrum analizátor scan képernyő implementáció (átdolgozott, dinamikus pásztázással és zoommal)
 */

#include "ScanScreen.h"
#include "ScreenManager.h"
#include "defines.h"
#include "rtVars.h"

// ===================================================================
// Konstruktor és inicializálás
// ===================================================================

ScanScreen::ScanScreen(TFT_eSPI &tft, Si4735Manager *si4735Manager) : UIScreen(tft, SCREEN_NAME_SCAN, si4735Manager) {
    DEBUG("ScanScreen initialized\n");

    // Scan állapot inicializálása
    scanState = ScanState::Idle;
    scanMode = ScanMode::Spectrum;
    scanPaused = true;
    lastScanTime = 0;

    // Frekvencia beállítások
    currentScanFreq = 0;
    scanStartFreq = 0;
    scanEndFreq = 0;
    scanStep = 1.0f;
    zoomLevel = 1.0f;
    currentScanPos = 0;

    // Sáv határok
    scanBeginBand = -1;
    scanEndBand = SCAN_AREA_WIDTH;
    scanMarkSNR = 3;
    scanEmpty = true; // Konfiguráció
    countScanSignal = 3;
    signalScale = 2.0f;

    // UI cache inicializálása
    lastStatusText = ""; // Tömbök inicializálása
    for (int i = 0; i < SCAN_AREA_WIDTH; i++) {
        scanValueRSSI[i] = SCAN_AREA_Y + SCAN_AREA_HEIGHT; // Spektrum alján kezdjük (nincs látható jel)
        scanValueSNR[i] = 0;
        scanMark[i] = false;
        scanScaleLine[i] = 0;
    }

    layoutComponents();
}

// ===================================================================
// UIScreen interface implementáció
// ===================================================================

void ScanScreen::activate() {
    UIScreen::activate();
    initializeScan();
    calculateScanParameters();
    resetScan();
}

void ScanScreen::deactivate() {
    stopScan();
    UIScreen::deactivate();
    DEBUG("ScanScreen deactivated\n");
}

void ScanScreen::layoutComponents() {

    // Vízszintes gombsor létrehozása
    createHorizontalButtonBar();
}

void ScanScreen::createHorizontalButtonBar() {
    constexpr int16_t margin = 5;
    uint16_t buttonHeight = UIButton::DEFAULT_BUTTON_HEIGHT;
    uint16_t buttonY = UIComponent::SCREEN_H - UIButton::DEFAULT_BUTTON_HEIGHT - margin;
    uint16_t buttonWidth = 70;
    uint16_t buttonSpacing = 5;

    // Play/Pause gomb
    uint16_t playPauseX = margin;
    Rect playPauseRect(playPauseX, buttonY, buttonWidth, buttonHeight);
    playPauseButton = std::make_shared<UIButton>(tft, PLAY_PAUSE_BUTTON_ID, playPauseRect, "Play", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                 [this](const UIButton::ButtonEvent &event) {
                                                     if (event.state == UIButton::EventButtonState::Clicked) {
                                                         if (scanPaused) {
                                                             startScan();
                                                         } else {
                                                             pauseScan();
                                                         }
                                                     }
                                                 });
    addChild(playPauseButton);

    // Zoom In gomb
    uint16_t zoomInX = playPauseX + buttonWidth + buttonSpacing;
    Rect zoomInRect(zoomInX, buttonY, buttonWidth, buttonHeight);
    zoomInButton = std::make_shared<UIButton>(tft, ZOOM_IN_BUTTON_ID, zoomInRect, "Zoom+", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                              [this](const UIButton::ButtonEvent &event) {
                                                  if (event.state == UIButton::EventButtonState::Clicked) {
                                                      zoomIn();
                                                  }
                                              });
    addChild(zoomInButton);

    // Zoom Out gomb
    uint16_t zoomOutX = zoomInX + buttonWidth + buttonSpacing;
    Rect zoomOutRect(zoomOutX, buttonY, buttonWidth, buttonHeight);
    zoomOutButton = std::make_shared<UIButton>(tft, ZOOM_OUT_BUTTON_ID, zoomOutRect, "Zoom-", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                               [this](const UIButton::ButtonEvent &event) {
                                                   if (event.state == UIButton::EventButtonState::Clicked) {
                                                       zoomOut();
                                                   }
                                               });
    addChild(zoomOutButton);

    // Reset gomb
    uint16_t resetX = zoomOutX + buttonWidth + buttonSpacing;
    Rect resetRect(resetX, buttonY, buttonWidth, buttonHeight);
    resetButton = std::make_shared<UIButton>(tft, RESET_BUTTON_ID, resetRect, "Reset", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                             [this](const UIButton::ButtonEvent &event) {
                                                 if (event.state == UIButton::EventButtonState::Clicked) {
                                                     resetScan();
                                                 }
                                             });
    addChild(resetButton);

    // Back gomb (jobbra igazítva)
    uint16_t backButtonWidth = 60;
    uint16_t backButtonX = UIComponent::SCREEN_W - backButtonWidth - margin;
    Rect backButtonRect(backButtonX, buttonY, backButtonWidth, buttonHeight);
    backButton = std::make_shared<UIButton>(tft, BACK_BUTTON_ID, backButtonRect, "Back", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                            [this](const UIButton::ButtonEvent &event) {
                                                if (event.state == UIButton::EventButtonState::Clicked) {
                                                    if (getScreenManager()) {
                                                        getScreenManager()->goBack();
                                                    }
                                                }
                                            });
    addChild(backButton);
}

void ScanScreen::drawContent() {
    tft.fillScreen(TFT_BLACK);

    // Cím rajzolása
    tft.setFreeFont();
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("SPECTRUM ANALYZER", tft.width() / 2, 10);

    // Spektrum terület kerete
    tft.drawRect(SCAN_AREA_X - 1, SCAN_AREA_Y - 1, SCAN_AREA_WIDTH + 2, SCAN_AREA_HEIGHT + 2, TFT_WHITE);

    // Spektrum rajzolása
    drawSpectrum();

    // Skála rajzolása
    drawScale();

    // Frekvencia címkék
    drawFrequencyLabels(); // Sáv határok
    drawBandBoundaries();

    // Statikus címkék egyszeri kirajzolás
    drawScanInfoStatic();

    // Változó információk
    drawScanInfo();
}

void ScanScreen::handleOwnLoop() {
    if (scanState == ScanState::Scanning && !scanPaused) {
        uint32_t currentTime = millis();
        if (currentTime - lastScanTime > 50) { // 50ms késleltetés a mérések között
            updateScan();
            lastScanTime = currentTime;
        }
    }
}

bool ScanScreen::handleTouch(const TouchEvent &event) { return UIScreen::handleTouch(event); }

bool ScanScreen::handleRotary(const RotaryEvent &event) {
    if (event.direction == RotaryEvent::Direction::Up) {
        // Jobbra forgatás - frekvencia növelése vagy zoom
        if (scanPaused) {
            // Ha pauseban van, akkor frekvencia változtatás
            uint32_t newFreq = currentScanFreq + (uint32_t)(scanStep * 10);
            if (newFreq <= scanEndFreq) {
                currentScanFreq = newFreq;
                setFrequency(currentScanFreq);
                drawScanInfo();
            }
        }
        return true;
    } else if (event.direction == RotaryEvent::Direction::Down) {
        // Balra forgatás - frekvencia csökkentése
        if (scanPaused) {
            uint32_t newFreq = currentScanFreq - (uint32_t)(scanStep * 10);
            if (newFreq >= scanStartFreq) {
                currentScanFreq = newFreq;
                setFrequency(currentScanFreq);
                drawScanInfo();
            }
        }
        return true;
    }
    return false;
}

// ===================================================================
// Scan inicializálás és vezérlés
// ===================================================================

void ScanScreen::initializeScan() {
    if (!pSi4735Manager)
        return;

    // Aktuális sáv információk lekérése a Si4735Manager-ből
    BandTable &currentBand = pSi4735Manager->getCurrentBand();

    scanStartFreq = currentBand.minimumFreq * 10.0; // 87500; // 87.5 MHz (FM sáv)
    scanEndFreq = currentBand.maximumFreq * 10.0;   // 108000;  // 108.0 MHz
    currentScanFreq = scanStartFreq;

    DEBUG("Scan initialized: %d - %d kHz\n", scanStartFreq, scanEndFreq);
}

void ScanScreen::calculateScanParameters() {
    // Scan lépésköz számítása a zoom szint alapján
    uint32_t totalBandwidth = scanEndFreq - scanStartFreq;
    scanStep = (float)totalBandwidth / (SCAN_AREA_WIDTH * zoomLevel);

    // Minimális lépésköz korlátozása
    if (scanStep < 1.0f)
        scanStep = 1.0f;

    DEBUG("Scan parameters: step=%.2f kHz, zoom=%.2f\n", scanStep, zoomLevel);
}

void ScanScreen::resetScan() {
    // Scan állapot teljes visszaállítása
    scanState = ScanState::Idle;
    scanPaused = true;
    scanEmpty = true;
    currentScanPos = 0;
    zoomLevel = 1.0f; // Zoom visszaállítása 1.0x-ra    // Frekvencia alaphelyzetbe állítása
    currentScanFreq = scanStartFreq;

    // UI cache visszaállítása
    lastStatusText = ""; // Scan paraméterek újraszámítása
    calculateScanParameters();

    // Adatok törlése
    for (int i = 0; i < SCAN_AREA_WIDTH; i++) {
        scanValueRSSI[i] = SCAN_AREA_Y + SCAN_AREA_HEIGHT; // Spektrum alján kezdjük (nincs látható jel)
        scanValueSNR[i] = 0;
        scanMark[i] = false;
        scanScaleLine[i] = 0;
    }

    // Sáv határok újraszámítása
    scanBeginBand = -1;
    scanEndBand = SCAN_AREA_WIDTH;
    // Gomb állapot frissítése
    if (playPauseButton) {
        playPauseButton->setLabel("Play"); // Reset után Play gomb
    } // Hang visszakapcsolása reset után
    if (pSi4735Manager) {
        // VALÓS audio unmute visszakapcsolása
        pSi4735Manager->getSi4735().setAudioMute(false);
        DEBUG("Audio unmuted - reset\n");
    }

    // Csak a spektrum területet rajzoljuk újra, ne a teljes képernyőt
    drawSpectrum();
    drawScale();
    drawFrequencyLabels();
    drawBandBoundaries();
    drawScanInfo();

    DEBUG("Scan reset - back to initial state\n");
}

void ScanScreen::startScan() {
    scanPaused = false;
    scanState = ScanState::Scanning;
    lastScanTime = millis();

    // Audio némítás a scan közben (gyors frekvencia váltások miatt)
    if (pSi4735Manager) {
        // VALÓS audio mute bekapcsolása
        pSi4735Manager->getSi4735().setAudioMute(true);
        DEBUG("Audio muted for scanning\n");
    }
    if (playPauseButton) {
        playPauseButton->setLabel("Pause"); // Scan közben Pause gomb
    }

    DEBUG("Scan started\n");
}

void ScanScreen::pauseScan() {
    scanPaused = true;

    // Hang visszakapcsolása pause módban, hogy hallhassuk az aktuális frekvenciát
    if (pSi4735Manager) {
        // VALÓS audio unmute visszakapcsolása
        pSi4735Manager->getSi4735().setAudioMute(false);
        DEBUG("Audio unmuted - paused\n");
    }
    if (playPauseButton) {
        playPauseButton->setLabel("Play"); // Pause után Play gomb
    }

    DEBUG("Scan paused\n");
}

void ScanScreen::stopScan() {
    scanState = ScanState::Idle;
    scanPaused = true;
    if (playPauseButton) {
        playPauseButton->setLabel("Play"); // Stop után Play gomb
    }

    DEBUG("Scan stopped\n");
}

void ScanScreen::updateScan() {
    if (!pSi4735Manager || scanPaused || currentScanPos >= SCAN_AREA_WIDTH) {
        return;
    }

    // Aktuális frekvencia számítása
    uint32_t scanFreq = positionToFreq(currentScanPos);
    setFrequency(scanFreq);

    // Jel mérése
    scanValueRSSI[currentScanPos] = getSignalRSSI();
    scanValueSNR[currentScanPos] = getSignalSNR();

    // Állomás jelzés SNR alapján
    if (scanValueSNR[currentScanPos] >= scanMarkSNR && currentScanPos > scanBeginBand && currentScanPos < scanEndBand) {
        scanMark[currentScanPos] = true;
    }

    // Spektrum vonal rajzolása
    drawSpectrumLine(currentScanPos);

    // Előző pozíció kurzorának törlése (ha volt)
    if (currentScanPos > 0) {
        // Újrarajzoljuk az előző pozíciót kurzor nélkül
        drawSpectrumLine(currentScanPos - 1);
    }

    // Pozíció léptetése
    currentScanPos++;

    // Ha végére értünk, újrakezdés
    if (currentScanPos >= SCAN_AREA_WIDTH) {
        currentScanPos = 0;
        scanEmpty = false;
    }

    // Információk frissítése
    drawScanInfo();
}

// ===================================================================
// Rajzolási funkciók
// ===================================================================

void ScanScreen::drawSpectrum() {
    // Spektrum terület törlése
    tft.fillRect(SCAN_AREA_X, SCAN_AREA_Y, SCAN_AREA_WIDTH, SCAN_AREA_HEIGHT, TFT_BLACK);

    // Ha még nincs scan elindítva, csak üres spektrumot rajzolunk
    if (scanPaused && currentScanPos == 0) {
        return; // Nem rajzolunk semmit, csak fekete háttér
    }

    // Összes spektrum vonal rajzolása (csak ha van adat)
    for (int x = 0; x < SCAN_AREA_WIDTH; x++) {
        // Csak azokat a vonalakat rajzoljuk meg, ahol van valós mért adat
        if (x <= currentScanPos || !scanPaused) {
            drawSpectrumLine(x);
        }
    }
}

void ScanScreen::drawSpectrumLine(uint16_t x) {
    if (x >= SCAN_AREA_WIDTH)
        return;

    uint32_t freq = positionToFreq(x);
    uint16_t screenX = SCAN_AREA_X + x;

    // Színek meghatározása
    uint16_t lineColor = TFT_NAVY;
    uint16_t bgColor = TFT_BLACK;
    bool isMainScale = false;

    // Skála vonalak ellenőrzése (csak ha már volt scan)
    if (scanScaleLine[x] == 0 && (!scanPaused || currentScanPos > 0)) {
        // Főskála vonalak (MHz határok)
        if (scanStep > 100) {
            if ((freq % 1000) < scanStep) {
                scanScaleLine[x] = 2;
                isMainScale = true;
            }
        } else {
            // Kisebb lépésközöknél 100 kHz határok
            if ((freq % 100) < scanStep) {
                scanScaleLine[x] = 2;
                isMainScale = true;
            }
        }
    }

    if (isMainScale) {
        lineColor = TFT_BLACK;
        bgColor = TFT_OLIVE;
    }

    // SNR alapú színezés
    if (scanValueSNR[x] > 0) {
        lineColor = TFT_NAVY;
        if (scanValueSNR[x] < 16) {
            lineColor += (scanValueSNR[x] * 2048);
        } else {
            lineColor = TFT_ORANGE;
            if (scanValueSNR[x] < 24) {
                lineColor += ((scanValueSNR[x] - 16) * 258);
            } else {
                lineColor = TFT_RED;
            }
        }
    }

    // Sáv határok ellenőrzése
    if (freq > scanEndFreq || freq < scanStartFreq) {
        lineColor = TFT_DARKGREY;
        if (freq > scanEndFreq && scanEndBand == SCAN_AREA_WIDTH) {
            scanEndBand = x;
        }
        if (freq < scanStartFreq && scanBeginBand < (int16_t)x) {
            scanBeginBand = x;
        }
    } // RSSI háttér rajzolása (csak ha van valós mért jel)
    int16_t rssiY = scanValueRSSI[x];

    // Ha még nincs mért adat (alapértelmezett érték a spektrum alján)
    if (rssiY >= SCAN_AREA_Y + SCAN_AREA_HEIGHT) {
        // Nincs jel, csak fekete háttér
        tft.drawLine(screenX, SCAN_AREA_Y, screenX, SCAN_AREA_Y + SCAN_AREA_HEIGHT, TFT_BLACK);
    } else {
        // Van mért jel, normál megjelenítés
        if (rssiY < SCAN_AREA_Y + 10)
            rssiY = SCAN_AREA_Y + 10;

        tft.drawLine(screenX, rssiY + 1, screenX, SCAN_AREA_Y + SCAN_AREA_HEIGHT, lineColor);
        tft.drawLine(screenX, SCAN_AREA_Y, screenX, rssiY - 1, bgColor);
    }

    // Skála vonalak rajzolása
    if (scanScaleLine[x] == 2) {
        tft.drawLine(screenX, SCAN_AREA_Y, screenX, SCAN_AREA_Y + 15, TFT_OLIVE);
    } // RSSI görbe rajzolása (csak ha van valós mért jel)
    if (rssiY < SCAN_AREA_Y + SCAN_AREA_HEIGHT) { // Van valós jel
        if (x > 0 && x != currentScanPos + 1) {
            int16_t prevRssiY = scanValueRSSI[x - 1];
            // Ha az előző pozícióban is van valós jel
            if (prevRssiY < SCAN_AREA_Y + SCAN_AREA_HEIGHT) {
                if (prevRssiY < SCAN_AREA_Y + 10)
                    prevRssiY = SCAN_AREA_Y + 10;
                tft.drawLine(screenX - 1, prevRssiY, screenX, rssiY, TFT_SILVER);
            }
        } else {
            tft.drawPixel(screenX, rssiY, TFT_SILVER);
        }
    }

    // Aktuális pozíció jelzése
    if (x == currentScanPos) {
        tft.drawLine(screenX, SCAN_AREA_Y, screenX, SCAN_AREA_Y + SCAN_AREA_HEIGHT, TFT_RED);
    }

    // Állomás jelzők (két fehér pont a spektrum tetején)
    if (scanMark[x]) {
        tft.fillCircle(screenX, SCAN_AREA_Y + 5, 2, TFT_WHITE);
        tft.fillCircle(screenX, SCAN_AREA_Y + 10, 2, TFT_WHITE);
    }
}

void ScanScreen::drawScale() {
    // Skála vonal a spektrum alatt
    tft.drawLine(SCAN_AREA_X, SCAN_AREA_Y + SCAN_AREA_HEIGHT + 5, SCAN_AREA_X + SCAN_AREA_WIDTH, SCAN_AREA_Y + SCAN_AREA_HEIGHT + 5, TFT_WHITE);
}

void ScanScreen::drawFrequencyLabels() {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setFreeFont();
    tft.setTextSize(1);

    // Frekvencia címkék megjelenítése 5-10 helyen
    int labelCount = 6;
    for (int i = 0; i <= labelCount; i++) {
        uint16_t x = SCAN_AREA_X + (i * SCAN_AREA_WIDTH / labelCount);
        uint32_t freq = positionToFreq(i * SCAN_AREA_WIDTH / labelCount);

        String freqText;
        if (freq >= 1000) {
            freqText = String(freq / 1000.0f, 1) + "M";
        } else {
            freqText = String(freq) + "k";
        }

        // Első címke jobbra igazítás, utolsó balra igazítás, közepesek középre
        if (i == 0) {
            tft.setTextDatum(TL_DATUM); // Első: bal szélhez igazított
        } else if (i == labelCount) {
            tft.setTextDatum(TR_DATUM); // Utolsó: jobb szélhez igazított
        } else {
            tft.setTextDatum(TC_DATUM); // Közepesek: középre igazított
        }

        tft.drawString(freqText, x, SCAN_AREA_Y + SCAN_AREA_HEIGHT + 15);
    }
}

void ScanScreen::drawBandBoundaries() {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setFreeFont();
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);

    constexpr uint16_t BAND_LABEL_OFFSET = 20; // felirat offset a spektrum tetejétől

    // Sáv kezdete - spektrum felső részében
    if (scanBeginBand > 0 && scanBeginBand < SCAN_AREA_WIDTH - 50) {
        uint16_t x = SCAN_AREA_X + scanBeginBand + 3;
        uint16_t y = SCAN_AREA_Y + BAND_LABEL_OFFSET; // 20px a spektrum tetejétől
        tft.fillRect(x, y, 50, 20, TFT_COLOR_BACKGROUND);
        tft.drawString("START", x, y);
        tft.drawString("OF BAND", x, y + 10);
    }

    // Sáv vége - spektrum felső részében
    if (scanEndBand < SCAN_AREA_WIDTH - 5 && scanEndBand > 50) {
        uint16_t x = SCAN_AREA_X + scanEndBand - 45;
        uint16_t y = SCAN_AREA_Y + BAND_LABEL_OFFSET; // 20px a spektrum tetejétől
        tft.fillRect(x, y, 50, 20, TFT_COLOR_BACKGROUND);
        tft.drawString("END", x, y);
        tft.drawString("OF BAND", x, y + 10);
    }
}

void ScanScreen::drawScanInfoStatic() {
    tft.setTextColor(TFT_WHITE, TFT_COLOR_BACKGROUND);
    tft.setFreeFont();
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);

    // Statikus címkék az info területen
    tft.drawString("Freq: ", 10, INFO_AREA_Y);
    tft.drawString("MHz", 100, INFO_AREA_Y); // MHz egység fix helyen

    tft.drawString("Zoom: ", 10, INFO_AREA_Y + 15);
    tft.drawString("x", 70, INFO_AREA_Y + 15); // x egység fix helyen

    tft.drawString("Status: ", 170, INFO_AREA_Y);

    // Statikus címkék a jel információkhoz
    tft.drawString("RSSI: ", 330, INFO_AREA_Y);
    tft.drawString("dBuV", 380, INFO_AREA_Y); // dBuV egység fix helyen

    tft.setTextColor(TFT_ORANGE, TFT_COLOR_BACKGROUND);
    tft.drawString("SNR: ", 330, INFO_AREA_Y + 15);
    tft.drawString("dB", 380, INFO_AREA_Y + 15); // dB egység fix helyen
}

void ScanScreen::drawScanInfo() {
    tft.setTextColor(TFT_WHITE, TFT_COLOR_BACKGROUND);
    tft.setFreeFont();
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);

    constexpr uint32_t FONT_HEIGHT = 7;

    // Csak az értékeket frissítjük (statikus címkék már ki vannak írva)
    // Aktuális frekvencia megjelenítése - csak az érték részét frissítjük
    String freqText = String(currentScanFreq / 1000.0f, 3);
    tft.fillRect(50, INFO_AREA_Y, 50, FONT_HEIGHT, TFT_RED); // Régi érték törlése
    tft.drawString(freqText, 50, INFO_AREA_Y);

    // Zoom szint - csak az érték részét frissítjük
    String zoomText = String(zoomLevel, 1);
    tft.fillRect(50, INFO_AREA_Y + 15, 20, FONT_HEIGHT, TFT_RED); // Régi érték törlése
    tft.drawString(zoomText, 50, INFO_AREA_Y + 15);

    // Scan állapot - csak akkor írjuk ki, ha változott (villogás elkerülése)
    String statusText;
    if (scanState == ScanState::Scanning && !scanPaused) {
        statusText = "Scanning...";
    } else if (scanPaused) {
        statusText = "Paused";
    } else {
        statusText = "Idle";
    }
    // Csak akkor frissítjük, ha változott
    if (statusText != lastStatusText) {
        tft.fillRect(220, INFO_AREA_Y, 100, FONT_HEIGHT, TFT_RED); // Régi érték törlése
        tft.drawString(statusText, 220, INFO_AREA_Y);
        lastStatusText = statusText; // Cache frissítése
    }

    // RSSI érték - csak az érték részét frissítjük
    int16_t rssi = getSignalRSSI();
    int16_t rssiValue = (SCAN_AREA_Y + SCAN_AREA_HEIGHT - rssi) / signalScale;
    String rssiText = String(rssiValue);
    tft.fillRect(365, INFO_AREA_Y, 15, FONT_HEIGHT, TFT_RED); // Régi érték törlése
    tft.drawString(rssiText, 365, INFO_AREA_Y);

    // SNR érték - csak az érték részét frissítjük
    uint8_t snr = getSignalSNR();
    String snrText = String(snr);
    tft.setTextColor(TFT_ORANGE, TFT_COLOR_BACKGROUND);
    tft.fillRect(365, INFO_AREA_Y + 15, 15, FONT_HEIGHT, TFT_RED); // Régi érték törlése
    tft.drawString(snrText, 365, INFO_AREA_Y + 15);
}

// ===================================================================
// Jel mérés és frekvencia kezelés
// ===================================================================

int16_t ScanScreen::getSignalRSSI() {
    if (!pSi4735Manager)
        return SCAN_AREA_Y + SCAN_AREA_HEIGHT - 20;

    // VALÓS RSSI mérés a Si4735 chipről
    int rssiSum = 0;
    for (int i = 0; i < countScanSignal; i++) {
        // VALÓS mérés a Si4735 chipről
        uint8_t rssi = pSi4735Manager->getRSSI();
        rssiSum += rssi;
    }

    int avgRssi = rssiSum / countScanSignal;
    // RSSI értéket Y koordinátára konvertálás (nagyobb RSSI = magasabb pozíció)
    return SCAN_AREA_Y + SCAN_AREA_HEIGHT - (avgRssi * signalScale);
}

uint8_t ScanScreen::getSignalSNR() {
    if (!pSi4735Manager)
        return 0;

    // SNR mérés a Si4735 chipről
    int snrSum = 0;
    for (int i = 0; i < countScanSignal; i++) {
        // mérés a Si4735 chipről
        uint8_t snr = pSi4735Manager->getSNR();
        snrSum += snr;
    }

    return snrSum / countScanSignal;
}

void ScanScreen::setFrequency(uint32_t freq) {
    currentScanFreq = freq;
    // Spektrum analizátor hangol át minden frekvenciára a méréshez!
    if (pSi4735Manager) {
        // frekvencia beállítás a Si4735 chipen
        pSi4735Manager->getSi4735().setFrequency(freq / 10); // Si4735 10kHz egységekben dolgozik
        DEBUG("Tuning to: %d kHz (Si4735: %d)\n", freq, freq / 10);

        // Kis várakozás a ráhangolódáshoz (stabilizálódás)
        delay(5); // 5ms várakozás
    }
}

// ===================================================================
// Zoom és pozíció számítások
// ===================================================================

void ScanScreen::zoomIn() {
    float newZoom = zoomLevel * 1.5f;
    if (newZoom <= 10.0f) {
        handleZoom(newZoom);
    }
}

void ScanScreen::zoomOut() {
    float newZoom = zoomLevel / 1.5f;
    if (newZoom >= 0.1f) {
        handleZoom(newZoom);
    }
}

void ScanScreen::handleZoom(float newZoomLevel) {
    zoomLevel = newZoomLevel;
    calculateScanParameters();

    // Zoom után csak a spektrum adatokat inicializáljuk újra, ne a teljes UI-t
    scanEmpty = true;
    currentScanPos = 0; // Adatok törlése
    for (int i = 0; i < SCAN_AREA_WIDTH; i++) {
        scanValueRSSI[i] = SCAN_AREA_Y + SCAN_AREA_HEIGHT; // Spektrum alján kezdjük (nincs látható jel)
        scanValueSNR[i] = 0;
        scanMark[i] = false;
        scanScaleLine[i] = 0;
    }

    // Sáv határok újraszámítása
    scanBeginBand = -1;
    scanEndBand = SCAN_AREA_WIDTH;

    // Csak a spektrum területet és az info panelt frissítjük
    drawSpectrum();
    drawScale();
    drawFrequencyLabels();
    drawBandBoundaries();
    drawScanInfo();

    DEBUG("Zoom changed to: %.2f\n", zoomLevel);
}

uint32_t ScanScreen::positionToFreq(uint16_t x) {
    if (x >= SCAN_AREA_WIDTH)
        return scanEndFreq;

    float ratio = (float)x / SCAN_AREA_WIDTH;
    uint32_t totalBandwidth = scanEndFreq - scanStartFreq;
    return scanStartFreq + (uint32_t)(ratio * totalBandwidth / zoomLevel);
}

uint16_t ScanScreen::freqToPosition(uint32_t freq) {
    if (freq <= scanStartFreq)
        return 0;
    if (freq >= scanEndFreq)
        return SCAN_AREA_WIDTH - 1;

    uint32_t totalBandwidth = scanEndFreq - scanStartFreq;
    float ratio = (float)(freq - scanStartFreq) / totalBandwidth * zoomLevel;
    return (uint16_t)(ratio * SCAN_AREA_WIDTH);
}
