#include "ScreenManager.h"
#include "EmptyScreen.h"
#include "FMScreen.h"
#include "ScreenSaverScreen.h"
#include "SetupScreen.h" // Hozzáadva
#include "TestScreen.h"

/**
 * @brief Képernyőkezelő osztály konstruktor
 */
void ScreenManager::registerDefaultScreenFactories() {
    registerScreenFactory(SCREEN_NAME_FM, [](TFT_eSPI &tft_param) { return std::make_shared<FMScreen>(tft_param); });
    registerScreenFactory(SCREEN_NAME_TEST, [](TFT_eSPI &tft_param) { return std::make_shared<TestScreen>(tft_param); });
    registerScreenFactory(SCREEN_NAME_EMPTY, [](TFT_eSPI &tft_param) { return std::make_shared<EmptyScreen>(tft_param); });
    registerScreenFactory(SCREEN_NAME_SCREENSAVER, [](TFT_eSPI &tft_param) { return std::make_shared<ScreenSaverScreen>(tft_param); });
    registerScreenFactory(SCREEN_NAME_SETUP, [](TFT_eSPI &tft_param) { return std::make_shared<SetupScreen>(tft_param); });
    // Ide jöhetnek további képernyők regisztrációi
}
