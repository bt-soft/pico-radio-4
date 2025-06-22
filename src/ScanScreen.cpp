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

ScanScreen::ScanScreen(TFT_eSPI &tft, Si4735Manager *si4735Manager)
    : UIScreen(tft, SCREEN_NAME_SCAN, si4735Manager), currentState(ScanState::Idle), currentMode(ScanMode::Spectrum), scanStartFreq(0), scanEndFreq(0), currentScanFreq(0),
      scanStep(5), currentScanLine(0), previousScanLine(0), lastScanUpdate(0), scanSpeed(25), snrThreshold(15), scanStepFloat(5.0), minScanStep(0.5), maxScanStep(50.0),
      autoScanStep(true), currentMinScanStep(0.5), currentMaxScanStep(50.0) {
    SPECTRUM_WIDTH = UIComponent::SCREEN_W - (2 * SPECTRUM_MARGIN);
    SPECTRUM_X = SPECTRUM_MARGIN;

    if (SPECTRUM_WIDTH > 350) {
        scanSpeed = 20;
    } else if (SPECTRUM_WIDTH > 280) {
        scanSpeed = 25;
    } else {
        scanSpeed = 35;
    }

    spectrumRSSI.resize(SPECTRUM_WIDTH, 0);
    spectrumSNR.resize(SPECTRUM_WIDTH, 0);
    stationMarks.resize(SPECTRUM_WIDTH, false);
    scaleLines.resize(SPECTRUM_WIDTH, 0);

    DEBUG("ScanScreen initialized\n");
}

// ===================================================================
// UIScreen interface implementáció
// ===================================================================

void ScanScreen::activate() {
    UIScreen::activate();
    calculateInitialScanParameters();
    createScanButtons();
    resetSpectrumData();

    if (pSi4735Manager) {
        currentScanFreq = pSi4735Manager->getSi4735().getCurrentFrequency();
        // Ha a frekvencia a sávon kívül van, állítsuk a közepére
        if (currentScanFreq < scanStartFreq || currentScanFreq > scanEndFreq) {
            currentScanFreq = scanStartFreq + (scanEndFreq - scanStartFreq) / 2;
            pSi4735Manager->getSi4735().setFrequency(currentScanFreq);
        }
        currentScanLine = frequencyToPixel(currentScanFreq);
    }

    DEBUG("ScanScreen activated - Band: %s, Range: %d-%d kHz\n", getBandName().c_str(), scanStartFreq, scanEndFreq);
}

void ScanScreen::deactivate() {
    if (currentState != ScanState::Idle) {
        stopScan();
    }
    UIScreen::deactivate();
    DEBUG("ScanScreen deactivated\n");
}

void ScanScreen::drawContent() {
    tft.fillScreen(TFT_BLACK);
    tft.setFreeFont();

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("SPECTRUM ANALYZER", tft.width() / 2, 10);

    drawSpectrumBackground();
    drawFrequencyScale();
    drawFrequencyLabels();
    drawSpectrumDisplay();
    drawScanStatus();
    drawSignalInfo();
    drawCursor();
}

void ScanScreen::handleOwnLoop() {
    uint32_t currentTime = millis();
    if (currentState == ScanState::Scanning && (currentTime - lastScanUpdate >= scanSpeed)) {
        updateSpectrum();
        lastScanUpdate = currentTime;
    }
}

bool ScanScreen::handleTouch(const TouchEvent &event) {
    if (event.pressed) {
        if (event.x >= SPECTRUM_X && event.x < SPECTRUM_X + SPECTRUM_WIDTH && event.y >= SPECTRUM_Y && event.y < SPECTRUM_Y + SPECTRUM_HEIGHT) {
            if (currentState != ScanState::Scanning) {
                handleSpectrumTouch(event.x, event.y);
                return true;
            }
            return true;
        }
    }
    return UIScreen::handleTouch(event);
}

