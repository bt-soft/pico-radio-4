#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include "pins.h"
#include <Arduino.h>
#include <ArduinoFFT.h>

/**
 * @brief Konstansok az AudioProcessor osztályhoz
 */
namespace AudioProcessorConstants {
constexpr uint16_t MIN_FFT_SAMPLES = 64;      // Minimális FFT minták száma
constexpr uint16_t MAX_FFT_SAMPLES = 1024;    // Maximális FFT minták száma
constexpr uint16_t DEFAULT_FFT_SAMPLES = 256; // Alapértelmezett FFT minták száma

constexpr float AMPLITUDE_SCALE = 30.0f;                    // Skálázási faktor az FFT eredményekhez
constexpr float LOW_FREQ_ATTENUATION_THRESHOLD_HZ = 200.0f; // Ez alatti frekvenciákat csillapítjuk
constexpr float LOW_FREQ_ATTENUATION_FACTOR = 8.0f;         // Alacsony frekvenciák csillapítása

// Auto Gain konstansok
constexpr float FFT_AUTO_GAIN_TARGET_PEAK = 800.0f; // Cél csúcsérték az Auto Gain módhoz
constexpr float FFT_AUTO_GAIN_MIN_FACTOR = 0.1f;    // Minimális erősítési faktor
constexpr float FFT_AUTO_GAIN_MAX_FACTOR = 15.0f;   // Maximális erősítési faktor
constexpr float AUTO_GAIN_ATTACK_COEFF = 0.6f;      // Erősítés csökkentésének sebessége
constexpr float AUTO_GAIN_RELEASE_COEFF = 0.03f;    // Erősítés növelésének sebessége

constexpr int MAX_INTERNAL_WIDTH = 86;           // Belső buffer szélessége
constexpr int OSCI_SAMPLE_DECIMATION_FACTOR = 2; // Oszcilloszkóp decimációs faktor

// Mintavételezési beállítások
constexpr float DEFAULT_SAMPLING_FREQUENCY = 48000.0f;                             // Hz
constexpr uint32_t SAMPLING_INTERVAL_US = 1000000.0f / DEFAULT_SAMPLING_FREQUENCY; // mikroszekundum
} // namespace AudioProcessorConstants

/**
 * @brief Audio feldolgozó osztály FFT analízissel
 *
 * Ez az osztály a Core1-en fut és folyamatosan végez FFT analízist
 * a PIN_AUDIO_INPUT-on érkező audio jelből.
 */
class AudioProcessor {
  public:
    /**
     * @brief AudioProcessor konstruktor
     * @param gainConfigRef Referencia a gain konfigurációs értékre
     * @param fftSize FFT méret (alapértelmezett: DEFAULT_FFT_SAMPLES)
     */
    AudioProcessor(float &gainConfigRef, uint16_t fftSize = AudioProcessorConstants::DEFAULT_FFT_SAMPLES);

    /**
     * @brief Destruktor
     */
    ~AudioProcessor();

    /**
     * @brief Inicializálás
     */
    void init();

    /**
     * @brief Főciklus - ezt kell meghívni a loop1()-ben
     */
    void loop();

    /**
     * @brief FFT méret beállítása
     * @param size Új FFT méret (2 hatványának kell lennie)
     * @return true ha sikeres, false ha érvénytelen méret
     */
    bool setFftSize(uint16_t size);

    /**
     * @brief Aktuális FFT méret lekérdezése
     */
    uint16_t getFftSize() const { return fftSize_; }

    /**
     * @brief Mintavételezési frekvencia beállítása
     */
    void setSamplingFrequency(float frequency);

    /**
     * @brief FFT eredmények lekérdezése (thread-safe)
     * @param output Kimeneti tömb (legalább fftSize/2 méretű legyen)
     * @param outputSize Kimeneti tömb mérete
     * @return true ha új adatok elérhetők
     */
    bool getFFTData(float *output, uint16_t outputSize);

    /**
     * @brief Oszcilloszkóp adatok lekérdezése (thread-safe)
     * @param output Kimeneti tömb
     * @param outputSize Kimeneti tömb mérete
     * @return true ha új adatok elérhetők
     */
    bool getOscilloscopeData(float *output, uint16_t outputSize);

    /**
     * @brief Auto gain állapot lekérdezése
     */
    float getAutoGainFactor() const { return autoGainFactor_; }

    /**
     * @brief Maximális amplitúdó lekérdezése
     */
    float getMaxAmplitude() const { return maxAmplitude_; }

  private:
    uint16_t fftSize_;
    float samplingFrequency_;
    float &gainConfigRef_;

    // FFT objektum
    ArduinoFFT<float> *fft_;

    // Audio bufferek
    float *realSamples_;
    float *imagSamples_;
    float *fftOutput_;
    float *oscOutput_;

    // Mintavételezés
    uint32_t lastSampleTime_;
    uint16_t sampleIndex_;

    // Auto gain
    float autoGainFactor_;
    float maxAmplitude_;

    // Thread-safe adatcsere
    volatile bool fftDataReady_;
    volatile bool oscDataReady_;
    float *fftDataBuffer_;
    float *oscDataBuffer_;

    // Privát segédfüggvények
    void allocateBuffers();
    void deallocateBuffers();
    void sampleAudio();
    void processFFT();
    void updateAutoGain();
    float readAudioInput();
    bool isPowerOfTwo(uint16_t value);
};

#endif // AUDIO_PROCESSOR_H
