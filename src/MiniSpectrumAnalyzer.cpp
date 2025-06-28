#include "MiniSpectrumAnalyzer.h"
#include "Core1Logic.h"
#include "defines.h"
#include <cmath>

MiniSpectrumAnalyzer::MiniSpectrumAnalyzer(TFT_eSPI &tft, const Rect &bounds, DisplayMode mode, const ColorScheme &colors)
    : MiniAudioDisplay(tft, bounds, colors), displayMode_(mode), bandCount_(DEFAULT_BAND_COUNT), minFrequency_(DEFAULT_MIN_FREQ), maxFrequency_(DEFAULT_MAX_FREQ_FM),
      fftData_(nullptr), bandData_(nullptr), peakHold_(nullptr), fftDataSize_(0) {

    allocateBuffers();
}

MiniSpectrumAnalyzer::~MiniSpectrumAnalyzer() { deallocateBuffers(); }

void MiniSpectrumAnalyzer::setDisplayMode(DisplayMode mode) {
    if (displayMode_ != mode) {
        displayMode_ = mode;
        markForRedraw();
    }
}

void MiniSpectrumAnalyzer::setFrequencyRange(float minFreq, float maxFreq) {
    if (minFreq != minFrequency_ || maxFreq != maxFrequency_) {
        minFrequency_ = minFreq;
        maxFrequency_ = maxFreq;
        markForRedraw();
    }
}

void MiniSpectrumAnalyzer::setBandCount(uint16_t bandCount) {
    if (bandCount != bandCount_ && bandCount > 0) {
        bandCount_ = bandCount;
        deallocateBuffers();
        allocateBuffers();
        markForRedraw();
    }
}

void MiniSpectrumAnalyzer::drawContent() {
    AudioProcessor *processor = ::getAudioProcessor();
    if (!processor) {
        return;
    }

    // FFT adatok lekérése
    if (processor->getFFTData(fftData_, fftDataSize_)) {
        updateBandData();
    }

    // Megjelenítés a kiválasztott mód szerint
    switch (displayMode_) {
        case DisplayMode::BARS:
            drawBars();
            break;
        case DisplayMode::LINE:
            drawLine();
            break;
        case DisplayMode::FILLED_LINE:
            drawFilledLine();
            break;
    }
}

void MiniSpectrumAnalyzer::allocateBuffers() {
    deallocateBuffers();

    AudioProcessor *processor = ::getAudioProcessor();
    if (processor) {
        fftDataSize_ = processor->getFftSize() / 2;
    } else {
        fftDataSize_ = AudioProcessorConstants::DEFAULT_FFT_SAMPLES / 2;
    }

    fftData_ = new float[fftDataSize_];
    bandData_ = new float[bandCount_];
    peakHold_ = new float[bandCount_];

    // Bufferek nullázása
    memset(fftData_, 0, fftDataSize_ * sizeof(float));
    memset(bandData_, 0, bandCount_ * sizeof(float));
    memset(peakHold_, 0, bandCount_ * sizeof(float));

    DEBUG("MiniSpectrumAnalyzer: Buffers allocated for %d bands, FFT size %d\n", bandCount_, fftDataSize_);
}

void MiniSpectrumAnalyzer::deallocateBuffers() {
    delete[] fftData_;
    delete[] bandData_;
    delete[] peakHold_;

    fftData_ = nullptr;
    bandData_ = nullptr;
    peakHold_ = nullptr;
}

void MiniSpectrumAnalyzer::updateBandData() {
    if (!fftData_ || !bandData_ || !peakHold_) {
        return;
    }

    AudioProcessor *processor = ::getAudioProcessor();
    if (!processor) {
        return;
    }

    float samplingFreq = AudioProcessorConstants::DEFAULT_SAMPLING_FREQUENCY;
    float freqStep = samplingFreq / (2.0f * fftDataSize_);

    // Sávok számítása
    for (uint16_t band = 0; band < bandCount_; band++) {
        // Logaritmikus frekvencia skála
        float bandMinFreq = minFrequency_ * pow(maxFrequency_ / minFrequency_, (float)band / bandCount_);
        float bandMaxFreq = minFrequency_ * pow(maxFrequency_ / minFrequency_, (float)(band + 1) / bandCount_);

        // FFT bin tartomány számítása
        uint16_t startBin = (uint16_t)(bandMinFreq / freqStep);
        uint16_t endBin = (uint16_t)(bandMaxFreq / freqStep);

        // Határok ellenőrzése
        startBin = min(startBin, fftDataSize_ - 1);
        endBin = min(endBin, fftDataSize_ - 1);

        // Sáv átlagának számítása
        float bandSum = 0.0f;
        uint16_t binCount = 0;

        for (uint16_t bin = startBin; bin <= endBin; bin++) {
            bandSum += fftData_[bin];
            binCount++;
        }

        if (binCount > 0) {
            bandData_[band] = bandSum / binCount;
        } else {
            bandData_[band] = 0.0f;
        }

        // Peak hold frissítése
        if (bandData_[band] > peakHold_[band]) {
            peakHold_[band] = bandData_[band];
        } else {
            peakHold_[band] *= PEAK_HOLD_DECAY;
        }
    }
}