bool ScanScreen::handleRotary(const RotaryEvent &event) {
    if (currentState == ScanState::Idle) {
        if (event.direction == RotaryEvent::Direction::Up || event.direction == RotaryEvent::Direction::Down) {
            clearCursor();

            uint32_t bandStep = pSi4735Manager ? pSi4735Manager->getCurrentBand().currStep : 1;
            float rotaryStepSize = std::max(1.0f, (float)bandStep / scanStepFloat);
            int16_t direction = (event.direction == RotaryEvent::Direction::Up) ? 1 : -1;

            int16_t newLine = currentScanLine + direction;

            // Pásztázás (Panning) logika
            if (newLine < 0) { // Balra pásztázás
                uint32_t freqShift = (uint32_t)(rotaryStepSize * scanStepFloat);
                if (scanStartFreq > getBandMinFreq()) {
                    scanStartFreq = std::max(getBandMinFreq(), scanStartFreq - freqShift);
                    scanEndFreq = scanStartFreq + (uint32_t)(SPECTRUM_WIDTH * scanStepFloat);
                    if (scanEndFreq > getBandMaxFreq()) { // Korrekció, ha túllépjük a sáv végét
                        scanEndFreq = getBandMaxFreq();
                        scanStartFreq = scanEndFreq - (uint32_t)(SPECTRUM_WIDTH * scanStepFloat);
                    }
                    currentScanLine = 0; // Kurzor a bal szélre
                    currentScanFreq = pixelToFrequency(currentScanLine);
                    pSi4735Manager->getSi4735().setFrequency(currentScanFreq);
                    resetSpectrumData();
                    markForRedraw();
                }
                return true;
            }

            if (newLine >= SPECTRUM_WIDTH) { // Jobbra pásztázás
                uint32_t freqShift = (uint32_t)(rotaryStepSize * scanStepFloat);
                if (scanEndFreq < getBandMaxFreq()) {
                    scanEndFreq = std::min(getBandMaxFreq(), scanEndFreq + freqShift);
                    scanStartFreq = scanEndFreq - (uint32_t)(SPECTRUM_WIDTH * scanStepFloat);
                    if (scanStartFreq < getBandMinFreq()) { // Korrekció, ha alulról kezdjük
                        scanStartFreq = getBandMinFreq();
                        scanEndFreq = scanStartFreq + (uint32_t)(SPECTRUM_WIDTH * scanStepFloat);
                    }
                    currentScanLine = SPECTRUM_WIDTH - 1; // Kurzor a jobb szélre
                    currentScanFreq = pixelToFrequency(currentScanLine);
                    pSi4735Manager->getSi4735().setFrequency(currentScanFreq);
                    resetSpectrumData();
                    markForRedraw();
                }
                return true;
            }

            // Normál kurzor mozgatás
            currentScanLine = newLine;
            currentScanFreq = pixelToFrequency(currentScanLine);
            pSi4735Manager->getSi4735().setFrequency(currentScanFreq);
            drawCursor();
            drawSignalInfo();
            return true;
        }
    }
    return false;
}

// ===================================================================
// Scan kontroll metódusok
// ===================================================================

void ScanScreen::startSpectruScan() {
    if (currentState == ScanState::Idle) {
        currentState = ScanState::Scanning;
        currentScanLine = 0;
        currentScanFreq = scanStartFreq;
        lastScanUpdate = millis();

        if (pSi4735Manager) {
            pSi4735Manager->getSi4735().setFrequency(currentScanFreq);
        }

        updateScanButtonStates();
        markForRedraw(true); // Kérjük a teljes képernyő és gyermekeinek újrarajzolását
        DEBUG("Scan started: %u-%u kHz\n", scanStartFreq, scanEndFreq); // Changed %d to %u
    }
}

