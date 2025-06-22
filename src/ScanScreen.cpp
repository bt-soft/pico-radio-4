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
    currentScanPos = 0; // Sáv határok
    scanBeginBand = -1;
    scanEndBand = SCAN_RESOLUTION;
    scanMarkSNR = 3;
    scanEmpty = true; // Konfiguráció
    countScanSignal = 3;
    signalScale = 2.0f;

    // UI cache inicializálása
    lastStatusText = ""; // Tömbök inicializálása
    for (int i = 0; i < SCAN_RESOLUTION; i++) {
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

bool ScanScreen::handleTouch(const TouchEvent &event) {
    // Debug: minden touch eseményt logolunk
    DEBUG("Touch event: pressed=%d, x=%d, y=%d\n", event.pressed, event.x, event.y);

    // ELŐSZÖR ellenőrizzük a spektrum területet (prioritás!)
    if (event.pressed && event.x >= SCAN_AREA_X && event.x < SCAN_AREA_X + SCAN_AREA_WIDTH && event.y >= SCAN_AREA_Y && event.y < SCAN_AREA_Y + SCAN_AREA_HEIGHT) {

        DEBUG("Touch in spectrum area! scanPaused=%d, scanEmpty=%d\n", scanPaused, scanEmpty);

        // Relatív pozíció számítás a spektrum területen belül
        uint16_t relativePixelX = event.x - SCAN_AREA_X;

        // Pozíció korlátozása
        if (relativePixelX >= SCAN_AREA_WIDTH) {
            relativePixelX = SCAN_AREA_WIDTH - 1;
        }

        // Pixel pozíciót adatponttá konvertálás
        uint16_t relativeDataPos = (relativePixelX * SCAN_RESOLUTION) / SCAN_AREA_WIDTH;

        DEBUG("Touch: relativePixelX=%d, relativeDataPos=%d, currentScanPos=%d\n", relativePixelX, relativeDataPos, currentScanPos);

        // Ha más pozíció, mint a jelenlegi
        if (relativeDataPos != currentScanPos) {
            // Ha scan fut, előbb pause-oljuk
            if (!scanPaused) {
                DEBUG("Touch: auto-pausing scan\n");
                pauseScan();
            }

            // Régi pozíció újrarajzolása kurzor nélkül (pixel koordinátában)
            uint16_t oldPixelPos = (currentScanPos * SCAN_AREA_WIDTH) / SCAN_RESOLUTION;
            drawSpectrumLine(oldPixelPos);

            // Új pozíció beállítása (adatpont koordinátában)
            currentScanPos = relativeDataPos;

            // Új pozíció frekvenciájának beállítása
            uint32_t newFreq = positionToFreq(currentScanPos);
            setFrequency(newFreq);

            // Új pozíció rajzolása kurzorozva (pixel koordinátában - újraszámított!)
            uint16_t newPixelPos = (currentScanPos * SCAN_AREA_WIDTH) / SCAN_RESOLUTION;
            drawSpectrumLine(newPixelPos);

            // Info frissítése
            drawScanInfo();

            DEBUG("Touch: moved cursor to dataPos=%d, freq=%d kHz\n", currentScanPos, newFreq);
        } else {
            DEBUG("Touch: same position, no change\n");
        }

        DEBUG("Touch handled by spectrum area\n");
        return true; // Spektrum területen történt érintés - NE adja tovább a gomboknak!
    }

    // Ha nem a spektrum területen történt, akkor adjuk át a szülő osztálynak (gombok kezelése)
    DEBUG("Touch outside spectrum area - passing to parent\n");
    if (UIScreen::handleTouch(event)) {
        DEBUG("Touch handled by parent (button)\n");
        return true;
    }

    DEBUG("Touch not handled by anyone\n");
    return false;
}

bool ScanScreen::handleRotary(const RotaryEvent &event) {
    if (event.direction == RotaryEvent::Direction::Up) {
        // Jobbra forgatás        if (scanPaused) {
        if (!scanEmpty) {
            // Ha van scan adat, kurzor mozgatás
            uint16_t newPos = currentScanPos + 1;
            if (newPos < SCAN_RESOLUTION) {
                // Régi pozíció újrarajzolása kurzor nélkül (pixel koordinátában)
                uint16_t oldPixelPos = (currentScanPos * SCAN_AREA_WIDTH) / SCAN_RESOLUTION;

                // Új pozíció beállítása
                currentScanPos = newPos;

                // Új pixel pozíció számítása
                uint16_t newPixelPos = (currentScanPos * SCAN_AREA_WIDTH) / SCAN_RESOLUTION;

                // Új pozíció frekvenciájának beállítása
                uint32_t newFreq = positionToFreq(currentScanPos);
                setFrequency(newFreq);

                // Ha más pixelnél vagyunk, rajzoljuk újra mindkettőt
                if (oldPixelPos != newPixelPos) {
                    drawSpectrumLine(oldPixelPos); // Régi törlése
                    drawSpectrumLine(newPixelPos); // Új rajzolása kurzorozva
                } else {
                    // Ugyanazon a pixelen belül vagyunk, csak újrarajzoljuk
                    drawSpectrumLine(newPixelPos);
                }

                // Info frissítése
                drawScanInfo();
            }
        } else {
            // Ha nincs scan adat, frekvencia változtatás
            uint32_t newFreq = currentScanFreq + (uint32_t)(scanStep * 10);
            if (newFreq <= scanEndFreq) {
                currentScanFreq = newFreq;
                setFrequency(currentScanFreq);
                drawScanInfo();
            }
        }
        return true;
    } else if (event.direction == RotaryEvent::Direction::Down) {
        // Balra forgatás        if (scanPaused) {
        if (!scanEmpty) {
            // Ha van scan adat, kurzor mozgatás
            if (currentScanPos > 0) {
                // Régi pozíció pixel koordinátája
                uint16_t oldPixelPos = (currentScanPos * SCAN_AREA_WIDTH) / SCAN_RESOLUTION;

                // Új pozíció beállítása
                currentScanPos = currentScanPos - 1;

                // Új pixel pozíció számítása
                uint16_t newPixelPos = (currentScanPos * SCAN_AREA_WIDTH) / SCAN_RESOLUTION;

                // Új pozíció frekvenciájának beállítása
                uint32_t newFreq = positionToFreq(currentScanPos);
                setFrequency(newFreq);

                // Ha más pixelnél vagyunk, rajzoljuk újra mindkettőt
                if (oldPixelPos != newPixelPos) {
                    drawSpectrumLine(oldPixelPos); // Régi törlése
                    drawSpectrumLine(newPixelPos); // Új rajzolása kurzorozva
                } else {
                    // Ugyanazon a pixelen belül vagyunk, csak újrarajzoljuk
                    drawSpectrumLine(newPixelPos);
                } // Info frissítése
                drawScanInfo();
            }
        } else {
            // Ha nincs scan adat, frekvencia változtatás
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

    scanStartFreq = currentBand.minimumFreq * 10; // kHz-ben
    scanEndFreq = currentBand.maximumFreq * 10;   // kHz-ben
    currentScanFreq = scanStartFreq;

    DEBUG("Scan initialized: %d - %d kHz\n", scanStartFreq, scanEndFreq);
}

void ScanScreen::calculateScanParameters() {
    // Scan lépésköz számítása az aktuális tartomány alapján (nagyobb felbontással)
    uint32_t totalBandwidth = scanEndFreq - scanStartFreq;
    scanStep = (float)totalBandwidth / SCAN_RESOLUTION;

    // Minimális lépésköz korlátozása
    if (scanStep < 1.0f)
        scanStep = 1.0f;

    DEBUG("Scan parameters: step=%.2f kHz, range=%d-%d kHz, resolution=%d points\n", scanStep, scanStartFreq, scanEndFreq, SCAN_RESOLUTION);
}

void ScanScreen::resetScan() {
    // Scan állapot teljes visszaállítása
    scanState = ScanState::Idle;
    scanPaused = true;
    scanEmpty = true;
    currentScanPos = 0;
    zoomLevel = 1.0f; // Zoom visszaállítása 1.0x-ra    // Teljes sáv tartomány visszaállítása
    if (pSi4735Manager) {
        BandTable &currentBand = pSi4735Manager->getCurrentBand();
        scanStartFreq = currentBand.minimumFreq * 10; // Teljes sáv kezdete
        scanEndFreq = currentBand.maximumFreq * 10;   // Teljes sáv vége
        currentScanFreq = scanStartFreq;
    }

    // UI cache visszaállítása
    lastStatusText = "";

    // Scan paraméterek újraszámítása
    calculateScanParameters(); // Adatok törlése
    for (int i = 0; i < SCAN_RESOLUTION; i++) {
        scanValueRSSI[i] = SCAN_AREA_Y + SCAN_AREA_HEIGHT; // Spektrum alján kezdjük (nincs látható jel)
        scanValueSNR[i] = 0;
        scanMark[i] = false;
        scanScaleLine[i] = 0;
    }

    // Sáv határok újraszámítása
    scanBeginBand = -1;
    scanEndBand = SCAN_RESOLUTION;
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
    if (!pSi4735Manager || scanPaused || currentScanPos >= SCAN_RESOLUTION) {
        return;
    }

    // Aktuális frekvencia számítása
    uint32_t scanFreq = positionToFreq(currentScanPos);
    setFrequency(scanFreq); // Jel mérése - optimalizált: egyszerre RSSI és SNR
    int16_t rssiY;
    uint8_t snr;
    getSignalQuality(rssiY, snr);

    scanValueRSSI[currentScanPos] = rssiY;
    scanValueSNR[currentScanPos] = snr;

    // Állomás jelzés SNR alapján - minden méréskor újra értékeljük!
    if (scanValueSNR[currentScanPos] >= scanMarkSNR && currentScanPos > scanBeginBand && currentScanPos < scanEndBand) {
        scanMark[currentScanPos] = true;
    } else {
        scanMark[currentScanPos] = false; // Töröljük a régi jelzést ha már nincs elég jel
    }

    // Spektrum vonal rajzolása (pixel pozícióra konvertálás)
    uint16_t pixelPos = (currentScanPos * SCAN_AREA_WIDTH) / SCAN_RESOLUTION;
    drawSpectrumLine(pixelPos);

    // Előző pozíció kurzorának törlése (ha volt)
    if (currentScanPos > 0) {
        uint16_t prevPixelPos = ((currentScanPos - 1) * SCAN_AREA_WIDTH) / SCAN_RESOLUTION;
        if (prevPixelPos != pixelPos) {
            // Újrarajzoljuk az előző pozíciót kurzor nélkül
            drawSpectrumLine(prevPixelPos);
        }
    }

    // Pozíció léptetése
    currentScanPos++;

    // Ha végére értünk, újrakezdés
    if (currentScanPos >= SCAN_RESOLUTION) {
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
    tft.fillRect(SCAN_AREA_X, SCAN_AREA_Y, SCAN_AREA_WIDTH, SCAN_AREA_HEIGHT, TFT_COLOR_BACKGROUND);

    DEBUG("drawSpectrum: scanPaused=%d, currentScanPos=%d, scanEmpty=%d\n", scanPaused, currentScanPos, scanEmpty);

    // Ha még nincs scan elindítva, csak üres spektrumot rajzolunk
    if (scanPaused && currentScanPos == 0) {
        DEBUG("drawSpectrum: early return - no data to draw\n");
        return; // Nem rajzolunk semmit, csak fekete háttér
    }

    // Összes spektrum vonal rajzolása (csak ha van adat)
    int linesDrawn = 0;
    for (int x = 0; x < SCAN_AREA_WIDTH; x++) {
        // Pixel pozíció konvertálása data pozícióra
        uint16_t dataPos = (x * SCAN_RESOLUTION) / SCAN_AREA_WIDTH;

        // Csak azokat a vonalakat rajzoljuk meg, ahol van valós mért adat
        if (dataPos <= currentScanPos || !scanPaused) {
            drawSpectrumLine(x);
            linesDrawn++;
        }
    }
    DEBUG("drawSpectrum: drew %d lines\n", linesDrawn);
}

void ScanScreen::drawSpectrumLine(uint16_t pixelX) {
    if (pixelX >= SCAN_AREA_WIDTH)
        return;

    uint16_t screenX = SCAN_AREA_X + pixelX;

    // Mintavételi pontok tartománya ennek a pixelnek
    uint16_t dataStart = (pixelX * SCAN_RESOLUTION) / SCAN_AREA_WIDTH;
    uint16_t dataEnd = ((pixelX + 1) * SCAN_RESOLUTION) / SCAN_AREA_WIDTH; // Átlagoljuk a mintavételi pontokat ennél a pixelnél
    int16_t avgRSSI = 0;
    uint8_t avgSNR = 0;
    bool hasStation = false;
    bool isMainScale = false;
    uint32_t avgFreq = 0;

    int validSamples = 0;
    for (uint16_t i = dataStart; i < dataEnd && i < SCAN_RESOLUTION; i++) {
        if (scanValueRSSI[i] < SCAN_AREA_Y + SCAN_AREA_HEIGHT) { // Van valós mért adat
            avgRSSI += scanValueRSSI[i];
            avgSNR += scanValueSNR[i];
            validSamples++;
        }
        if (scanMark[i])
            hasStation = true; // Frekvencia számítás az adatponthoz
        uint32_t freq = scanStartFreq + (uint32_t)(i * scanStep);
        avgFreq += freq; // Skála vonalak ellenőrzése - dinamikus ritkítás zoom szint alapján        // MINDIG ellenőrizzük a skála vonalakat, függetlenül a mérési adatoktól
        if (scanScaleLine[i] == 0) {
            if (zoomLevel < 2.0f) {
                // Teljes sáv vagy kis zoom esetén: csak minden 5 MHz-nél
                if ((freq % 5000) < scanStep) {
                    scanScaleLine[i] = 2;
                    isMainScale = true;
                }
            } else if (zoomLevel < 5.0f) {
                // Közepes zoom (2x-5x): minden 2 MHz-nél
                if ((freq % 2000) < scanStep) {
                    scanScaleLine[i] = 2;
                    isMainScale = true;
                }
            } else if (scanStep > 50) {
                // Nagy zoom (5x-10x), de még nagy lépésköz: minden 1 MHz-nél
                if ((freq % 1000) < scanStep) {
                    scanScaleLine[i] = 2;
                    isMainScale = true;
                }
            } else {
                // Nagyon nagy zoom: minden 100 kHz-nél
                if ((freq % 100) < scanStep) {
                    scanScaleLine[i] = 2;
                    isMainScale = true;
                }
            }
        }
        if (scanScaleLine[i] == 2)
            isMainScale = true;
    }

    if (validSamples > 0) {
        avgRSSI /= validSamples;
        avgSNR /= validSamples;
    } else {
        avgRSSI = SCAN_AREA_Y + SCAN_AREA_HEIGHT; // Nincs jel
        avgSNR = 0;
    }
    avgFreq /= (dataEnd - dataStart);

    // Színek meghatározása
    uint16_t lineColor = TFT_NAVY;
    uint16_t bgColor = TFT_BLACK;

    if (isMainScale) {
        lineColor = TFT_BLACK;
        bgColor = TFT_OLIVE;
    }

    // SNR alapú színezés
    if (avgSNR > 0) {
        lineColor = TFT_NAVY;
        if (avgSNR < 16) {
            lineColor += (avgSNR * 2048);
        } else {
            lineColor = TFT_ORANGE;
            if (avgSNR < 24) {
                lineColor += ((avgSNR - 16) * 258);
            } else {
                lineColor = TFT_RED;
            }
        }
    }

    // Sáv határok ellenőrzése
    if (avgFreq > scanEndFreq || avgFreq < scanStartFreq) {
        lineColor = TFT_DARKGREY;
        if (avgFreq > scanEndFreq && scanEndBand == SCAN_RESOLUTION) {
            scanEndBand = dataStart;
        }
        if (avgFreq < scanStartFreq && scanBeginBand < (int16_t)dataStart) {
            scanBeginBand = dataStart;
        }
    }

    // RSSI háttér rajzolása
    int16_t rssiY = avgRSSI;
    if (rssiY >= SCAN_AREA_Y + SCAN_AREA_HEIGHT) {
        // Nincs jel, csak fekete háttér
        tft.drawLine(screenX, SCAN_AREA_Y, screenX, SCAN_AREA_Y + SCAN_AREA_HEIGHT, TFT_COLOR_BACKGROUND);
    } else {
        // Van mért jel, normál megjelenítés
        if (rssiY < SCAN_AREA_Y + 10)
            rssiY = SCAN_AREA_Y + 10;

        tft.drawLine(screenX, rssiY + 1, screenX, SCAN_AREA_Y + SCAN_AREA_HEIGHT, lineColor);
        tft.drawLine(screenX, SCAN_AREA_Y, screenX, rssiY - 1, bgColor);
    }

    // Skála vonalak rajzolása
    if (isMainScale) {
        tft.drawLine(screenX, SCAN_AREA_Y, screenX, SCAN_AREA_Y + 15, TFT_OLIVE);
    }

    // RSSI görbe rajzolása (simított)
    if (rssiY < SCAN_AREA_Y + SCAN_AREA_HEIGHT && pixelX > 0) {
        // Előző pixel átlagolt RSSI értéke
        uint16_t prevDataStart = ((pixelX - 1) * SCAN_RESOLUTION) / SCAN_AREA_WIDTH;
        uint16_t prevDataEnd = (pixelX * SCAN_RESOLUTION) / SCAN_AREA_WIDTH;

        int16_t prevAvgRSSI = SCAN_AREA_Y + SCAN_AREA_HEIGHT;
        int prevValidSamples = 0;
        for (uint16_t i = prevDataStart; i < prevDataEnd && i < SCAN_RESOLUTION; i++) {
            if (scanValueRSSI[i] < SCAN_AREA_Y + SCAN_AREA_HEIGHT) {
                prevAvgRSSI += scanValueRSSI[i];
                prevValidSamples++;
            }
        }
        if (prevValidSamples > 0) {
            prevAvgRSSI /= prevValidSamples;
        }

        if (prevAvgRSSI < SCAN_AREA_Y + SCAN_AREA_HEIGHT) {
            if (prevAvgRSSI < SCAN_AREA_Y + 10)
                prevAvgRSSI = SCAN_AREA_Y + 10;
            tft.drawLine(screenX - 1, prevAvgRSSI, screenX, rssiY, TFT_SILVER);
        } else {
            tft.drawPixel(screenX, rssiY, TFT_SILVER);
        }
    }

    // Aktuális pozíció jelzése (pixel koordinátákban)
    uint16_t currentPixelPos = (currentScanPos * SCAN_AREA_WIDTH) / SCAN_RESOLUTION;
    if (pixelX == currentPixelPos) {
        tft.drawLine(screenX, SCAN_AREA_Y, screenX, SCAN_AREA_Y + SCAN_AREA_HEIGHT, TFT_RED);
    } // Állomás jelzők
    if (hasStation) {
        tft.fillCircle(screenX, SCAN_AREA_Y + 5, 2, TFT_WHITE);
        tft.fillCircle(screenX, SCAN_AREA_Y + 10, 2, TFT_WHITE);
    }

    // Debug info (csak első néhány pixelhez)
    if (pixelX < 5 && validSamples > 0) {
        DEBUG("drawSpectrumLine: pixel=%d, dataRange=%d-%d, validSamples=%d, avgRSSI=%d, rssiY=%d\n", pixelX, dataStart, dataEnd, validSamples, avgRSSI, rssiY);
    }
}

void ScanScreen::drawScale() {
    // Skála vonal a spektrum alatt
    tft.drawLine(SCAN_AREA_X, SCAN_AREA_Y + SCAN_AREA_HEIGHT + 5, SCAN_AREA_X + SCAN_AREA_WIDTH, SCAN_AREA_Y + SCAN_AREA_HEIGHT + 5, TFT_WHITE);
}

void ScanScreen::drawFrequencyLabels() {
    // Előbb töröljük a régi címkéket
    tft.fillRect(SCAN_AREA_X, SCAN_AREA_Y + SCAN_AREA_HEIGHT + 10, SCAN_AREA_WIDTH, 20, TFT_BLACK);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setFreeFont();
    tft.setTextSize(1);

    // Debug: aktuális scan tartomány ellenőrzése
    DEBUG("drawFrequencyLabels: scanStartFreq=%d, scanEndFreq=%d kHz\n", scanStartFreq, scanEndFreq);

    // Frekvencia címkék megjelenítése 5-10 helyen
    int labelCount = 6;
    for (int i = 0; i <= labelCount; i++) {
        uint16_t x = SCAN_AREA_X + (i * SCAN_AREA_WIDTH / labelCount);

        // JAVÍTÁS: Pixel pozíció helyett adatpont pozíciót használunk
        uint16_t dataPos = (i * SCAN_RESOLUTION) / labelCount;
        uint32_t freq = positionToFreq(dataPos);

        // Debug: számított frekvencia ellenőrzése
        DEBUG("Label %d: pixelPos=%d, dataPos=%d, freq=%d kHz\n", i, i * SCAN_AREA_WIDTH / labelCount, dataPos, freq);

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
    tft.fillRect(50, INFO_AREA_Y, 50, FONT_HEIGHT, TFT_COLOR_BACKGROUND); // Régi érték törlése
    tft.drawString(freqText, 50, INFO_AREA_Y);

    // Zoom szint - csak az érték részét frissítjük
    String zoomText = String(zoomLevel, 1);
    tft.fillRect(50, INFO_AREA_Y + 15, 20, FONT_HEIGHT, TFT_COLOR_BACKGROUND); // Régi érték törlése
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
        tft.fillRect(220, INFO_AREA_Y, 100, FONT_HEIGHT, TFT_COLOR_BACKGROUND); // Régi érték törlése
        tft.drawString(statusText, 220, INFO_AREA_Y);
        lastStatusText = statusText; // Cache frissítése
    }

    // RSSI érték - csak az érték részét frissítjük
    int16_t rssi;
    uint8_t snr;
    getSignalQuality(rssi, snr);
    int16_t rssiValue = (SCAN_AREA_Y + SCAN_AREA_HEIGHT - rssi) / signalScale;
    String rssiText = String(rssiValue);
    tft.fillRect(365, INFO_AREA_Y, 15, FONT_HEIGHT, TFT_COLOR_BACKGROUND); // Régi érték törlése
    tft.drawString(rssiText, 365, INFO_AREA_Y);

    // SNR érték - csak az érték részét frissítjük
    String snrText = String(snr);
    tft.setTextColor(TFT_ORANGE, TFT_COLOR_BACKGROUND);
    tft.fillRect(365, INFO_AREA_Y + 15, 15, FONT_HEIGHT, TFT_COLOR_BACKGROUND); // Régi érték törlése
    tft.drawString(snrText, 365, INFO_AREA_Y + 15);
}

// ===================================================================
// Jel mérés és frekvencia kezelés
// ===================================================================

// Közös jel mérés - egyszerre RSSI és SNR
void ScanScreen::getSignalQuality(int16_t &rssiY, uint8_t &snr) {
    if (!pSi4735Manager) {
        rssiY = SCAN_AREA_Y + SCAN_AREA_HEIGHT - 20;
        snr = 0;
        return;
    }

    // Együttes mérés a Si4735 chipről - hatékonyabb!
    int rssiSum = 0;
    int snrSum = 0;

    for (int i = 0; i < countScanSignal; i++) {
        // Egyszerre mérjük mind az RSSI-t, mind az SNR-t
        SignalQualityData signalQuality = pSi4735Manager->getSignalQualityRealtime();
        rssiSum += signalQuality.rssi;
        snrSum += signalQuality.snr;
    }

    // RSSI átlag és Y koordinátára konvertálás
    int avgRssi = rssiSum / countScanSignal;
    rssiY = SCAN_AREA_Y + SCAN_AREA_HEIGHT - (avgRssi * signalScale * 2);

    // RSSI korlátozás
    if (rssiY < SCAN_AREA_Y + 10)
        rssiY = SCAN_AREA_Y + 10;
    if (rssiY > SCAN_AREA_Y + SCAN_AREA_HEIGHT - 10)
        rssiY = SCAN_AREA_Y + SCAN_AREA_HEIGHT - 10;

    // SNR átlag
    snr = snrSum / countScanSignal;

    // Debug info - csak időnként
    static int debugCounter = 0;
    if (debugCounter++ % 50 == 0) {
        DEBUG("getSignalQuality: avgRSSI=%d, avgSNR=%d, rssiY=%d\n", avgRssi, snr, rssiY);
    }
}

void ScanScreen::setFrequency(uint32_t freq) {
    currentScanFreq = freq;
    // Spektrum analizátor hangol át minden frekvenciára a méréshez!
    if (pSi4735Manager) {
        // frekvencia beállítás a Si4735 chipen
        pSi4735Manager->getSi4735().setFrequency(freq / 10); // Si4735 10kHz egységekben dolgozik

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
    if (newZoom >= 1.0f) { // Ne zoómoljunk ki a teljes sávnál jobban
        handleZoom(newZoom);
    }
}

void ScanScreen::handleZoom(float newZoomLevel) {
    if (!pSi4735Manager)
        return;

    // Ne lehessen kizoómolni a teljes sávnál jobban
    if (newZoomLevel < 1.0f) {
        DEBUG("Zoom limit reached - cannot zoom out beyond full band\n");
        return;
    }

    // Aktuális sáv információk lekérése
    BandTable &currentBand = pSi4735Manager->getCurrentBand();
    uint32_t bandStartFreq = currentBand.minimumFreq * 10; // kHz-ben
    uint32_t bandEndFreq = currentBand.maximumFreq * 10;   // kHz-ben

    // Zoom központ meghatározása (aktuális frekvencia)
    uint32_t zoomCenter = currentScanFreq;

    // Új scan tartomány számítása a zoom körül
    uint32_t totalBandwidth = bandEndFreq - bandStartFreq;
    uint32_t newScanBandwidth = (uint32_t)(totalBandwidth / newZoomLevel);

    // Új scan határok számítása a központ körül
    uint32_t halfBandwidth = newScanBandwidth / 2;
    uint32_t newScanStart = (zoomCenter > halfBandwidth) ? zoomCenter - halfBandwidth : bandStartFreq;
    uint32_t newScanEnd = (zoomCenter + halfBandwidth < bandEndFreq) ? zoomCenter + halfBandwidth : bandEndFreq;

    // Sáv határokon belül tartás
    if (newScanStart < bandStartFreq) {
        newScanStart = bandStartFreq;
        newScanEnd = newScanStart + newScanBandwidth;
        if (newScanEnd > bandEndFreq) {
            newScanEnd = bandEndFreq;
        }
    }
    if (newScanEnd > bandEndFreq) {
        newScanEnd = bandEndFreq;
        newScanStart = newScanEnd - newScanBandwidth;
        if (newScanStart < bandStartFreq) {
            newScanStart = bandStartFreq;
        }
    } // Minimális zoom ellenőrzés (ne legyen túl kicsi a tartomány)
    uint32_t minBandwidth = totalBandwidth / 10; // Minimum a teljes sáv 10%-a
    if (minBandwidth < 500)
        minBandwidth = 500; // De legalább 500 kHz

    if (newScanEnd - newScanStart < minBandwidth) {
        DEBUG("Zoom limit reached - minimum bandwidth: %d kHz\n", minBandwidth);
        return;
    }

    // Értékek frissítése
    zoomLevel = newZoomLevel;
    scanStartFreq = newScanStart;
    scanEndFreq = newScanEnd;

    calculateScanParameters();

    // Zoom után csak a spektrum adatokat inicializáljuk újra, ne a teljes UI-t
    scanEmpty = true;
    currentScanPos = 0; // Adatok törlése
    for (int i = 0; i < SCAN_RESOLUTION; i++) {
        scanValueRSSI[i] = SCAN_AREA_Y + SCAN_AREA_HEIGHT; // Spektrum alján kezdjük (nincs látható jel)
        scanValueSNR[i] = 0;
        scanMark[i] = false;
        scanScaleLine[i] = 0;
    }

    // Sáv határok újraszámítása
    scanBeginBand = -1;
    scanEndBand = SCAN_RESOLUTION;

    // Spektrum és címkék frissítése
    drawSpectrum();
    drawScale();
    drawFrequencyLabels(); // Ez most frissülni fog!
    drawBandBoundaries();
    drawScanInfo();

    DEBUG("Zoom changed to: %.2f, range: %d-%d kHz\n", zoomLevel, scanStartFreq, scanEndFreq);
}

uint32_t ScanScreen::positionToFreq(uint16_t dataPos) {
    if (dataPos >= SCAN_RESOLUTION)
        return scanEndFreq;

    float ratio = (float)dataPos / SCAN_RESOLUTION;
    uint32_t totalBandwidth = scanEndFreq - scanStartFreq;
    return scanStartFreq + (uint32_t)(ratio * totalBandwidth);
}

uint16_t ScanScreen::freqToPosition(uint32_t freq) {
    if (freq <= scanStartFreq)
        return 0;
    if (freq >= scanEndFreq)
        return SCAN_RESOLUTION - 1;

    uint32_t totalBandwidth = scanEndFreq - scanStartFreq;
    float ratio = (float)(freq - scanStartFreq) / totalBandwidth;
    return (uint16_t)(ratio * SCAN_RESOLUTION);
}
