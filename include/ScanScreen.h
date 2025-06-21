/**
 * @file ScanScreen.h
 * @brief Spektrum analizátor scan képernyő
 * @details Grafikus frekvencia pásztázás RSSI/SNR megjelenítéssel
 */

#ifndef __SCANSCREEN_H
#define __SCANSCREEN_H

#include "Config.h"
#include "Si4735Manager.h"
#include "UIButton.h"
#include "UIHorizontalButtonBar.h"
#include "UIScreen.h"
#include <memory>
#include <vector>

// Képernyő azonosító
#define SCREEN_NAME_SCAN "ScanScreen"

/**
 * @brief Scan képernyő állapotok
 */
enum class ScanState {
    Idle,     ///< Nincs aktív scan
    Scanning, ///< Aktívan pásztáz
    Paused    ///< Szüneteltetve
};

/**
 * @brief Scan módok
 */
enum class ScanMode {
    Spectrum, ///< Spektrum analizátor mód
    Seek,     ///< Gyors seek-based pásztázás
    Memory    ///< Mentett állomások pásztázása
};

/**
 * @brief Spektrum analizátor scan képernyő
 * @details Grafikus frekvencia pásztázás valós idejű RSSI/SNR megjelenítéssel
 */
class ScanScreen : public UIScreen {
  public:
    /**
     * @brief ScanScreen konstruktor
     * @param tft TFT display referencia
     * @param si4735Manager Si4735Manager referencia
     */
    ScanScreen(TFT_eSPI &tft, Si4735Manager *si4735Manager);

    /**
     * @brief Destruktor
     */
    virtual ~ScanScreen() = default;

    // UIScreen interface implementáció
    void activate() override;
    void deactivate() override;
    void drawContent() override;
    void handleOwnLoop() override;
    bool handleTouch(const TouchEvent &event) override;
    bool handleRotary(const RotaryEvent &event) override;

  private:                                                // === Scan paraméterek ===
    static constexpr uint16_t SPECTRUM_WIDTH = 310;       ///< Spektrum grafikon szélessége (pixel) - teljes képernyő
    static constexpr uint16_t SPECTRUM_HEIGHT = 120;      ///< Spektrum grafikon magassága (pixel)
    static constexpr uint16_t SPECTRUM_X = 5;             ///< Spektrum grafikon X pozíciója (5px bal margó)
    static constexpr uint16_t SPECTRUM_Y = 80;            ///< Spektrum grafikon Y pozíciója
    static constexpr uint16_t CURSOR_COLOR = TFT_RED;     ///< Kurzor színe
    static constexpr uint16_t RSSI_COLOR = TFT_WHITE;     ///< RSSI vonal színe (fehér, jól látható)
    static constexpr uint16_t SCALE_COLOR = TFT_DARKGREY; ///< Skála vonalak színe
    static constexpr uint16_t MARK_COLOR = TFT_YELLOW;    ///< Erős állomás jelölés színe
    static constexpr uint8_t SIGNAL_SAMPLES = 3;          ///< Jelerősség mérési minták száma    // === Állapot változók ===
    ScanState currentState;                               ///< Jelenlegi scan állapot
    ScanMode currentMode;                                 ///< Jelenlegi scan mód
    uint32_t scanStartFreq;                               ///< Scan kezdő frekvencia (kHz)
    uint32_t scanEndFreq;                                 ///< Scan vég frekvencia (kHz)
    uint32_t currentScanFreq;                             ///< Aktuális scan frekvencia (kHz)
    uint16_t scanStep;                                    ///< Scan lépésköz (kHz)
    uint16_t currentScanLine;                             ///< Aktuális pozíció a spektrumban (0-319)
    uint16_t previousScanLine;                            ///< Előző kurzor pozíció törléshez
    int16_t deltaLine;                                    ///< Eltolás a spektrumban
    uint32_t lastScanUpdate;                              ///< Utolsó scan frissítés ideje
    uint16_t scanSpeed;                                   ///< Scan sebesség (ms/lépés)

    // === Spektrum adatok ===
    std::vector<uint8_t> spectrumRSSI; ///< RSSI értékek (0-319)
    std::vector<uint8_t> spectrumSNR;  ///< SNR értékek (0-319)
    std::vector<bool> stationMarks;    ///< Erős állomás jelölések
    std::vector<uint8_t> scaleLines;   ///< Skála vonalak típusai
    uint8_t snrThreshold;              ///< SNR küszöb erős állomásokhoz

    // === UI komponensek ===
    std::shared_ptr<UIHorizontalButtonBar> scanButtonBar; ///< Scan gombok

    // === Scan kontroll metódusok ===
    void startSpectruScan();
    void pauseScan();
    void stopScan();
    void resumeScan();

    // === Spektrum kezelés ===
    void updateSpectrum();
    void measureSignalAtCurrentFreq();
    uint8_t getSignalValue(bool useRSSI);
    void moveToNextFrequency();
    bool isValidScanFrequency(uint32_t freq); // === Grafikus megjelenítés ===
    void drawSpectrumDisplay();
    void drawSpectrumLine(uint16_t lineIndex);
    void drawCursor();
    void clearCursor();
    void drawFrequencyScale();
    void drawFrequencyLabels();
    void drawSpectrumBackground();
    void drawSignalInfo();
    void drawScanStatus();

    // === Érintés kezelés ===
    void handleSpectrumTouch(uint16_t x, uint16_t y);
    uint32_t pixelToFrequency(uint16_t pixelX);
    uint16_t frequencyToPixel(uint32_t frequency);

    // === Gomb események ===
    void createScanButtons();
    void handleStartStopButton(const UIButton::ButtonEvent &event);
    void handlePauseButton(const UIButton::ButtonEvent &event);
    void handleModeButton(const UIButton::ButtonEvent &event);
    void handleSpeedButton(const UIButton::ButtonEvent &event);
    void handleBackButton(const UIButton::ButtonEvent &event);

    // === Utility metódusok ===
    void calculateScanParameters();
    void resetSpectrumData();
    uint16_t rssiToPixelY(uint8_t rssi);
    uint16_t snrToColor(uint8_t snr);
    String formatFrequency(uint32_t frequency);
    void updateScanButtonStates();

    // === Band információ ===
    uint32_t getBandMinFreq();
    uint32_t getBandMaxFreq();
    String getBandName();
};

// Gomb azonosítók
namespace ScanButtonIDs {
constexpr uint8_t START_STOP_BUTTON = 1;
constexpr uint8_t PAUSE_BUTTON = 2;
constexpr uint8_t MODE_BUTTON = 3;
constexpr uint8_t SPEED_BUTTON = 4;
constexpr uint8_t BACK_BUTTON = 5;
} // namespace ScanButtonIDs

#endif // __SCANSCREEN_H
