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
    scanEmpty = true;

    // Konfiguráció
    countScanSignal = 3;
    signalScale = 2.0f;

    // Tömbök inicializálása
    for (int i = 0; i < SCAN_AREA_WIDTH; i++) {
        scanValueRSSI[i] = SCAN_AREA_Y + SCAN_AREA_HEIGHT - 20; // Alapértelmezett RSSI pozíció
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
    drawFrequencyLabels();

    // Sáv határok
    drawBandBoundaries();

    // Információk
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
                drawSignalInfo();
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
                drawSignalInfo();
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

    // TODO: Aktuális sáv információk lekérése a Si4735Manager-ből
    // Jelenleg fix értékekkel dolgozunk
    scanStartFreq = 87500; // 87.5 MHz (FM sáv)
    scanEndFreq = 108000;  // 108.0 MHz
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
    scanEmpty = true;
    currentScanPos = 0;
    scanPaused = true;
    scanState = ScanState::Idle; // Gomb állapot frissítése
    if (playPauseButton) {
        // A gomb újrarajzolása szükséges, mivel nincs setText metódus
        playPauseButton->markForRedraw();
    }

    // Adatok törlése
    for (int i = 0; i < SCAN_AREA_WIDTH; i++) {
        scanValueRSSI[i] = SCAN_AREA_Y + SCAN_AREA_HEIGHT - 20;
        scanValueSNR[i] = 0;
        scanMark[i] = false;
        scanScaleLine[i] = 0;
    }

    // Sáv határok újraszámítása
    scanBeginBand = -1;
    scanEndBand = SCAN_AREA_WIDTH;

    drawContent();
    DEBUG("Scan reset\n");
}

void ScanScreen::startScan() {
    scanPaused = false;
    scanState = ScanState::Scanning;
    lastScanTime = millis();
    if (playPauseButton) {
        // A gomb újrarajzolása szükséges
        playPauseButton->markForRedraw();
    }

    DEBUG("Scan started\n");
}

void ScanScreen::pauseScan() {
    scanPaused = true;

    if (playPauseButton) {
        // A gomb újrarajzolása szükséges
        playPauseButton->markForRedraw();
    }

    DEBUG("Scan paused\n");
}

void ScanScreen::stopScan() {
    scanState = ScanState::Idle;
    scanPaused = true;

    if (playPauseButton) {
        // A gomb újrarajzolása szükséges
        playPauseButton->markForRedraw();
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

    // Pozíció léptetése
    currentScanPos++;

    // Ha végére értünk, újrakezdés
    if (currentScanPos >= SCAN_AREA_WIDTH) {
        currentScanPos = 0;
        scanEmpty = false;
    }

    // Információk frissítése
    drawScanInfo();
    drawSignalInfo();
}

// ===================================================================
// Rajzolási funkciók
// ===================================================================

void ScanScreen::drawSpectrum() {
    // Spektrum terület törlése
    tft.fillRect(SCAN_AREA_X, SCAN_AREA_Y, SCAN_AREA_WIDTH, SCAN_AREA_HEIGHT, TFT_BLACK);

    // Összes spektrum vonal rajzolása
    for (int x = 0; x < SCAN_AREA_WIDTH; x++) {
        drawSpectrumLine(x);
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

    // Skála vonalak ellenőrzése
    if (scanScaleLine[x] == 0) {
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
    }

    // RSSI háttér rajzolása
    int16_t rssiY = scanValueRSSI[x];
    if (rssiY < SCAN_AREA_Y + 10)
        rssiY = SCAN_AREA_Y + 10;

    tft.drawLine(screenX, rssiY + 1, screenX, SCAN_AREA_Y + SCAN_AREA_HEIGHT, lineColor);
    tft.drawLine(screenX, SCAN_AREA_Y, screenX, rssiY - 1, bgColor);

    // Skála vonalak rajzolása
    if (scanScaleLine[x] == 2) {
        tft.drawLine(screenX, SCAN_AREA_Y, screenX, SCAN_AREA_Y + 15, TFT_OLIVE);
    }

    // RSSI görbe rajzolása
    if (x > 0 && x != currentScanPos + 1) {
        int16_t prevRssiY = scanValueRSSI[x - 1];
        if (prevRssiY < SCAN_AREA_Y + 10)
            prevRssiY = SCAN_AREA_Y + 10;
        tft.drawLine(screenX - 1, prevRssiY, screenX, rssiY, TFT_SILVER);
    } else {
        tft.drawPixel(screenX, rssiY, TFT_SILVER);
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
    tft.setTextSize(1);
    tft.setTextDatum(TC_DATUM);

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

        tft.drawString(freqText, x, SCAN_AREA_Y + SCAN_AREA_HEIGHT + 15);
    }
}

void ScanScreen::drawBandBoundaries() {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);

    // Sáv kezdete
    if (scanBeginBand > 0 && scanBeginBand < SCAN_AREA_WIDTH - 50) {
        uint16_t x = SCAN_AREA_X + scanBeginBand + 3;
        tft.fillRect(x, INFO_AREA_Y - 35, 50, 30, TFT_BLACK);
        tft.drawString("START", x, INFO_AREA_Y - 35);
        tft.drawString("OF BAND", x, INFO_AREA_Y - 25);
    }

    // Sáv vége
    if (scanEndBand < SCAN_AREA_WIDTH - 5 && scanEndBand > 50) {
        uint16_t x = SCAN_AREA_X + scanEndBand - 45;
        tft.fillRect(x, INFO_AREA_Y - 35, 50, 30, TFT_BLACK);
        tft.drawString("END", x, INFO_AREA_Y - 35);
        tft.drawString("OF BAND", x, INFO_AREA_Y - 25);
    }
}

void ScanScreen::drawScanInfo() {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setFreeFont();
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);

    // Aktuális frekvencia megjelenítése
    String freqText = "Freq: " + String(currentScanFreq / 1000.0f, 3) + " MHz";
    tft.fillRect(10, INFO_AREA_Y, 150, 20, TFT_BLACK);
    tft.drawString(freqText, 10, INFO_AREA_Y);

    // Zoom szint
    String zoomText = "Zoom: " + String(zoomLevel, 1) + "x";
    tft.fillRect(10, INFO_AREA_Y + 15, 150, 20, TFT_BLACK);
    tft.drawString(zoomText, 10, INFO_AREA_Y + 15);

    // Scan állapot
    String statusText = "Status: ";
    if (scanState == ScanState::Scanning && !scanPaused) {
        statusText += "Scanning...";
    } else if (scanPaused) {
        statusText += "Paused";
    } else {
        statusText += "Idle";
    }
    tft.fillRect(170, INFO_AREA_Y, 150, 20, TFT_BLACK);
    tft.drawString(statusText, 170, INFO_AREA_Y);
}

