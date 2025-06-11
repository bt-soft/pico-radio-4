#include "FMScreen.h"
#include "FreqDisplay.h" // Új include
#include "SMeter.h"
#include "StatusLine.h"            // StatusLine include
#include "UIColorPalette.h"        // TFT_COLOR_BACKGROUND makróhoz
#include "UIHorizontalButtonBar.h" // Új include a vízszintes gombokhoz
#include "UIVerticalButtonBar.h"
#include "rtVars.h" // rtv::muteStat változóhoz

// Függőleges gomb ID konstansai
namespace FMScreenButtonIDs {
static constexpr uint8_t MUTE = 10;
static constexpr uint8_t VOLUME = 11;
static constexpr uint8_t AGC = 12;
static constexpr uint8_t ATT = 13;
static constexpr uint8_t SQUELCH = 14;
static constexpr uint8_t FREQ = 15;
static constexpr uint8_t SETUP = 16;
static constexpr uint8_t MEMO = 17;
} // namespace FMScreenButtonIDs

// Vízszintes gomb ID konstansai
namespace FMScreenHorizontalButtonIDs {
static constexpr uint8_t AM_BUTTON = 20;
static constexpr uint8_t TEST_BUTTON = 21;
static constexpr uint8_t SETUP_BUTTON = 22;
} // namespace FMScreenHorizontalButtonIDs

/**
 * @brief FMScreen konstruktor
 * @param tft TFT display referencia
 * @param si4735Manager Si4735Manager referencia
 *
 */
FMScreen::FMScreen(TFT_eSPI &tft, Si4735Manager &si4735Manager) : UIScreen(tft, SCREEN_NAME_FM, &si4735Manager) {
    si4735Manager.init(); // Si4735 inicializálása
    layoutComponents();
}

/**
 * @brief UI komponensek létrehozása és elhelyezése
 */
void FMScreen::layoutComponents() {

    // Állapotsor komponens létrehozása
    UIScreen::createStatusLine();

    // FreqDisplay komponens
    uint16_t freqDisplayHeight = 38 + 20 + 10; // Szegmens + unit + aláhúzás + margók (becslés)
    uint16_t freqDisplayWidth = 240;           // Becsült szélesség
    uint16_t freqDisplayX = (tft.width() - freqDisplayWidth) / 2;
    uint16_t freqDisplayY = 60;
    Rect freqBounds(freqDisplayX, freqDisplayY, freqDisplayWidth, freqDisplayHeight);
    UIScreen::createFreqDisplay(freqBounds);

    // S-Meter komponens
    uint16_t smeterX = 2;                                     // Kis margó balról
    uint16_t smeterY = freqDisplayY + freqDisplayHeight + 10; // FreqDisplay alatt 10px réssel
    uint16_t smeterWidth = 240;                               // S-Meter szélessége
    uint16_t smeterHeight = 60;                               // S-Meter magassága (becsült)
    Rect smeterBounds(smeterX, smeterY, smeterWidth, smeterHeight);
    ColorScheme smeterColors = ColorScheme::defaultScheme();
    smeterColors.background = TFT_COLOR_BACKGROUND;     // Fekete háttér
    UIScreen::createSMeter(smeterBounds, smeterColors); // Függőleges gombsor létrehozása
    createVerticalButtonBar();

    // Vízszintes gombsor létrehozása
    createHorizontalButtonBar();
}

/**
 * @brief Rotary encoder eseménykezelés felülírása
 * @param event Rotary encoder esemény
 * @return true ha kezelte az eseményt, false egyébként
 */
bool FMScreen::handleRotary(const RotaryEvent &event) {

    // Csak ha nincs aktív dialóg és nem egy rotary klikk hangzott el
    if (!isDialogActive() && event.buttonState != RotaryEvent::ButtonState::Clicked) {

        // Léptetjük (és el is mentjük a bandtable-ba) a frekvenciát a rotary értéke alapján
        pSi4735Manager->stepFrequency(event.value);

        // Frekvencia frissítése a kijelzőn
        if (freqDisplayComp) {
            uint16_t currentRadioFreq = pSi4735Manager->getCurrentBand().currFreq;
            freqDisplayComp->setFrequency(currentRadioFreq);
        }

        return true;
    }

    // Ha nem kezeltük az eseményt, akkor továbbítjuk a UIScreen-nek, ami továbbítja a lehetséges további a gyerek komponenseknek
    return UIScreen::handleRotary(event);
}

