#ifndef __SMETER_H
#define __SMETER_H

#include "defines.h" // Színekhez (TFT_BLACK, stb.)
#include <TFT_eSPI.h>
#include <algorithm> // std::min miatt

namespace SMeterConstants {
// Skála méretei és pozíciója
constexpr uint8_t ScaleWidth = 236;
constexpr uint8_t ScaleHeight = 46;
constexpr uint8_t ScaleStartXOffset = 2;
constexpr uint8_t ScaleStartYOffset = 6;
constexpr uint8_t ScaleEndXOffset = ScaleStartXOffset + ScaleWidth;
constexpr uint8_t ScaleEndYOffset = ScaleStartYOffset + ScaleHeight;

// S-Pont skála rajzolása
constexpr uint8_t SPointStartX = 15;
constexpr uint8_t SPointY = 24;
constexpr uint8_t SPointTickWidth = 2;
constexpr uint8_t SPointTickHeight = 8;
constexpr uint8_t SPointNumberY = 13;
constexpr uint8_t SPointSpacing = 12;
constexpr uint8_t SPointCount = 10; // 0-9

// Plusz skála rajzolása
constexpr uint8_t PlusScaleStartX = 123;
constexpr uint8_t PlusScaleY = 24;
constexpr uint8_t PlusScaleTickWidth = 3;
constexpr uint8_t PlusScaleTickHeight = 8;
constexpr uint8_t PlusScaleNumberY = 13;
constexpr uint8_t PlusScaleSpacing = 16;
constexpr uint8_t PlusScaleCount = 6; // +10-től +60-ig

// Skála sávok rajzolása
constexpr uint8_t SBarY = 32;
constexpr uint8_t SBarHeight = 3;
constexpr uint8_t SBarSPointWidth = 112;
constexpr uint8_t SBarPlusStartX = 127;
constexpr uint8_t SBarPlusWidth = 100;

// Mérősáv rajzolása
constexpr uint8_t MeterBarY = 38;
constexpr uint8_t MeterBarHeight = 6;

constexpr uint8_t MeterBarRedStartX = 15; // Piros S0 sáv kezdete
constexpr uint8_t MeterBarRedWidth = 15;  // Piros S0 sáv szélessége

// Az S1 (első narancs) az S0 (piros) után 2px réssel kezdődik
constexpr uint8_t MeterBarOrangeStartX = MeterBarRedStartX + MeterBarRedWidth + 2; // 15 + 15 + 2 = 32
constexpr uint8_t MeterBarOrangeWidth = 10;                                        // Narancs S1-S8 sávok szélessége
constexpr uint8_t MeterBarOrangeSpacing = 12;                                      // Narancs sávok kezdőpontjai közötti távolság (10px sáv + 2px rés)

// Az S9+10dB (első zöld) az S8 (utolsó narancs) után 2px réssel kezdődik
// S8 vége: MeterBarOrangeStartX + (7 * MeterBarOrangeSpacing) + MeterBarOrangeWidth = 32 + 84 + 10 = 126
constexpr uint8_t MeterBarGreenStartX = MeterBarOrangeStartX + ((8 - 1) * MeterBarOrangeSpacing) + MeterBarOrangeWidth + 2; // 126 + 2 = 128
constexpr uint8_t MeterBarGreenWidth = 14;                                                                                  // Zöld S9+dB sávok szélessége
constexpr uint8_t MeterBarGreenSpacing = 16; // Zöld sávok kezdőpontjai közötti távolság (14px sáv + 2px rés)

constexpr uint8_t MeterBarFinalOrangeStartX = MeterBarGreenStartX + ((6 - 1) * MeterBarGreenSpacing) + MeterBarGreenWidth +
                                              2; // Utolsó narancs sáv (S9+60dB felett)
                                                 // S9+60dB (6. zöld sáv) vége: 128 + (5*16) + 14 = 128 + 80 + 14 = 222. Utána 2px rés. -> 222 + 2 = 224
constexpr uint8_t MeterBarFinalOrangeWidth = 3;  // Ennek a sávnak a szélessége

constexpr uint8_t MeterBarMaxPixelValue = 208;                  // A teljes mérősáv hossza pixelben, az rssiConverter max kimenete
constexpr uint8_t MeterBarSPointLimit = 9;                      // S-pontok száma (S0-S8), azaz 9 sáv (1 piros, 8 narancs)
constexpr uint8_t MeterBarTotalLimit = MeterBarSPointLimit + 6; // Összes sáv (S0-S8 és 6db S9+dB), azaz 9 + 6 = 15 sáv

// Szöveges címkék rajzolása
constexpr uint8_t RssiLabelXOffset = 10;
constexpr uint8_t SignalLabelYOffsetInFM = 60; // FM módban a felirat Y pozíciója (ha máshova kerülne)

// Kezdeti állapot a prev_spoint-hoz
constexpr uint8_t InitialPrevSpoint = 0xFF; // Érvénytelen érték, hogy az első frissítés biztosan megtörténjen
} // namespace SMeterConstants

