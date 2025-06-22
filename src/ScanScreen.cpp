/**
 * @file ScanScreen.cpp
 * @brief Spektrum analizátor scan képernyő implementáció
 */

#include "ScanScreen.h"
#include "ScreenManager.h"
#include "defines.h"
#include "rtVars.h"

// ===================================================================
// Konstruktor és inicializálás
// ===================================================================

/**
 * @brief ScanScreen konstruktor
 */
ScanScreen::ScanScreen(TFT_eSPI &tft, Si4735Manager *si4735Manager)
    : UIScreen(tft, SCREEN_NAME_SCAN, si4735Manager), currentState(ScanState::Idle), currentMode(ScanMode::Spectrum), scanStartFreq(0), scanEndFreq(0), currentScanFreq(0),
      scanStep(5),                                                                             // Default 5kHz lépés
      currentScanLine(0), previousScanLine(0), deltaLine(0), lastScanUpdate(0), scanSpeed(25), // Default 25ms/lépés (gyorsabb széles kijelzőkön)
      snrThreshold(15),
      // Zoom és sávkezelés inicializálása (orosz alapján)
      scanStepFloat(5.0), minScanStep(0.5), maxScanStep(50.0), autoScanStep(true), currentMinScanStep(0.5), currentMaxScanStep(50.0), scanBeginBand(-1), scanEndBand(0),
      deltaScanLine(0.0) { // Spektrum méretek számítása a tényleges képernyő méret alapján
    SPECTRUM_WIDTH = UIComponent::SCREEN_W - (2 * SPECTRUM_MARGIN);
    SPECTRUM_X = SPECTRUM_MARGIN;

    // Adaptív scan sebesség: szélesebb kijelzőkön gyorsabb
    if (SPECTRUM_WIDTH > 350) {
        scanSpeed = 20; // Nagyon gyors széles kijelzőkön (480px+)
    } else if (SPECTRUM_WIDTH > 280) {
        scanSpeed = 25; // Közepes gyorsaság
    } else {
        scanSpeed = 35; // Lassabb keskeny kijelzőkön
    }

    DEBUG("ScanScreen initialized - Screen width: %d, Spectrum width: %d, Margin: %d, Scan speed: %d ms\n", UIComponent::SCREEN_W, SPECTRUM_WIDTH, SPECTRUM_MARGIN, scanSpeed);

    // Spektrum tömbök inicializálása
    spectrumRSSI.resize(SPECTRUM_WIDTH, 0);
    spectrumSNR.resize(SPECTRUM_WIDTH, 0);
    stationMarks.resize(SPECTRUM_WIDTH, false);
    scaleLines.resize(SPECTRUM_WIDTH, 0);

    DEBUG("ScanScreen initialized\n");
}

// ===================================================================
// UIScreen interface implementáció
// ===================================================================

/**
 * @brief Scan képernyő aktiválása
 */
void ScanScreen::activate() {
    UIScreen::activate();

    // Scan paraméterek számítása az aktuális band alapján
    calculateScanParameters();

    // UI komponensek létrehozása
    createScanButtons();

    // Spektrum alapállapot beállítása
    resetSpectrumData();

    // Kezdő frekvencia beállítása az aktuális frekvenciára
    if (pSi4735Manager) {
        currentScanFreq = pSi4735Manager->getSi4735().getCurrentFrequency();
        currentScanLine = frequencyToPixel(currentScanFreq);
    }

    DEBUG("ScanScreen activated - Band: %s, Range: %d-%d kHz\n", getBandName().c_str(), scanStartFreq, scanEndFreq);
}

/**
 * @brief Scan képernyő deaktiválása
 */
void ScanScreen::deactivate() {
    // Scan leállítása
    if (currentState != ScanState::Idle) {
        stopScan();
    }

    UIScreen::deactivate();
    DEBUG("ScanScreen deactivated\n");
}

/**
 * @brief Statikus tartalom rajzolása
 */
void ScanScreen::drawContent() {
    // Háttér törlése
    tft.fillScreen(TFT_BLACK);

    // Font inicializálása
    tft.setFreeFont(); // Alapértelmezett font beállítása

    // Címsor
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("SPECTRUM ANALYZER", tft.width() / 2, 10);

    // Band információ
    tft.setTextSize(1);
    String bandInfo = getBandName() + " BAND (" + formatFrequency(scanStartFreq) + " - " + formatFrequency(scanEndFreq) + ")";
    tft.drawString(bandInfo, tft.width() / 2, 35); // Spektrum háttér
    drawSpectrumBackground();

    // Frekvencia skála
    drawFrequencyScale();

    // Frekvencia címkék
    drawFrequencyLabels();

    // Teljes spektrum kirajzolása
    drawSpectrumDisplay();

    // Státusz információk
    drawScanStatus();
    drawSignalInfo();
}

/**
 * @brief Fő loop kezelés
 */
void ScanScreen::handleOwnLoop() {
    uint32_t currentTime = millis();

    // Scan frissítés időzítéssel
    if (currentState == ScanState::Scanning && (currentTime - lastScanUpdate >= scanSpeed)) {
        DEBUG("[LOOP] updateSpectrum called - currentTime: %lu, lastScanUpdate: %lu, scanSpeed: %d\n", currentTime, lastScanUpdate, scanSpeed);
        updateSpectrum();
        lastScanUpdate = currentTime;
    }

    // Gomb állapotok frissítése csak állapotváltozáskor (nem minden loop-ban)
    // Az updateScanButtonStates()-t csak a scan indítás/leállítás metódusokban hívjuk
}

/**
 * @brief Érintés esemény kezelése
 */
bool ScanScreen::handleTouch(const TouchEvent &event) {
    if (event.pressed) {
        // Spektrum terület érintése - scan alatt letiltjuk a spektrum érintést
        if (event.x >= SPECTRUM_X && event.x < SPECTRUM_X + SPECTRUM_WIDTH && event.y >= SPECTRUM_Y && event.y < SPECTRUM_Y + SPECTRUM_HEIGHT) {
            // Csak akkor engedjük meg a spektrum érintést, ha nincs aktív scan
            if (currentState != ScanState::Scanning) {
                handleSpectrumTouch(event.x, event.y);
                return true;
            } else {
                DEBUG("[TOUCH] Spectrum touch ignored during scanning\n");
                return true; // Esemény elfogyasztva, de nincs változás
            }
        }
    }

    // Alapértelmezett érintés kezelés (gombok)
    return UIScreen::handleTouch(event);
}

/**
 * @brief Rotary encoder kezelése
 */
