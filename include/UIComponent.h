#ifndef __UICOMPONENT_H
#define __UICOMPONENT_H

#include <TFT_eSPI.h>

#include "UIColorPalette.h"
#include "defines.h"

// Touch esemény struktúra
struct TouchEvent {
    uint16_t x, y;
    bool pressed;

    TouchEvent(uint16_t x, uint16_t y, bool pressed) : x(x), y(y), pressed(pressed) {}
};

// Rotary encoder esemény struktúra
struct RotaryEvent {
    enum Direction { None, Up, Down };
    enum ButtonState { NotPressed, Clicked, DoubleClicked };

    Direction direction;
    ButtonState buttonState;
    RotaryEvent(Direction dir, ButtonState btnState) : direction(dir), buttonState(btnState) {}
};

// Téglalap struktúra
struct Rect {
    int16_t x, y;
    uint16_t width, height;

    Rect(int16_t x = 0, int16_t y = 0, uint16_t width = 0, uint16_t height = 0) : x(x), y(y), width(width), height(height) {}

    bool contains(int16_t px, int16_t py) const { return px >= x && px < x + width && py >= y && py < y + height; }

    int16_t centerX() const { return x + width / 2; }
    int16_t centerY() const { return y + height / 2; }
};

/**
 * @file UIComponent.h
 * @brief Az UIComponent osztály a felhasználói felület komponenseinek alapját képezi.
 * @details Ez az osztály tartalmazza a felhasználói felület komponensek közös jellemzőit és metódusait.
 */
class UIComponent {

  protected:
    TFT_eSPI &tft;
    Rect bounds;
    ColorScheme colors;
    bool disabled = false;   // Komponens tiltott állapot
    bool pressed = false;    // Komponens lenyomva állapot
    bool needsRedraw = true; // Dirty flag, kinduláskor minden komponens újrarajzolást igényel

  public:
    UIComponent(TFT_eSPI &tft, const Rect &bounds, const ColorScheme &colors = ColorScheme::defaultScheme()) : tft(tft), bounds(bounds), colors(colors) {}

    virtual ~UIComponent() = default;

    // Touch margin beállítása (felülírható a származtatott osztályokban)
    virtual int16_t getTouchMargin() const { return 0; } // 2 pixel alapértelmezett tolerancia

    // Touch területének ellenőrzése (kiterjesztett érzékenységgel)
    virtual bool isPointInside(int16_t x, int16_t y) const {
        // Konfigurálható margin a pontosabb érintéshez - lehet hogy a származtatott osztályok felülírják
        const int16_t TOUCH_MARGIN = getTouchMargin();
        return (x >= bounds.x - TOUCH_MARGIN && x <= bounds.x + bounds.width + TOUCH_MARGIN && y >= bounds.y - TOUCH_MARGIN && y <= bounds.y + bounds.height + TOUCH_MARGIN);
    }

    // Touch esemény kezelése - visszatérés: true ha feldolgozta az eseményt
    virtual bool handleTouch(const TouchEvent &event) {

        // Ha le van tiltva, akkor nem kezeljük az eseményt
        if (disabled) {
            return false;
        }

        bool inside = isPointInside(event.x, event.y);
        bool oldPressed = pressed;
        static uint32_t touchDownTime = 0;

        if (event.pressed && inside && !pressed) {
            pressed = true;
            touchDownTime = millis();
            onTouchDown(event);
            // Ha pressed állapot változott, újrarajzolás szükséges
            if (oldPressed != pressed) {
                markForRedraw();
            }
            return true;

        } else if (!event.pressed && pressed) {
            pressed = false;
            uint32_t touchDuration = millis() - touchDownTime;

            // Kiterjesztett tolerancia a release eseményhez - ha az original touch területen belül volt a lenyomás
            constexpr int16_t RELEASE_TOLERANCE = 8; // 8 pixel tolerancia a felengedéshez
            bool releaseInside = (event.x >= bounds.x - RELEASE_TOLERANCE && event.x <= bounds.x + bounds.width + RELEASE_TOLERANCE && event.y >= bounds.y - RELEASE_TOLERANCE &&
                                  event.y <= bounds.y + bounds.height + RELEASE_TOLERANCE);

            onTouchUp(event);
            if (releaseInside && touchDuration >= 30 && touchDuration <= 2000) { // Érvényes touch duration
                onClick(event);
                DEBUG("UIComponent: Valid click detected: duration=%dms\n", touchDuration);
            } else {
                onTouchCancel(event);
                DEBUG("UIComponent: Touch cancelled: inside=%s duration=%dms\n", releaseInside ? "true" : "false", touchDuration);
            }

            // Ha pressed állapot változott, újrarajzolás szükséges
            if (oldPressed != pressed) {
                markForRedraw();
            }
            return true;
        }

        return false;
    };

    // Rotary encoder esemény kezelése - visszatérés: true ha feldolgozta az eseményt
    virtual bool handleRotary(const RotaryEvent &event) { return false; }

    // Rajzolás
    virtual void draw() = 0;

    // Loop hívás - ezt minden komponens megkapja
    virtual void loop() {
        // Alapértelmezetten nincs loop logika
    }

    // ================================
    // Getters/Setters
    // ================================

    // Színséma getter/setter
    void setColorScheme(const ColorScheme &newColors) {
        colors = newColors;
        markForRedraw();
    }
    const ColorScheme &getColorScheme() const { return colors; }

    // Tiltott állapot getter/setter
    bool isDisabled() const { return disabled; }
    void setDisabled(bool disabled) { this->disabled = disabled; }

    // Újrarajzolás getter/setter
    virtual void markForRedraw() { needsRedraw = true; }
    virtual bool isRedrawNeeded() const { return needsRedraw; }

  protected:
    // Eseménykezelő metódusok
    virtual void onTouchDown(const TouchEvent &event) { DEBUG("UIComponent: Touch DOWN at (%d,%d)\n", event.x, event.y); }
    virtual void onTouchUp(const TouchEvent &event) { DEBUG("UIComponent: Touch UP at (%d,%d)\n", event.x, event.y); }
    virtual void onTouchCancel(const TouchEvent &event) {}
    virtual void onClick(const TouchEvent &event) { DEBUG("UIComponent: CLICK at (%d,%d)\n", event.x, event.y); }
};

#endif // __UICOMPONENT_H