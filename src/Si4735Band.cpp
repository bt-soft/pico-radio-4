
#include <pgmspace.h> // For PROGMEM support, ez kell a patch_full elé
//
#include <patch_full.h> // SSB patch for whole SSBRX full download

#include "Si4735Band.h"

/**
 * Band inicializálása konfig szerint
 * @param sysStart Rendszer indítás van?
 *                 Ha true, akkor a konfigból betölti az alapértelmezett értékeket
 */
void Si4735Band::bandInit(bool sysStart) {

    DEBUG("Si4735Band::BandInit() ->currentBandIdx: %d\n", config.data.currentBandIdx); // Rendszer indítás van?
    if (sysStart) {
        DEBUG("Si4735Band::bandInit() -> System start, loading defaults...\n");

        // Band adatok betöltése a BandStore-ból
        loadBandData();

        rtv::freqstep = 1000; // hz
        rtv::freqDec = rtv::currentBFO;
    }

    // Currentband beállítása
    BandTable &currentBand = getCurrentBand();

    if (getCurrentBandType() == FM_BAND_TYPE) {
        si4735.setup(PIN_SI4735_RESET, FM_BAND_TYPE);
        si4735.setFM(); // RDS is typically automatically enabled for FM mode in Si4735

        // RDS inicializálás és konfiguráció RÖGTÖN az FM setup után
        si4735.RdsInit();
        si4735.setRdsConfig(1, 2, 2, 2, 2); // enable=1, threshold=2 (mint a working projektben)

        // Seek beállítások
        si4735.setSeekFmRssiThreshold(2); // 2dB RSSI threshold
        si4735.setSeekFmSrnThreshold(2);  // 2dB SNR threshold
        si4735.setSeekFmSpacing(10);      // 10kHz seek lépésköz
                                          // 87.5MHz - 108MHz között
        si4735.setSeekFmLimits(currentBand.minimumFreq, currentBand.maximumFreq);

        // RDS status lekérdezése a setup után
        delay(100); // Kis várakozás, hogy a chip beálljon
        si4735.getRdsStatus();

    } else {
        si4735.setup(PIN_SI4735_RESET, MW_BAND_TYPE);
        si4735.setAM();

        // Seek beállítások
        si4735.setSeekAmRssiThreshold(50); // 50dB RSSI threshold
        si4735.setSeekAmSrnThreshold(20);  // 20dB SNR threshold
    }
}

/**
 * SSB patch betöltése
 */
void Si4735Band::loadSSB() {

    // Ha már be van töltve, akkor nem megyünk tovább
    if (ssbLoaded) {
        return;
    }

    si4735.reset();
    si4735.queryLibraryId(); // Is it really necessary here? I will check it.
    si4735.patchPowerUp();
    delay(50);

    si4735.setI2CFastMode(); // Recommended
    si4735.downloadPatch(ssb_patch_content, sizeof(ssb_patch_content));
    si4735.setI2CStandardMode(); // goes back to default (100KHz)
    delay(50);

    // Parameters
    // AUDIOBW - SSB Audio bandwidth; 0 = 1.2KHz (default); 1=2.2KHz; 2=3KHz; 3=4KHz; 4=500Hz; 5=1KHz;
    // SBCUTFLT SSB - side band cutoff filter for band passand low pass filter ( 0 or 1)
    // AVC_DIVIDER  - set 0 for SSB mode; set 3 for SYNC mode.
    // AVCEN - SSB Automatic Volume Control (AVC) enable; 0=disable; 1=enable (default).
    // SMUTESEL - SSB Soft-mute Based on RSSI or SNR (0 or 1).
    // DSP_AFCDIS - DSP AFC Disable or enable; 0=SYNC MODE, AFC enable; 1=SSB MODE, AFC disable.
    si4735.setSSBConfig(config.data.bwIdxSSB, 1, 0, 1, 0, 1);
    delay(25);
    ssbLoaded = true;
}

/**
 * Band beállítása
 * @param useDefaults prefereált demodulációt betöltsük?
 */