void ScanScreen::stopScan() {
    if (currentState != ScanState::Idle) {
        currentState = ScanState::Idle;
        if (pSi4735Manager) {
            pSi4735Manager->getSi4735().setFrequency(currentScanFreq);
        }
        updateScanButtonStates();
        markForRedraw(true); // Kérjük a teljes képernyő és a gyerekek újrarajzolását
    }
}

// ===================================================================
// Spektrum kezelés
// ===================================================================

void ScanScreen::updateSpectrum() {
    if (currentState != ScanState::Scanning)
        return;

    measureSignalAtCurrentFreq();
    drawSpectrumLine(currentScanLine);
    moveToNextFrequency();
}

void ScanScreen::measureSignalAtCurrentFreq() {
    if (!pSi4735Manager)
        return;

    uint8_t rssiSum = 0, snrSum = 0, validSamples = 0;
    for (int i = 0; i < 3; i++) {
        SignalQualityData signal = pSi4735Manager->getSignalQualityRealtime();
        if (signal.isValid) {
            rssiSum += signal.rssi;
            snrSum += signal.snr;
            validSamples++;
        }
        delay(5);
    }

    if (validSamples > 0 && currentScanLine < SPECTRUM_WIDTH) {
        uint8_t avgRSSI = rssiSum / validSamples;
        uint8_t avgSNR = snrSum / validSamples;

        if (spectrumRSSI[currentScanLine] > 0) {
            spectrumRSSI[currentScanLine] = (spectrumRSSI[currentScanLine] + avgRSSI) / 2;
            spectrumSNR[currentScanLine] = (spectrumSNR[currentScanLine] + avgSNR) / 2;
        } else {
            spectrumRSSI[currentScanLine] = avgRSSI;
            spectrumSNR[currentScanLine] = avgSNR;
        }

        stationMarks[currentScanLine] = (avgSNR >= snrThreshold);
    }
}

void ScanScreen::moveToNextFrequency() {
    currentScanLine++;
    if (currentScanLine >= SPECTRUM_WIDTH) {
        currentScanLine = 0; // Újraindítás
    }
    currentScanFreq = pixelToFrequency(currentScanLine);
    if (pSi4735Manager) {
        pSi4735Manager->getSi4735().setFrequency(currentScanFreq);
    }
}

// ===================================================================
// Grafikus megjelenítés
// ===================================================================

void ScanScreen::drawSpectrumBackground() {
    tft.drawRect(SPECTRUM_X - 1, SPECTRUM_Y - 1, SPECTRUM_WIDTH + 2, SPECTRUM_HEIGHT + 2, TFT_WHITE);
    tft.fillRect(SPECTRUM_X, SPECTRUM_Y, SPECTRUM_WIDTH, SPECTRUM_HEIGHT, TFT_BLACK);
}

void ScanScreen::drawFrequencyScale() {
    if (!pSi4735Manager)
        return;
    for (uint16_t i = 0; i < SPECTRUM_WIDTH; i++) {
        uint32_t freq = pixelToFrequency(i);
        scaleLines[i] = 0;
        if (pSi4735Manager->isCurrentBandFM()) {
            if ((freq % 500) == 0)
                scaleLines[i] = 2;
            else if ((freq % 100) == 0)
                scaleLines[i] = 1;
        } else {
            uint32_t freqRange = scanEndFreq - scanStartFreq;
            if (freqRange > 20000) {
                if ((freq % 1000) == 0)
                    scaleLines[i] = 2;
                else if ((freq % 200) == 0)
                    scaleLines[i] = 1;
            } else {
                if ((freq % 500) == 0)
                    scaleLines[i] = 2;
                else if ((freq % 100) == 0)
                    scaleLines[i] = 1;
            }
        }
    }
}

