#include "ScreenManager.h"
#include "Band.h"
#include "Config.h"
#include "EmptyScreen.h"
#include "FMScreen.h"
#include "ScreenSaverScreen.h"
#include "SetupDecodersScreen.h"
#include "SetupDisplayScreen.h"
#include "SetupScreen.h"
#include "SetupSi4735Screen.h"
#include "TestScreen.h"

extern Band band;     // Assuming global instance
extern Config config; // Assuming global instance

/**
 * @brief Képernyőkezelő osztály konstruktor
 */
void ScreenManager::registerDefaultScreenFactories() {
    registerScreenFactory(SCREEN_NAME_FM, [](TFT_eSPI &tft_param) { return std::make_shared<FMScreen>(tft_param); });
    registerScreenFactory(SCREEN_NAME_TEST, [](TFT_eSPI &tft_param) { return std::make_shared<TestScreen>(tft_param); });
    registerScreenFactory(SCREEN_NAME_EMPTY, [](TFT_eSPI &tft_param) { return std::make_shared<EmptyScreen>(tft_param); });
    registerScreenFactory(SCREEN_NAME_SCREENSAVER, [](TFT_eSPI &tft_param) { return std::make_shared<ScreenSaverScreen>(tft_param, band, config); });
    registerScreenFactory(SCREEN_NAME_SETUP, [](TFT_eSPI &tft_param) { return std::make_shared<SetupScreen>(tft_param); });
    registerScreenFactory("SETUP_DISPLAY", [](TFT_eSPI &tft_param) { return std::make_shared<SetupDisplayScreen>(tft_param); });
    registerScreenFactory("SETUP_SI4735", [](TFT_eSPI &tft_param) { return std::make_shared<SetupSi4735Screen>(tft_param); });
    registerScreenFactory("SETUP_DECODERS", [](TFT_eSPI &tft_param) { return std::make_shared<SetupDecodersScreen>(tft_param); });
}
