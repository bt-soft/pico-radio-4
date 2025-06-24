#include "DmaAudioProcessor.h"
#include "ArmFFT.h"
#include "defines.h"
#include "pins.h"
#include <Arduino.h>
#include <cstring>
#include <hardware/clocks.h>
#include <hardware/gpio.h>

// Globális instance
DmaAudioProcessor *g_audioProcessor = nullptr;

// Static instance pointer az interrupt handler számára
static DmaAudioProcessor *s_processorInstance = nullptr;

// === Konstruktor ===
DmaAudioProcessor::DmaAudioProcessor()
    : dmaChannel(AUDIO_DMA_CHANNEL), dmaChanMask(1u << dmaChannel), buffer1Ready(false), buffer2Ready(false), currentBufferIsFirst(true), ringBufferWritePos(0),
      ringBufferReadPos(0), currentFftSize(FFT_SIZE_LOW_RES), fftInputBuffer(nullptr), fftOutputBuffer(nullptr), magnitudeBuffer(nullptr), dataReady(false),
      processingActive(false), lastProcessTime(0), processingStartTime(0), cpuUsage(0.0f), processedSampleCount(0), dmaOverrunFlag(false), fftOverrunFlag(false) {
    // Buffer-ek nullázása
    memset(dmaBuffer1, 0, sizeof(dmaBuffer1));
    memset(dmaBuffer2, 0, sizeof(dmaBuffer2));
    memset(ringBuffer, 0, sizeof(ringBuffer));
    memset(oscilloscopeBuffer, 0, sizeof(oscilloscopeBuffer));
    memset(envelopeBuffer, 0, sizeof(envelopeBuffer));
    memset(waterfallBuffer, 0, sizeof(waterfallBuffer));

    // Mutex inicializálás
    mutex_init(&dataMutex);

    // Static instance beállítása
    s_processorInstance = this;
}

// === Destruktor ===
DmaAudioProcessor::~DmaAudioProcessor() {
    cleanup();
    s_processorInstance = nullptr;
}

// === Inicializálás ===
bool DmaAudioProcessor::initialize() {
    DEBUG("DmaAudioProcessor::initialize() start\n");

    // Buffer allokálás
    if (!allocateBuffers()) {
        DEBUG("DmaAudioProcessor: Buffer allocation failed!\n");
        return false;
    }

    // ADC inicializálás
    if (!initializeAdc()) {
        DEBUG("DmaAudioProcessor: ADC initialization failed!\n");
        return false;
    }

    // DMA inicializálás
    if (!initializeDma()) {
        DEBUG("DmaAudioProcessor: DMA initialization failed!\n");
        return false;
    }

    processingActive = true;
    DEBUG("DmaAudioProcessor::initialize() completed successfully\n");
    return true;
}

// === Buffer allokálás ===
bool DmaAudioProcessor::allocateBuffers() {
    // FFT input buffer
    fftInputBuffer = new (std::nothrow) float[FFT_SIZE_MAX];
    if (!fftInputBuffer) {
        DEBUG("Failed to allocate FFT input buffer\n");
        return false;
    }

    // FFT output buffer (complex: real + imaginary)
    fftOutputBuffer = new (std::nothrow) float[FFT_SIZE_MAX * 2];
    if (!fftOutputBuffer) {
        DEBUG("Failed to allocate FFT output buffer\n");
        return false;
    }

    // Magnitude buffer
    magnitudeBuffer = new (std::nothrow) double[FFT_SIZE_MAX / 2];
    if (!magnitudeBuffer) {
        DEBUG("Failed to allocate magnitude buffer\n");
        return false;
    }

    // Bufferek nullázása
    memset(fftInputBuffer, 0, FFT_SIZE_MAX * sizeof(float));
    memset(fftOutputBuffer, 0, FFT_SIZE_MAX * 2 * sizeof(float));
    memset(magnitudeBuffer, 0, (FFT_SIZE_MAX / 2) * sizeof(double));

    DEBUG("Audio buffers allocated successfully\n");
    return true;
}

// === ADC inicializálás ===
bool DmaAudioProcessor::initializeAdc() {
    // Egyszerű analog olvasás inicializálás
    // A PIN_AUDIO_INPUT (A1/GPIO27) már be van állítva analog módra
    DEBUG("ADC initialized: Simple analogRead mode for PIN_AUDIO_INPUT (A1/GPIO27)\n");
    return true;
}

// === DMA inicializálás ===
bool DmaAudioProcessor::initializeDma() {
    // Egyszerű analog olvasás módban nem használunk DMA-t
    DEBUG("DMA disabled: Using simple analogRead mode\n");
    return true;
}

