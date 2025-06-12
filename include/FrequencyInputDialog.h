/**
 * @file FrequencyInputDialog.h
 * @brief Frekvencia beviteli dialógus osztály definíciója
 *
 * A FrequencyInputDialog egy speciális dialógus frekvencia bevitelhez,
 * amely FreqDisplay-hez hasonló 7-szegmenses fontos használ.
 * Sávtól függően kHz vagy MHz egységben működik.
 *
 * Főbb funkciók:
 * - Sávspecifikus frekvencia formátum (FM: MHz, MW/LW: kHz, SW: MHz)
 * - Numerikus beviteli billentyűzet
 * - Digitenkénti szerkesztés és törlés
 * - DSEG7 7-szegmenses font megjelenítés
 * - Valós idejű validáció
 * - OK gomb csak valid frekvencia esetén aktiválódik
 *
 * @version 1.0
 * @date 2025.12.06
 */

#ifndef __FREQUENCY_INPUT_DIALOG_H
#define __FREQUENCY_INPUT_DIALOG_H

#include <TFT_eSPI.h>

#include "DSEG7_Classic_Mini_Regular_34.h"
#include "MessageDialog.h"
#include "Si4735Manager.h"
#include <functional>

/**
 * @class FrequencyInputDialog
 * @brief Speciális frekvencia beviteli dialógus
 *
 * Ezt a dialógust frekvencia bevitelhez használjuk sávspecifikus formátummal.
 * A FreqDisplay komponenshez hasonló 7-szegmenses megjelenítést biztosít.
 */
class FrequencyInputDialog : public MessageDialog {

  public:
    /**
     * @brief Frekvencia változás callback típus
     * @param newFrequency Az új frekvencia érték (nyers formátum: FM: x100, MW/LW/SW: x1)
     */
    using FrequencyChangeCallback = std::function<void(uint16_t newFrequency)>;

  protected:
    // === Frekvencia kezelés ===
    Si4735Manager *_si4735Manager; ///< Si4735Manager referencia
    uint8_t _currentBandType;      ///< Aktuális sáv típusa (FM_BAND_TYPE, MW_BAND_TYPE, stb.)
    uint16_t _minFreq, _maxFreq;   ///< Frekvencia határoló (sáv minimum/maximum)
    String _inputString;           ///< Aktuális bevitt frekvencia string
    String _unitString;            ///< Egység string ("MHz" vagy "kHz")
    String _maskString;            ///< 7-szegmenses maszk pattern
    bool _isValid;                 ///< Az aktuális frekvencia valid-e

    // === Callback ===
    FrequencyChangeCallback _frequencyCallback; ///< Frekvencia változás callback

    // === UI komponensek ===
    std::vector<std::shared_ptr<UIButton>> _digitButtons; ///< Numerikus gombok (0-9)
    std::shared_ptr<UIButton> _dotButton;                 ///< Tizedes pont gomb (FM/SW-hez)
    std::shared_ptr<UIButton> _clearButton;               ///< Egy digit törlés gomb
    std::shared_ptr<UIButton> _clearAllButton;            ///< Minden törlés gomb

    // === Layout konstansok ===
    static constexpr uint16_t DISPLAY_AREA_HEIGHT = 60;    ///< Frekvencia kijelző terület magassága
    static constexpr uint16_t BUTTON_AREA_HEIGHT = 160;    ///< Gombsor terület magassága
    static constexpr uint16_t NUMERIC_BUTTON_SIZE = 35;    ///< Numerikus gombok mérete
    static constexpr uint16_t FUNCTION_BUTTON_WIDTH = 50;  ///< Funkció gombok szélessége
    static constexpr uint16_t FUNCTION_BUTTON_HEIGHT = 30; ///< Funkció gombok magassága
    static constexpr uint16_t BUTTON_SPACING = 5;          ///< Gombok közötti távolság
    static constexpr uint16_t FREQ_DISPLAY_FONT_SIZE = 3;  ///< 7-szegmenses font méret

