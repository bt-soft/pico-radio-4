#include "MiniAudioDisplay.h"
#include "defines.h"
#include <cmath>

MiniAudioDisplay::MiniAudioDisplay(TFT_eSPI &tft, const Rect &bounds, IAudioDataProvider *audioProvider, uint8_t &configModeRef, float maxDisplayFreqHz)
    : UIComponent(tft, bounds), currentMode(DisplayMode::SpectrumLowRes), currentTuningAidType(TuningAidType::CW_TUNING), configModeFieldRef(configModeRef),
      audioProvider(audioProvider), maxDisplayFreqHz(maxDisplayFreqHz), lastModeChangeTime(0), lastTouchTime(0), lastPeakResetTime(0), needsRedraw(true),
      modeIndicatorVisible(false), spectrumSprite(nullptr), spriteCreated(false) {

    // Buffer-ek inicializálása
    memset(peakBuffer, 0, sizeof(peakBuffer));
    memset(waterfallBuffer, 0, sizeof(waterfallBuffer));

    // Alapértelmezett mód: SpectrumLowRes
    setDisplayMode(DisplayMode::SpectrumLowRes);
}

MiniAudioDisplay::~MiniAudioDisplay() {
    // Cleanup sprites
    cleanupSpectrumSprite();
}

void MiniAudioDisplay::draw() {
    if (!audioProvider) {
        return;
    }
    uint32_t now = millis();
    static uint32_t lastDrawTime = 0;

    // Mód kijelző elrejtése timeout után
    if (modeIndicatorVisible && (now - lastModeChangeTime) > MODE_INDICATOR_TIMEOUT_MS) {
        modeIndicatorVisible = false;
        needsRedraw = true;
    }

    // Periodikus peak reset (15 másodpercenként) - csak spektrum módokban
    if ((currentMode == DisplayMode::SpectrumLowRes || currentMode == DisplayMode::SpectrumHighRes) && (now - lastPeakResetTime) > PEAK_RESET_INTERVAL_MS) {
        memset(peakBuffer, 0, sizeof(peakBuffer));
        lastPeakResetTime = now;
    }

    // Frissítési frekvencia korlátozása (max 30 FPS, kivéve vízesés mód)
    bool skipFrameLimit = (currentMode == DisplayMode::Waterfall);
    if (!skipFrameLimit && (now - lastDrawTime) < 33) { // 30 FPS limit
        return;
    } // Csak akkor rajzolunk, ha szükséges és van friss audio adat
    bool hasData = audioProvider->isDataReady();

    // Első megjelenéskor vagy mode váltáskor mindig rajzoljuk ki a hátteret
    if (needsRedraw || (!hasData && needsRedraw)) {
        clearDisplay();
        needsRedraw = false;
    }

    // Ha nincs adat, ne folytassuk a rajzolást
    if (!hasData) {
        return;
    }

    lastDrawTime = now;

    // Aktuális mód szerint rajzolás
    switch (currentMode) {
        case DisplayMode::Off:
            // Nincs mit rajzolni
            break;

        case DisplayMode::SpectrumLowRes:
            drawSpectrumLowRes();
            break;

        case DisplayMode::SpectrumHighRes:
            drawSpectrumHighRes();
            break;

        case DisplayMode::Oscilloscope:
            drawOscilloscope();
            break;

        case DisplayMode::Waterfall:
            drawWaterfall();
            break;

        case DisplayMode::Envelope:
            drawEnvelope();
            break;

        case DisplayMode::TuningAid:
            drawTuningAid();
            break;
    }

    // Mód kijelző rajzolása ha aktív (csak egyszer)
    if (modeIndicatorVisible) {
        drawModeIndicator();
    }

    // Audio adat felhasználva
    if (hasData) {
        audioProvider->markDataConsumed();
    }
}

void MiniAudioDisplay::setDisplayMode(DisplayMode mode) {
    if (currentMode != mode) {
        currentMode = mode;
        // Note: No longer syncing to config - mode is controlled by touch only

        // Peak buffer reset mód váltásnál - tiszta kezdés
        memset(peakBuffer, 0, sizeof(peakBuffer));

        // Sprite újra inicializálása az új módhoz
        initializeSpectrumSprite();

        lastModeChangeTime = millis();
        lastPeakResetTime = lastModeChangeTime; // Peak reset timer inizializálása
        modeIndicatorVisible = true;
        needsRedraw = true;
    }
}