bool ScanScreen::handleRotary(const RotaryEvent &event) {
    if (currentState == ScanState::Idle) {
        // Orosz-stílusú rotary kezelés: figyelembe veszi a band step-et és zoom szintet
        if (event.direction == RotaryEvent::Direction::Up || event.direction == RotaryEvent::Direction::Down) {
            clearCursor();         // Lépés méret számítása a jelenlegi band alapján
            uint32_t bandStep = 1; // Default 1 kHz
            if (pSi4735Manager) {
                auto currentBand = pSi4735Manager->getCurrentBand();
                bandStep = currentBand.currStep; // A band-specifikus lépésköz használata
            }

            // Orosz logika: rotary lépés arányos a scan step-pel
            float rotaryStepSize = (float)bandStep / scanStepFloat;
            if (rotaryStepSize < 1.0)
                rotaryStepSize = 1.0; // Minimum 1 pixel lépés

            int16_t direction = (event.direction == RotaryEvent::Direction::Up) ? 1 : -1;

            // Új pozíció számítása
            float newLine = currentScanLine + (direction * rotaryStepSize);

            // Határok ellenőrzése és sáv határok respektálása
            if (newLine < 0)
                newLine = 0;
            if (newLine >= SPECTRUM_WIDTH)
                newLine = SPECTRUM_WIDTH - 1;

            currentScanLine = (uint16_t)newLine;
            uint32_t newFreq = pixelToFrequency(currentScanLine);
            // Sáv határ ellenőrzés és automatikus scroll (orosz logika)
            if (isFrequencyInBand(newFreq)) {
                currentScanFreq = newFreq;
                if (pSi4735Manager) {
                    pSi4735Manager->getSi4735().setFrequency(currentScanFreq);
                }
                drawCursor();
                drawSignalInfo();
                DEBUG("[ROTARY] %s - Line: %d, Freq: %d kHz, Step: %.2f\n", (direction > 0) ? "UP" : "DOWN", currentScanLine, currentScanFreq, rotaryStepSize);
            } else {
                // Orosz logika: ha sávon kívülre mennénk, scroll-ozzunk a spektrumban
                if (newFreq > scanEndFreq && direction > 0) {
                    // Jobbra scroll, ha a sáv még engedi
                    deltaScanLine += rotaryStepSize;
                    currentScanLine = SPECTRUM_WIDTH / 2; // Vissza középre
                    updateScanParameters();
                    drawSpectrumDisplay();
                    DEBUG("[ROTARY] Scroll right: delta=%.2f\n", deltaScanLine);
                } else if (newFreq < scanStartFreq && direction < 0) {
                    // Balra scroll, ha a sáv még engedi
                    deltaScanLine -= rotaryStepSize;
                    currentScanLine = SPECTRUM_WIDTH / 2; // Vissza középre
                    updateScanParameters();
                    drawSpectrumDisplay();
                    DEBUG("[ROTARY] Scroll left: delta=%.2f\n", deltaScanLine);
                } else {
                    DEBUG("[ROTARY] Cannot scroll further: %d kHz\n", newFreq);
                }
            }
        }
        return true;
    }

    return false;
}

// ===================================================================
// Scan kontroll metódusok
// ===================================================================

/**
 * @brief Spektrum scan indítása
 */
void ScanScreen::startSpectruScan() {
    DEBUG("startSpectruScan called - currentState: %d\n", (int)currentState);

    if (currentState == ScanState::Idle) {
        DEBUG("Starting spectrum scan (full coverage) - SPECTRUM_WIDTH: %d\n", SPECTRUM_WIDTH);
        currentState = ScanState::Scanning;

        // Teljes spektrum lefedése: kezdés a bal széltől
        currentScanLine = 0;
        currentScanFreq = scanStartFreq;
        deltaScanLine = 0.0;
        lastScanUpdate = millis();

        DEBUG("startSpectruScan: scanStartFreq=%d, scanEndFreq=%d, currentScanFreq=%d\n", scanStartFreq, scanEndFreq, currentScanFreq);

        // Spektrum adatok és paraméterek inicializálása
        resetSpectrumData();
        updateScanParameters(); // Sávhatárok és skála vonalak számítása
        drawSpectrumBackground();

        // Első pozíció beállítása
        if (pSi4735Manager) {
            pSi4735Manager->getSi4735().setFrequency(currentScanFreq);
            DEBUG("startSpectruScan: Si4735 frequency set to %d kHz\n", currentScanFreq);
        } else {
            DEBUG("ERROR: pSi4735Manager is null in startSpectruScan!\n");
        }
        DEBUG("Scan started: start freq=%d kHz, end freq=%d kHz, scanStep=%.2f, target width=%d\n", scanStartFreq, scanEndFreq, scanStepFloat, SPECTRUM_WIDTH);

        // Gomb állapot frissítése
        updateScanButtonStates();

        // Státusz kijelző frissítése
        drawScanStatus();
    } else {
        DEBUG("startSpectruScan: Cannot start scan - currentState is not Idle (%d)\n", (int)currentState);
    }
}

/**
 * @brief Scan leállítása
 */
void ScanScreen::stopScan() {
    if (currentState != ScanState::Idle) {
        DEBUG("Stopping scan\n");
        currentState = ScanState::Idle;

        // Utolsó frekvencia beállítása a rádióban
        if (pSi4735Manager) {
            pSi4735Manager->getSi4735().setFrequency(currentScanFreq);
        }

        // Gomb állapot frissítése
        updateScanButtonStates();

        // Státusz kijelző frissítése
        drawScanStatus();
    }
}

// ===================================================================
// Spektrum kezelés
// ===================================================================

/**
 * @brief Spektrum frissítése
 */
void ScanScreen::updateSpectrum() {
    if (currentState != ScanState::Scanning) {
        DEBUG("[SPECTRUM] updateSpectrum called but not scanning - currentState: %d\n", (int)currentState);
        return;
    }

    DEBUG("[SPECTRUM] updateSpectrum: Line %d, Freq %d kHz\n", currentScanLine, currentScanFreq);

    // Jelerősség mérés az aktuális frekvencián
    measureSignalAtCurrentFreq();

    // Aktuális spektrum vonal kirajzolása
    drawSpectrumLine(currentScanLine);

    // Következő frekvenciára lépés
    moveToNextFrequency();

    // Orosz logika: minden 10. mérés után frissítsük a teljes spektrumot
    static uint16_t refreshCounter = 0;
    refreshCounter++;
    if (refreshCounter >= 10) {
        refreshCounter = 0;
        // Teljes spektrum újrarajzolása a kitöltöttség biztosításához
        for (uint16_t i = 0; i < SPECTRUM_WIDTH; i++) {
            // Csak azokat a vonalakat rajzoljuk újra, amelyeknek van adatuk
            if (spectrumRSSI[i] > 0) {
                drawSpectrumLine(i);
            }
        }
        DEBUG("[SCAN] Full spectrum refresh completed\n");
    }

    // Kurzor csak akkor, ha nem scan módban vagyunk, vagy ha megállt
    if (currentState != ScanState::Scanning) {
        drawCursor();
    }
}

