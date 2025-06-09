/**
 * @file UIColorPalette.cpp
 * @brief UI színpaletta implementáció
 *
 * Ez a fájl tartalmazza a UIColorPalette osztály metódusainak implementációját,
 * amely centralizált színkezelést biztosít a felhasználói felület komponenseihez.
 */

#include "UIColorPalette.h"
#include "FreqDisplay.h" // FreqSegmentColors struktúra definíciójához

/**
 * @brief FreqSegmentColors létrehozása normál (SSB/CW) módhoz
 * @return Normál mód színkonfigurációja
 */
FreqSegmentColors UIColorPalette::createNormalFreqColors() { return {FREQ_NORMAL_ACTIVE, FREQ_NORMAL_INACTIVE, FREQ_NORMAL_INDICATOR}; }

/**
 * @brief FreqSegmentColors létrehozása BFO módhoz
 * @return BFO mód színkonfigurációja
 */
FreqSegmentColors UIColorPalette::createBfoFreqColors() { return {FREQ_BFO_ACTIVE, FREQ_BFO_INACTIVE, FREQ_BFO_INDICATOR}; }

/**
 * @brief FreqSegmentColors létrehozása képernyővédő módhoz (kék színséma)
 * @return Képernyővédő mód színkonfigurációja
 */
FreqSegmentColors UIColorPalette::createScreenSaverFreqColors() { return {FREQ_SCREENSAVER_ACTIVE, FREQ_SCREENSAVER_INACTIVE, FREQ_SCREENSAVER_INDICATOR}; }