void Si4735Band::bandSet(bool useDefaults) {

    DEBUG("Si4735Band::bandSet() -> useDefaults: %s\n", useDefaults ? "true" : "false");

    // Beállítjuk az aktuális Band rekordot - FONTOS: pointer frissítése!
    BandTable &currentBand = getCurrentBand();

    // Demoduláció beállítása
    uint8_t currMod = currentBand.currMod;

    // A sávhoz preferált demodulációs módot állítunk be?
    if (useDefaults) {
        // Átmásoljuk a preferált modulációs módot
        currMod = currentBand.currMod = currentBand.prefMod;
        ssbLoaded = false; // SSB patch betöltése szükséges
    }

    if (currMod == AM or currMod == FM) {

        ssbLoaded = false; // FIXME: Ez kell? Band váltás után megint be kell tölteni az SSB-t?

    } else if (currMod == LSB or currMod == USB or currMod == CW) {
        if (ssbLoaded == false) {
            this->loadSSB();
        }
    }
    useBand();
    setBandWidth();

    // Antenna Tunning Capacitor beállítása
    si4735.setTuneFrequencyAntennaCapacitor(currentBand.antCap);
}

/**
 * Band beállítása
 */
void Si4735Band::useBand() {

    DEBUG("Si4735Band::useBand() -> currentBandIdx: %d\n", config.data.currentBandIdx);

    BandTable &currentBand = getCurrentBand();

    // Index ellenőrzés (biztonsági okokból)
    uint8_t stepIndex;

    // AM esetén 1...1000Khz között bármi lehet - {"1kHz", "5kHz", "9kHz", "10kHz"};
    uint8_t currentBandType = currentBand.bandType;
    if (currentBandType == MW_BAND_TYPE or currentBandType == LW_BAND_TYPE) {
        // currentBand->currentStep = static_cast<uint8_t>(atoi(Band::stepSizeAM[config.data.ssIdxMW]));
        stepIndex = config.data.ssIdxMW;
        // Határellenőrzés
        if (stepIndex >= ARRAY_ITEM_COUNT(stepSizeAM)) {
            DEBUG("Si4735Band Hiba: Érvénytelen ssIdxMW index: %d. Alapértelmezett használata.\n", stepIndex);
            stepIndex = 0;                   // Visszaállás alapértelmezettre (pl. 1kHz)
            config.data.ssIdxMW = stepIndex; // Opcionális: Konfig frissítése
            currentBand.currStep = stepSizeAM[stepIndex].value;

        } else if (currentBandType == SW_BAND_TYPE) {
            // currentBand->currentStep = static_cast<uint8_t>(atoi(Band::stepSizeAM[config.data.ssIdxAM]));
            // AM/SSB/CW Shortwave esetén
            stepIndex = config.data.ssIdxAM;
            // Határellenőrzés
            if (stepIndex >= ARRAY_ITEM_COUNT(stepSizeAM)) {
                DEBUG("Si4735Band Hiba: Érvénytelen ssIdxAM index: %d. Alapértelmezett használata.\n", stepIndex);
                stepIndex = 0;                   // Visszaállás alapértelmezettre
                config.data.ssIdxAM = stepIndex; // Opcionális: Konfig frissítése
            }
            currentBand.currStep = stepSizeAM[stepIndex].value;

        } else {
            // FM esetén csak 3 érték lehet - {"50Khz", "100KHz", "1MHz"};
            // static_cast<uint8_t>(atoi(Band::stepSizeFM[config.data.ssIdxFM]));
            stepIndex = config.data.ssIdxFM; // Határellenőrzés
            if (stepIndex >= ARRAY_ITEM_COUNT(stepSizeFM)) {
                DEBUG("Si4735Band Hiba: Érvénytelen ssIdxFM index: %d. Alapértelmezett használata.\n", stepIndex);
                stepIndex = 0;                   // Visszaállás alapértelmezettre
                config.data.ssIdxFM = stepIndex; // Opcionális: Konfig frissítése
            }
            currentBand.currStep = stepSizeFM[stepIndex].value;
        }

        if (currentBandType == FM_BAND_TYPE) {
            ssbLoaded = false;
            rtv::bfoOn = false;
            // Antenna tuning capacitor beállítása (FM esetén antenna tuning capacitor nem kell)
            currentBand.antCap = getDefaultAntCapValue();
            si4735.setTuneFrequencyAntennaCapacitor(currentBand.antCap);
            delay(100);
            si4735.setFM(currentBand.minimumFreq, currentBand.maximumFreq, currentBand.currFreq, currentBand.currStep);
            si4735.setFMDeEmphasis(1); // 1 = 50 μs. Usedin Europe, Australia, Japan;  2 = 75 μs. Used in USA (default)

            // RDS inicializálás és konfiguráció
            si4735.RdsInit();
#define RDS_ENABLE 1
#define RDS_BLOCK_ERROR_TRESHOLD 2
            si4735.setRdsConfig(RDS_ENABLE, RDS_BLOCK_ERROR_TRESHOLD, RDS_BLOCK_ERROR_TRESHOLD, RDS_BLOCK_ERROR_TRESHOLD, RDS_BLOCK_ERROR_TRESHOLD);

            // RDS státusz ellenőrzése a konfiguráció után
            delay(200); // Várakozás hogy a chip feldolgozza
            si4735.getRdsStatus();
        } else {
            // AM-ben vagyunk
            currentBand.antCap = getDefaultAntCapValue(); // Sima AM esetén antenna tuning capacitor nem kell
            si4735.setTuneFrequencyAntennaCapacitor(currentBand.antCap);

            if (ssbLoaded) {
                // SSB vagy CW mód
                bool isCWMode = (currentBand.currMod == CW);

                // Mód beállítása (LSB-t használunk alapnak CW-hez)
                uint8_t modeForChip = isCWMode ? LSB : currentBand.currMod;
                si4735.setSSB(currentBand.minimumFreq, currentBand.maximumFreq, currentBand.currFreq,
                              1, // SSB/CW esetén a step mindig 1kHz a chipen belül
                              modeForChip);

                // BFO beállítása                        // CW mód: Fix BFO offset (pl. 700 Hz) + manuális finomhangolás
                const int16_t cwBaseOffset = isCWMode ? config.data.cwReceiverOffsetHz : 0; // Alap CW eltolás a configból
                si4735.setSSBBfo(cwBaseOffset + rtv::currentBFO + rtv::currentBFOmanu);
                rtv::CWShift = isCWMode; // Jelezzük a kijelzőnek

                // SSB/CW esetén a lépésköz a chipen mindig 1kHz, de a finomhangolás BFO-val történik
                currentBand.currStep = 1;
                si4735.setFrequencyStep(currentBand.currStep);

            } else { // Sima AM mód
                si4735.setAM(currentBand.minimumFreq, currentBand.maximumFreq, currentBand.currFreq, currentBand.currStep);
                // si4735.setAutomaticGainControl(1, 0);
                // si4735.setAmSoftMuteMaxAttenuation(0); // // Disable Soft Mute for AM
                rtv::bfoOn = false;
                rtv::CWShift = false; // AM módban biztosan nincs CW shift
            }
        }
    }
}