/**
 * @brief Jelerősség mérés az aktuális frekvencián
 *
 * Megjegyzés: A scan közben nem muteljük a rádiót (az orosz kód alapján).
 * Ez azt jelenti, hogy a felhasználó hallja a scan közben az aktuális frekvenciát,
 * ami hasznos lehet állomások azonosításához.
 */
void ScanScreen::measureSignalAtCurrentFreq() {
    if (!pSi4735Manager)
        return;

    // Megjegyzés: Nincs muting, a rádió hallatszik scan közben
    // Ez az orosz eredeti kód viselkedésének megfelelő

    uint8_t rssiSum = 0;
    uint8_t snrSum = 0; // Több minta átlagolása
    for (int i = 0; i < 3; i++) {
        SignalQualityData signal = pSi4735Manager->getSignalQualityRealtime();
        if (signal.isValid) {
            rssiSum += signal.rssi;
            snrSum += signal.snr;
        }
        delay(5); // Rövid várakozás minták között
    }

    // Átlagértékek számítása
    uint8_t avgRSSI = rssiSum / 3;
    uint8_t avgSNR = snrSum / 3; // Debug információ
    DEBUG("[SCAN] Freq: %d kHz, Line: %d, RSSI: %d dBuV, SNR: %d dB\n", currentScanFreq, currentScanLine, avgRSSI, avgSNR);

    // Spektrum adatok frissítése (orosz logika: minden pozícióra)
    if (currentScanLine < SPECTRUM_WIDTH) {
        // Átlagolás a meglévő adatokkal (simítás)
        if (spectrumRSSI[currentScanLine] > 0) {
            spectrumRSSI[currentScanLine] = (spectrumRSSI[currentScanLine] + avgRSSI) / 2;
            spectrumSNR[currentScanLine] = (spectrumSNR[currentScanLine] + avgSNR) / 2;
        } else {
            spectrumRSSI[currentScanLine] = avgRSSI;
            spectrumSNR[currentScanLine] = avgSNR;
        }

        // Erős állomás jelölése
        if (avgSNR >= snrThreshold) {
            stationMarks[currentScanLine] = true;
            DEBUG("[SCAN] Strong station detected at %d kHz (SNR: %d dB)\n", currentScanFreq, avgSNR);
        }
    }
}

/**
 * @brief Következő frekvenciára lépés (orosz logika szerint)
 */
void ScanScreen::moveToNextFrequency() {
    // Egyszerű lineáris haladás a teljes spektrum szélességében
    currentScanLine++; // Ha elértük a spektrum végét, kezdjük újra
    if (currentScanLine >= SPECTRUM_WIDTH) {
        currentScanLine = 0; // Újrakezdés a bal széltől
        DEBUG("Scan cycle completed, restarting from beginning (SPECTRUM_WIDTH=%d)\n", SPECTRUM_WIDTH);
    }

    // Frekvencia számítása lineáris interpolációval
    uint32_t freqRange = scanEndFreq - scanStartFreq;
    currentScanFreq = scanStartFreq + (currentScanLine * freqRange) / (SPECTRUM_WIDTH - 1);

    // Rádió frekvencia beállítása
    if (pSi4735Manager) {
        pSi4735Manager->getSi4735().setFrequency(currentScanFreq);
    }

    DEBUG("[SCAN] Line: %d/%d (%0.1f%%), Freq: %d kHz\n", currentScanLine, SPECTRUM_WIDTH, (currentScanLine * 100.0) / SPECTRUM_WIDTH, currentScanFreq);
}

/**
 * @brief Frekvencia érvényességének ellenőrzése
 */
bool ScanScreen::isValidScanFrequency(uint32_t freq) { return freq >= scanStartFreq && freq <= scanEndFreq; }

// ===================================================================
// Grafikus megjelenítés
// ===================================================================

/**
 * @brief Spektrum háttér kirajzolása
 */
void ScanScreen::drawSpectrumBackground() {
    // Spektrum terület kerete
    tft.drawRect(SPECTRUM_X - 1, SPECTRUM_Y - 1, SPECTRUM_WIDTH + 2, SPECTRUM_HEIGHT + 2, TFT_WHITE);

    // Háttér törlése
    tft.fillRect(SPECTRUM_X, SPECTRUM_Y, SPECTRUM_WIDTH, SPECTRUM_HEIGHT, TFT_BLACK);
}

/**
 * @brief Frekvencia skála kirajzolása
 */
void ScanScreen::drawFrequencyScale() {
    if (!pSi4735Manager)
        return;

    // Skála vonalak számítása a sáv típusa alapján
    for (uint16_t i = 0; i < SPECTRUM_WIDTH; i++) {
        uint32_t freq = pixelToFrequency(i);
        scaleLines[i] = 0;

        if (pSi4735Manager->isCurrentBandFM()) {
            // FM sáv: frekvencia 10 kHz egységekben (6400 = 64.0 MHz)
            // Fő vonalak: 5 MHz lépésekben (500 egység)
            // Kis vonalak: 1 MHz lépésekben (100 egység)
            if ((freq % 500) == 0) {
                scaleLines[i] = 2; // Fő vonalak 5 MHz-nél
            } else if ((freq % 100) == 0) {
                scaleLines[i] = 1; // Kis vonalak 1 MHz-nél
            }
        } else {
            // AM/SW sávok: kezelje eltérően a frekvencia egységek szerint
            uint32_t freqRange = scanEndFreq - scanStartFreq;

            if (freqRange > 20000) { // Nagy tartomány (pl. MW)
                if ((freq % 1000) == 0) {
                    scaleLines[i] = 2; // Fő vonalak 1 MHz-nél
                } else if ((freq % 200) == 0) {
                    scaleLines[i] = 1; // Kis vonalak 200 kHz-nél
                }
            } else { // Kisebb tartomány (pl. SW)
                if ((freq % 500) == 0) {
                    scaleLines[i] = 2; // Fő vonalak 500 kHz-nél
                } else if ((freq % 100) == 0) {
                    scaleLines[i] = 1; // Kis vonalak 100 kHz-nél
                }
            }
        }
    }
}

