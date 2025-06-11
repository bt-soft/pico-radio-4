/**
 * @file RefactoredScreenExample.cpp
 * @brief Példa a közös gombkezelő használatára
 * @details Megmutatja, hogyan egyszerűsödne az AM és FM képernyők implementációja
 */

#include "AMScreen.h"
#include "CommonVerticalButtonHandlers.h"
#include "FMScreen.h"

// =====================================================================
// REFACTORED AMScreen Implementation - Közös kezelőkkel
// =====================================================================

/**
 * @brief AMScreen gombsor létrehozása - közös kezelőkkel
 */
void AMScreen::createVerticalButtonBarRefactored() {
    const uint16_t buttonBarWidth = 65;
    const uint16_t buttonBarX = tft.width() - buttonBarWidth;
    const uint16_t buttonBarY = 0;
    const uint16_t buttonBarHeight = tft.height();

    // ===================================================================
    // KÖZÖS GOMBKEZELŐ HASZNÁLATA - Nincs kód duplikáció!
    // ===================================================================
    std::vector<UIVerticalButtonBar::ButtonConfig> configs = {// Közös kezelők használata - DRY principle
                                                              {AMScreenButtonIDs::MUTE, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonVerticalButtonHandlers::handleMuteButton(e, pSi4735Manager); }},

                                                              {AMScreenButtonIDs::VOLUME, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonVerticalButtonHandlers::handleVolumeButton(e, pSi4735Manager); }},

                                                              {AMScreenButtonIDs::AGC, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonVerticalButtonHandlers::handleAGCButton(e, pSi4735Manager); }},

                                                              {AMScreenButtonIDs::ATT, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonVerticalButtonHandlers::handleAttenuatorButton(e, pSi4735Manager); }},

                                                              {AMScreenButtonIDs::SQUELCH, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonVerticalButtonHandlers::handleSquelchButton(e, pSi4735Manager); }},

                                                              {AMScreenButtonIDs::FREQ, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonVerticalButtonHandlers::handleFrequencyButton(e, pSi4735Manager); }},

                                                              {AMScreenButtonIDs::SETUP, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonVerticalButtonHandlers::handleSetupButton(e, getManager()); }},

                                                              {AMScreenButtonIDs::MEMO, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonVerticalButtonHandlers::handleMemoryButton(e, pSi4735Manager); }}};

    verticalButtonBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(buttonBarX, buttonBarY, buttonBarWidth, buttonBarHeight), configs, 60, 32, 4);
    addChild(verticalButtonBar);
}

/**
 * @brief AMScreen gombállapot szinkronizálás - közös kezelőkkel
 */
void AMScreen::updateVerticalButtonStatesRefactored() {
    // ===================================================================
    // EGYETLEN METÓDUS HÍVÁS - Nincs kód duplikáció!
    // ===================================================================
    CommonVerticalButtonHandlers::updateAllButtonStates(verticalButtonBar.get(), AMScreenButtonIDs{}, pSi4735Manager, getManager());
}

// =====================================================================
// REFACTORED FMScreen Implementation - Azonos egyszerűség!
// =====================================================================

/**
 * @brief FMScreen gombsor létrehozása - közös kezelőkkel
 */