void MiniAudioDisplay::setTuningAidType(TuningAidType type) {
    if (currentTuningAidType != type) {
        currentTuningAidType = type;
        if (currentMode == DisplayMode::TuningAid) {
            needsRedraw = true;
        }
    }
}

void MiniAudioDisplay::onClick(const TouchEvent &event) {
    // Touch debounce ellenőrzése
    uint32_t now = millis();
    if (now - lastTouchTime < TOUCH_DEBOUNCE_MS) {
        return; // Túl gyors egymás utáni érintések elkerülése
    }

    lastTouchTime = now;

    // Mód váltása
    cycleModes();
}

void MiniAudioDisplay::drawModeIndicator() {
    const char *modeText = "";

    switch (currentMode) {
        case DisplayMode::Off:
            modeText = "OFF";
            break;
        case DisplayMode::SpectrumLowRes:
            modeText = "SPECTRUM";
            break;
        case DisplayMode::SpectrumHighRes:
            modeText = "SPECTRUM HI";
            break;
        case DisplayMode::Oscilloscope:
            modeText = "SCOPE";
            break;
        case DisplayMode::Waterfall:
            modeText = "WATERFALL";
            break;
        case DisplayMode::Envelope:
            modeText = "ENVELOPE";
            break;
        case DisplayMode::TuningAid:
            if (currentTuningAidType == TuningAidType::CW_TUNING) {
                modeText = "CW TUNE";
            } else if (currentTuningAidType == TuningAidType::RTTY_TUNING) {
                modeText = "RTTY TUNE";
            } else {
                modeText = "TUNE OFF";
            }
            break;
    } // Szöveg kirajzolása a komponens aljára, középre
    tft.setTextColor(TFT_WHITE, COLOR_BACKGROUND);
    tft.setTextSize(1);
    tft.setTextDatum(TC_DATUM); // Top Center - felül középre igazítva
    int16_t centerX = bounds.x + bounds.width / 2;
    int16_t bottomY = bounds.y + bounds.height - 12; // 12 pixel a tetejétől
    tft.drawString(modeText, centerX, bottomY);
}