/**
 * @brief Frekvencia címkék kirajzolása (orosz kód alapján)
 */
void ScanScreen::drawFrequencyLabels() {
    // Csak ha van hely a skálának
    if (SPECTRUM_Y + SPECTRUM_HEIGHT + 50 > UIComponent::SCREEN_H) {
        return;
    }
    tft.setFreeFont();
    tft.setTextSize(1);

    uint16_t labelY = SPECTRUM_Y + SPECTRUM_HEIGHT + 5; // Bal oldal - kezdő frekvencia (orosz stílus)
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    String startText;
    if (pSi4735Manager && pSi4735Manager->isCurrentBandFM()) {
        // FM frekvenciák 10 kHz egységekben vannak tárolva (pl. 6400 = 64.0 MHz)
        startText = String(scanStartFreq / 100.0, 1) + " MHz";
    } else {
        startText = String(int(scanStartFreq)) + " KHz";
    }
    tft.drawString(startText, SPECTRUM_X, labelY);

    // Jobb oldal - záró frekvencia
    tft.setTextDatum(TR_DATUM);
    String endText;
    if (pSi4735Manager && pSi4735Manager->isCurrentBandFM()) {
        // FM frekvenciák 10 kHz egységekben vannak tárolva (pl. 10800 = 108.0 MHz)
        endText = String(scanEndFreq / 100.0, 1) + " MHz";
    } else {
        endText = String(int(scanEndFreq)) + " KHz";
    }
    tft.drawString(endText, SPECTRUM_X + SPECTRUM_WIDTH, labelY);

    // Skála információ (orosz stílus) - lépésköz megjelenítés
    tft.fillRect(SPECTRUM_X, labelY + 15, 60, 17, TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);

    // Lépésköz megjelenítés az orosz kód logikája alapján
    if (pSi4735Manager && pSi4735Manager->isCurrentBandFM()) {
        if (scanStep > 4) {
            tft.drawString("10 MHz", SPECTRUM_X, labelY + 30);
        } else if (scanStep == 4) {
            tft.drawString("1 MHz", SPECTRUM_X, labelY + 30);
        } else if (scanStep == 2) {
            tft.drawString("500 KHz", SPECTRUM_X, labelY + 30);
        } else if (scanStep < 2) {
            tft.drawString("100 KHz", SPECTRUM_X, labelY + 30);
        }
    } else {
        // AM/SW/MW/LW band
        if (scanStep > 4) {
            tft.drawString("1 MHz", SPECTRUM_X, labelY + 30);
        } else if (scanStep == 4) {
            tft.drawString("100 KHz", SPECTRUM_X, labelY + 30);
        } else if (scanStep == 2) {
            tft.drawString("50 KHz", SPECTRUM_X, labelY + 30);
        } else if (scanStep < 2) {
            tft.drawString("10 KHz", SPECTRUM_X, labelY + 30);
        }
    }

    // Jobb oldal - zoom információ (orosz stílus)
    uint16_t zoomInfoWidth = 70; // dinamikus, szélesebb kijelzőn több hely
    tft.fillRect(SPECTRUM_X + SPECTRUM_WIDTH - zoomInfoWidth, labelY + 15, zoomInfoWidth, 17, TFT_BLACK);
    tft.setTextDatum(TR_DATUM);
    if (scanStep >= 1) {
        tft.drawString("1:" + String(int(scanStep)), SPECTRUM_X + SPECTRUM_WIDTH, labelY + 30);
    } else {
        tft.drawString("x" + String(int(1.0 / scanStep)), SPECTRUM_X + SPECTRUM_WIDTH, labelY + 30);
    }
}

/**
 * @brief Teljes spektrum kirajzolása
 */
void ScanScreen::drawSpectrumDisplay() {
    for (uint16_t i = 0; i < SPECTRUM_WIDTH; i++) {
        drawSpectrumLine(i);
    }
}

/**
 * @brief Egy spektrum vonal kirajzolása
 */
void ScanScreen::drawSpectrumLine(uint16_t lineIndex) {
    if (lineIndex >= SPECTRUM_WIDTH)
        return;
    uint16_t x = SPECTRUM_X + lineIndex;
    uint8_t rssi = spectrumRSSI[lineIndex];
    uint8_t snr = spectrumSNR[lineIndex];

    // Alapértelmezett színek
    uint16_t backgroundColor = TFT_BLACK;
    uint16_t spectrumColor = snrToColor(snr);

    // RSSI alapú Y pozíció
    uint16_t rssiY = rssiToPixelY(rssi);

    // Háttér törlése
    tft.drawLine(x, SPECTRUM_Y, x, SPECTRUM_Y + SPECTRUM_HEIGHT - 1, backgroundColor);

    // Skála vonalak rajzolása (mindig látható, csak az alsó/felső részen)
    if (scaleLines[lineIndex] == 2) {
        // Fő skála vonalak - alsó rész (10 pixel magas)
        tft.drawLine(x, SPECTRUM_Y + SPECTRUM_HEIGHT - 10, x, SPECTRUM_Y + SPECTRUM_HEIGHT - 1, SCALE_COLOR);
        // Fő skála vonalak - felső rész (5 pixel magas)
        tft.drawLine(x, SPECTRUM_Y, x, SPECTRUM_Y + 5, SCALE_COLOR);
    } else if (scaleLines[lineIndex] == 1) {
        // Kisebb skála vonalak - csak alsó rész (5 pixel magas)
        tft.drawLine(x, SPECTRUM_Y + SPECTRUM_HEIGHT - 5, x, SPECTRUM_Y + SPECTRUM_HEIGHT - 1, TFT_DARKGREY);
    } // Ha van érvényes jel (RSSI > 0), spektrum vonal rajzolása
    if (rssi > 0) {
        // Spektrum vonal rajzolása alsó résztől az RSSI magasságig
        if (rssiY < SPECTRUM_Y + SPECTRUM_HEIGHT) {
            tft.drawLine(x, rssiY, x, SPECTRUM_Y + SPECTRUM_HEIGHT - 1, spectrumColor);

            // RSSI csúcs fehér ponttal
            tft.drawPixel(x, rssiY, RSSI_COLOR);
        }
    }

    // Erős állomás jelölése
    if (stationMarks[lineIndex]) {
        tft.fillRect(x, SPECTRUM_Y + SPECTRUM_HEIGHT - 3, 1, 3, MARK_COLOR);
    }
}

/**
 * @brief Kurzor kirajzolása
 */
