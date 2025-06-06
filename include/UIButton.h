#ifndef __UI_BUTTON_H
#define __UI_BUTTON_H

#include <functional>

#include "UIComponent.h"

/**
 * @brief UI Button komponens
 *
 * Egyszerű gomb komponens, amely kezeli a lenyomást és a megjelenítést.
 */
class UIButton : public UIComponent {
  public:
    // Alapértelmezett gomb méretek (növelve a jobb érinthetőség érdekében)
    static constexpr uint16_t DEFAULT_BUTTON_WIDTH = 63;
    static constexpr uint16_t DEFAULT_BUTTON_HEIGHT = 35;

    // Gomb típusok
    enum class ButtonType {
        Pushable,  // Egyszerű nyomógomb
        Toggleable // Váltógomb (on/off)
    };

    // Gomb állapotok (eseményekhez és külső lekérdezéshez, a belső logikai állapotot a LogicalButtonState tárolja)
    enum class EventButtonState {
        Off = 0,
        On,
        Disabled,
        CurrentActive, // Jelenleg aktív mód jelzése
        Clicked,       // Lenyomás történt (Pushable gomboknál)
        LongPressed    // Hosszú nyomás
    };

    // Gomb esemény struktúra
    struct ButtonEvent {
        uint8_t id;
        const char *label;
        EventButtonState state;
        uint32_t timestamp;

        ButtonEvent(uint8_t id, const char *label, EventButtonState state) : id(id), label(label), state(state), timestamp(millis()) {}
    };

    enum class ButtonState { Off, On, Disabled, CurrentActive };

  private:
    uint8_t buttonId;
    const char *label;
    ButtonType buttonType = ButtonType::Pushable;
    ButtonState currentState = ButtonState::Off;
    uint8_t textSize = 2;
    uint8_t cornerRadius = 5;
    bool useMiniFont = false;
    uint32_t longPressThreshold = 1000; // ms
    uint32_t pressStartTime = 0;
    bool longPressDetected = false;

    std::function<void(const ButtonEvent &)> eventCallback;
    std::function<void()> clickCallback; // Backward compatibility

    // Gomb állapot színek
    struct StateColors {
        uint16_t background;
        uint16_t border;
        uint16_t text;
        uint16_t led;
    };

    StateColors getStateColors() const {
        StateColors resultColors;
        // Az alap UIComponent::colors tagot használjuk (ami egy ColorScheme)

        if (currentState == ButtonState::Disabled) {
            resultColors.background = this->colors.disabledBackground;
            resultColors.border = this->colors.disabledBorder;
            resultColors.text = this->colors.disabledForeground;
            resultColors.led = TFT_BLACK; // Vagy this->colors.disabledLedColor, ha lenne

        } else if (this->pressed) {
            resultColors.background = this->colors.pressedBackground; // Gradiens alapja
            resultColors.border = this->colors.pressedBorder;
            resultColors.text = this->colors.pressedForeground;

            // LED színe lenyomott állapotban
            if (buttonType == ButtonType::Toggleable) {
                resultColors.led = (currentState == ButtonState::On) ? this->colors.ledOnColor : this->colors.ledOffColor;
                // Vagy egyedi szín lenyomáskor, pl. TFT_ORANGE
            } else {
                resultColors.led = TFT_BLACK;
            }

        } else {
            if (currentState == ButtonState::On && buttonType == ButtonType::Toggleable) {
                resultColors.background = this->colors.activeBackground;
                resultColors.border = this->colors.activeBorder;
                resultColors.text = this->colors.activeForeground;
                resultColors.led = this->colors.ledOnColor;

            } else if (currentState == ButtonState::CurrentActive) {
                resultColors.background = this->colors.background; // Vagy this->colors.activeBackground, ha más, mint az ON
                resultColors.border = TFT_BLUE;                    // Speciális eset, vagy this->colors.activeBorder
                resultColors.text = this->colors.foreground;       // Vagy this->colors.activeForeground

                resultColors.led = (buttonType == ButtonType::Toggleable && currentState == ButtonState::On)
                                       ? this->colors.ledOnColor
                                       : ((buttonType == ButtonType::Toggleable && currentState == ButtonState::Off) ? this->colors.ledOffColor : TFT_BLACK); // Módosítva

            } else { // Normal Off state for Toggleable, or any state for Pushable
                resultColors.background = this->colors.background;
                resultColors.border = this->colors.border;
                resultColors.text = this->colors.foreground;
                resultColors.led = (buttonType == ButtonType::Toggleable) ? this->colors.ledOffColor : TFT_BLACK;
            }
        }
        return resultColors;
    }