void MiniAudioDisplay::drawSpectrumLowRes() {
    if (!audioProvider->isDataReady()) {
        return;
    }

    // Sprite inicializálása ha szükséges
    if (!spriteCreated) {
        initializeSpectrumSprite();
    }

    const double *magnitudeData = audioProvider->getMagnitudeData();
    uint16_t fftSize = audioProvider->getFftSize();
    float binWidth = audioProvider->getBinWidthHz();

    // 24 sávos spektrum rajzolása
    int numBands = 24;
    int bandWidth = bounds.width / numBands;

    // Sprite vagy közvetlen rajzolás
    TFT_eSPI *canvas = spriteCreated ? (TFT_eSPI *)spectrumSprite : &tft;
    int offsetX = spriteCreated ? 0 : bounds.x;
    int offsetY = spriteCreated ? 0 : bounds.y; // Sprite esetén teljes háttér törlése
    if (spriteCreated) {
        spectrumSprite->fillScreen(COLOR_BACKGROUND);

        // Keret rajzolása sprite-ba
        spectrumSprite->drawRect(0, 0, bounds.width, bounds.height, COLOR_GRID);

        // Rácsvonalak
        for (int i = 1; i < 4; i++) {
            int lineY = 10 + (i * (bounds.height - 30) / 4); // -30 hogy hely legyen a címkéknek
            spectrumSprite->drawFastHLine(1, lineY, bounds.width - 2, COLOR_GRID);
        }
        for (int i = 1; i < 4; i++) {
            int lineX = (i * bounds.width / 4);
            spectrumSprite->drawFastVLine(lineX, 10, bounds.height - 30, COLOR_GRID); // -30 hogy hely legyen a címkéknek
        } // Frekvencia címkék hozzáadása a függőleges vonalak alá
        spectrumSprite->setTextColor(COLOR_GRID, COLOR_BACKGROUND);
        spectrumSprite->setTextSize(1);

        for (int i = 0; i <= 4; i++) { // 0, 1, 2, 3, 4 - összesen 5 címke
            int lineX = (i * bounds.width / 4);
            float freq = LOW_RES_MIN_FREQ_HZ + (i * (maxDisplayFreqHz - LOW_RES_MIN_FREQ_HZ) / 4);

            String freqText;
            if (freq >= 1000.0f) {
                freqText = String((int)(freq / 1000.0f)) + "k"; // kHz formátum
            } else {
                freqText = String((int)freq); // Hz formátum
            } // Címke igazítás: első balra, utolsó jobbra, középsők középre
            if (i == 0) {
                spectrumSprite->setTextDatum(TL_DATUM); // Top Left - első címke balra igazítva (bal szélnél)
            } else if (i == 4) {
                spectrumSprite->setTextDatum(TR_DATUM); // Top Right - utolsó címke jobbra igazítva (jobb szélnél)
            } else {
                spectrumSprite->setTextDatum(TC_DATUM); // Top Center - középsők középre igazítva
            }

            spectrumSprite->drawString(freqText, lineX, bounds.height - 18); // 18px a keret aljától
        }
    }

    for (int band = 0; band < numBands; band++) {
        // Frekvencia tartomány számítása ehhez a sávhoz
        float freqStart = LOW_RES_MIN_FREQ_HZ + (band * (maxDisplayFreqHz - LOW_RES_MIN_FREQ_HZ) / numBands);
        float freqEnd = LOW_RES_MIN_FREQ_HZ + ((band + 1) * (maxDisplayFreqHz - LOW_RES_MIN_FREQ_HZ) / numBands);

        // FFT bin-ek tartománya
        int binStart = (int)(freqStart / binWidth);
        int binEnd = (int)(freqEnd / binWidth);

        // Átlagos magnitude számítása a sávban
        double avgMagnitude = 0.0;
        int binCount = 0;
        for (int bin = binStart; bin <= binEnd && bin < fftSize / 2; bin++) {
            avgMagnitude += magnitudeData[bin];
            binCount++;
        }
        if (binCount > 0) {
            avgMagnitude /= binCount;
        } // Magnitude -> pixel magasság konverzió - 25x nagyobb érzékenység
        int barHeight = (int)(avgMagnitude * (bounds.height - 30) * 25.0); // 25x skálázás, -30 a címkéknek
        barHeight = constrain(barHeight, 0, bounds.height - 35);           // -35 hogy hely legyen a címkéknek        // Csúcsérték követés - lassú, de hatékony decay
        if (barHeight > peakBuffer[band]) {
            peakBuffer[band] = barHeight; // Új csúcs
        } else {
            // Lassú, gravitációs decay
            if (peakBuffer[band] > 0) {
                // Konstans lassú csökkenés, függetlenül a magasságtól
                peakBuffer[band] = max(0, peakBuffer[band] - 1);

                // Ha nincs jel hosszabb ideje, akkor gyorsabb tisztítás
                if (barHeight == 0 && peakBuffer[band] > (bounds.height - 30) / 3) { // Frissített magasság
                    // Ha nincs jel és túl magas a peak, akkor 2 pixel csökkentés
                    peakBuffer[band] = max(0, peakBuffer[band] - 2);
                }
            }
        }

        // Sáv rajzolása - sprite vagy közvetlen
        int bandX = offsetX + band * bandWidth;

        // Spektrum sáv
        if (barHeight > 0) {
            canvas->fillRect(bandX + 1, offsetY + bounds.height - 30 - barHeight, bandWidth - 1, barHeight, COLOR_SPECTRUM); // -30 a címkéknek
        } // Csúcsérték vonal - vastagabb és láthatóbb
        if (peakBuffer[band] > 0) {
            int peakY = offsetY + bounds.height - 30 - peakBuffer[band]; // -30 a címkéknek
            // Dupla vastagságú peak vonal a jobb láthatóságért
            canvas->drawFastHLine(bandX + 1, peakY, bandWidth - 1, COLOR_PEAK);
            if (peakY > offsetY + 5) {
                canvas->drawFastHLine(bandX + 1, peakY - 1, bandWidth - 1, COLOR_PEAK);
            }
        }
    } // Sprite esetén függőleges rácsvonalak újrarajzolása a spektrum sávok után
    if (spriteCreated) {
        for (int i = 1; i < 4; i++) {
            int lineX = (i * bounds.width / 4);
            spectrumSprite->drawFastVLine(lineX, 10, bounds.height - 30, COLOR_GRID); // -30 a címkéknek
        }

        // Sprite tartalom megjelenítése
        spectrumSprite->pushSprite(bounds.x, bounds.y);
    } else {
        // Függőleges rácsvonalak újrarajzolása a spektrum sávok után (közvetlen rajzolásnál)
        for (int i = 1; i < 4; i++) {
            int lineX = bounds.x + (i * bounds.width / 4);
            tft.drawFastVLine(lineX, bounds.y + 10, bounds.height - 30, COLOR_GRID); // -30 a címkéknek
        }
    }
}