void ScanScreen::drawCursor() {
    if (currentScanLine < SPECTRUM_WIDTH) {
        uint16_t x = SPECTRUM_X + currentScanLine;
        tft.drawLine(x, SPECTRUM_Y, x, SPECTRUM_Y + SPECTRUM_HEIGHT - 1, CURSOR_COLOR);
        previousScanLine = currentScanLine; // Pozíció mentése
    }
}

/**
 * @brief Kurzor törlése
 */
void ScanScreen::clearCursor() {
    if (previousScanLine < SPECTRUM_WIDTH) {
        // Előző kurzor helyének újrarajzolása
        drawSpectrumLine(previousScanLine);
    }
}

/**
 * @brief Scan státusz kirajzolása
 */
void ScanScreen::drawScanStatus() {
    uint16_t statusY = SPECTRUM_Y + SPECTRUM_HEIGHT + 20; // 20 pixel lejjebb a címkék után

    // Font inicializálása
    tft.setFreeFont();
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TL_DATUM); // Állapot
    String status;
    switch (currentState) {
        case ScanState::Idle:
            status = "IDLE";
            break;
        case ScanState::Scanning:
            status = "SCANNING";
            break;
    }
    tft.drawString("Status: " + status, 10, statusY);

    // Sebesség és lépésköz
    tft.drawString("Speed: " + String(scanSpeed) + "ms", 120, statusY);
    tft.drawString("Step: " + String(scanStep) + "kHz", 220, statusY);
}

/**
 * @brief Jel információk kirajzolása
 */
void ScanScreen::drawSignalInfo() {
    uint16_t infoY = SPECTRUM_Y + SPECTRUM_HEIGHT + 35; // 35 pixel lejjebb

    // Font inicializálása
    tft.setFreeFont();
    tft.setTextSize(1);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);

    // Aktuális frekvencia
    tft.drawString("Freq: " + formatFrequency(currentScanFreq), 10, infoY);

    // RSSI és SNR értékek
    if (currentScanLine < SPECTRUM_WIDTH) {
        uint8_t rssi = spectrumRSSI[currentScanLine];
        uint8_t snr = spectrumSNR[currentScanLine];

        tft.drawString("RSSI: " + String(rssi) + "dBuV", 120, infoY);
        tft.drawString("SNR: " + String(snr) + "dB", 220, infoY);
    }
}

// ===================================================================
// Érintés kezelés
// ===================================================================

/**
 * @brief Spektrum érintés kezelése (zoom-kompatibilis)
 */
void ScanScreen::handleSpectrumTouch(uint16_t x, uint16_t y) {
    if (currentState == ScanState::Scanning) {
        // Scan alatt nem engedjük az érintést
        return;
    }

    clearCursor();

    // X koordináta átalakítása relatív pozícióvá
    int16_t relativeX = x - SPECTRUM_X;
    if (relativeX < 0)
        relativeX = 0;
    if (relativeX >= SPECTRUM_WIDTH)
        relativeX = SPECTRUM_WIDTH - 1;

    // Új frekvencia számítása a zoom-kompatibilis pixelToFrequency függvénnyel
    uint32_t newFreq = pixelToFrequency(relativeX);

    // Kurzor pozíció és frekvencia frissítése
    currentScanLine = relativeX;
    currentScanFreq = newFreq;

    // Rádió frekvencia beállítása
    if (pSi4735Manager && isFrequencyInBand(currentScanFreq)) {
        pSi4735Manager->getSi4735().setFrequency(currentScanFreq);
    }

    drawCursor();
    drawSignalInfo();

    DEBUG("[TOUCH] Line: %d, Freq: %d kHz (Touch X: %d)\n", currentScanLine, currentScanFreq, x);
}

// ===================================================================
// Gomb események
// ===================================================================

/**
 * @brief Scan gombok létrehozása
 */
void ScanScreen::createScanButtons() {
    // Funkcionális gombok (bal oldal)
    std::vector<UIHorizontalButtonBar::ButtonConfig> mainButtonConfigs = {
        {ScanButtonIDs::START_STOP_BUTTON, "Scan", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleStartStopButton(event); }},

        {ScanButtonIDs::MODE_BUTTON, "Mode", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleModeButton(event); }},

        {ScanButtonIDs::SPEED_BUTTON, "Speed", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleSpeedButton(event); }},
        {ScanButtonIDs::SCALE_BUTTON, "Zoom", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleZoomButton(event); }}};

    // Back gomb (jobb oldal)
    std::vector<UIHorizontalButtonBar::ButtonConfig> backButtonConfigs = {
        {ScanButtonIDs::BACK_BUTTON, "Back", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleBackButton(event); }}};

    // Gombsor elrendezés - képernyő aljára igazítva
    uint16_t buttonHeight = 30;
    uint16_t buttonY = tft.height() - buttonHeight; // Közvetlenül a képernyő aljára
    uint16_t buttonGap = 3;
    uint16_t margin = 10;

    // Funkcionális gombok bal oldalon
    uint16_t mainButtonCount = mainButtonConfigs.size();
    uint16_t mainButtonWidth = 70; // Fix szélesség
    uint16_t mainTotalWidth = mainButtonCount * mainButtonWidth + (mainButtonCount - 1) * buttonGap;
    uint16_t mainButtonX = margin;

    // Back gomb jobb oldalon
    uint16_t backButtonWidth = 60;
    uint16_t backButtonX = tft.width() - margin - backButtonWidth;

    DEBUG("Button layout: main buttons at %d, back button at %d, screen width: %d\n", mainButtonX, backButtonX, tft.width());

    // Fő gombok létrehozása
    scanButtonBar =
        std::make_shared<UIHorizontalButtonBar>(tft, Rect(mainButtonX, buttonY, mainTotalWidth, buttonHeight), mainButtonConfigs, mainButtonWidth, buttonHeight, buttonGap);
    addChild(scanButtonBar);

    // Back gomb létrehozása
    backButtonBar = std::make_shared<UIHorizontalButtonBar>(tft, Rect(backButtonX, buttonY, backButtonWidth, buttonHeight), backButtonConfigs, backButtonWidth, buttonHeight, 0);
    addChild(backButtonBar);
}

/**
 * @brief Start/Stop gomb kezelése
 */