    // Szín sötétítése gradiens effekthez - TftButton.h alapján
    uint16_t darkenColor(uint16_t color, uint8_t amount) const {
        // Kivonjuk a piros, zöld és kék színösszetevőket
        uint8_t r = (color & 0xF800) >> 11;
        uint8_t g = (color & 0x07E0) >> 5;
        uint8_t b = (color & 0x001F);

        // Finomítjuk a csökkentési mértéket, figyelembe véve a színösszetevők közötti eltéréseket
        uint8_t darkenAmount = amount > 0 ? (amount >> 3) : 0;

        // A csökkentésnél biztosítjuk, hogy ne menjenek 0 alá az értékek
        r = (r > darkenAmount) ? r - darkenAmount : 0;
        g = (g > darkenAmount) ? g - darkenAmount : 0;
        b = (b > darkenAmount) ? b - darkenAmount : 0;

        // Visszaalakítjuk a színt 16 bites RGB formátumba
        return (r << 11) | (g << 5) | b;
    }

    // Gradiens effekt rajzolása pressed állapotban
    void drawPressedEffect(uint16_t baseColorForEffect) {
        const uint8_t steps = 6; // TFT_BUTTON_DARKEN_COLORS_STEPS
        uint8_t stepWidth = bounds.width / steps;
        uint8_t stepHeight = bounds.height / steps;

        uint16_t baseColor = baseColorForEffect;

        for (uint8_t i = 0; i < steps; i++) {
            uint16_t fadedColor = darkenColor(baseColor, i * 30); // Erősebb sötétítés
            tft.fillRoundRect(bounds.x + i * stepWidth / 2, bounds.y + i * stepHeight / 2, bounds.width - i * stepWidth, bounds.height - i * stepHeight, cornerRadius, fadedColor);
        }
    }

  public:
    /**
     * @brief Gomb komponens konstruktora
     * @param tft TFT_eSPI referencia
     * @param id Gomb azonosítója
     * @param bounds A gomb határai (Rect)
     * @param label A gomb felirata
     * @param type A gomb típusa (Pushable vagy Toggleable)
     * @param state A gomb kezdeti állapota (Off, On, Disabled, CurrentActive)
     * @param callback Eseménykezelő függvény, amely a gomb eseményeit kezeli
     * @param colors Színpaletta a gombhoz
     * @note A bounds szélessége és magassága 0 esetén az alapértelmezett méreteket használja (DEFAULT_BUTTON_WIDTH és DEFAULT_BUTTON_HEIGHT).
     */
    UIButton(TFT_eSPI &tft,
             uint8_t id,                                                             // ID
             const Rect &bounds,                                                     // rect
             const char *label,                                                      // label
             ButtonType type = ButtonType::Pushable,                                 // type
             ButtonState state = ButtonState::Off,                                   // initial state
             std::function<void(const ButtonEvent &)> callback = nullptr,            // callback
             const ColorScheme &colors = UIColorPalette::createDefaultButtonScheme() // colors
             )
        : UIComponent(tft, Rect(bounds.x, bounds.y, (bounds.width == 0 ? DEFAULT_BUTTON_WIDTH : bounds.width), (bounds.height == 0 ? DEFAULT_BUTTON_HEIGHT : bounds.height)),
                      colors),
          buttonId(id), label(label), buttonType(type), currentState(state), eventCallback(callback) {
        // Ha a gombot eleve letiltott állapottal hozzuk létre,
        // akkor az ősosztály 'disabled' flag-jét is be kell állítani.
        if (this->currentState == ButtonState::Disabled) {
            UIComponent::disabled = true;
        }
    }