void MiniAudioDisplay::drawSpectrumHighRes() {
    if (!audioProvider->isDataReady())
        return;

    // Sprite inicializálása ha szükséges
    if (!spriteCreated) {
        initializeSpectrumSprite();
    }

    const double *magnitudeData = audioProvider->getMagnitudeData();
    uint16_t fftSize = audioProvider->getFftSize();
    float binWidth = audioProvider->getBinWidthHz();

    // Sprite vagy közvetlen rajzolás
    TFT_eSPI *canvas = spriteCreated ? (TFT_eSPI *)spectrumSprite : &tft;
    int offsetX = spriteCreated ? 0 : bounds.x;
    int offsetY = spriteCreated ? 0 : bounds.y; // Sprite esetén teljes háttér törlése
    if (spriteCreated) {
        spectrumSprite->fillScreen(COLOR_BACKGROUND);

        // Keret rajzolása sprite-ba
        spectrumSprite->drawRect(0, 0, bounds.width, bounds.height, COLOR_GRID);

        // Rácsvonalak
        for (int i = 1; i < 4; i++) {
            int lineY = 10 + (i * (bounds.height - 30) / 4); // -30 hogy hely legyen a címkéknek
            spectrumSprite->drawFastHLine(1, lineY, bounds.width - 2, COLOR_GRID);
        }
        for (int i = 1; i < 4; i++) {
            int lineX = (i * bounds.width / 4);
            spectrumSprite->drawFastVLine(lineX, 10, bounds.height - 30, COLOR_GRID); // -30 hogy hely legyen a címkéknek
        } // Frekvencia címkék hozzáadása a függőleges vonalak alá
        spectrumSprite->setTextColor(COLOR_GRID, COLOR_BACKGROUND);
        spectrumSprite->setTextSize(1);

        for (int i = 0; i <= 4; i++) { // 0, 1, 2, 3, 4 - összesen 5 címke
            int lineX = (i * bounds.width / 4);
            float freq = LOW_RES_MIN_FREQ_HZ + (i * (maxDisplayFreqHz - LOW_RES_MIN_FREQ_HZ) / 4);

            String freqText;
            if (freq >= 1000.0f) {
                freqText = String((int)(freq / 1000.0f)) + "k"; // kHz formátum
            } else {
                freqText = String((int)freq); // Hz formátum
            } // Címke igazítás: első balra, utolsó jobbra, középsők középre
            if (i == 0) {
                spectrumSprite->setTextDatum(TL_DATUM); // Top Left - első címke balra igazítva (bal szélnél)
            } else if (i == 4) {
                spectrumSprite->setTextDatum(TR_DATUM); // Top Right - utolsó címke jobbra igazítva (jobb szélnél)
            } else {
                spectrumSprite->setTextDatum(TC_DATUM); // Top Center - középsők középre igazítva
            }

            spectrumSprite->drawString(freqText, lineX, bounds.height - 18); // 18px a keret aljától
        }
    }

    // Magas felbontású spektrum - minden pixel egy tartományt reprezentál
    for (int px = 0; px < bounds.width; px++) {
        // Frekvencia számítása ehhez a pixelhez
        float freq = LOW_RES_MIN_FREQ_HZ + (px * (maxDisplayFreqHz - LOW_RES_MIN_FREQ_HZ) / bounds.width);
        int bin = (int)(freq / binWidth);
        if (bin < fftSize / 2) {
            // Magnitude -> pixel magasság konverzió - 40x nagyobb érzékenység
            int lineHeight = (int)(magnitudeData[bin] * (bounds.height - 30) * 40.0); // 40x skálázás, -30 a címkéknek
            lineHeight = constrain(lineHeight, 0, bounds.height - 35);                // -35 hogy hely legyen a címkéknek

            // Spektrum vonal
            if (lineHeight > 0) {
                canvas->drawFastVLine(offsetX + px + 1, offsetY + bounds.height - 30 - lineHeight, lineHeight, COLOR_SPECTRUM); // -30 a címkéknek
            }
        }
    }

    // Sprite esetén sprite tartalom megjelenítése
    if (spriteCreated) {
        spectrumSprite->pushSprite(bounds.x, bounds.y);
    }
}

