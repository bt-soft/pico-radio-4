#include "UIComponent.h"

// Statikus tagváltozók definíciója
uint16_t UIComponent::SCREEN_W = 0;
uint16_t UIComponent::SCREEN_H = 0;

// Statikus inicializáló metódus implementációja
void UIComponent::initScreenDimensions(TFT_eSPI &tft) {
    if (SCREEN_W == 0 || SCREEN_H == 0) {
        SCREEN_W = tft.width();
        SCREEN_H = tft.height();
    }
}
