#include "ScreenManager.h"
#include "AMScreen.h"
#include "Band.h"
#include "Config.h"
#include "EmptyScreen.h"
#include "FMScreen.h"
#include "MemoryScreen.h"
#include "ScreenSaverScreen.h"
#include "SetupDecodersScreen.h"
#include "SetupScreen.h"
#include "SetupSi4735Screen.h"
#include "SetupSystemScreen.h"
#include "TestScreen.h"

#include "Si4735Manager.h"
extern Si4735Manager *si4735Manager;

/**
 * @brief Képernyőkezelő osztály konstruktor
 */
void ScreenManager::registerDefaultScreenFactories() {
    registerScreenFactory(SCREEN_NAME_FM, [](TFT_eSPI &tft_param) { return std::make_shared<FMScreen>(tft_param, *si4735Manager); });
    registerScreenFactory(SCREEN_NAME_AM, [](TFT_eSPI &tft_param) { return std::make_shared<AMScreen>(tft_param, *si4735Manager); });
    registerScreenFactory(SCREEN_NAME_MEMORY, [](TFT_eSPI &tft_param) { return std::make_shared<MemoryScreen>(tft_param, *si4735Manager); });
    registerScreenFactory(SCREEN_NAME_SCREENSAVER,
                          [](TFT_eSPI &tft_param) { return std::make_shared<ScreenSaverScreen>(tft_param, *si4735Manager); }); // setup képernyők regisztrálása
    registerScreenFactory(SCREEN_NAME_SETUP, [](TFT_eSPI &tft_param) { return std::make_shared<SetupScreen>(tft_param); });
    registerScreenFactory("SETUP_SYSTEM", [](TFT_eSPI &tft_param) { return std::make_shared<SetupSystemScreen>(tft_param); });
    registerScreenFactory("SETUP_SI4735", [](TFT_eSPI &tft_param) { return std::make_shared<SetupSi4735Screen>(tft_param); });
    registerScreenFactory("SETUP_DECODERS", [](TFT_eSPI &tft_param) { return std::make_shared<SetupDecodersScreen>(tft_param); });

    // test képernyők regisztrálása
    registerScreenFactory(SCREEN_NAME_TEST, [](TFT_eSPI &tft_param) { return std::make_shared<TestScreen>(tft_param); });
    registerScreenFactory(SCREEN_NAME_EMPTY, [](TFT_eSPI &tft_param) { return std::make_shared<EmptyScreen>(tft_param); });
}