void ScanScreen::drawFrequencyLabels() {
    tft.setFreeFont();
    tft.setTextSize(1);
    uint16_t labelY = SPECTRUM_Y + SPECTRUM_HEIGHT + 5;
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(formatFrequency(scanStartFreq), SPECTRUM_X, labelY);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(formatFrequency(scanEndFreq), SPECTRUM_X + SPECTRUM_WIDTH, labelY);

    // Band info frissítése a képernyő tetején
    tft.fillRect(0, 35, tft.width(), 15, TFT_BLACK);
    String bandInfo = getBandName() + " BAND (" + formatFrequency(scanStartFreq) + " - " + formatFrequency(scanEndFreq) + ")";
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(bandInfo, tft.width() / 2, 35);
}

void ScanScreen::drawSpectrumDisplay() {
    for (uint16_t i = 0; i < SPECTRUM_WIDTH; i++) {
        drawSpectrumLine(i);
    }
}

void ScanScreen::drawSpectrumLine(uint16_t lineIndex) {
    if (lineIndex >= SPECTRUM_WIDTH)
        return;
    uint16_t x = SPECTRUM_X + lineIndex;
    uint8_t rssi = spectrumRSSI[lineIndex];
    uint8_t snr = spectrumSNR[lineIndex];
    uint16_t rssiY = rssiToPixelY(rssi);

    tft.drawLine(x, SPECTRUM_Y, x, SPECTRUM_Y + SPECTRUM_HEIGHT - 1, TFT_BLACK);

    if (scaleLines[lineIndex] == 2) {
        tft.drawLine(x, SPECTRUM_Y + SPECTRUM_HEIGHT - 10, x, SPECTRUM_Y + SPECTRUM_HEIGHT - 1, SCALE_COLOR);
        tft.drawLine(x, SPECTRUM_Y, x, SPECTRUM_Y + 5, SCALE_COLOR);
    } else if (scaleLines[lineIndex] == 1) {
        tft.drawLine(x, SPECTRUM_Y + SPECTRUM_HEIGHT - 5, x, SPECTRUM_Y + SPECTRUM_HEIGHT - 1, TFT_DARKGREY);
    }

    if (rssi > 0) {
        if (rssiY < SPECTRUM_Y + SPECTRUM_HEIGHT) {
            tft.drawLine(x, rssiY, x, SPECTRUM_Y + SPECTRUM_HEIGHT - 1, snrToColor(snr));
            tft.drawPixel(x, rssiY, RSSI_COLOR);
        }
    }

    if (stationMarks[lineIndex]) {
        tft.fillRect(x, SPECTRUM_Y + SPECTRUM_HEIGHT - 3, 1, 3, MARK_COLOR);
    }
}

void ScanScreen::drawCursor() {
    if (currentScanLine < SPECTRUM_WIDTH) {
        uint16_t x = SPECTRUM_X + currentScanLine;
        tft.drawLine(x, SPECTRUM_Y, x, SPECTRUM_Y + SPECTRUM_HEIGHT - 1, CURSOR_COLOR);
        previousScanLine = currentScanLine;
    }
}

void ScanScreen::clearCursor() {
    if (previousScanLine < SPECTRUM_WIDTH) {
        drawSpectrumLine(previousScanLine);
    }
}

void ScanScreen::drawScanStatus() {
    uint16_t statusY = SPECTRUM_Y + SPECTRUM_HEIGHT + 25;
    tft.fillRect(0, statusY, tft.width(), 15, TFT_BLACK);
    tft.setFreeFont();
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
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
    tft.drawString("Speed: " + String(scanSpeed) + "ms", 120, statusY);
    tft.drawString("Step: " + String(scanStep) + "kHz", 220, statusY);
}

void ScanScreen::drawSignalInfo() {
    uint16_t infoY = SPECTRUM_Y + SPECTRUM_HEIGHT + 40;
    tft.fillRect(0, infoY, tft.width(), 15, TFT_BLACK);
    tft.setFreeFont();
    tft.setTextSize(1);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Freq: " + formatFrequency(currentScanFreq), 10, infoY);
    if (currentScanLine < SPECTRUM_WIDTH) {
        uint8_t rssi = spectrumRSSI[currentScanLine];
        uint8_t snr = spectrumSNR[currentScanLine];
        tft.drawString("RSSI: " + String(rssi) + "dBuV", 120, infoY);
        tft.drawString("SNR: " + String(snr) + "dB", 220, infoY);
    }
}