    /**
     * @brief Gomb komponens konstruktora
     * @param tft TFT_eSPI referencia
     * @param id Gomb azonosítója
     * @param bounds A gomb határai (Rect)
     * @param label A gomb felirata
     * @param type A gomb típusa (Pushable vagy Toggleable)
     * @param callback Eseménykezelő függvény, amely a gomb eseményeit kezeli
     * @param colors Színpaletta a gombhoz
     * @note A bounds szélessége és magassága 0 esetén az alapértelmezett méreteket használja (DEFAULT_BUTTON_WIDTH és DEFAULT_BUTTON_HEIGHT).
     */
    UIButton(TFT_eSPI &tft,
             uint8_t id,                                                             // ID
             const Rect &bounds,                                                     // rect
             const char *label,                                                      // label
             ButtonType type = ButtonType::Pushable,                                 // type
             std::function<void(const ButtonEvent &)> callback = nullptr,            // callback
             const ColorScheme &colors = UIColorPalette::createDefaultButtonScheme() // colors
             )
        : UIComponent(tft, Rect(bounds.x, bounds.y, (bounds.width == 0 ? DEFAULT_BUTTON_WIDTH : bounds.width), (bounds.height == 0 ? DEFAULT_BUTTON_HEIGHT : bounds.height)),
                      colors), // Alapértelmezett méretek használata, ha a bounds szélessége vagy magassága 0
          buttonId(id), label(label), buttonType(type), eventCallback(callback) {}

    /**
     * @brief Gomb állapotának szöveges megjelenítése
     * @param state Az állapot, amelyet szövegesen szeretnénk megjeleníteni
     */
    static const char *eventButtonStateToString(EventButtonState state) {
        switch (state) {
        case EventButtonState::Off:
            return "Off";
        case EventButtonState::On:
            return "On";
        case EventButtonState::Disabled:
            return "Disabled";
        case EventButtonState::CurrentActive:
            return "CurrentActive";
        case EventButtonState::Clicked:
            return "Clicked";
        case EventButtonState::LongPressed:
            return "LongPressed";
        default:
            return "Unknown";
        }
    }

    // // Segédfüggvény a ButtonType szöveges megjelenítéséhez (ha később kellene)
    // static const char *buttonTypeToString(ButtonType type) {
    //     switch (type) {
    //     case ButtonType::Pushable:
    //         return "Pushable";
    //     case ButtonType::Toggleable:
    //         return "Toggleable";
    //     default:
    //         return "Unknown";
    //     }
    // }

    // ================================
    // Getters/Setters
    // ================================

    // Getters & Setters
    uint8_t getId() const { return buttonId; }
    void setId(uint8_t id) { buttonId = id; }

    ButtonType getButtonType() const { return buttonType; }
    void setButtonType(ButtonType type) {
        if (buttonType != type) {
            buttonType = type;
            markForRedraw();
        }
    }

    ButtonState getButtonState() const { return currentState; }

    /**
     * @brief Gomb állapotának beállítása
     * @param newState Az új állapot, amelyet be szeretnénk állítani
     */
    void setButtonState(ButtonState newState) {
        if (currentState == newState) {
            // Ha az állapot már ugyanaz, ellenőrizzük és javítjuk a konzisztenciát UIComponent::enabled-del
            if (newState == ButtonState::Disabled) {
                if (UIComponent::isDisabled())
                    UIComponent::setDisabled(true);

            } else { // newState egy engedélyezett állapot
                if (!UIComponent::isDisabled())
                    UIComponent::setDisabled(false);
            }
            // Ha csak a konzisztencia javítása történt, és az enabled állapot nem változott, nincs szükség további markForRedraw-ra innen.
            return;
        }

        ButtonState oldState = currentState;
        currentState = newState;

        if (newState == ButtonState::Disabled) {
            UIComponent::setDisabled(true);             // Az ősosztály setDisabled metódusa kezeli a markForRedraw-t, ha az 'enabled' változik.
        } else if (oldState == ButtonState::Disabled) { // Tiltottról engedélyezett állapotra váltás
            UIComponent::setDisabled(false);            // Az ősosztály setDisabled metódusa kezeli a markForRedraw-t, ha az 'enabled' változik.
        }

        // Mindig újrarajzolást kérünk, ha a logikai állapot változott (pl. On -> Off),
        // mert a gomb kinézete ettől függhet, még ha az UIComponent::enabled nem is változott.
        markForRedraw();
    }

