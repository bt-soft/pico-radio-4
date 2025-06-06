#include "ScreenManager.h"
#include "FMScreen.h"

void ScreenManager::registerDefaultScreenFactories() {
    // FMScreen factory
    registerScreenFactory(SCREEN_NAME_FM, [](TFT_eSPI &tft) -> std::shared_ptr<UIScreen> { return std::make_shared<FMScreen>(tft); });

    // AMScreen factory
    // registerScreenFactory(SCREEN_NAME_AM, [](TFT_eSPI &tft) -> std::shared_ptr<UIScreen> { return std::make_shared<AMScreen>(tft); });

    // TestScreen factory
    // registerScreenFactory(SCREEN_NAME_TEST, [](TFT_eSPI &tft) -> std::shared_ptr<UIScreen> { return std::make_shared<TestScreen>(tft); });
}