void ScanScreen::handleStartStopButton(const UIButton::ButtonEvent &event) {
    DEBUG("handleStartStopButton: event.state = %d\n", (int)event.state);

    if (event.state == UIButton::EventButtonState::On || event.state == UIButton::EventButtonState::Off) {
        DEBUG("handleStartStopButton: Button toggled to %s\n", event.state == UIButton::EventButtonState::On ? "ON" : "OFF");

        if (event.state == UIButton::EventButtonState::On) {
            // Button turned ON - start scan
            DEBUG("handleStartStopButton: Starting scan (button ON)...\n");
            startSpectruScan();
            DEBUG("Scan started via toggle button\n");
        } else {
            // Button turned OFF - stop scan
            DEBUG("handleStartStopButton: Stopping scan (button OFF)...\n");
            stopScan();
            DEBUG("Scan stopped via toggle button\n");
        }
    } else {
        DEBUG("handleStartStopButton: Button event ignored - state = %d\n", (int)event.state);
    }
}

/**
 * @brief Mode gomb kezelése
 */
void ScanScreen::handleModeButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        // Mode váltás (TODO: implementálni)
        DEBUG("Mode button clicked\n");
    }
}

/**
 * @brief Speed gomb kezelése
 */
void ScanScreen::handleSpeedButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        // Adaptív sebesség opciók a képernyő szélesség alapján
        if (SPECTRUM_WIDTH > 350) {
            // Széles kijelzők (480px+): 10, 20, 35, 50 ms
            if (scanSpeed <= 10)
                scanSpeed = 20;
            else if (scanSpeed <= 20)
                scanSpeed = 35;
            else if (scanSpeed <= 35)
                scanSpeed = 50;
            else
                scanSpeed = 10;
        } else {
            // Keskeny kijelzők (320px): 25, 50, 100, 200 ms
            if (scanSpeed <= 25)
                scanSpeed = 50;
            else if (scanSpeed <= 50)
                scanSpeed = 100;
            else if (scanSpeed <= 100)
                scanSpeed = 200;
            else
                scanSpeed = 25;
        }

        DEBUG("Scan speed changed to %d ms (adaptive for %dpx width)\n", scanSpeed, SPECTRUM_WIDTH);
        drawScanStatus();
    }
}

/**
 * @brief Zoom gomb kezelése (orosz logika alapján)
 */
void ScanScreen::handleZoomButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        // Scan alatt ne engedjük a zoom változtatást
        if (currentState == ScanState::Scanning) {
            DEBUG("[ZOOM] Zoom ignored during scanning\n");
            return;
        }

        // Kurzor frekvencia mentése zoom előtt
        uint32_t cursorFreq = currentScanFreq; // Körkörös zoom logika - diszkrét zoom szintek használata
        float oldStep = scanStepFloat;

        // Zoom szintek lekérése
        float zoomLevels[10];
        int numZoomLevels;
        getZoomLevels(zoomLevels, &numZoomLevels);

        // Aktuális zoom szint megkeresése
        int currentZoomIndex = getCurrentZoomIndex();
        // Következő zoom szintre váltás (körkörös)
        currentZoomIndex = (currentZoomIndex + 1) % numZoomLevels;
        scanStepFloat = zoomLevels[currentZoomIndex];
        scanStep = (uint16_t)scanStepFloat;

        DEBUG("[ZOOM] Circular zoom: level %d/%d, step %.2f -> %.2f kHz\n", currentZoomIndex + 1, numZoomLevels, oldStep, scanStepFloat);

        // ZOOM LOGIKA: A spektrum tartomány újraszámítása a kurzor körül
        // Az új zoom tartomány a kurzor frekvencia körül lesz központosítva
        uint32_t halfRange = (SPECTRUM_WIDTH / 2) * scanStepFloat;
        uint32_t bandMin = getBandMinFreq();
        uint32_t bandMax = getBandMaxFreq();

        // Új zoom tartomány számítása
        uint32_t zoomStartFreq = (cursorFreq > halfRange) ? (cursorFreq - halfRange) : bandMin;
        uint32_t zoomEndFreq = (cursorFreq + halfRange < bandMax) ? (cursorFreq + halfRange) : bandMax;

        // Határok korrekciója
        if (zoomStartFreq < bandMin)
            zoomStartFreq = bandMin;
        if (zoomEndFreq > bandMax)
            zoomEndFreq = bandMax;
        if (zoomEndFreq - zoomStartFreq < SPECTRUM_WIDTH * scanStepFloat) {
            // Ha túl kicsi a tartomány, növeljük
            uint32_t needed = SPECTRUM_WIDTH * scanStepFloat;
            if (cursorFreq + needed / 2 <= bandMax && cursorFreq >= needed / 2) {
                zoomStartFreq = cursorFreq - needed / 2;
                zoomEndFreq = cursorFreq + needed / 2;
            }
        } // Eredeti tartomány mentése és zoom tartomány beállítása
        scanStartFreq = zoomStartFreq;
        scanEndFreq = zoomEndFreq;

        // Kurzor pozíció újraszámítása az új tartományban
        currentScanFreq = cursorFreq;         // Kurzor frekvencia marad ugyanaz
        currentScanLine = SPECTRUM_WIDTH / 2; // Kurzor a képernyő közepére        deltaScanLine = 0.0; // Delta nullázása

        DEBUG("ZOOM: New range %d-%d kHz, cursor at %d kHz\n", scanStartFreq, scanEndFreq, cursorFreq);

        // Spektrum adatok törlése - zoom után új tartomány, régi adatok érvénytelenek
        resetSpectrumData();

        // Frekvencia skála újraszámítása az új zoom szinthez
        drawFrequencyScale(); // Csak a spektrum terület újrarajzolása (nem a gombokat!)
        drawSpectrumBackground();
        drawFrequencyLabels();
        drawSpectrumDisplay();
        drawCursor();
        drawSignalInfo();
        drawScanStatus();

        // Band információ frissítése az új tartománnyal
        tft.setTextSize(1);
        tft.fillRect(0, 35, tft.width(), 15, TFT_BLACK); // Töröljük a régi band infót
        String bandInfo = getBandName() + " BAND (" + formatFrequency(scanStartFreq) + " - " + formatFrequency(scanEndFreq) + ")";
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextDatum(TC_DATUM);
        tft.drawString(bandInfo, tft.width() / 2, 35);

        DEBUG("Zoom button: step %.2f -> %.2f kHz, cursor centered at %d kHz\n", oldStep, scanStepFloat, cursorFreq);
    }
}

/**
 * @brief Back gomb kezelése
 */
void ScanScreen::handleBackButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        // Visszatérés az előző képernyőre
        const char *targetScreen = pSi4735Manager->isCurrentBandFM() ? SCREEN_NAME_FM : SCREEN_NAME_AM;
        getScreenManager()->switchToScreen(targetScreen);
    }
}

