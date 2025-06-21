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
      currentScanLine(0), previousScanLine(0), deltaLine(0), lastScanUpdate(0), scanSpeed(50), // Default 50ms/lépés
      snrThreshold(15) {

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
        updateSpectrum();
        lastScanUpdate = currentTime;
    }

    // Gomb állapotok frissítése
    updateScanButtonStates();
}

/**
 * @brief Érintés esemény kezelése
 */
bool ScanScreen::handleTouch(const TouchEvent &event) {
    if (event.pressed) {
        // Spektrum terület érintése
        if (event.x >= SPECTRUM_X && event.x < SPECTRUM_X + SPECTRUM_WIDTH && event.y >= SPECTRUM_Y && event.y < SPECTRUM_Y + SPECTRUM_HEIGHT) {
            handleSpectrumTouch(event.x, event.y);
            return true;
        }
    }

    // Alapértelmezett érintés kezelés (gombok)
    return UIScreen::handleTouch(event);
}

/**
 * @brief Rotary encoder kezelése
 */
bool ScanScreen::handleRotary(const RotaryEvent &event) {
    if (currentState == ScanState::Paused || currentState == ScanState::Idle) {
        // Kézi frekvencia léptetés scan közben
        if (event.direction == RotaryEvent::Direction::Up) {
            if (currentScanLine < SPECTRUM_WIDTH - 1) {
                clearCursor();
                currentScanLine++;
                currentScanFreq = pixelToFrequency(currentScanLine);
                if (pSi4735Manager) {
                    pSi4735Manager->getSi4735().setFrequency(currentScanFreq);
                }
                drawCursor();
                drawSignalInfo();
                DEBUG("[ROTARY] UP - Line: %d, Freq: %d kHz\n", currentScanLine, currentScanFreq);
            }
        } else if (event.direction == RotaryEvent::Direction::Down) {
            if (currentScanLine > 0) {
                clearCursor();
                currentScanLine--;
                currentScanFreq = pixelToFrequency(currentScanLine);
                if (pSi4735Manager) {
                    pSi4735Manager->getSi4735().setFrequency(currentScanFreq);
                }
                drawCursor();
                drawSignalInfo();
                DEBUG("[ROTARY] DOWN - Line: %d, Freq: %d kHz\n", currentScanLine, currentScanFreq);
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
    if (currentState == ScanState::Idle) {
        DEBUG("Starting spectrum scan\n");
        currentState = ScanState::Scanning;
        currentScanLine = 0;
        currentScanFreq = scanStartFreq;
        lastScanUpdate = millis();

        // Spektrum adatok törölése
        resetSpectrumData();
        drawSpectrumBackground();

        // Első mérés
        if (pSi4735Manager) {
            pSi4735Manager->getSi4735().setFrequency(currentScanFreq);
        }
    }
}

/**
 * @brief Scan szüneteltetése
 */
void ScanScreen::pauseScan() {
    if (currentState == ScanState::Scanning) {
        DEBUG("Pausing scan\n");
        currentState = ScanState::Paused;
    }
}

/**
 * @brief Scan folytatása
 */
void ScanScreen::resumeScan() {
    if (currentState == ScanState::Paused) {
        DEBUG("Resuming scan\n");
        currentState = ScanState::Scanning;
        lastScanUpdate = millis();
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
    }
}

// ===================================================================
// Spektrum kezelés
// ===================================================================

/**
 * @brief Spektrum frissítése
 */
void ScanScreen::updateSpectrum() {
    if (currentState != ScanState::Scanning)
        return;

    // Jelerősség mérés az aktuális frekvencián
    measureSignalAtCurrentFreq();

    // Spektrum vonal kirajzolása
    drawSpectrumLine(currentScanLine);

    // Következő frekvenciára lépés
    moveToNextFrequency();

    // Kurzor csak akkor, ha nem scan módban vagyunk, vagy ha megállt
    if (currentState != ScanState::Scanning) {
        drawCursor();
    }
}

/**
 * @brief Jelerősség mérés az aktuális frekvencián
 */
void ScanScreen::measureSignalAtCurrentFreq() {
    if (!pSi4735Manager)
        return;

    uint8_t rssiSum = 0;
    uint8_t snrSum = 0;

    // Több minta átlagolása
    for (int i = 0; i < SIGNAL_SAMPLES; i++) {
        SignalQualityData signal = pSi4735Manager->getSignalQualityRealtime();
        if (signal.isValid) {
            rssiSum += signal.rssi;
            snrSum += signal.snr;
        }
        delay(5); // Rövid várakozás minták között
    }

    // Átlagértékek számítása
    uint8_t avgRSSI = rssiSum / SIGNAL_SAMPLES;
    uint8_t avgSNR = snrSum / SIGNAL_SAMPLES;

    // Debug információ
    DEBUG("[SCAN] Freq: %d kHz, Line: %d, RSSI: %d dBuV, SNR: %d dB\n", currentScanFreq, currentScanLine, avgRSSI, avgSNR);

    // Spektrum adatok frissítése
    if (currentScanLine < SPECTRUM_WIDTH) {
        spectrumRSSI[currentScanLine] = avgRSSI;
        spectrumSNR[currentScanLine] = avgSNR;

        // Erős állomás jelölése
        if (avgSNR >= snrThreshold) {
            stationMarks[currentScanLine] = true;
            DEBUG("[SCAN] Strong station detected at %d kHz (SNR: %d dB)\n", currentScanFreq, avgSNR);
        }
    }
}

/**
 * @brief Következő frekvenciára lépés
 */
void ScanScreen::moveToNextFrequency() {
    currentScanLine++;

    // Spektrum végének ellenőrzése
    if (currentScanLine >= SPECTRUM_WIDTH) {
        // Scan újrakezdése
        currentScanLine = 0;
        currentScanFreq = scanStartFreq;
    } else {
        currentScanFreq = pixelToFrequency(currentScanLine);
    }

    // Frekvencia beállítása
    if (pSi4735Manager && isValidScanFrequency(currentScanFreq)) {
        pSi4735Manager->getSi4735().setFrequency(currentScanFreq);
    }
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
    // Skála vonalak számítása
    uint32_t freqRange = scanEndFreq - scanStartFreq;

    // Skála vonalak meghatározása
    for (uint16_t i = 0; i < SPECTRUM_WIDTH; i++) {
        uint32_t freq = pixelToFrequency(i);
        scaleLines[i] = 0;

        // Fő skála vonalak és kisebb vonalak
        if (freqRange > 20000) { // Nagy tartomány (pl. MW)
            if ((freq % 1000) == 0)
                scaleLines[i] = 2; // Nagy vonalak minden 1 MHz-nél
            else if ((freq % 100) == 0)
                scaleLines[i] = 1;     // Kis vonalak minden 100 kHz-nél
        } else if (freqRange > 5000) { // Közepes tartomány (pl. FM)
            if ((freq % 1000) == 0)
                scaleLines[i] = 2; // Nagy vonalak minden 1 MHz-nél
            else if ((freq % 200) == 0)
                scaleLines[i] = 1; // Kis vonalak minden 200 kHz-nél
        } else {                   // Kis tartomány (pl. SW)
            if ((freq % 500) == 0)
                scaleLines[i] = 2; // Nagy vonalak minden 500 kHz-nél
            else if ((freq % 100) == 0)
                scaleLines[i] = 1; // Kis vonalak minden 100 kHz-nél
        }
    }
}

/**
 * @brief Frekvencia címkék kirajzolása
 */
void ScanScreen::drawFrequencyLabels() {
    tft.setFreeFont();
    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);

    uint16_t labelY = SPECTRUM_Y + SPECTRUM_HEIGHT + 5;
    uint32_t freqRange = scanEndFreq - scanStartFreq;

    // Címkék száma (5-8 között)
    int labelCount = (freqRange > 20000) ? 5 : 8;

    for (int i = 0; i <= labelCount; i++) {
        uint16_t x = SPECTRUM_X + (i * SPECTRUM_WIDTH) / labelCount;
        uint32_t freq = scanStartFreq + (i * freqRange) / labelCount;

        String freqLabel;
        if (freq >= 1000) {
            freqLabel = String(freq / 1000.0, 1) + "M";
        } else {
            freqLabel = String(freq) + "k";
        }

        tft.drawString(freqLabel, x, labelY);
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

    // RSSI alapú Y pozíció
    uint16_t rssiY = rssiToPixelY(rssi);

    // Háttér vonal (fekete vagy skála)
    uint16_t bgColor = TFT_BLACK;
    if (scaleLines[lineIndex] == 2) {
        bgColor = SCALE_COLOR; // Fő skála vonalak
    } else if (scaleLines[lineIndex] == 1) {
        bgColor = TFT_DARKGREY; // Kisebb skála vonalak
    }

    // Teljes magasság háttér vonal
    tft.drawLine(x, SPECTRUM_Y, x, SPECTRUM_Y + SPECTRUM_HEIGHT - 1, bgColor);

    // Ha van érvényes jel (RSSI > 0), spektrum vonal rajzolása
    if (rssi > 0) {
        // Spektrum vonal színe SNR alapján
        uint16_t spectrumColor = snrToColor(snr);

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
    tft.setTextDatum(TL_DATUM);

    // Állapot
    String status;
    switch (currentState) {
        case ScanState::Idle:
            status = "IDLE";
            break;
        case ScanState::Scanning:
            status = "SCANNING";
            break;
        case ScanState::Paused:
            status = "PAUSED";
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
 * @brief Spektrum érintés kezelése
 */
void ScanScreen::handleSpectrumTouch(uint16_t x, uint16_t y) {
    if (currentState == ScanState::Scanning) {
        pauseScan();
    }

    // Új pozíció beállítása
    clearCursor();
    currentScanLine = x - SPECTRUM_X;
    if (currentScanLine >= SPECTRUM_WIDTH) {
        currentScanLine = SPECTRUM_WIDTH - 1;
    }

    currentScanFreq = pixelToFrequency(currentScanLine);

    // Rádió frekvencia beállítása
    if (pSi4735Manager) {
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
    std::vector<UIHorizontalButtonBar::ButtonConfig> buttonConfigs = {
        {ScanButtonIDs::START_STOP_BUTTON, "Start", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleStartStopButton(event); }},

        {ScanButtonIDs::PAUSE_BUTTON, "Pause", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handlePauseButton(event); }},

        {ScanButtonIDs::MODE_BUTTON, "Mode", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleModeButton(event); }},

        {ScanButtonIDs::SPEED_BUTTON, "Speed", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleSpeedButton(event); }},

        {ScanButtonIDs::BACK_BUTTON, "Back", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleBackButton(event); }}};

    // Gombsor létrehozása a képernyő alján
    uint16_t buttonY = tft.height() - 35;
    uint16_t buttonWidth = 60;
    uint16_t buttonHeight = 30;
    uint16_t buttonGap = 3;
    uint16_t totalWidth = buttonConfigs.size() * buttonWidth + (buttonConfigs.size() - 1) * buttonGap;
    uint16_t buttonX = (tft.width() - totalWidth) / 2;

    scanButtonBar = std::make_shared<UIHorizontalButtonBar>(tft, Rect(buttonX, buttonY, totalWidth, buttonHeight), buttonConfigs, buttonWidth, buttonHeight, buttonGap);

    addChild(scanButtonBar);
}

/**
 * @brief Start/Stop gomb kezelése
 */
void ScanScreen::handleStartStopButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        switch (currentState) {
            case ScanState::Idle:
                startSpectruScan();
                break;
            case ScanState::Scanning:
            case ScanState::Paused:
                stopScan();
                break;
        }
    }
}