/**
 * Sávszélesség beállítása
 */
void Si4735Band::setBandWidth() {

    DEBUG("Si4735Band::setBandWidth() -> currentBandIdx: %d\n", config.data.currentBandIdx);

    BandTable &currentBand = getCurrentBand();

    uint8_t currMod = currentBand.currMod;
    if (currMod == LSB or currMod == USB or currMod == CW) {
        /**
         * @ingroup group17 Patch and SSB support
         *
         * @brief SSB Audio Bandwidth for SSB mode
         *
         * @details 0 = 1.2 kHz low-pass filter  (default).
         * @details 1 = 2.2 kHz low-pass filter.
         * @details 2 = 3.0 kHz low-pass filter.
         * @details 3 = 4.0 kHz low-pass filter.
         * @details 4 = 500 Hz band-pass filter for receiving CW signal, i.e. [250 Hz, 750 Hz] with center
         * frequency at 500 Hz when USB is selected or [-250 Hz, -750 1Hz] with center frequency at -500Hz
         * when LSB is selected* .
         * @details 5 = 1 kHz band-pass filter for receiving CW signal, i.e. [500 Hz, 1500 Hz] with center
         * frequency at 1 kHz when USB is selected or [-500 Hz, -1500 1 Hz] with center frequency
         *     at -1kHz when LSB is selected.
         * @details Other values = reserved.
         *
         * @details If audio bandwidth selected is about 2 kHz or below, it is recommended to set SBCUTFLT[3:0] to 0
         * to enable the band pass filter for better high- cut performance on the wanted side band. Otherwise, set it to 1.
         *
         * @see AN332 REV 0.8 UNIVERSAL PROGRAMMING GUIDE; page 24
         *
         * @param AUDIOBW the valid values are 0, 1, 2, 3, 4 or 5; see description above
         */
        si4735.setSSBAudioBandwidth(config.data.bwIdxSSB);

        // If audio bandwidth selected is about 2 kHz or below, it is recommended to set Sideband Cutoff Filter to 0.
        if (config.data.bwIdxSSB == 0 or config.data.bwIdxSSB == 4 or config.data.bwIdxSSB == 5) {
            // Band pass filter to cutoff both the unwanted side band and high frequency components > 2.0 kHz of the wanted side band. (default)
            si4735.setSSBSidebandCutoffFilter(0);
        } else {
            // Low pass filter to cutoff the unwanted side band.
            si4735.setSSBSidebandCutoffFilter(1);
        }

    } else if (currMod == AM) {
        /**
         * @ingroup group08 Set bandwidth
         * @brief Selects the bandwidth of the channel filter for AM reception.
         * @details The choices are 6, 4, 3, 2, 2.5, 1.8, or 1 (kHz). The default bandwidth is 2 kHz. It works only in AM / SSB (LW/MW/SW)
         * @see Si47XX PROGRAMMING GUIDE; AN332 (REV 1.0); pages 125, 151, 277, 181.
         * @param AMCHFLT the choices are:   0 = 6 kHz Bandwidth
         *                                   1 = 4 kHz Bandwidth
         *                                   2 = 3 kHz Bandwidth
         *                                   3 = 2 kHz Bandwidth
         *                                   4 = 1 kHz Bandwidth
         *                                   5 = 1.8 kHz Bandwidth
         *                                   6 = 2.5 kHz Bandwidth, gradual roll off
         *                                   7–15 = Reserved (Do not use).
         * @param AMPLFLT Enables the AM Power Line Noise Rejection Filter.
         */
        si4735.setBandwidth(config.data.bwIdxAM, 0);

    } else if (currMod == FM) {
        /**
         * @brief Sets the Bandwith on FM mode
         * @details Selects bandwidth of channel filter applied at the demodulation stage. Default is automatic which means the device automatically selects proper
         * channel filter. <BR>
         * @details | Filter  | Description |
         * @details | ------- | -------------|
         * @details |    0    | Automatically select proper channel filter (Default) |
         * @details |    1    | Force wide (110 kHz) channel filter |
         * @details |    2    | Force narrow (84 kHz) channel filter |
         * @details |    3    | Force narrower (60 kHz) channel filter |
         * @details |    4    | Force narrowest (40 kHz) channel filter |
         *
         * @param filter_value
         */
        si4735.setFmBandwidth(config.data.bwIdxFM);
    }
}