    /**
     * @brief Gomb engedélyezése vagy letiltása
     * @param enable true ha engedélyezni szeretnénk, false ha letiltani
     */
    void setEnabled(bool enable) {
        if (enable) {
            // Engedélyezés
            if (currentState == ButtonState::Disabled) { // Módosítva
                // Ha logikailag le volt tiltva, állítsuk vissza alapértelmezett engedélyezett állapotra (pl. Off).
                // A setButtonState gondoskodik az UIComponent::setEnabled(true) hívásáról.
                setButtonState(ButtonState::Off);
            } else {
                // Nem volt logikailag letiltva, de biztosítsuk, hogy az ősosztály komponens is engedélyezve legyen.
                UIComponent::setDisabled(false);
            }
        } else {
            // Letiltás
            if (currentState != ButtonState::Disabled) { // Módosítva
                // Ha nem volt logikailag letiltva, állítsuk letiltott állapotra.
                // A setButtonState gondoskodik az UIComponent::setEnabled(false) hívásáról.
                setButtonState(ButtonState::Disabled);
            } else {
                // Már logikailag le volt tiltva, csak biztosítsuk, hogy az ősosztály komponens is le legyen tiltva.
                UIComponent::setDisabled(true);
            }
        }
    }

    // Szöveg beállítása
    void setLabel(const char *newLabel) {
        if (!STREQ(label, newLabel)) {
            label = newLabel;
            markForRedraw();
        }
    }
    const char *getText() const { return label; }

    // Szöveg méret
    void setTextSize(uint8_t size) {
        if (textSize != size) {
            textSize = size;
            markForRedraw();
        }
    }
    uint8_t getTextSize() const { return textSize; }

    // Sarok lekerekítés
    void setCornerRadius(uint8_t radius) {
        if (cornerRadius != radius) {
            cornerRadius = radius;
            markForRedraw();
        }
    }
    uint8_t getCornerRadius() const { return cornerRadius; }

    // Mini font használata
    void setUseMiniFont(bool mini) {
        if (useMiniFont != mini) {
            useMiniFont = mini;
            markForRedraw();
        }
    }
    bool getUseMiniFont() const { return useMiniFont; }

    // Hosszú nyomás küszöb
    void setLongPressThreshold(uint32_t threshold) { longPressThreshold = threshold; }
    uint32_t getLongPressThreshold() const { return longPressThreshold; }

    // Event callback beállítása
    void setEventCallback(std::function<void(const ButtonEvent &)> callback) { eventCallback = callback; }

    // Kattintás callback (backward compatibility)
    void setClickCallback(std::function<void()> callback) { clickCallback = callback; }

    // "Default" gomb stílus beállítása (MultiButtonDialog-ban használatos)
    // Beállítja a gombot letiltott állapotba és alkalmazza a default choice színsémát
    void setAsDefaultChoiceButton() {
        setEnabled(false);                                                 // Letiltjuk a gombot
        setColorScheme(UIColorPalette::createDefaultChoiceButtonScheme()); // Alkalmazzuk a default choice színsémát
    }

    /**
     * @brief Gomb megjelenítése
     */
    virtual void draw() override {
        if (!needsRedraw)
            return;

        StateColors currentDrawColors = getStateColors();

        if (this->pressed) {                                 // Ha lenyomva van, rajzoljuk a pressed effektet
            drawPressedEffect(currentDrawColors.background); // Gradiens effekt rajzolása lenyomott állapotban
        } else {
            tft.fillRoundRect(bounds.x, bounds.y, bounds.width, bounds.height, cornerRadius, currentDrawColors.background);
        }

        // Keret rajzolása
        tft.drawRoundRect(bounds.x, bounds.y, bounds.width, bounds.height, cornerRadius, currentDrawColors.border);

        // Szöveg rajzolása
        if (label != nullptr) {
            tft.setTextSize(useMiniFont ? 1 : textSize);
            tft.setTextColor(currentDrawColors.text);
            tft.setTextDatum(MC_DATUM); // Middle Center

            // Szöveg pozíció finomhangolása
            int16_t textY = bounds.centerY();
            if (useMiniFont)
                textY += 1; // Mini font esetén kicsit lejjebb

            tft.drawString(label, bounds.centerX(), textY);
        }

        // LED csík rajzolása (csak toggleable gomboknál, ha nem mini font és van LED szín)
        if (buttonType == ButtonType::Toggleable && !useMiniFont && currentDrawColors.led != TFT_BLACK) {
            constexpr uint8_t LED_HEIGHT = 5;
            constexpr uint8_t LED_MARGIN = 10;
            tft.fillRect(bounds.x + LED_MARGIN, bounds.y + bounds.height - LED_HEIGHT - 3, bounds.width - 2 * LED_MARGIN, LED_HEIGHT, currentDrawColors.led);
        }

        needsRedraw = false;
    }