/**
 * @brief Pause gomb kezelése
 */
void ScanScreen::handlePauseButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        switch (currentState) {
            case ScanState::Scanning:
                pauseScan();
                break;
            case ScanState::Paused:
                resumeScan();
                break;
            default:
                break;
        }
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
        // Sebesség ciklikus váltása
        if (scanSpeed == 50)
            scanSpeed = 100;
        else if (scanSpeed == 100)
            scanSpeed = 200;
        else if (scanSpeed == 200)
            scanSpeed = 25;
        else
            scanSpeed = 50;

        DEBUG("Scan speed changed to %d ms\n", scanSpeed);
        drawScanStatus();
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
    scanStartFreq = getBandMinFreq();
    scanEndFreq = getBandMaxFreq();

    // Optimális lépésköz számítása
    uint32_t range = scanEndFreq - scanStartFreq;
    scanStep = range / SPECTRUM_WIDTH;
    if (scanStep < 1)
        scanStep = 1;

    DEBUG("Scan parameters: %d-%d kHz, step %d kHz\n", scanStartFreq, scanEndFreq, scanStep);
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
 * @brief Pixel X pozíció frekvenciává konvertálása
 */
uint32_t ScanScreen::pixelToFrequency(uint16_t pixelX) {
    if (pixelX >= SPECTRUM_WIDTH)
        pixelX = SPECTRUM_WIDTH - 1;

    // Spektrum szélesség alapján lineáris interpoláció
    uint32_t freqRange = scanEndFreq - scanStartFreq;
    return scanStartFreq + (pixelX * freqRange) / SPECTRUM_WIDTH;
}