/**
 * SMeter osztály az S-Meter kezelésére
 */
class SMeter {
  private:
    TFT_eSPI &tft;
    uint32_t smeterX;           // S-Meter komponens bal felső sarkának X koordinátája
    uint32_t smeterY;           // S-Meter komponens bal felső sarkának Y koordinátája
    uint8_t prev_spoint_bars;   // Előző S-pont érték a grafikus sávokhoz
    uint8_t prev_rssi_for_text; // Előző RSSI érték a szöveges kiíráshoz
    uint8_t prev_snr_for_text;  // Előző SNR érték a szöveges kiíráshoz

    // Pozíciók és méretek a szöveges RSSI/SNR értékekhez
    uint16_t rssi_label_x_pos;
    uint16_t rssi_value_x_pos;
    uint16_t rssi_value_max_w;
    uint16_t snr_label_x_pos;
    uint16_t snr_value_x_pos;
    uint16_t snr_value_max_w;
    uint16_t text_y_pos;
    uint8_t text_h;

    /**
     * RSSI érték konvertálása S-pont értékre (pixelben).
     * @param rssi Bemenő RSSI érték (0-127 dBuV).
     * @param isFMMode Igaz, ha FM módban vagyunk, hamis AM/SSB/CW esetén.
     * @return A jelerősség pixelben (0-MeterBarMaxPixelValue).
     */
    uint8_t rssiConverter(uint8_t rssi, bool isFMMode);

    /**
     * S-Meter grafikus sávjainak kirajzolása a mért RSSI alapján.
     * @param rssi Aktuális RSSI érték.
     * @param isFMMode Igaz, ha FM módban vagyunk.
     */
    void drawMeterBars(uint8_t rssi, bool isFMMode);

  public:
    /**
     * Konstruktor.
     * @param tft Referencia a TFT kijelző objektumra.
     * @param smeterX Az S-Meter komponens bal felső sarkának X koordinátája.
     * @param smeterY Az S-Meter komponens bal felső sarkának Y koordinátája.
     */
    SMeter(TFT_eSPI &tft, uint8_t smeterX, uint8_t smeterY);

    /**
     * S-Meter skála kirajzolása (a statikus részek: vonalak, számok).
     * Ezt általában egyszer kell meghívni a képernyő inicializálásakor.
     */
    void drawSmeterScale();

    /**
     * S-Meter érték és RSSI/SNR szöveg megjelenítése.
     * @param rssi Aktuális RSSI érték (0–127 dBμV).
     * @param snr Aktuális SNR érték (0–127 dB).
     * @param isFMMode Igaz, ha FM módban vagyunk, hamis egyébként (AM/SSB/CW).
     */
    void showRSSI(uint8_t rssi, uint8_t snr, bool isFMMode);
};

#endif