void FMScreen::createVerticalButtonBarRefactored() {
    const uint16_t buttonBarWidth = 65;
    const uint16_t buttonBarX = tft.width() - buttonBarWidth;
    const uint16_t buttonBarY = 0;
    const uint16_t buttonBarHeight = tft.height();

    // ===================================================================
    // UGYANAZ A LOGIKA, MINT AM-BEN - Maximális kód újrafelhasználás!
    // ===================================================================
    std::vector<UIVerticalButtonBar::ButtonConfig> configs = {{FMScreenButtonIDs::MUTE, "Mute", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonVerticalButtonHandlers::handleMuteButton(e, pSi4735Manager); }},

                                                              {FMScreenButtonIDs::VOLUME, "Vol", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonVerticalButtonHandlers::handleVolumeButton(e, pSi4735Manager); }},

                                                              {FMScreenButtonIDs::AGC, "AGC", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonVerticalButtonHandlers::handleAGCButton(e, pSi4735Manager); }},

                                                              {FMScreenButtonIDs::ATT, "Att", UIButton::ButtonType::Toggleable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonVerticalButtonHandlers::handleAttenuatorButton(e, pSi4735Manager); }},

                                                              {FMScreenButtonIDs::SQUELCH, "Sql", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonVerticalButtonHandlers::handleSquelchButton(e, pSi4735Manager); }},

                                                              {FMScreenButtonIDs::FREQ, "Freq", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonVerticalButtonHandlers::handleFrequencyButton(e, pSi4735Manager); }},

                                                              {FMScreenButtonIDs::SETUP, "Setup", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonVerticalButtonHandlers::handleSetupButton(e, getManager()); }},

                                                              {FMScreenButtonIDs::MEMO, "Memo", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                                               [this](const UIButton::ButtonEvent &e) { CommonVerticalButtonHandlers::handleMemoryButton(e, pSi4735Manager); }}};

    verticalButtonBar = std::make_shared<UIVerticalButtonBar>(tft, Rect(buttonBarX, buttonBarY, buttonBarWidth, buttonBarHeight), configs, 60, 32, 4);
    addChild(verticalButtonBar);
}

/**
 * @brief FMScreen gombállapot szinkronizálás - közös kezelőkkel
 */
void FMScreen::updateVerticalButtonStatesRefactored() {
    // ===================================================================
    // UGYANAZ A METÓDUS, MINT AM-BEN - Tökéletes DRY!
    // ===================================================================
    CommonVerticalButtonHandlers::updateAllButtonStates(verticalButtonBar.get(), FMScreenButtonIDs{}, pSi4735Manager, getManager());
}

// =====================================================================
// EREDMÉNY ÖSSZEHASONLÍTÁS
// =====================================================================

/*
ELŐTTE - Kód duplikáció:
========================

AMScreen.cpp:
- handleMuteButton()      : 12 sor
- handleVolumeButton()    : 6 sor
- handleAGCButton()       : 8 sor
- handleAttButton()       : 8 sor
- handleSquelchButton()   : 6 sor
- handleFreqButton()      : 6 sor
- handleSetupButton()     : 6 sor
- handleMemoButton()      : 6 sor
- updateVerticalButtonStates() : 15 sor
ÖSSZESEN: ~73 sor kód

FMScreen.cpp:
- handleMuteButton()      : 12 sor
- handleVolumeButton()    : 6 sor
- handleAGCButton()       : 8 sor
- handleAttButton()       : 8 sor
- handleSquelchButton()   : 6 sor
- handleFreqButton()      : 6 sor
- handleSetupButton()     : 6 sor
- handleMemoButton()      : 6 sor
- updateVerticalButtonStates() : 15 sor
ÖSSZESEN: ~73 sor kód

TELJES DUPLIKÁCIÓ: ~146 sor !!!

UTÁNA - Közös kezelő:
====================

CommonVerticalButtonHandlers.h:
- Összes handleXXX() metódus : ~80 sor (dokumentációval)
- Állapot szinkronizáló metódusok : ~30 sor

AMScreen.cpp:
- createVerticalButtonBarRefactored() : ~25 sor (lambda-k)
- updateVerticalButtonStatesRefactored() : 7 sor

FMScreen.cpp:
- createVerticalButtonBarRefactored() : ~25 sor (lambda-k)
- updateVerticalButtonStatesRefactored() : 7 sor

ÖSSZESEN: ~147 sor, DE:
- ✅ NINCS DUPLIKÁCIÓ
- ✅ Egy helyen karbantartható logika
- ✅ Band-független implementáció
- ✅ Könnyen bővíthető új képernyőkkel (SW, LW, stb.)
- ✅ Automatikusan konzisztens viselkedés
*/
