#include "AudioProcessor.h"
#include "defines.h"

AudioProcessor::AudioProcessor(float &gainConfigRef, uint16_t fftSize)
    : fftSize_(fftSize), samplingFrequency_(AudioProcessorConstants::DEFAULT_SAMPLING_FREQUENCY), gainConfigRef_(gainConfigRef), fft_(nullptr), realSamples_(nullptr),
      imagSamples_(nullptr), fftOutput_(nullptr), oscOutput_(nullptr), lastSampleTime_(0), sampleIndex_(0), autoGainFactor_(1.0f), maxAmplitude_(0.0f), fftDataReady_(false),
      oscDataReady_(false), fftDataBuffer_(nullptr), oscDataBuffer_(nullptr) {

    if (!isPowerOfTwo(fftSize_)) {
        DEBUG("AudioProcessor: FFT size must be power of 2, using default\n");
        fftSize_ = AudioProcessorConstants::DEFAULT_FFT_SAMPLES;
    }
}

AudioProcessor::~AudioProcessor() {
    deallocateBuffers();
    delete fft_;
}

void AudioProcessor::init() {
    DEBUG("AudioProcessor::init() - FFT size: %d, Sampling freq: %.1f Hz\n", fftSize_, samplingFrequency_);

    // Audio bemenet inicializálása
    pinMode(PIN_AUDIO_INPUT, INPUT); // Bufferek allokálása
    allocateBuffers();

    // FFT objektum létrehozása
    fft_ = new ArduinoFFT<float>(realSamples_, imagSamples_, fftSize_, samplingFrequency_);

    DEBUG("AudioProcessor initialized successfully\n");
}

void AudioProcessor::loop() {
    uint32_t currentTime = micros();

    // Mintavételezés időzítése
    if (currentTime - lastSampleTime_ >= AudioProcessorConstants::SAMPLING_INTERVAL_US) {
        sampleAudio();
        lastSampleTime_ = currentTime;

        // Ha megvan az összes minta, végezzük el az FFT-t
        if (sampleIndex_ >= fftSize_) {
            processFFT();
            sampleIndex_ = 0;
        }
    }
}

bool AudioProcessor::setFftSize(uint16_t size) {
    if (!isPowerOfTwo(size) || size < AudioProcessorConstants::MIN_FFT_SAMPLES || size > AudioProcessorConstants::MAX_FFT_SAMPLES) {
        return false;
    }

    if (size != fftSize_) {
        deallocateBuffers();
        delete fft_;
        fft_ = nullptr;
        fftSize_ = size;
        allocateBuffers();
        fft_ = new ArduinoFFT<float>(realSamples_, imagSamples_, fftSize_, samplingFrequency_);
        sampleIndex_ = 0;
    }

    return true;
}

void AudioProcessor::setSamplingFrequency(float frequency) {
    if (frequency > 0) {
        samplingFrequency_ = frequency;
        if (fft_) {
            delete fft_;
            fft_ = new ArduinoFFT<float>(realSamples_, imagSamples_, fftSize_, samplingFrequency_);
        }
    }
}

bool AudioProcessor::getFFTData(float *output, uint16_t outputSize) {
    if (!fftDataReady_ || !output || !fftDataBuffer_) {
        return false;
    }

    uint16_t copySize = min(outputSize, fftSize_ / 2);
    memcpy(output, fftDataBuffer_, copySize * sizeof(float));
    fftDataReady_ = false;

    return true;
}

bool AudioProcessor::getOscilloscopeData(float *output, uint16_t outputSize) {
    if (!oscDataReady_ || !output || !oscDataBuffer_) {
        return false;
    }

    uint16_t copySize = min(outputSize, fftSize_);
    memcpy(output, oscDataBuffer_, copySize * sizeof(float));
    oscDataReady_ = false;

    return true;
}

void AudioProcessor::allocateBuffers() {
    deallocateBuffers();

    realSamples_ = new float[fftSize_];
    imagSamples_ = new float[fftSize_];
    fftDataBuffer_ = new float[fftSize_ / 2];
    oscDataBuffer_ = new float[fftSize_];

    // Bufferek nullázása
    memset(realSamples_, 0, fftSize_ * sizeof(float));
    memset(imagSamples_, 0, fftSize_ * sizeof(float));
    memset(fftDataBuffer_, 0, (fftSize_ / 2) * sizeof(float));
    memset(oscDataBuffer_, 0, fftSize_ * sizeof(float));

    DEBUG("AudioProcessor: Buffers allocated for FFT size %d\n", fftSize_);
}