void MiniAudioDisplay::drawOscilloscope() {
    if (!audioProvider->isDataReady())
        return;

    const int16_t *oscData = audioProvider->getOscilloscopeData();
    if (!oscData)
        return;

    // Háttér törlése
    tft.fillRect(bounds.x, bounds.y + 10, bounds.width, bounds.height - 20, COLOR_BACKGROUND);

    // Középvonal és rács
    int centerY = bounds.y + (bounds.height - 10) / 2;
    tft.drawFastHLine(bounds.x, centerY, bounds.width, COLOR_GRID);

    // Függőleges rács vonalak
    for (int i = 1; i < 4; i++) {
        int x = bounds.x + (i * bounds.width) / 4;
        tft.drawFastVLine(x, bounds.y + 10, bounds.height - 20, COLOR_GRID);
    } // Oszcilloszkóp rajzolása - csatlakozott vonalakkal
    int prevY = centerY;
    for (int px = 0; px < bounds.width; px++) {
        // Index számítása az oszcilloszkóp adatokban (320 sample széles buffer)
        int dataIndex = (px * 320) / bounds.width;
        if (dataIndex >= 320)
            dataIndex = 319;

        // Sample -> Y koordináta konverzió (16-bit signed értékhez) - 5x érzékenyebb
        int amplitude = (bounds.height - 30) / 2;                             // Amplitúdó terület
        int sampleY = centerY - (oscData[dataIndex] * amplitude * 5) / 32768; // 5x nagyobb érzékenység
        sampleY = constrain(sampleY, bounds.y + 10, bounds.y + bounds.height - 15);

        // Első pont vagy vonal rajzolása
        if (px == 0) {
            prevY = sampleY;
        } else {
            tft.drawLine(bounds.x + px - 1, prevY, bounds.x + px, sampleY, COLOR_SPECTRUM);
        }
        prevY = sampleY;
    }
}

void MiniAudioDisplay::drawWaterfall() {
    if (!audioProvider->isDataReady())
        return;

    const double *magnitudeData = audioProvider->getMagnitudeData();
    uint16_t fftSize = audioProvider->getFftSize();
    float binWidth = audioProvider->getBinWidthHz();

    static uint32_t lastWaterfallUpdate = 0;
    uint32_t now = millis();

    // Frissítési frekvencia korlátozása (max 20 FPS)
    if (now - lastWaterfallUpdate < 50) {
        return;
    }
    lastWaterfallUpdate = now;

    // Vízesés buffer görgetése - optimalizált memória mozgatás
    int rowBytes = constrain(bounds.width, 1, 128);
    int totalRows = constrain(bounds.height - 20, 1, 80);

    // Egy lépésben mozgatjuk az egész buffert
    memmove(&waterfallBuffer[1][0], &waterfallBuffer[0][0], (totalRows - 1) * rowBytes);

    // Új sor hozzáadása a tetejére
    for (int px = 0; px < rowBytes; px++) {
        float freq = LOW_RES_MIN_FREQ_HZ + (px * (maxDisplayFreqHz - LOW_RES_MIN_FREQ_HZ) / bounds.width);
        int bin = (int)(freq / binWidth);

        if (bin < fftSize / 2) {
            // Magnitude -> szín intenzitás (csökkentett erősítés a simább megjelenésért)
            uint8_t intensity = (uint8_t)constrain(magnitudeData[bin] * 255 * 30, 0, 255);
            waterfallBuffer[0][px] = intensity;
        } else {
            waterfallBuffer[0][px] = 0;
        }
    }

    // Optimalizált vízesés kirajzolás - blokkokban
    for (int row = 0; row < totalRows; row += 4) { // 4 sor egyszerre
        int endRow = min(row + 4, totalRows);
        for (int col = 0; col < rowBytes; col += 8) { // 8 pixel egyszerre
            int endCol = min(col + 8, rowBytes);

            // Kisebb blokk rajzolása
            for (int r = row; r < endRow; r++) {
                for (int c = col; c < endCol; c++) {
                    uint16_t color = getWaterfallColor(waterfallBuffer[r][c] / 255.0f);
                    tft.drawPixel(bounds.x + c, bounds.y + 10 + r, color);
                }
            }

            // Kis szünet a UI válaszképesség megőrzéséhez
            if ((col & 0x1F) == 0) { // Minden 32. pixel után
                delayMicroseconds(10);
            }
        }
    }
}

