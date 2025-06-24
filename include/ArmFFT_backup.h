#ifndef ARM_FFT_H
#define ARM_FFT_H

#include "defines.h"
#include <cmath>
#include <cstdint>

// Choose FFT implementation based on configuration
#if AUDIO_FFT_IMPLEMENTATION == AUDIO_FFT_IMPLEMENTATION_CMSIS
#include <arm_math.h>
// Note: arm_const_structs.h may not be available in all CMSIS-DSP versions
// #include <arm_const_structs.h>
#elif AUDIO_FFT_IMPLEMENTATION == AUDIO_FFT_IMPLEMENTATION_ARDUINO
#include <ArduinoFft.h>
#else
#error "Invalid AUDIO_FFT_IMPLEMENTATION selected"
#endif

/**
 * @brief Unified FFT wrapper class for audio processing
 * @details This class provides a unified interface for different FFT implementations:
 *          - ArduinoFFT (default, good compatibility)
 *          - CMSIS-DSP (best performance on ARM)
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
#if AUDIO_FFT_IMPLEMENTATION == AUDIO_FFT_IMPLEMENTATION_ARDUINO
    /**
     * @brief ArduinoFFT példány létrehozása
     * @param N FFT méret
     * @return ArduinoFFT példány
     */
    static ArduinoFFT<float> createFFTInstance(uint16_t N);
#elif AUDIO_FFT_IMPLEMENTATION == AUDIO_FFT_IMPLEMENTATION_CMSIS
    /**
     * @brief Get CMSIS-DSP FFT instance for given size
     * @param N FFT size
     * @return Pointer to CMSIS FFT instance or nullptr if not supported
     */
    static const arm_cfft_instance_f32 *getCmsisFftInstance(uint16_t N);
#endif
};

#endif // ARM_FFT_H