/**
 * @brief Loop hívás felülírása animációs vagy egyéb saját logika végrehajtására
 * @note Ez a metódus NEM hívja meg a gyerek komponensek loop-ját, csak saját logikát tartalmaz.
 */
void FMScreen::handleOwnLoop() {

    // S-Meter frissítése
    if (smeterComp) {
        // Si4735Manager-től lekérjük az RSSI és SNR értékeket cache-elt módon
        SignalQualityData signalCache = pSi4735Manager->getSignalQuality();
        if (signalCache.isValid) {
            smeterComp->showRSSI(signalCache.rssi, signalCache.snr, true /* fm mód*/);
        }
    }

    // Függőleges gombállapotok frissítése
    updateVerticalButtonStates();
}

/**
 * @brief Kirajzolja a képernyő saját tartalmát
 */
void FMScreen::drawContent() {
    // S-Meter skála kirajzolása (statikus rész)
    if (smeterComp) {
        smeterComp->drawSmeterScale();
    }
}

/**
 * @brief Függőleges gombsor létrehozása a jobb felső sarokban
 */
void FMScreen::createVerticalButtonBar() {

    // Gombsor pozíciója és mérete - jobb felső sarok
    const uint16_t buttonBarWidth = 65;
    const uint16_t buttonBarX = tft.width() - buttonBarWidth; // Pontosan a jobb szélhez illesztve
    const uint16_t buttonBarY = 0;                            // A legfelső pixeltől kezdve
    const uint16_t buttonBarHeight = tft.height();            // Teljes képernyő magasság

    // Gomb konfiguráció
    std::vector<UIVerticalButtonBar::ButtonConfig> buttonConfigs = {
        {FMScreenButtonIDs::MUTE, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleMuteButton(event); }},

        {FMScreenButtonIDs::VOLUME, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleVolumeButton(event); }},

        {FMScreenButtonIDs::AGC, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleAGCButton(event); }},

        {FMScreenButtonIDs::ATT, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleAttButton(event); }},

        {FMScreenButtonIDs::SQUELCH, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleSquelchButton(event); }},

        {FMScreenButtonIDs::FREQ, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleFreqButton(event); }},

        {FMScreenButtonIDs::SETUP, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
         [this](const UIButton::ButtonEvent &event) { handleSetupButtonVertical(event); }},

        {FMScreenButtonIDs::MEMO, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleMemoButton(event); }}};

    // UIVerticalButtonBar létrehozása
    verticalButtonBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(buttonBarX, buttonBarY, buttonBarWidth, buttonBarHeight), buttonConfigs,
                                                              60, // gomb szélessége
                                                              32, // gomb magassága
                                                              4   // gombok közötti távolság
    );

    // Hozzáadás a képernyőhöz
    addChild(verticalButtonBar);
}

// ================================
// Függőleges gomb eseménykezelők
// ================================

void FMScreen::handleMuteButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::On) {
        DEBUG("FMScreen: Mute ON\n");
        rtv::muteStat = true;
        pSi4735Manager->getSi4735().setAudioMute(true);
    } else if (event.state == UIButton::EventButtonState::Off) {
        DEBUG("FMScreen: Mute OFF\n");
        rtv::muteStat = false;
        pSi4735Manager->getSi4735().setAudioMute(false);
    }
}

void FMScreen::handleVolumeButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Volume adjustment requested\n");
        // TODO: Volume dialog megjelenítése
        // showVolumeDialog();
    }
}

void FMScreen::handleAGCButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::On) {
        DEBUG("FMScreen: AGC ON\n");
        // TODO: AGC bekapcsolása
        // pSi4735Manager->setAGC(true);
    } else if (event.state == UIButton::EventButtonState::Off) {
        DEBUG("FMScreen: AGC OFF\n");
        // TODO: AGC kikapcsolása
        // pSi4735Manager->setAGC(false);
    }
}

