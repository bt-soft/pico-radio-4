#include "Core1Logic.h"
#include "Config.h"
#include "defines.h"

// Globális változók a Core1-hez
static AudioProcessor *audioProcessor = nullptr;
static float audioGain = 1.0f;

void core1_init() {
    DEBUG("Core1 initializing...\n");

    // Audio gain inicializálása a konfigból
    extern Config config;
    audioGain = config.data.audioFftGain;

    // AudioProcessor létrehozása
    audioProcessor = new AudioProcessor(audioGain);

    if (audioProcessor) {
        audioProcessor->init();
        DEBUG("Core1 AudioProcessor initialized successfully\n");
    } else {
        DEBUG("Core1 AudioProcessor initialization FAILED!\n");
    }
}

void core1_loop() {
    if (audioProcessor) {
        audioProcessor->loop();
    }
}

AudioProcessor *getAudioProcessor() { return audioProcessor; }

void setAudioGain(float gain) {
    audioGain = gain;
    if (audioProcessor) {
        // A gain értéket az AudioProcessor automatikusan használja a referencia miatt
        DEBUG("Audio gain set to: %.2f\n", gain);
    }
}

bool setFFTSize(uint16_t size) {
    if (audioProcessor) {
        return audioProcessor->setFftSize(size);
    }
    return false;
}