void MiniAudioDisplay::drawEnvelope() {
    if (!audioProvider->isDataReady())
        return;

    const uint8_t *envData = audioProvider->getEnvelopeData();
    if (!envData)
        return;

    // Háttér törlése
    tft.fillRect(bounds.x, bounds.y + 10, bounds.width, bounds.height - 20, COLOR_BACKGROUND);

    // Rács vonalak - horizontális
    int quarterY1 = bounds.y + (bounds.height - 10) / 4;
    int halfY = bounds.y + (bounds.height - 10) / 2;
    int quarterY3 = bounds.y + 3 * (bounds.height - 10) / 4;

    tft.drawFastHLine(bounds.x, quarterY1, bounds.width, COLOR_GRID);
    tft.drawFastHLine(bounds.x, halfY, bounds.width, COLOR_GRID);
    tft.drawFastHLine(bounds.x, quarterY3, bounds.width, COLOR_GRID);

    // Burkológörbe rajzolása - kitöltött terület
    int bottomY = bounds.y + bounds.height - 15;

    for (int px = 0; px < bounds.width; px++) {
        // Index számítása az envelope adatokban (320 sample széles buffer)
        int dataIndex = (px * 320) / bounds.width;
        if (dataIndex >= 320)
            dataIndex = 319;                                               // Envelope érték -> Y koordináta konverzió (8-bit envelope) - 3x érzékenyebb
        int amplitude = bounds.height - 25;                                // Rajzolási terület magassága
        int envY = bottomY - ((envData[dataIndex] * amplitude * 3) / 255); // 3x nagyobb érzékenység
        envY = constrain(envY, bounds.y + 10, bottomY);

        // Függőleges vonal rajzolása az aljától az envelope szintig
        if (envY < bottomY) {
            tft.drawFastVLine(bounds.x + px, envY, bottomY - envY, COLOR_SPECTRUM);
        }

        // Felső vonal (envelope görbe) - vastagabb
        tft.drawPixel(bounds.x + px, envY, COLOR_PEAK);
        if (envY > bounds.y + 10) {
            tft.drawPixel(bounds.x + px, envY - 1, COLOR_PEAK); // Dupla pixel vastagság
        }
    }
}