void FMScreen::handleAttButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::On) {
        DEBUG("FMScreen: Attenuator ON\n");
        // TODO: Attenuátor bekapcsolása
        // pSi4735Manager->setAttenuator(true);
    } else if (event.state == UIButton::EventButtonState::Off) {
        DEBUG("FMScreen: Attenuator OFF\n");
        // TODO: Attenuátor kikapcsolása
        // pSi4735Manager->setAttenuator(false);
    }
}

void FMScreen::handleSquelchButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Squelch adjustment requested\n");
        // TODO: Squelch beállító dialógus
        // showSquelchDialog();
    }
}

void FMScreen::handleFreqButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Frequency input requested\n");
        // TODO: Frekvencia input dialógus
        // showFrequencyInputDialog();
    }
}

void FMScreen::handleSetupButtonVertical(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Switching to Setup screen (from vertical button)\n");
        UIScreen::getManager()->switchToScreen(SCREEN_NAME_SETUP);
    }
}

void FMScreen::handleMemoButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Memory functions requested\n");
        // TODO: Memória funkciók (save/recall állomások)
        // showMemoryDialog();
    }
}

void FMScreen::updateVerticalButtonStates() {
    if (!verticalButtonBar) {
        return;
    }

    // Mute gomb állapot szinkronizálása
    verticalButtonBar->setButtonState(FMScreenButtonIDs::MUTE, rtv::muteStat ? UIButton::ButtonState::On : UIButton::ButtonState::Off);

    // AGC gomb állapot szinkronizálása
    // TODO: Ha van AGC állapot lekérdezés
    // bool agcEnabled = pSi4735Manager->isAGCEnabled();
    // verticalButtonBar->setButtonState(FMScreenButtonIDs::AGC,
    //                                  agcEnabled ? UIButton::ButtonState::On : UIButton::ButtonState::Off);

    // További gombállapotok szinkronizálása...
}

/**
 * @brief Vízszintes gombsor létrehozása a bal alsó sarokban
 */
void FMScreen::createHorizontalButtonBar() {
    // Gombsor pozíciója - bal alsó sarok
    const uint16_t buttonBarHeight = 35;
    const uint16_t buttonBarX = 0;                              // Bal szélhez illesztve
    const uint16_t buttonBarY = tft.height() - buttonBarHeight; // Alsó szélhez illesztve
    const uint16_t buttonBarWidth = 220;                        // 3 gomb + margók számára    // Vízszintes gomb konfiguráció
    std::vector<UIHorizontalButtonBar::ButtonConfig> buttonConfigs = {{FMScreenHorizontalButtonIDs::AM_BUTTON, "AM", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                                       [this](const UIButton::ButtonEvent &event) { handleAMButton(event); }},

                                                                      {FMScreenHorizontalButtonIDs::TEST_BUTTON, "Test", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                                       [this](const UIButton::ButtonEvent &event) { handleTestButton(event); }},

                                                                      {FMScreenHorizontalButtonIDs::SETUP_BUTTON, "Setup", UIButton::ButtonType::Pushable,
                                                                       UIButton::ButtonState::Off,
                                                                       [this](const UIButton::ButtonEvent &event) { handleSetupButtonHorizontal(event); }}};

    // UIHorizontalButtonBar létrehozása
    horizontalButtonBar = std::make_shared<UIHorizontalButtonBar>(tft, Rect(buttonBarX, buttonBarY, buttonBarWidth, buttonBarHeight), buttonConfigs,
                                                                  70, // gomb szélessége
                                                                  30, // gomb magassága
                                                                  3   // gombok közötti távolság
    );

    // Hozzáadás a képernyőhöz
    addChild(horizontalButtonBar);
}

// ================================
// Vízszintes gomb eseménykezelők
// ================================

void FMScreen::handleAMButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Switching to AM screen\n");
        UIScreen::getManager()->switchToScreen(SCREEN_NAME_AM);
    }
}

void FMScreen::handleTestButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Switching to Test screen\n");
        UIScreen::getManager()->switchToScreen(SCREEN_NAME_TEST);
    }
}

void FMScreen::handleSetupButtonHorizontal(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        DEBUG("FMScreen: Switching to Setup screen (from horizontal button)\n");
        UIScreen::getManager()->switchToScreen(SCREEN_NAME_SETUP);
    }
}