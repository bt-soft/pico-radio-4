#ifndef AUDIO_DEFINES_H
#define AUDIO_DEFINES_H

// === Audio Processing Konstansok ===

// Audio input és sampling
#define AUDIO_SAMPLE_RATE 40000      // 40kHz sampling rate
#define AUDIO_SAMPLE_RATE_HALF 20000 // Nyquist frequency
#define AUDIO_DMA_BUFFER_SIZE 2048   // DMA buffer méret (ping-pong bufferek)
#define AUDIO_RING_BUFFER_SIZE 8192  // Ring buffer méret (nagyobb a stabilitásért)

// FFT konfigurációk
#define FFT_SIZE_LOW_RES 512   // Alacsony felbontású FFT
#define FFT_SIZE_HIGH_RES 2048 // Magas felbontású FFT
#define FFT_SIZE_MAX 4096      // Maximális FFT méret

// Spektrum analizátor
#define SPECTRUM_LOW_RES_BANDS 24        // Alacsony felbontású spektrum sávok
#define SPECTRUM_MIN_FREQ_HZ 300.0f      // Minimum megjelenített frekvencia
#define SPECTRUM_MAX_FREQ_AM_HZ 6000.0f  // Maximum audio frekvencia AM módban
#define SPECTRUM_MAX_FREQ_FM_HZ 15000.0f // Maximum audio frekvencia FM módban

// Oszcilloszkóp
#define OSCILLOSCOPE_BUFFER_SIZE 320 // Oszcilloszkóp buffer méret (screen width)
#define OSCILLOSCOPE_DECIMATION 4    // Mintavétel ritkítás

// Envelope detektor
#define ENVELOPE_BUFFER_SIZE 320     // Envelope buffer méret
#define ENVELOPE_SMOOTH_FACTOR 0.25f // Simítási faktor
#define ENVELOPE_DECAY_FACTOR 0.95f  // Lecsengési faktor

// Waterfall
#define WATERFALL_MAX_WIDTH 320   // Maximum szélesség
#define WATERFALL_MAX_HEIGHT 80   // Maximum magasság
#define WATERFALL_COLOR_LEVELS 16 // Színszintek száma

// Dekóderek
#define CW_TUNING_SPAN_HZ 600.0f    // CW hangolási sávszélesség
#define RTTY_TUNING_SPAN_HZ 1000.0f // RTTY hangolási sávszélesség

// DMA és interrupt prioritás
#define AUDIO_DMA_CHANNEL 0  // DMA csatorna az audio-hoz
#define AUDIO_IRQ_PRIORITY 0 // Legmagasabb prioritás (real-time)

// Core assignment
#define AUDIO_PROCESSING_CORE 1 // Audio feldolgozás core1-en
#define UI_PROCESSING_CORE 0    // UI core0-n

// Gain és scaling
#define AUDIO_INPUT_GAIN 10.0f      // Bemeneti erősítés (növelve a jobb láthatóságért)
#define FFT_AMPLITUDE_SCALE 1000.0f // FFT amplitude skálázás
#define ADC_RESOLUTION_BITS 12      // ADC felbontás
#define ADC_MAX_VALUE 4095          // 2^12 - 1

// Thread communication
#define AUDIO_DATA_READY_FLAG_BIT 0 // Bit flag az új audio adatokhoz
#define AUDIO_MUTEX_TIMEOUT_MS 10   // Mutex timeout

#endif // AUDIO_DEFINES_H