void MiniAudioDisplay::drawTuningAid() {
    if (!audioProvider->isDataReady())
        return;

    const double *magnitudeData = audioProvider->getMagnitudeData();
    uint16_t fftSize = audioProvider->getFftSize();
    float binWidth = audioProvider->getBinWidthHz();

    // Háttér törlése
    tft.fillRect(bounds.x, bounds.y + 10, bounds.width, bounds.height - 25, COLOR_BACKGROUND);

    // CW hangolási segéd
    if (currentTuningAidType == TuningAidType::CW_TUNING) {
        // CW spektrum keskeny sávban (600 Hz, 700-1000 Hz között)
        float centerFreq = 850.0f; // CW center frequency
        float spanFreq = 600.0f;   // CW span
        float minFreq = centerFreq - spanFreq / 2;
        float maxFreq = centerFreq + spanFreq / 2;

        // Vízesés-szerű megjelenítés - minden pixel egy frekvenciát reprezentál
        for (int px = 0; px < bounds.width; px++) {
            float freq = minFreq + (px * spanFreq / bounds.width);
            int bin = (int)(freq / binWidth);

            if (bin < fftSize / 2) {
                // Magnitude -> szín konverzió (vízesés stílus)
                double magnitude = magnitudeData[bin];
                uint16_t color = getWaterfallColor(magnitude * 20.0f); // 20x erősítés

                // Függőleges csík rajzolása
                tft.drawFastVLine(bounds.x + px, bounds.y + 10, bounds.height - 30, color);
            }
        }

        // Célfrekvencia vonal (középen)
        int targetX = bounds.x + (bounds.width / 2);
        tft.drawFastVLine(targetX, bounds.y + 10, bounds.height - 30, TFT_WHITE);

        // Frekvencia címke
        tft.setTextColor(TFT_WHITE, COLOR_BACKGROUND);
        tft.setTextSize(1);
        tft.setTextDatum(TC_DATUM);
        tft.drawString("CW 850Hz", bounds.x + bounds.width / 2, bounds.y + bounds.height - 12);

    } else if (currentTuningAidType == TuningAidType::RTTY_TUNING) {
        // RTTY hangolási segéd - Mark és Space frekvenciák
        float markFreq = 1100.0f;            // Mark frequency
        float spaceFreq = markFreq + 425.0f; // Space frequency (425 Hz shift)
        float centerFreq = (markFreq + spaceFreq) / 2;
        float spanFreq = 800.0f; // RTTY sáv
        float minFreq = centerFreq - spanFreq / 2;
        float maxFreq = centerFreq + spanFreq / 2;

        // Vízesés-szerű megjelenítés
        for (int px = 0; px < bounds.width; px++) {
            float freq = minFreq + (px * spanFreq / bounds.width);
            int bin = (int)(freq / binWidth);

            if (bin < fftSize / 2) {
                // Magnitude -> szín konverzió
                double magnitude = magnitudeData[bin];
                uint16_t color = getWaterfallColor(magnitude * 15.0f); // 15x erősítés

                // Függőleges csík rajzolása
                tft.drawFastVLine(bounds.x + px, bounds.y + 10, bounds.height - 30, color);
            }
        }

        // Mark és Space vonalak
        int markX = bounds.x + ((markFreq - minFreq) * bounds.width / spanFreq);
        int spaceX = bounds.x + ((spaceFreq - minFreq) * bounds.width / spanFreq);

        if (markX >= bounds.x && markX < bounds.x + bounds.width) {
            tft.drawFastVLine(markX, bounds.y + 10, bounds.height - 30, COLOR_RTTY_MARK);
        }
        if (spaceX >= bounds.x && spaceX < bounds.x + bounds.width) {
            tft.drawFastVLine(spaceX, bounds.y + 10, bounds.height - 30, COLOR_RTTY_SPACE);
        }

        // Frekvencia címkék
        tft.setTextColor(TFT_WHITE, COLOR_BACKGROUND);
        tft.setTextSize(1);
        tft.setTextDatum(TC_DATUM);
        tft.drawString("RTTY M/S", bounds.x + bounds.width / 2, bounds.y + bounds.height - 12);
    }
}

void MiniAudioDisplay::cycleModes() {
    int nextMode = static_cast<int>(currentMode) + 1;
    if (nextMode > static_cast<int>(DisplayMode::TuningAid)) {
        nextMode = static_cast<int>(DisplayMode::Off);
    }
    setDisplayMode(static_cast<DisplayMode>(nextMode));
}

uint16_t MiniAudioDisplay::frequencyToX(float freqHz) {
    if (freqHz < LOW_RES_MIN_FREQ_HZ)
        return bounds.x;
    if (freqHz > maxDisplayFreqHz)
        return bounds.x + bounds.width - 1;

    float ratio = (freqHz - LOW_RES_MIN_FREQ_HZ) / (maxDisplayFreqHz - LOW_RES_MIN_FREQ_HZ);
    return bounds.x + (uint16_t)(ratio * bounds.width);
}

float MiniAudioDisplay::xToFrequency(uint16_t x_coord) {
    if (x_coord < bounds.x)
        return LOW_RES_MIN_FREQ_HZ;
    if (x_coord >= bounds.x + bounds.width)
        return maxDisplayFreqHz;

    float ratio = (float)(x_coord - bounds.x) / bounds.width;
    return LOW_RES_MIN_FREQ_HZ + ratio * (maxDisplayFreqHz - LOW_RES_MIN_FREQ_HZ);
}