// ===================================================================
// Utility metódusok
// ===================================================================

/**
 * @brief Scan paraméterek számítása
 */
void ScanScreen::calculateScanParameters() {
    DEBUG("calculateScanParameters: pSi4735Manager = %p\n", pSi4735Manager);

    scanStartFreq = getBandMinFreq();
    scanEndFreq = getBandMaxFreq();

    DEBUG("calculateScanParameters: scanStartFreq=%d, scanEndFreq=%d\n", scanStartFreq, scanEndFreq);

    // Orosz logika alapján: automatikus scan step kalkuláció
    if (autoScanStep) {
        float tmp = float(scanEndFreq - scanStartFreq) / float(SPECTRUM_WIDTH);
        float i = maxScanStep / 2.0;

        // Optimális lépésköz keresése
        while (i >= minScanStep) {
            if (tmp > i) {
                currentMinScanStep = i / 4.0;
                currentMaxScanStep = i * 2.0;
                i = 0;
            }
            i /= 2.0;
        }

        // Korrekciók
        if (currentMinScanStep > 0.5)
            currentMinScanStep = 0.5;
        if (currentMinScanStep < minScanStep)
            currentMinScanStep = minScanStep;
        if (currentMaxScanStep == minScanStep)
            currentMaxScanStep *= 2.0;
    } else {
        currentMinScanStep = minScanStep;
        currentMaxScanStep = maxScanStep;
    } // Kezdő scan step beállítása - középső zoom szinttel indítunk
    float zoomLevels[10];
    int numZoomLevels;
    getZoomLevels(zoomLevels, &numZoomLevels);
    int initialZoomIndex = numZoomLevels / 2; // Középső zoom szint
    scanStepFloat = zoomLevels[initialZoomIndex];
    scanStep = (uint16_t)scanStepFloat;

    DEBUG("Scan parameters: %d-%d kHz, initial zoom level %d/%d, step %.2f kHz (range: %.2f-%.2f)\n", scanStartFreq, scanEndFreq, initialZoomIndex + 1, numZoomLevels,
          scanStepFloat, currentMinScanStep, currentMaxScanStep);
}

/**
 * @brief Spektrum adatok törlése
 */
void ScanScreen::resetSpectrumData() {
    std::fill(spectrumRSSI.begin(), spectrumRSSI.end(), 0);
    std::fill(spectrumSNR.begin(), spectrumSNR.end(), 0);
    std::fill(stationMarks.begin(), stationMarks.end(), false);
    std::fill(scaleLines.begin(), scaleLines.end(), 0);
}

/**
 * @brief RSSI érték pixel Y pozícióvá konvertálása
 */
uint16_t ScanScreen::rssiToPixelY(uint8_t rssi) {
    // RSSI skálázása a spektrum magasságára
    uint16_t scaledRssi = (rssi * SPECTRUM_HEIGHT) / 127;
    return SPECTRUM_Y + SPECTRUM_HEIGHT - scaledRssi;
}

/**
 * @brief SNR érték színné konvertálása
 */
uint16_t ScanScreen::snrToColor(uint8_t snr) {
    if (snr < 8)
        return TFT_NAVY;
    else if (snr < 16)
        return TFT_BLUE;
    else if (snr < 24)
        return TFT_GREEN;
    else
        return TFT_YELLOW;
}

/**
 * @brief Pixel X pozíció frekvenciává konvertálása (zoom-kompatibilis)
 */
uint32_t ScanScreen::pixelToFrequency(uint16_t pixelX) {
    if (pixelX >= SPECTRUM_WIDTH)
        pixelX = SPECTRUM_WIDTH - 1;

    // Lineáris interpoláció a jelenlegi spektrum tartományban (zoom-kompatibilis)
    uint32_t freqRange = scanEndFreq - scanStartFreq;
    uint32_t freq = scanStartFreq + (pixelX * freqRange) / (SPECTRUM_WIDTH - 1);

    // Sáv határok ellenőrzése
    if (freq < scanStartFreq)
        freq = scanStartFreq;
    if (freq > scanEndFreq)
        freq = scanEndFreq;

    return freq;
}

/**
 * @brief Frekvencia pixel X pozícióvá konvertálása (zoom-kompatibilis)
 */
uint16_t ScanScreen::frequencyToPixel(uint32_t frequency) {
    // Lineáris interpoláció a jelenlegi spektrum tartományban (zoom-kompatibilis)
    if (frequency <= scanStartFreq)
        return 0;
    if (frequency >= scanEndFreq)
        return SPECTRUM_WIDTH - 1;

    uint32_t freqRange = scanEndFreq - scanStartFreq;
    if (freqRange == 0)
        return SPECTRUM_WIDTH / 2;

    uint32_t relativeFreq = frequency - scanStartFreq;
    uint16_t pixelX = (relativeFreq * (SPECTRUM_WIDTH - 1)) / freqRange;

    if (pixelX >= SPECTRUM_WIDTH)
        return SPECTRUM_WIDTH - 1;

    return pixelX;
}

/**
 * @brief Frekvencia formázása szöveggé
 */
String ScanScreen::formatFrequency(uint32_t frequency) {
    // FM esetében 10 kHz egységben vannak a frekvenciák (pl. 6400 = 64.0 MHz)
    // AM/SW/MW/LW esetében kHz egységben (pl. 540 = 540 kHz)
    if (pSi4735Manager && pSi4735Manager->isCurrentBandFM()) {
        return String(frequency / 100.0, 1) + " MHz";
    } else {
        if (frequency >= 1000) {
            return String(frequency / 1000.0, 3) + " MHz";
        } else {
            return String(frequency) + " kHz";
        }
    }
}

/**
 * @brief Gomb állapotok frissítése
 */
void ScanScreen::updateScanButtonStates() {
    if (!scanButtonBar)
        return; // Toggle gomb állapot frissítése
    auto startButton = scanButtonBar->getButton(ScanButtonIDs::START_STOP_BUTTON);
    if (startButton) {
        if (currentState == ScanState::Scanning) {
            // Scan fut: gomb ON állapotban (zöld)
            startButton->setButtonState(UIButton::ButtonState::On);
        } else {
            // Scan nem fut: gomb OFF állapotban
            startButton->setButtonState(UIButton::ButtonState::Off);
        }
    }

    DEBUG("Button states updated - Scan state: %d\n", (int)currentState);
}

/**
 * @brief Band minimum frekvencia lekérése
 */