    /**
     * @brief Dialógus tartalom létrehozása
     */
    virtual void createDialogContent() override;

    /**
     * @brief Dialógus tartalom elrendezése
     */
    virtual void layoutDialogContent() override;

    /**
     * @brief Saját tartalom rajzolása (frekvencia kijelző + üzenet)
     */
    virtual void drawSelf() override;

    /**
     * @brief Rotary encoder kezelés
     */
    virtual bool handleRotary(const RotaryEvent &event) override;

  private:
    /**
     * @brief Sáv paraméterek inicializálása
     */
    void initializeBandParameters();

    /**
     * @brief Numerikus gombok létrehozása
     */
    void createNumericButtons();

    /**
     * @brief Funkció gombok létrehozása (pont, törlés)
     */
    void createFunctionButtons();

    /**
     * @brief Frekvencia kijelző rajzolása 7-szegmenses fonttal
     */
    void drawFrequencyDisplay();

    /**
     * @brief Numerikus gomb megnyomás kezelése
     * @param digit A megnyomott számjegy (0-9)
     */
    void handleDigitInput(uint8_t digit);

    /**
     * @brief Tizedes pont bevitel kezelése
     */
    void handleDotInput();

    /**
     * @brief Egy digit törlése (Backspace)
     */
    void handleClearDigit();

    /**
     * @brief Minden digit törlése
     */
    void handleClearAll();

    /**
     * @brief Frekvencia string validálása és parsing
     * @return true ha valid frekvencia
     */
    bool validateAndParseFrequency();

    /**
     * @brief Frekvencia string-ből nyers érték kiszámolása
     * @return Nyers frekvencia érték (Si4735-hez)
     */
    uint16_t calculateRawFrequency() const;

    /**
     * @brief Sáv határokon belül van-e a frekvencia
     * @param rawFreq A nyers frekvencia érték
     * @return true ha határokon belül van
     */
    bool isFrequencyInBounds(uint16_t rawFreq) const;

    /**
     * @brief OK gomb állapot frissítése (valid frekvencia esetén engedélyezett)
     */
    void updateOkButtonState();

    /**
     * @brief Frekvencia kijelző terület frissítése
     */
    void updateFrequencyDisplay();

    /**
     * @brief Aktuális frekvencia megszerzése string formátumban
     */
    String getCurrentFrequencyString() const;

    /**
     * @brief Maszk pattern generálása a sáv típus alapján
     */
    void generateMaskPattern();

  public:
    /**
     * @brief Konstruktor
     * @param parentScreen Szülő képernyő
     * @param tft TFT meghajtó referencia
     * @param title Dialógus címe
     * @param message Magyarázó szöveg
     * @param si4735Manager Si4735Manager referencia
     * @param callback Frekvencia változás callback
     * @param userDialogCb Dialógus lezárásakor hívandó callback
     * @param bounds Dialógus mérete és pozíciója
     * @param cs Színséma
     */
    FrequencyInputDialog(UIScreen *parentScreen, TFT_eSPI &tft, const Rect &bounds, const char *title, const char *message, Si4735Manager *si4735Manager,
                         FrequencyChangeCallback callback = nullptr, const ColorScheme &cs = ColorScheme::defaultScheme());

    /**
     * @brief Virtuális destruktor
     */
    virtual ~FrequencyInputDialog() = default; /**
                                                * @brief OK gomb megnyomás kezelése - frekvencia beállítása
                                                */
    void onOkClicked();

    /**
     * @brief Cancel gomb megnyomás kezelése
     */
    void onCancelClicked();

    /**
     * @brief Aktuális frekvencia beállítása (inicializáláshoz)
     * @param rawFrequency A nyers frekvencia érték
     */
    void setCurrentFrequency(uint16_t rawFrequency);
};

#endif // __FREQUENCY_INPUT_DIALOG_H