uint16_t MiniAudioDisplay::getWaterfallColor(float magnitude) {
    // Egyszerű színátmenet: fekete -> kék -> zöld -> sárga -> piros -> fehér
    magnitude = constrain(magnitude, 0.0f, 1.0f);

    if (magnitude < 0.2f) {
        // Fekete -> sötétkék
        uint8_t blue = (uint8_t)(magnitude * 5 * 31);
        return (blue & 0x1F);
    } else if (magnitude < 0.4f) {
        // Sötétkék -> zöld
        magnitude -= 0.2f;
        uint8_t green = (uint8_t)(magnitude * 5 * 63);
        return ((green & 0x3F) << 5) | 0x1F;
    } else if (magnitude < 0.6f) {
        // Zöld -> sárga
        magnitude -= 0.4f;
        uint8_t red = (uint8_t)(magnitude * 5 * 31);
        return ((red & 0x1F) << 11) | (0x3F << 5);
    } else if (magnitude < 0.8f) {
        // Sárga -> piros
        magnitude -= 0.6f;
        uint8_t green = 63 - (uint8_t)(magnitude * 5 * 63);
        return (0x1F << 11) | ((green & 0x3F) << 5);
    } else {
        // Piros -> fehér
        magnitude -= 0.8f;
        uint8_t gb = (uint8_t)(magnitude * 5 * 31);
        return (0x1F << 11) | ((gb & 0x3F) << 5) | (gb & 0x1F);
    }
}

void MiniAudioDisplay::clearDisplay() {
    // Háttér törlése
    tft.fillRect(bounds.x, bounds.y, bounds.width, bounds.height, COLOR_BACKGROUND);

    // Keret rajzolása
    tft.drawRect(bounds.x, bounds.y, bounds.width, bounds.height, COLOR_GRID); // Rácsvonalak kirajzolása (opcionális) - spektrum módokban
    if (currentMode == DisplayMode::SpectrumLowRes || currentMode == DisplayMode::SpectrumHighRes) {
        // Vízszintes vonalak
        for (int i = 1; i < 4; i++) {
            int lineY = bounds.y + 10 + (i * (bounds.height - 20) / 4);
            tft.drawFastHLine(bounds.x + 1, lineY, bounds.width - 2, COLOR_GRID);
        }

        // Függőleges vonalak - finomabb elhelyezés
        for (int i = 1; i < 4; i++) {
            int lineX = bounds.x + (i * bounds.width / 4);
            tft.drawFastVLine(lineX, bounds.y + 10, bounds.height - 20, COLOR_GRID);
        }
    } else if (currentMode != DisplayMode::Off && currentMode != DisplayMode::Waterfall) {
        // Más módokban csak vízszintes vonalak
        for (int i = 1; i < 4; i++) {
            int lineY = bounds.y + 10 + (i * (bounds.height - 20) / 4);
            tft.drawFastHLine(bounds.x + 1, lineY, bounds.width - 2, COLOR_GRID);
        }
    }
}

void MiniAudioDisplay::update() {
    // Frissítés hívása - egyszerűen meghívja a draw() metódust
    draw();
}

void MiniAudioDisplay::initializeSpectrumSprite() {
    if (spectrumSprite || spriteCreated) {
        cleanupSpectrumSprite();
    }

    // Csak spektrum módokban hozzuk létre a sprite-ot
    if (currentMode == DisplayMode::SpectrumLowRes || currentMode == DisplayMode::SpectrumHighRes) {
        spectrumSprite = new TFT_eSprite(&tft);
        if (spectrumSprite->createSprite(bounds.width, bounds.height)) {
            spriteCreated = true;
        } else {
            delete spectrumSprite;
            spectrumSprite = nullptr;
            spriteCreated = false;
        }
    }
}

void MiniAudioDisplay::cleanupSpectrumSprite() {
    if (spectrumSprite) {
        if (spriteCreated) {
            spectrumSprite->deleteSprite();
        }
        delete spectrumSprite;
        spectrumSprite = nullptr;
        spriteCreated = false;
    }
}