// === DMA interrupt handler (static wrapper) ===
void DmaAudioProcessor::dmaIrqHandler() {
    if (s_processorInstance) {
        s_processorInstance->handleDmaInterrupt();
    }
}

// === DMA interrupt feldolgozás ===
void DmaAudioProcessor::handleDmaInterrupt() {
    // Clear interrupt
    dma_hw->ints0 = dmaChanMask;

    if (currentBufferIsFirst) {
        // Buffer1 filled, setup buffer2
        buffer1Ready = true;
        currentBufferIsFirst = false;

        // Setup next transfer to buffer2
        dma_channel_set_write_addr(dmaChannel, dmaBuffer2, false);
        dma_channel_set_trans_count(dmaChannel, AUDIO_DMA_BUFFER_SIZE, true);
    } else {
        // Buffer2 filled, setup buffer1
        buffer2Ready = true;
        currentBufferIsFirst = true;

        // Setup next transfer to buffer1
        dma_channel_set_write_addr(dmaChannel, dmaBuffer1, false);
        dma_channel_set_trans_count(dmaChannel, AUDIO_DMA_BUFFER_SIZE, true);
    }

    // Performance stats update
    processedSampleCount += AUDIO_DMA_BUFFER_SIZE;
}

// === Core1 audio processing main loop ===
void DmaAudioProcessor::processAudioCore1() {
    DEBUG("Audio processing started on Core1 (analogRead mode)\n");

    uint32_t lastStatsUpdate = 0;
    uint32_t lastDebugUpdate = 0;
    const uint32_t STATS_UPDATE_INTERVAL = 1000;                 // 1 sec
    const uint32_t DEBUG_UPDATE_INTERVAL = 3000;                 // 3 sec debug
    uint32_t sampleIntervalMicros = 1000000 / AUDIO_SAMPLE_RATE; // mintavételezési intervallum

    while (true) {
        processingStartTime = micros(); // Debug információk
        uint32_t currentTime = millis();
        if (currentTime - lastDebugUpdate > DEBUG_UPDATE_INTERVAL) {
            DEBUG("Audio Core1: Running, dataReady=%s, ringBuffer space=%d\n", dataReady ? "YES" : "NO", ringBufferFreeSpace());
            lastDebugUpdate = currentTime;
        }

        // Egyszerű analogRead alapú mintavételezés (a korábbi projekt módszerével)
        if (ringBufferFreeSpace() >= currentFftSize) {
            int16_t samples[FFT_SIZE_MAX];

            // Mintavételezés
            for (uint16_t i = 0; i < currentFftSize; i++) {
                uint32_t loopStartMicros = micros();

                // 4 minta átlagolása a zajcsökkentés érdekében (mint a korábbi projektben)
                uint32_t sum = 0;
                for (int j = 0; j < 4; j++) {
                    sum += analogRead(PIN_AUDIO_INPUT);
                }
                double averaged_sample = sum / 4.0;

                // Középre igazítás (2048 a nulla szint 12 bites ADC-nél)
                samples[i] = (int16_t)(averaged_sample - 2048.0);

                // Időzítés a cél mintavételezési frekvencia eléréséhez
                uint32_t processingTimeMicros = micros() - loopStartMicros;
                if (processingTimeMicros < sampleIntervalMicros) {
                    delayMicroseconds(sampleIntervalMicros - processingTimeMicros);
                }
            }

            // Minták ring buffer-be írása
            ringBufferWrite(samples, currentFftSize);
            processedSampleCount += currentFftSize;
        }

        // FFT feldolgozás ha van elég adat
        if (ringBufferAvailable() >= currentFftSize) {
            int16_t fftSamples[FFT_SIZE_MAX];
            uint16_t samplesRead = ringBufferRead(fftSamples, currentFftSize);

            if (samplesRead == currentFftSize) {
                // Convert to float
                for (uint16_t i = 0; i < currentFftSize; i++) {
                    fftInputBuffer[i] = (float)fftSamples[i] / 2048.0f * AUDIO_INPUT_GAIN;
                }

                // FFT számítás
                computeFFT(fftInputBuffer, fftOutputBuffer, currentFftSize);

                // Magnitude számítás
                computeMagnitude();

                // További adatok generálása
                generateOscilloscopeData();
                generateEnvelopeData();
                updateWaterfallData();

                // Thread-safe adatcsere
                if (mutex_try_enter(&dataMutex, nullptr)) {
                    dataReady = true;
                    mutex_exit(&dataMutex);
                }
            }
        }

        updatePerformanceStats();

        // Performance stats kiírás
        uint32_t now = millis();
        if (now - lastStatsUpdate > STATS_UPDATE_INTERVAL) {
            // Számoljuk az átlagos magnitude-ot az audio aktivitás mérésére
            double avgMagnitude = 0.0;
            for (uint16_t i = 1; i < currentFftSize / 2 && i < 10; i++) { // Skip DC, csak első 10 bin
                avgMagnitude += magnitudeBuffer[i];
            }
            avgMagnitude /= 9.0; // 9 bin átlaga

            DEBUG("Audio Core1: CPU=%.1f%%, Samples=%lu, Ring=%d/%d, AvgMag=%.3f, FFTReady=%s\n", cpuUsage, processedSampleCount, ringBufferAvailable(), AUDIO_RING_BUFFER_SIZE,
                  avgMagnitude, dataReady ? "YES" : "NO");
            lastStatsUpdate = now;
        }

        // Kis szünet ha nincs mit csinálni
        sleep_us(100);
    }
}