void AudioProcessor::deallocateBuffers() {
    delete[] realSamples_;
    delete[] imagSamples_;
    delete[] fftDataBuffer_;
    delete[] oscDataBuffer_;

    realSamples_ = nullptr;
    imagSamples_ = nullptr;
    fftDataBuffer_ = nullptr;
    oscDataBuffer_ = nullptr;
}

void AudioProcessor::sampleAudio() {
    if (sampleIndex_ >= fftSize_) {
        return;
    }

    float sample = readAudioInput();

    // Gain alkalmazása
    sample *= gainConfigRef_ * autoGainFactor_;

    // Minta tárolása
    realSamples_[sampleIndex_] = sample;
    imagSamples_[sampleIndex_] = 0.0f;

    // Oszcilloszkóp buffer frissítése
    if (oscDataBuffer_) {
        oscDataBuffer_[sampleIndex_] = sample;
    }

    sampleIndex_++;
}

void AudioProcessor::processFFT() {
    if (!realSamples_ || !imagSamples_ || !fftDataBuffer_ || !fft_) {
        return;
    }

    // Windowing alkalmazása
    fft_->windowing(FFTWindow::Hamming, FFTDirection::Forward);

    // FFT számítás
    fft_->compute(FFTDirection::Forward);

    // Komplex magnitúdó számítása
    fft_->complexToMagnitude();

    // Auto gain frissítése
    updateAutoGain();

    // Eredmények másolása a kimeneti bufferbe
    uint16_t outputBins = fftSize_ / 2;
    maxAmplitude_ = 0.0f;

    for (uint16_t i = 0; i < outputBins; i++) {
        float magnitude = realSamples_[i];

        // Alacsony frekvenciák csillapítása
        float frequency = (float)i * samplingFrequency_ / fftSize_;
        if (frequency < AudioProcessorConstants::LOW_FREQ_ATTENUATION_THRESHOLD_HZ) {
            magnitude /= AudioProcessorConstants::LOW_FREQ_ATTENUATION_FACTOR;
        }

        // Amplitúdó skálázás
        magnitude *= AudioProcessorConstants::AMPLITUDE_SCALE;

        fftDataBuffer_[i] = magnitude;

        if (magnitude > maxAmplitude_) {
            maxAmplitude_ = magnitude;
        }
    }

    // Jelzés, hogy új FFT adatok állnak rendelkezésre
    fftDataReady_ = true;
    oscDataReady_ = true;
}

void AudioProcessor::updateAutoGain() {
    // Csúcsérték keresése az FFT eredményekben
    float peak = 0.0f;
    uint16_t outputBins = fftSize_ / 2;

    for (uint16_t i = 1; i < outputBins; i++) { // Kihagyjuk a DC komponenst
        if (realSamples_[i] > peak) {
            peak = realSamples_[i];
        }
    }

    if (peak > 0.0f) {
        float targetGain = AudioProcessorConstants::FFT_AUTO_GAIN_TARGET_PEAK / peak;

        // Gain korlátozása
        targetGain = constrain(targetGain, AudioProcessorConstants::FFT_AUTO_GAIN_MIN_FACTOR, AudioProcessorConstants::FFT_AUTO_GAIN_MAX_FACTOR);

        // Simított gain változtatás
        if (targetGain < autoGainFactor_) {
            // Gyors csökkentés (attack)
            autoGainFactor_ = autoGainFactor_ * (1.0f - AudioProcessorConstants::AUTO_GAIN_ATTACK_COEFF) + targetGain * AudioProcessorConstants::AUTO_GAIN_ATTACK_COEFF;
        } else {
            // Lassú növelés (release)
            autoGainFactor_ = autoGainFactor_ * (1.0f - AudioProcessorConstants::AUTO_GAIN_RELEASE_COEFF) + targetGain * AudioProcessorConstants::AUTO_GAIN_RELEASE_COEFF;
        }
    }
}

float AudioProcessor::readAudioInput() {
    // ADC olvasás (0-4095) konvertálása -1.0 és 1.0 közötti értékre
    int adcValue = analogRead(PIN_AUDIO_INPUT);
    return (float)(adcValue - 2048) / 2048.0f;
}

bool AudioProcessor::isPowerOfTwo(uint16_t value) { return value > 0 && (value & (value - 1)) == 0; }