/**
 * @brief Frekvencia pixel X pozícióvá konvertálása
 */
uint16_t ScanScreen::frequencyToPixel(uint32_t frequency) {
    if (frequency < scanStartFreq)
        return 0;
    if (frequency > scanEndFreq)
        return SPECTRUM_WIDTH - 1;

    // Spektrum szélesség alapján lineáris interpoláció
    uint32_t freqRange = scanEndFreq - scanStartFreq;
    return ((frequency - scanStartFreq) * SPECTRUM_WIDTH) / freqRange;
}

/**
 * @brief Frekvencia formázása szöveggé
 */
String ScanScreen::formatFrequency(uint32_t frequency) {
    if (frequency >= 1000) {
        return String(frequency / 1000.0, 3) + " MHz";
    } else {
        return String(frequency) + " kHz";
    }
}

/**
 * @brief Gomb állapotok frissítése
 */
void ScanScreen::updateScanButtonStates() {
    if (!scanButtonBar)
        return;

    // Start/Stop gomb felirata
    String startStopLabel = (currentState == ScanState::Idle) ? "Start" : "Stop";
    // TODO: Gomb felirat frissítése implementálása

    // Pause gomb engedélyezése/tiltása
    // bool pauseEnabled = (currentState != ScanState::Idle);
    // TODO: Gomb engedélyezés implementálása
}

/**
 * @brief Band minimum frekvencia lekérése
 */
uint32_t ScanScreen::getBandMinFreq() {
    if (!pSi4735Manager)
        return 87500; // Default FM
    return pSi4735Manager->getCurrentBand().minimumFreq;
}

/**
 * @brief Band maximum frekvencia lekérése
 */
uint32_t ScanScreen::getBandMaxFreq() {
    if (!pSi4735Manager)
        return 108000; // Default FM
    return pSi4735Manager->getCurrentBand().maximumFreq;
}

/**
 * @brief Band név lekérése
 */
String ScanScreen::getBandName() {
    if (!pSi4735Manager)
        return "FM";
    return String(pSi4735Manager->getCurrentBandName());
}