/**
 * @brief Hangolás a memória állomásra
 * @param bandIndex A band indexe, amelyre hangolunk
 * @param frequency A hangolási frekvencia (Hz)
 * @param demodModIndex A demodulációs mód indexe (FM, AM, LSB, USB, CW)
 * @param bandwidthIndex A sávszélesség indexe
 */
void Si4735Band::tuneMemoryStation(uint8_t bandIndex, uint16_t frequency, uint8_t demodModIndex, uint8_t bandwidthIndex) {

    // 1. Elkérjük a Band táblát
    BandTable &currentBand = getCurrentBand();

    // Band index beállítása a konfigban
    config.data.currentBandIdx = bandIndex;

    // 2. Demodulátor beállítása a chipen.  Ha CW módra
    uint8_t savedMod = demodModIndex; // A demodulációs mód kiemelése

    if (savedMod != CW and rtv::CWShift == true) {
        // TODO: ezt még kidolgozni
        rtv::CWShift = false;
    }

    // Átállítjuk a demodulációs módot
    currentBand.currMod = demodModIndex;

    // 3. Sávszélesség index beállítása a configban a MENTETT érték alapján ---
    uint8_t savedBwIndex = bandwidthIndex;
    if (savedMod == FM) {
        config.data.bwIdxFM = savedBwIndex;
    } else if (savedMod == AM) {
        config.data.bwIdxAM = savedBwIndex;
    } else { // LSB, USB, CW
        config.data.bwIdxSSB = savedBwIndex;
    }

    // 4. Újra beállítjuk a sávot az új móddal (false -> ne a preferált adatokat töltse be)
    this->bandSet(false); // 5. Explicit módon állítsd be a frekvenciát és a módot a chipen
    si4735.setFrequency(frequency);

    // A tényleges frekvenciát olvassuk vissza a chip-ből (lehet, hogy nem pontosan azt állította be, amit kértünk)
    currentBand.currFreq = si4735.getCurrentFrequency();

    // BFO eltolás visszaállítása SSB/CW esetén ---
    if (demodModIndex == LSB || demodModIndex == USB || demodModIndex == CW) {
        const int16_t cwBaseOffset = (demodModIndex == CW) ? config.data.cwReceiverOffsetHz : 0;

        si4735.setSSBBfo(cwBaseOffset);
        rtv::CWShift = (demodModIndex == CW); // CW shift állapot frissítése

    } else {
        // AM/FM esetén biztosítjuk, hogy a BFO nullázva legyen
        rtv::lastBFO = 0;
        rtv::currentBFO = 0;
        rtv::freqDec = 0;
        rtv::CWShift = false;
    }

    // 6. Hangerő visszaállítása
    si4735.setVolume(config.data.currVolume);
}
