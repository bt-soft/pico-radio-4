#ifndef CORE1_LOGIC_H
#define CORE1_LOGIC_H

#include "AudioProcessor.h"

/**
 * @brief Core1 inicializálása
 *
 * Ezt kell meghívni a setup1() függvényben.
 */
void core1_init();

/**
 * @brief Core1 főciklus
 *
 * Ezt kell meghívni a loop1() függvényben.
 */
void core1_loop();

/**
 * @brief AudioProcessor példány lekérése (thread-safe)
 *
 * @return AudioProcessor pointer vagy nullptr ha még nincs inicializálva
 */
AudioProcessor *getAudioProcessor();

/**
 * @brief Audio gain beállítása
 *
 * @param gain Új gain érték
 */
void setAudioGain(float gain);

/**
 * @brief FFT méret beállítása
 *
 * @param size Új FFT méret
 * @return true ha sikeres
 */
bool setFFTSize(uint16_t size);

#endif // CORE1_LOGIC_H
