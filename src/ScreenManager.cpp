#include "ScreenManager.h"
#include "FMScreen.h"
#include "TestScreen.h"

/**
 * @file ScreenManager.cpp
 * @brief Képernyő menedzser implementáció
 * @details Ez az osztály kezeli a képernyők közötti váltást és a képernyő gyárakat.
 */
void ScreenManager::registerDefaultScreenFactories() {
    // FMScreen factory
    registerScreenFactory(SCREEN_NAME_FM, [](TFT_eSPI &tft) -> std::shared_ptr<UIScreen> { return std::make_shared<FMScreen>(tft); });

    // AMScreen factory
    // registerScreenFactory(SCREEN_NAME_AM, [](TFT_eSPI &tft) -> std::shared_ptr<UIScreen> { return std::make_shared<AMScreen>(tft); });

    // TestScreen factory
    registerScreenFactory(SCREEN_NAME_TEST, [](TFT_eSPI &tft) -> std::shared_ptr<UIScreen> { return std::make_shared<TestScreen>(tft); });
}