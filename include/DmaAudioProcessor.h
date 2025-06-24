#ifndef DMA_AUDIO_PROCESSOR_H
#define DMA_AUDIO_PROCESSOR_H

#include "AudioDefines.h"
#include "IAudioDataProvider.h"
#include <cmath>
#include <cstdint>
#include <hardware/adc.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <pico/multicore.h>
#include <pico/mutex.h>

/**
 * @brief DMA alapú audio processzor osztály
 * @details Core1-en futó audio feldolgozó, DMA-val és FFT-vel
 *
 * Funkciók:
 * - DMA alapú ADC sampling (ping-pong buffers)
 * - Real-time FFT processing (ARM optimalizált)
 * - Thread-safe adatcsere Core0-val
 * - Oszcilloszkóp, spektrum, envelope generálás
 */
class DmaAudioProcessor : public IAudioDataProvider {
  public:
    /**
     * @brief Konstruktor
     */
    DmaAudioProcessor();

    /**
     * @brief Destruktor
     */
    virtual ~DmaAudioProcessor();

    /**
     * @brief Inicializálás és DMA beállítás
     * @return True ha sikeres
     */
    bool initialize();

    /**
     * @brief Audio feldolgozás indítása Core1-en
     * @details Ez a függvény fut Core1-en végtelen ciklusban
     */
    void processAudioCore1();

    /**
     * @brief Cleanup és erőforrások felszabadítása
     */
    void cleanup();

    // === IAudioDataProvider interface ===
    const double *getMagnitudeData() const override;
    const int16_t *getOscilloscopeData() const override;
    const uint8_t *getEnvelopeData() const override;
    const uint8_t *getWaterfallData() const override;
    float getBinWidthHz() const override;
    uint16_t getFftSize() const override;
    uint32_t getSampleRate() const override;
    bool isDataReady() const override;
    void markDataConsumed() override;
    bool isProcessingActive() const override;
    void setProcessingActive(bool active) override;

    /**
     * @brief Státusz információk lekérése
     * @return AudioDataStatus struktúra
     */
    AudioDataStatus getStatus() const;

    /**
     * @brief FFT méret váltása runtime-ban
     * @param newSize Új FFT méret (512, 1024, 2048, 4096)
     * @return True ha sikeres
     */
    bool setFftSize(uint16_t newSize);

  private:
    // === DMA és buffer management ===
    uint dmaChannel;
    uint dmaChanMask;

    // Ping-pong DMA buffers (aligned for DMA)
    int16_t dmaBuffer1[AUDIO_DMA_BUFFER_SIZE] __attribute__((aligned(4)));
    int16_t dmaBuffer2[AUDIO_DMA_BUFFER_SIZE] __attribute__((aligned(4)));

    volatile bool buffer1Ready;
    volatile bool buffer2Ready;
    volatile bool currentBufferIsFirst;

    // Ring buffer a folyamatos audio adatokhoz
    int16_t ringBuffer[AUDIO_RING_BUFFER_SIZE];
    volatile uint16_t ringBufferWritePos;
    volatile uint16_t ringBufferReadPos;

    // === FFT és feldolgozás ===
    uint16_t currentFftSize;
    float *fftInputBuffer;   // Float input a FFT-hez
    float *fftOutputBuffer;  // FFT kimenet (complex)
    double *magnitudeBuffer; // Magnitude számítás eredménye

    // === Kimeneti adatok ===
    int16_t oscilloscopeBuffer[OSCILLOSCOPE_BUFFER_SIZE];
    uint8_t envelopeBuffer[ENVELOPE_BUFFER_SIZE];
    uint8_t waterfallBuffer[WATERFALL_MAX_WIDTH * WATERFALL_MAX_HEIGHT];

    // === Thread synchronization ===
    mutable mutex_t dataMutex;
    volatile bool dataReady;
    volatile bool processingActive;

    // === Performance monitoring ===
    uint32_t lastProcessTime;
    uint32_t processingStartTime;
    float cpuUsage;
    uint32_t processedSampleCount;
    bool dmaOverrunFlag;
    bool fftOverrunFlag;

    // === Privát metódusok ===

    /**
     * @brief DMA inicializálás
     * @return True ha sikeres
     */
    bool initializeDma();

    /**
     * @brief ADC inicializálás
     * @return True ha sikeres
     */
    bool initializeAdc();

    /**
     * @brief FFT bufferek allokálása
     * @return True ha sikeres
     */
    bool allocateBuffers();

    /**
     * @brief DMA interrupt handler (static wrapper)
     */
    static void dmaIrqHandler();

    /**
     * @brief DMA interrupt feldolgozás
     */
    void handleDmaInterrupt();

    /**
     * @brief Audio buffer feldolgozása
     * @param buffer Bemeneti buffer
     * @param size Buffer méret
     */
    void processAudioBuffer(const int16_t *buffer, uint16_t size);

    /**
     * @brief FFT számítás (ARM optimalizált)
     * @param inputBuffer Bemeneti adatok
     * @param outputBuffer Kimeneti komplex adatok
     * @param size FFT méret
     */
    void computeFFT(const float *inputBuffer, float *outputBuffer, uint16_t size);

    /**
     * @brief Magnitude számítás FFT kimenetéből
     */
    void computeMagnitude();

    /**
     * @brief Oszcilloszkóp adatok generálása
     */
    void generateOscilloscopeData();

    /**
     * @brief Envelope adatok generálása
     */
    void generateEnvelopeData();

    /**
     * @brief Waterfall adatok frissítése
     */
    void updateWaterfallData();

    /**
     * @brief CPU használat számítása
     */
    void updatePerformanceStats();

    /**
     * @brief Ring buffer írás
     * @param data Adat pointer
     * @param size Adatok száma
     */
    void ringBufferWrite(const int16_t *data, uint16_t size);

    /**
     * @brief Ring buffer olvasás
     * @param data Kimeneti buffer
     * @param size Kért adatok száma
     * @return Ténylegesen olvasott adatok száma
     */
    uint16_t ringBufferRead(int16_t *data, uint16_t size);

    /**
     * @brief Ring buffer szabad hely
     * @return Szabad helyek száma
     */
    uint16_t ringBufferFreeSpace() const;

    /**
     * @brief Ring buffer elérhető adatok
     * @return Elérhető adatok száma
     */
    uint16_t ringBufferAvailable() const;
};

// === Globális instance és Core1 entry point ===
extern DmaAudioProcessor *g_audioProcessor;

/**
 * @brief Core1 entry point
 * @details Ez a függvény fut Core1-en a setup1() hívás után
 */
void audioProcessingCore1Entry();

#endif // DMA_AUDIO_PROCESSOR_H
