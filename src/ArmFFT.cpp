#include "ArmFFT.h"
#include <cstring>

// === Public Methods ===

void ArmFFT::compute(float *real_data, float *imag_data, uint16_t N) {
    // Input validation
    if (!real_data || !imag_data || N < 2 || (N & (N - 1)) != 0) {
        return; // N must be power of 2
    }

    // Clear imaginary part
    memset(imag_data, 0, N * sizeof(float));

    // Apply Hanning window to reduce spectral leakage
    applyHanningWindow(real_data, N);

    // Use ArduinoFFT
    ArduinoFFT<float> fft = createFFTInstance(N);
    fft.compute(real_data, imag_data, N, FFTDirection::Forward);
}

void ArmFFT::computeMagnitude(const float *real_data, const float *imag_data, double *magnitude_data, uint16_t N) {
    if (!real_data || !imag_data || !magnitude_data)
        return;

    // Only compute positive frequencies (N/2)
    uint16_t half_N = N / 2;

    // Standard magnitude calculation
    for (uint16_t i = 0; i < half_N; i++) {
        float real = real_data[i];
        float imag = imag_data[i];

        // Magnitude = sqrt(real^2 + imag^2)
        float magnitude_squared = real * real + imag * imag;

        if (magnitude_squared > 0.0f) {
            magnitude_data[i] = sqrt(magnitude_squared);
        } else {
            magnitude_data[i] = 0.0;
        }

        // Normalization
        if (i == 0) {
            magnitude_data[i] /= N; // DC normalization
        } else {
            magnitude_data[i] *= 2.0 / N; // Other bins normalization
        }
    }
}

void ArmFFT::applyHanningWindow(float *data, uint16_t N) {
    const float PI2_N = 2.0f * M_PI / (float)(N - 1);

    for (uint16_t i = 0; i < N; i++) {
        float window = 0.5f * (1.0f - cosf(PI2_N * i));
        data[i] *= window;
    }
}

const char *ArmFFT::getImplementationName() { return "ArduinoFFT"; }

// === Private Methods ===

ArduinoFFT<float> ArmFFT::createFFTInstance(uint16_t N) {
    // ArduinoFFT constructor doesn't need parameters in newer versions
    return ArduinoFFT<float>();
}
