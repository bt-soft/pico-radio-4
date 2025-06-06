#ifndef __PICO_SENSOR_UTILS_H
#define __PICO_SENSOR_UTILS_H

#include <Arduino.h>

#include "defines.h" // PIN_VBUS

namespace PicoSensorUtils {

// --- Konstansok ---
#define AD_RESOLUTION 12 // 12 bites az ADC
#define V_REFERENCE 3.3f
#define CONVERSION_FACTOR (1 << AD_RESOLUTION)

// Külső feszültségosztó ellenállásai a VBUS méréshez (A0-ra kötve)
#define DIVIDER_RATIO ((VBUS_DIVIDER_R1 + VBUS_DIVIDER_R2) / VBUS_DIVIDER_R2) // Feszültségosztó aránya

/**
 * AD inicializálása
 */
inline void init() { analogReadResolution(AD_RESOLUTION); }

/**
 * ADC olvasás és VBUS feszültség kiszámítása külső osztóval
 * @return A VBUS mért feszültsége Voltban.
 */
inline float readVBus() {

    // ADC érték átalakítása feszültséggé
    float voltageOut = (analogRead(PIN_VBUS_INPUT) * V_REFERENCE) / CONVERSION_FACTOR;

    // Eredeti feszültség számítása a feszültségosztó alapján
    return voltageOut * DIVIDER_RATIO;
}

/**
 * Kiolvassa a processzor hőmérsékletét
 * @return processzor hőmérséklete Celsius fokban
 */
inline float readCoreTemperature() { return analogReadTemp(); }

}; // namespace PicoSensorUtils

#endif // __PICO_SENSOR_UTILS_H