void ScanScreen::drawSignalInfo() {
    if (!pSi4735Manager)
        return;

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);

    // RSSI érték
    int16_t rssi = getSignalRSSI();
    int16_t rssiValue = (SCAN_AREA_Y + SCAN_AREA_HEIGHT - rssi) / signalScale;
    String rssiText = "RSSI: " + String(rssiValue) + " dBuV";
    tft.fillRect(330, INFO_AREA_Y, 120, 15, TFT_BLACK);
    tft.drawString(rssiText, 330, INFO_AREA_Y);

    // SNR érték
    uint8_t snr = getSignalSNR();
    String snrText = "SNR: " + String(snr) + " dB";
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.fillRect(330, INFO_AREA_Y + 15, 120, 15, TFT_BLACK);
    tft.drawString(snrText, 330, INFO_AREA_Y + 15);
}

// ===================================================================
// Jel mérés és frekvencia kezelés
// ===================================================================

int16_t ScanScreen::getSignalRSSI() {
    if (!pSi4735Manager)
        return SCAN_AREA_Y + SCAN_AREA_HEIGHT - 20;

    // TODO: Si4735Manager RSSI mérés implementálása
    // Jelenleg szimulált értékkel dolgozunk
    int rssiSum = 0;
    for (int i = 0; i < countScanSignal; i++) {
        // rssiSum += si4735Manager->getCurrentRSSI();
        rssiSum += 30 + random(-10, 20); // Szimulált érték
    }

    int avgRssi = rssiSum / countScanSignal;
    return SCAN_AREA_Y + SCAN_AREA_HEIGHT - (avgRssi * signalScale);
}

uint8_t ScanScreen::getSignalSNR() {
    if (!pSi4735Manager)
        return 0;

    // TODO: Si4735Manager SNR mérés implementálása
    // Jelenleg szimulált értékkel dolgozunk
    int snrSum = 0;
    for (int i = 0; i < countScanSignal; i++) {
        // snrSum += si4735Manager->getCurrentSNR();
        snrSum += random(0, 30); // Szimulált érték
    }

    return snrSum / countScanSignal;
}

void ScanScreen::setFrequency(uint32_t freq) {
    currentScanFreq = freq;
    if (pSi4735Manager) {
        // TODO: Si4735Manager frekvencia beállítás
        // si4735Manager->setFrequency(freq);
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

    // Zoom után újrainicializálás
    resetScan();

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