void MiniSpectrumAnalyzer::drawBars() {
    if (!bandData_) {
        return;
    }

    const Rect &bounds = getBounds();
    uint16_t barWidth = bounds.width / bandCount_;
    uint16_t barSpacing = max(1, barWidth / 4);
    uint16_t actualBarWidth = barWidth - barSpacing;

    for (uint16_t band = 0; band < bandCount_; band++) {
        uint16_t barX = bounds.x + band * barWidth;
        uint16_t barHeight = amplitudeToHeight(bandData_[band]);
        uint16_t barY = bounds.y + bounds.height - barHeight;

        // Oszlop kirajzolása
        if (barHeight > 0) {
            uint16_t barColor = getBarColor(bandData_[band], 1000.0f); // Max amplitude reference
            tft.fillRect(barX, barY, actualBarWidth, barHeight, barColor);
        }

        // Peak hold kirajzolása
        uint16_t peakHeight = amplitudeToHeight(peakHold_[band]);
        if (peakHeight > 0) {
            uint16_t peakY = bounds.y + bounds.height - peakHeight;
            tft.drawFastHLine(barX, peakY, actualBarWidth, primaryColor_);
        }
    }
}

void MiniSpectrumAnalyzer::drawLine() {
    if (!bandData_) {
        return;
    }

    const Rect &bounds = getBounds();
    uint16_t stepX = bounds.width / (bandCount_ - 1);

    for (uint16_t band = 0; band < bandCount_ - 1; band++) {
        uint16_t x1 = bounds.x + band * stepX;
        uint16_t y1 = bounds.y + bounds.height - amplitudeToHeight(bandData_[band]);
        uint16_t x2 = bounds.x + (band + 1) * stepX;
        uint16_t y2 = bounds.y + bounds.height - amplitudeToHeight(bandData_[band + 1]);

        tft.drawLine(x1, y1, x2, y2, primaryColor_);
    }
}

void MiniSpectrumAnalyzer::drawFilledLine() {
    if (!bandData_) {
        return;
    }

    const Rect &bounds = getBounds();
    uint16_t stepX = bounds.width / (bandCount_ - 1);

    for (uint16_t band = 0; band < bandCount_ - 1; band++) {
        uint16_t x1 = bounds.x + band * stepX;
        uint16_t y1 = bounds.y + bounds.height - amplitudeToHeight(bandData_[band]);
        uint16_t x2 = bounds.x + (band + 1) * stepX;
        uint16_t y2 = bounds.y + bounds.height - amplitudeToHeight(bandData_[band + 1]);

        // Vonal kirajzolása
        tft.drawLine(x1, y1, x2, y2, primaryColor_);

        // Kitöltés az alapvonalig
        for (uint16_t px = x1; px <= x2; px++) {
            uint16_t py = map(px, x1, x2, y1, y2);
            tft.drawFastVLine(px, py, (bounds.y + bounds.height) - py, secondaryColor_);
        }
    }
}

void MiniSpectrumAnalyzer::drawContentToSprite(TFT_eSprite *sprite) {
    // Adatok frissítése (FFT lekérés, bandData update)
    drawContent();
    // Ezután sprite-ra rajzolás (mint eddig)
    if (!bandData_)
        return;
    const Rect &bounds = getBounds();
    uint16_t barWidth = bounds.width / bandCount_;
    uint16_t barSpacing = std::max(1, barWidth / 4);
    uint16_t actualBarWidth = barWidth - barSpacing;

    for (uint16_t band = 0; band < bandCount_; band++) {
        uint16_t barX = band * barWidth;
        uint16_t barHeight = amplitudeToHeight(bandData_[band]);
        uint16_t barY = bounds.height - barHeight;

        // Sáv törlése sprite-on (háttérrel)
        sprite->fillRect(barX, 0, actualBarWidth, bounds.height, backgroundColor_);

        // Sáv kirajzolása
        if (barHeight > 0) {
            uint16_t barColor = getBarColor(bandData_[band], 1000.0f);
            sprite->fillRect(barX, barY, actualBarWidth, barHeight, barColor);
        }

        // Peak hold kirajzolása (sárga vonal, lassú csökkenéssel)
        uint16_t peakHeight = amplitudeToHeight(peakHold_[band]);
        if (peakHeight > 0) {
            uint16_t peakY = bounds.height - peakHeight;
            sprite->drawFastHLine(barX, peakY, actualBarWidth, TFT_YELLOW);
        }
    }
}

uint16_t MiniSpectrumAnalyzer::amplitudeToHeight(float amplitude) {
    // Logaritmikus skálázás a jobb vizuális megjelenítéshez
    if (amplitude <= 0.0f) {
        return 0;
    }

    const Rect &bounds = getBounds();
    float logAmp = log10(amplitude + 1.0f) / log10(1001.0f); // 0-1000 range log scale
    return (uint16_t)(logAmp * bounds.height);
}

uint16_t MiniSpectrumAnalyzer::getBarColor(float amplitude, float maxAmplitude) {
    float ratio = amplitude / maxAmplitude;

    if (ratio < 0.3f) {
        return TFT_GREEN;
    } else if (ratio < 0.7f) {
        return TFT_YELLOW;
    } else {
        return TFT_RED;
    }
}