// ===================================================================
// Érintés és gomb kezelés
// ===================================================================

void ScanScreen::handleSpectrumTouch(uint16_t x, uint16_t y) {
    if (currentState == ScanState::Scanning)
        return;
    clearCursor();
    int16_t relativeX = x - SPECTRUM_X;
    if (relativeX < 0)
        relativeX = 0;
    if (relativeX >= SPECTRUM_WIDTH)
        relativeX = SPECTRUM_WIDTH - 1;

    currentScanLine = relativeX;
    currentScanFreq = pixelToFrequency(currentScanLine);

    if (pSi4735Manager) {
        pSi4735Manager->getSi4735().setFrequency(currentScanFreq);
    }

    drawCursor();
    drawSignalInfo();
}

void ScanScreen::createScanButtons() {
    std::vector<UIHorizontalButtonBar::ButtonConfig> mainButtonConfigs = {
        {ScanButtonIDs::START_STOP_BUTTON, "Scan", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleStartStopButton(event); }},
        {ScanButtonIDs::MODE_BUTTON, "Mode", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleModeButton(event); }},
        {ScanButtonIDs::SPEED_BUTTON, "Speed", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleSpeedButton(event); }},
        {ScanButtonIDs::SCALE_BUTTON, "Zoom", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleZoomButton(event); }}};

    std::vector<UIHorizontalButtonBar::ButtonConfig> backButtonConfigs = {
        {ScanButtonIDs::BACK_BUTTON, "Back", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleBackButton(event); }}};

    uint16_t buttonHeight = 30;
    uint16_t buttonY = tft.height() - buttonHeight - 5;
    uint16_t buttonGap = 3;
    uint16_t margin = 10;

    uint16_t mainButtonCount = mainButtonConfigs.size();
    uint16_t mainButtonWidth = 70;
    uint16_t mainTotalWidth = mainButtonCount * mainButtonWidth + (mainButtonCount - 1) * buttonGap;
    uint16_t mainButtonX = margin;

    uint16_t backButtonWidth = 60;
    uint16_t backButtonX = tft.width() - margin - backButtonWidth;

    scanButtonBar =
        std::make_shared<UIHorizontalButtonBar>(tft, Rect(mainButtonX, buttonY, mainTotalWidth, buttonHeight), mainButtonConfigs, mainButtonWidth, buttonHeight, buttonGap);
    addChild(scanButtonBar);

    backButtonBar = std::make_shared<UIHorizontalButtonBar>(tft, Rect(backButtonX, buttonY, backButtonWidth, buttonHeight), backButtonConfigs, backButtonWidth, buttonHeight, 0);
    addChild(backButtonBar);
}

void ScanScreen::handleStartStopButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::On) {
        startSpectruScan();
    } else if (event.state == UIButton::EventButtonState::Off) {
        stopScan();
    }
}

void ScanScreen::handleModeButton(const UIButton::ButtonEvent &event) {
    // TODO
}

void ScanScreen::handleSpeedButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        if (SPECTRUM_WIDTH > 350) {
            if (scanSpeed <= 10)
                scanSpeed = 20;
            else if (scanSpeed <= 20)
                scanSpeed = 35;
            else if (scanSpeed <= 35)
                scanSpeed = 50;
            else
                scanSpeed = 10;
        } else {
            if (scanSpeed <= 25)
                scanSpeed = 50;
            else if (scanSpeed <= 50)
                scanSpeed = 100;
            else if (scanSpeed <= 100)
                scanSpeed = 200;
            else
                scanSpeed = 25;
        }
        drawScanStatus();
    }
}