// === Audio buffer feldolgozása - DEPRECATED (most analogRead-et használunk) ===
// Ezt a függvényt nem használjuk már, de megtartjuk kompatibilitás miatt
void DmaAudioProcessor::processAudioBuffer(const int16_t *buffer, uint16_t size) {
    // Most már nem használjuk, az analogRead közvetlenül a ring buffer-be ír
}

// === Ring buffer írás ===
void DmaAudioProcessor::ringBufferWrite(const int16_t *data, uint16_t size) {
    for (uint16_t i = 0; i < size; i++) {
        ringBuffer[ringBufferWritePos] = data[i];
        ringBufferWritePos = (ringBufferWritePos + 1) % AUDIO_RING_BUFFER_SIZE;

        // Overrun check
        if (ringBufferWritePos == ringBufferReadPos) {
            dmaOverrunFlag = true;
            // Move read pointer to prevent deadlock
            ringBufferReadPos = (ringBufferReadPos + 1) % AUDIO_RING_BUFFER_SIZE;
        }
    }
}

// === Ring buffer olvasás ===
uint16_t DmaAudioProcessor::ringBufferRead(int16_t *data, uint16_t size) {
    uint16_t available = ringBufferAvailable();
    uint16_t toRead = (size < available) ? size : available;

    for (uint16_t i = 0; i < toRead; i++) {
        data[i] = ringBuffer[ringBufferReadPos];
        ringBufferReadPos = (ringBufferReadPos + 1) % AUDIO_RING_BUFFER_SIZE;
    }

    return toRead;
}

// === Ring buffer elérhető adatok ===
uint16_t DmaAudioProcessor::ringBufferAvailable() const {
    if (ringBufferWritePos >= ringBufferReadPos) {
        return ringBufferWritePos - ringBufferReadPos;
    } else {
        return AUDIO_RING_BUFFER_SIZE - ringBufferReadPos + ringBufferWritePos;
    }
}

// === Ring buffer szabad hely ===
uint16_t DmaAudioProcessor::ringBufferFreeSpace() const {
    return AUDIO_RING_BUFFER_SIZE - ringBufferAvailable() - 1; // -1 to avoid full=empty ambiguity
}

// === Cleanup ===
void DmaAudioProcessor::cleanup() {
    if (processingActive) {
        processingActive = false;
        // Egyszerű analogRead módban nincs mit leállítani
    }

    // Buffer felszabadítás
    delete[] fftInputBuffer;
    delete[] fftOutputBuffer;
    delete[] magnitudeBuffer;

    fftInputBuffer = nullptr;
    fftOutputBuffer = nullptr;
    magnitudeBuffer = nullptr;
}

// === IAudioDataProvider interface implementation ===

const double *DmaAudioProcessor::getMagnitudeData() const { return magnitudeBuffer; }

const int16_t *DmaAudioProcessor::getOscilloscopeData() const { return oscilloscopeBuffer; }

const uint8_t *DmaAudioProcessor::getEnvelopeData() const { return envelopeBuffer; }

const uint8_t *DmaAudioProcessor::getWaterfallData() const { return waterfallBuffer; }

float DmaAudioProcessor::getBinWidthHz() const { return (float)AUDIO_SAMPLE_RATE / (float)currentFftSize; }

uint16_t DmaAudioProcessor::getFftSize() const { return currentFftSize; }

uint32_t DmaAudioProcessor::getSampleRate() const { return AUDIO_SAMPLE_RATE; }

bool DmaAudioProcessor::isDataReady() const {
    bool ready = false;
    if (mutex_try_enter(&dataMutex, nullptr)) {
        ready = dataReady;
        mutex_exit(&dataMutex);
    }
    return ready;
}

