#ifndef ARM_FFT_H
#define ARM_FFT_H

#include <ArduinoFft.h>
#include <cmath>
#include <cstdint>

/**
 * @brief ArduinoFFT wrapper class for audio processing
 * @details Simple wrapper around ArduinoFFT library with unified interface
 */
class ArmFFT {
  public:
    /**
     * @brief FFT számítás (in-place)
     * @param real_data Valós rész (input/output)
     * @param imag_data Képzetes rész (input nullázva, output)
     * @param N FFT méret (power of 2)
     */
    static void compute(float *real_data, float *imag_data, uint16_t N);

    /**
     * @brief Magnitude számítás FFT eredményből
     * @param real_data Valós rész
     * @param imag_data Képzetes rész
     * @param magnitude_data Kimenet (magnitude)
     * @param N FFT méret
     */
    static void computeMagnitude(const float *real_data, const float *imag_data, double *magnitude_data, uint16_t N);

    /**
     * @brief Hanning window alkalmazása
     * @param data Input/output adat
     * @param N Adatok száma
     */
    static void applyHanningWindow(float *data, uint16_t N);

    /**
     * @brief Get FFT implementation name for debugging
     * @return Implementation name string
     */
    static const char *getImplementationName();

  private:
    /**
     * @brief ArduinoFFT példány létrehozása
     * @param N FFT méret
     * @return ArduinoFFT példány
     */
    static ArduinoFFT<float> createFFTInstance(uint16_t N);
};

#endif // ARM_FFT_H