uint32_t ScanScreen::getBandMinFreq() {
    if (!pSi4735Manager) {
        DEBUG("ERROR: pSi4735Manager is null in getBandMinFreq!\n");
        return 87500; // Default FM start
    }
    return pSi4735Manager->getCurrentBand().minimumFreq;
}

/**
 * @brief Band maximum frekvencia lekérése
 */
uint32_t ScanScreen::getBandMaxFreq() {
    if (!pSi4735Manager) {
        DEBUG("ERROR: pSi4735Manager is null in getBandMaxFreq!\n");
        return 108000; // Default FM end
    }
    return pSi4735Manager->getCurrentBand().maximumFreq;
}

/**
 * @brief Band név lekérése
 */
String ScanScreen::getBandName() { return String(pSi4735Manager->getCurrentBandName()); }

// ===================================================================
// Zoom és sávkezelés (orosz alapján)
// ===================================================================

/**
 * @brief Scan step automatikus kalkulációja
 */
void ScanScreen::calculateScanStep() {
    if (autoScanStep) {
        float bandWidth = float(scanEndFreq - scanStartFreq);
        float tmp = bandWidth / float(SPECTRUM_WIDTH);
        float i = maxScanStep / 2.0;

        while (i >= minScanStep) {
            if (tmp > i) {
                currentMinScanStep = i / 4.0;
                currentMaxScanStep = i * 2.0;
                break;
            }
            i /= 2.0;
        }

        if (currentMinScanStep > 0.5)
            currentMinScanStep = 0.5;
        if (currentMinScanStep < minScanStep)
            currentMinScanStep = minScanStep;
        if (currentMaxScanStep == minScanStep)
            currentMaxScanStep *= 2.0;
    }

    DEBUG("Calculated scan step range: %.2f - %.2f kHz\n", currentMinScanStep, currentMaxScanStep);
}

/**
 * @brief Scan paraméterek frissítése
 */
void ScanScreen::updateScanParameters() {
    // Sáv határok újraszámítása
    scanBeginBand = -1;
    scanEndBand = SPECTRUM_WIDTH;

    // Spektrum vonalak újraszámítása (lineáris logika)
    for (int n = 0; n < SPECTRUM_WIDTH; n++) {
        uint32_t freqRange = scanEndFreq - scanStartFreq;
        uint32_t freq = scanStartFreq + (n * freqRange) / (SPECTRUM_WIDTH - 1);

        // Lineáris interpoláció: minden pozíció a sávon belül van
        // scanBeginBand és scanEndBand már nem szükségesek

        // Skála vonalak újraszámítása
        scaleLines[n] = 0;
        float stepSize = float(freqRange) / float(SPECTRUM_WIDTH);
        if (stepSize > 4.0) {
            if ((freq % 1000) < (uint32_t)stepSize)
                scaleLines[n] = 2; // Fő vonal
        } else {
            if ((freq % 100) < (uint32_t)stepSize)
                scaleLines[n] = 2; // Fő vonal
        }
    }

    DEBUG("Band limits: begin=%d, end=%d\n", scanBeginBand, scanEndBand);
}

/**
 * @brief Zoom in (nagyítás)
 */
void ScanScreen::zoomIn() {
    if (currentState == ScanState::Scanning) {
        // Scan alatt nem engedjük a zoom-ot
        return;
    }

    // Orosz logika: scan step kétszerezése = zoom out, felezés = zoom in
    scanStepFloat /= 2.0;
    if (scanStepFloat < currentMinScanStep) {
        scanStepFloat = currentMaxScanStep; // Körkörös visszatérés
        deltaScanLine *= (currentMaxScanStep / currentMinScanStep);
    }
    scanStep = (uint16_t)scanStepFloat;
    updateScanParameters();
    resetSpectrumData(); // Spektrum adatok törlése zoom után
    drawSpectrumDisplay();

    DEBUG("Zoom in: step=%.2f kHz\n", scanStepFloat);
}

/**
 * @brief Zoom out (kicsinyítés)
 */
void ScanScreen::zoomOut() {
    if (currentState == ScanState::Scanning) {
        // Scan alatt nem engedjük a zoom-ot
        return;
    }

    // Orosz logika: scan step duplikálása
    float oldStep = scanStepFloat;
    scanStepFloat *= 2.0;
    if (scanStepFloat > currentMaxScanStep) {
        scanStepFloat = currentMinScanStep;
        if (oldStep == currentMaxScanStep) {
            deltaScanLine *= (currentMaxScanStep / currentMinScanStep);
        }
    }
    scanStep = (uint16_t)scanStepFloat;
    updateScanParameters();
    resetSpectrumData(); // Spektrum adatok törlése zoom után
    drawSpectrumDisplay();

    DEBUG("Zoom out: step=%.2f kHz\n", scanStepFloat);
}

/**
 * @brief Zoom reset alapértelmezett értékre
 */
void ScanScreen::resetZoom() {
    scanStepFloat = currentMaxScanStep / 2.0;
    scanStep = (uint16_t)scanStepFloat;
    deltaScanLine = 0.0;
    currentScanLine = SPECTRUM_WIDTH / 2;

    updateScanParameters();
    resetSpectrumData(); // Spektrum adatok törlése zoom reset után
    drawSpectrumDisplay();

    DEBUG("Zoom reset: step=%.2f kHz\n", scanStepFloat);
}

/**
 * @brief Zoom szintek lekérése
 */
void ScanScreen::getZoomLevels(float *levels, int *count) {
    static float zoomLevels[] = {currentMinScanStep, currentMinScanStep * 2.0f, currentMinScanStep * 4.0f, currentMinScanStep * 8.0f, currentMaxScanStep};
    *count = sizeof(zoomLevels) / sizeof(zoomLevels[0]);
    for (int i = 0; i < *count; i++) {
        levels[i] = zoomLevels[i];
    }
}

/**
 * @brief Aktuális zoom szint index lekérése
 */
int ScanScreen::getCurrentZoomIndex() {
    float zoomLevels[10];
    int numZoomLevels;
    getZoomLevels(zoomLevels, &numZoomLevels);

    // Aktuális zoom szint megkeresése
    for (int i = 0; i < numZoomLevels; i++) {
        if (abs(scanStepFloat - zoomLevels[i]) < 0.01f) {
            return i;
        }
    }

    // Ha nem találjuk, visszaadjuk a középső szintet
    return numZoomLevels / 2;
}

/**
 * @brief Frekvencia sáv határok ellenőrzése
 */
bool ScanScreen::isFrequencyInBand(uint32_t frequency) { return (frequency >= scanStartFreq && frequency <= scanEndFreq); }