void DmaAudioProcessor::markDataConsumed() {
    if (mutex_try_enter(&dataMutex, nullptr)) {
        dataReady = false;
        mutex_exit(&dataMutex);
    }
}

bool DmaAudioProcessor::isProcessingActive() const { return processingActive; }

void DmaAudioProcessor::setProcessingActive(bool active) { processingActive = active; }

// === Placeholder implementációk (FFT és egyéb algoritmusok) ===

void DmaAudioProcessor::computeFFT(const float *inputBuffer, float *outputBuffer, uint16_t size) {
    // ARM optimized FFT használata
    float *realPart = fftInputBuffer;
    float *imagPart = fftOutputBuffer;

    // Input adatok másolása
    memcpy(realPart, inputBuffer, size * sizeof(float));

    // FFT számítás
    ArmFFT::compute(realPart, imagPart, size);

    // Magnitude számítás
    ArmFFT::computeMagnitude(realPart, imagPart, magnitudeBuffer, size);
}

void DmaAudioProcessor::computeMagnitude() {
    // Már a computeFFT-ben megtörtént
}

void DmaAudioProcessor::generateOscilloscopeData() {
    // Simple decimation from ring buffer
    uint16_t available = ringBufferAvailable();
    if (available >= OSCILLOSCOPE_BUFFER_SIZE * OSCILLOSCOPE_DECIMATION) {
        for (uint16_t i = 0; i < OSCILLOSCOPE_BUFFER_SIZE; i++) {
            uint16_t srcIndex = (ringBufferReadPos + i * OSCILLOSCOPE_DECIMATION) % AUDIO_RING_BUFFER_SIZE;
            oscilloscopeBuffer[i] = ringBuffer[srcIndex];
        }
    }
}

void DmaAudioProcessor::generateEnvelopeData() {
    // Simple envelope detection using magnitude data
    for (uint16_t i = 0; i < ENVELOPE_BUFFER_SIZE && i < currentFftSize / 2; i++) {
        uint8_t envValue = constrain(magnitudeBuffer[i] / 100.0, 0, 255);
        envelopeBuffer[i] = envValue;
    }
}

void DmaAudioProcessor::updateWaterfallData() {
    static uint32_t lastWaterfallUpdate = 0;
    uint32_t now = millis();

    // Waterfall frissítési frekvencia korlátozása (max 15 FPS)
    if (now - lastWaterfallUpdate < 66) {
        return;
    }
    lastWaterfallUpdate = now;

    // Optimalizált buffer görgetés - egy lépésben
    int rowSize = WATERFALL_MAX_WIDTH;
    int totalRows = WATERFALL_MAX_HEIGHT;

    // Scroll waterfall data down - optimalizált memória mozgatás
    memmove(&waterfallBuffer[rowSize], waterfallBuffer, rowSize * (totalRows - 1));

    // Add new line from magnitude data - csak a szükséges részt
    for (uint16_t i = 0; i < WATERFALL_MAX_WIDTH && i < currentFftSize / 2; i++) {
        // Csökkentett erősítés a simább megjelenésért
        uint8_t intensity = constrain(magnitudeBuffer[i] * 30.0, 0, WATERFALL_COLOR_LEVELS - 1);
        waterfallBuffer[i] = intensity;
    }

    // Ha kevesebb bin van, nullázza a maradékot
    for (uint16_t i = currentFftSize / 2; i < WATERFALL_MAX_WIDTH; i++) {
        waterfallBuffer[i] = 0;
    }
}

void DmaAudioProcessor::updatePerformanceStats() {
    uint32_t processingTime = micros() - processingStartTime;
    lastProcessTime = processingTime;

    // CPU usage calculation (simple moving average)
    float currentUsage = (float)processingTime / 1000.0f; // Assume 1ms budget
    cpuUsage = cpuUsage * 0.9f + currentUsage * 0.1f;
}

bool DmaAudioProcessor::setFftSize(uint16_t newSize) {
    if (newSize > FFT_SIZE_MAX || (newSize & (newSize - 1)) != 0) {
        return false; // Must be power of 2 and <= max
    }
    currentFftSize = newSize;
    return true;
}

AudioDataStatus DmaAudioProcessor::getStatus() const {
    AudioDataStatus status;
    status.timestamp = millis();
    status.processedSamples = processedSampleCount;
    status.cpuUsagePercent = cpuUsage;
    status.dmaOverrun = dmaOverrunFlag;
    status.fftOverrun = fftOverrunFlag;
    return status;
}

// === Core1 entry point ===
void audioProcessingCore1Entry() {
    if (g_audioProcessor) {
        g_audioProcessor->processAudioCore1();
    }
}