void ScanScreen::handleZoomButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        if (currentState == ScanState::Scanning)
            return;

        uint32_t cursorFreq = currentScanFreq;

        float zoomLevels[10];
        int numZoomLevels;
        getZoomLevels(zoomLevels, &numZoomLevels);
        scanStepFloat = zoomLevels[(getCurrentZoomIndex() + 1) % numZoomLevels];
        scanStep = (uint16_t)scanStepFloat;

        uint32_t bandMin = getBandMinFreq();
        uint32_t bandMax = getBandMaxFreq();
        uint32_t newRangeSize = (uint32_t)(SPECTRUM_WIDTH * scanStepFloat);
        uint32_t halfNewRange = newRangeSize / 2;

        // Számítsuk ki a kívánt kezdő és végfrekvenciákat előjeles egészekkel, hogy elkerüljük az alulcsordulást
        int32_t desiredStartFreq = (int32_t)cursorFreq - (int32_t)halfNewRange;
        int32_t desiredEndFreq = (int32_t)cursorFreq + (int32_t)halfNewRange;

        // Clamp to band boundaries
        // Ensure start is not below bandMin
        if (desiredStartFreq < (int32_t)bandMin) {
            desiredStartFreq = bandMin;
            desiredEndFreq = desiredStartFreq + newRangeSize; // Adjust end to maintain range size
        }
        // Ensure end is not above bandMax
        if (desiredEndFreq > (int32_t)bandMax) {
            desiredEndFreq = bandMax;
            desiredStartFreq = desiredEndFreq - newRangeSize; // Adjust start to maintain range size
        }

        // Final check to ensure it's within bounds after adjustments
        // This handles cases where maintaining range size might push it out again
        if (desiredStartFreq < (int32_t)bandMin) desiredStartFreq = bandMin;
        if (desiredEndFreq > (int32_t)bandMax) desiredEndFreq = bandMax;

        // Assign to uint32_t. Since we clamped, these should now be non-negative.
        scanStartFreq = (uint32_t)desiredStartFreq;
        scanEndFreq = (uint32_t)desiredEndFreq;

        currentScanFreq = cursorFreq;
        currentScanLine = frequencyToPixel(cursorFreq);

        DEBUG("ZOOM: New range %u-%u kHz, cursor at %u kHz\n", scanStartFreq, scanEndFreq, cursorFreq); // Changed %d to %u

        resetSpectrumData();
        startSpectruScan(); // Ez már beállítja a redraw-ot
    }
}

void ScanScreen::handleBackButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        const char *targetScreen = pSi4735Manager->isCurrentBandFM() ? SCREEN_NAME_FM : SCREEN_NAME_AM;
        getScreenManager()->switchToScreen(targetScreen);
    }
}

// ===================================================================
// Utility metódusok
// ===================================================================

void ScanScreen::calculateInitialScanParameters() {
    scanStartFreq = getBandMinFreq();
    scanEndFreq = getBandMaxFreq();

    float zoomLevels[10];
    int numZoomLevels;
    getZoomLevels(zoomLevels, &numZoomLevels);
    int initialZoomIndex = numZoomLevels / 2;
    scanStepFloat = zoomLevels[initialZoomIndex];
    scanStep = (uint16_t)scanStepFloat;

    uint32_t bandCenter = getBandMinFreq() + (getBandMaxFreq() - getBandMinFreq()) / 2;
    uint32_t initialRange = (uint32_t)(SPECTRUM_WIDTH * scanStepFloat);
    scanStartFreq = bandCenter - initialRange / 2;
    scanEndFreq = bandCenter + initialRange / 2;

    if (scanStartFreq < getBandMinFreq())
        scanStartFreq = getBandMinFreq();
    if (scanEndFreq > getBandMaxFreq())
        scanEndFreq = getBandMaxFreq();
}

void ScanScreen::resetSpectrumData() {
    std::fill(spectrumRSSI.begin(), spectrumRSSI.end(), 0);
    std::fill(spectrumSNR.begin(), spectrumSNR.end(), 0);
    std::fill(stationMarks.begin(), stationMarks.end(), false);
}