  protected:
    // UIComponent::pressed már kezeli a lenyomott vizuális állapotot
    // UIComponent::handleTouch hívja ezeket

    /**
     * @brief Gomb lenyomása esemény kezelése
     * @param event A touch esemény, amely tartalmazza a koordinátákat és a lenyomás állapotát
     */
    virtual void onTouchDown(const TouchEvent &event) override {
        UIComponent::onTouchDown(event); // Alap implementáció (pressed = true, markForRedraw)
        if (currentState == ButtonState::Disabled)
            return;

        longPressDetected = false;
        pressStartTime = millis();
        // A vizuális "lenyomott" állapotot a UIComponent::pressed és a draw() kezeli
    }

    /**
     * @brief Gomb felengedése esemény kezelése
     */
    virtual void onClick(const TouchEvent &event) override {

        UIComponent::onClick(event); // Alap implementáció

        if (currentState == ButtonState::Disabled)
            return;

        if (longPressDetected) {
            // Ha hosszú lenyomás történt, az onClick eseményt esetleg nem kellene feldolgozni,
            // vagy másképp kell kezelni. Most feltételezzük, hogy a hosszú nyomás eseménye különálló.
            return;
        }

        if (buttonType == ButtonType::Toggleable) {
            currentState = (currentState == ButtonState::Off || currentState == ButtonState::CurrentActive) ? ButtonState::On : ButtonState::Off; // Módosítva
            if (eventCallback) {
                eventCallback(ButtonEvent(buttonId, label, (currentState == ButtonState::On) ? EventButtonState::On : EventButtonState::Off)); // Módosítva
            }
        } else { // Pushable
            if (eventCallback) {
                eventCallback(ButtonEvent(buttonId, label, EventButtonState::Clicked));
            }
        }

        if (clickCallback) { // Backward compatibility
            clickCallback();
        }
        markForRedraw(); // Logikai állapot változott
    }

    virtual void onTouchCancel(const TouchEvent &event) override {
        UIComponent::onTouchCancel(event); // Alap implementáció (pressed = false, markForRedraw)
        if (currentState == ButtonState::Disabled)
            return;
        pressStartTime = 0;
        longPressDetected = false;
    }

  public:
    virtual void loop() override {
        UIComponent::loop(); // Alap osztály loop-ja (ha van)

        if (currentState == ButtonState::Disabled) // Módosítva
            return;

        if (pressed && !longPressDetected && pressStartTime > 0) { // `pressed` a UIComponent-ből jön
            if (millis() - pressStartTime >= longPressThreshold) {
                longPressDetected = true;
                if (eventCallback) {
                    eventCallback(ButtonEvent(buttonId, label, EventButtonState::LongPressed));
                }
                // Hosszú nyomásnak lehet saját vizuális állapota, vagy csak eseményt vált ki
                // Ha a logikai állapot változik, itt kell beállítani és markForRedraw()
                // Pl. setLogicalButtonState(LogicalButtonState::LongPressedState); (ha lenne ilyen) // Eredeti
                // Pl. setButtonState(ButtonState::LongPressedState); (ha lenne ilyen) // Módosítva
                markForRedraw(); // Akár csak az esemény miatt is lehet újrarajzolás
            }
        }
    }

    // Az onClick felülírása az UIComponent-ben már nem szükséges, mert az új onClick ezt kezeli.
    // A régi onClick(const TouchEvent &event) override törölhető, ha az UIComponent-ben
    // a protected onClick üres volt, vagy az új logika lefedi.
    // Jelenleg az UIComponent::onClick üres, tehát a UIButton-ban lévő onClick felülírása a helyes.

    // Touch sensitivity növelése gomboknál
    virtual int16_t getTouchMargin() const override { return 6; } // 6 pixel tolerancia gomboknál (nagyobb mint az alapértelmezett 4)
};

#endif // __UI_BUTTON_H