uint16_t ScanScreen::rssiToPixelY(uint8_t rssi) {
    uint16_t scaledRssi = (rssi * SPECTRUM_HEIGHT) / 127;
    return SPECTRUM_Y + SPECTRUM_HEIGHT - scaledRssi;
}

uint16_t ScanScreen::snrToColor(uint8_t snr) {
    if (snr < 8)
        return TFT_NAVY;
    if (snr < 16)
        return TFT_BLUE;
    if (snr < 24)
        return TFT_GREEN;
    return TFT_YELLOW;
}

uint32_t ScanScreen::pixelToFrequency(uint16_t pixelX) {
    if (pixelX >= SPECTRUM_WIDTH)
        pixelX = SPECTRUM_WIDTH - 1;
    uint32_t freqRange = scanEndFreq - scanStartFreq;
    if (freqRange == 0)
        return scanStartFreq;
    return scanStartFreq + (uint32_t)((uint64_t)pixelX * freqRange / (SPECTRUM_WIDTH - 1));
}

uint16_t ScanScreen::frequencyToPixel(uint32_t frequency) {
    if (frequency <= scanStartFreq)
        return 0;
    if (frequency >= scanEndFreq)
        return SPECTRUM_WIDTH - 1;
    uint32_t freqRange = scanEndFreq - scanStartFreq;
    if (freqRange == 0)
        return SPECTRUM_WIDTH / 2;
    return (uint16_t)((uint64_t)(frequency - scanStartFreq) * (SPECTRUM_WIDTH - 1) / freqRange);
}

String ScanScreen::formatFrequency(uint32_t frequency) {
    if (pSi4735Manager && pSi4735Manager->isCurrentBandFM()) {
        return String(frequency / 100.0, 2) + " MHz";
    } else {
        if (frequency >= 1000) {
            return String(frequency / 1000.0, 3) + " MHz";
        } else {
            return String(frequency) + " kHz";
        }
    }
}

void ScanScreen::updateScanButtonStates() {
    if (!scanButtonBar)
        return;
    auto startButton = scanButtonBar->getButton(ScanButtonIDs::START_STOP_BUTTON);
    if (startButton) {
        startButton->setButtonState(currentState == ScanState::Scanning ? UIButton::ButtonState::On : UIButton::ButtonState::Off);
    }
}

uint32_t ScanScreen::getBandMinFreq() { return pSi4735Manager ? pSi4735Manager->getCurrentBand().minimumFreq : 87500; }

uint32_t ScanScreen::getBandMaxFreq() { return pSi4735Manager ? pSi4735Manager->getCurrentBand().maximumFreq : 108000; }

String ScanScreen::getBandName() { return pSi4735Manager ? String(pSi4735Manager->getCurrentBandName()) : "N/A"; }

void ScanScreen::getZoomLevels(float *levels, int *count) {
    if (pSi4735Manager && pSi4735Manager->isCurrentBandFM()) {
        levels[0] = 5.0f;
        levels[1] = 10.0f;
        levels[2] = 20.0f;
        levels[3] = 50.0f;
        levels[4] = 100.0f;
        *count = 5;
    } else {
        levels[0] = 0.5f;
        levels[1] = 1.0f;
        levels[2] = 2.0f;
        levels[3] = 5.0f;
        levels[4] = 10.0f;
        levels[5] = 20.0f;
        levels[6] = 50.0f;
        *count = 7;
    }
}

int ScanScreen::getCurrentZoomIndex() {
    float zoomLevels[10];
    int numZoomLevels;
    getZoomLevels(zoomLevels, &numZoomLevels);
    for (int i = 0; i < numZoomLevels; i++) {
        if (abs(scanStepFloat - zoomLevels[i]) < 0.01f) {
            return i;
        }
    }
    return numZoomLevels / 2;
}
