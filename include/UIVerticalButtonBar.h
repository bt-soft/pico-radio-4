#ifndef __UI_VERTICAL_BUTTON_BAR_H
#define __UI_VERTICAL_BUTTON_BAR_H

#include "UIButton.h"
#include "UIContainerComponent.h"
#include <functional>
#include <memory>
#include <vector>

/**
 * @brief Függőleges gombsor komponens
 *
 * Ez a komponens automatikusan elrendezi a gombokat függőlegesen,
 * egységes mérettel és távolsággal. Speciálisan rádió képernyőkhöz
 * tervezve, ahol gyakran használt funkció gombok vannak.
 */
class UIVerticalButtonBar : public UIContainerComponent {
  public:
    /**
     * @brief Gomb konfiguráció struktúra
     */
    struct ButtonConfig {
        uint8_t id;
        const char *label;
        UIButton::ButtonType type;
        UIButton::ButtonState initialState;
        std::function<void(const UIButton::ButtonEvent &)> callback;

        ButtonConfig(uint8_t id, const char *label, UIButton::ButtonType type = UIButton::ButtonType::Pushable, UIButton::ButtonState initialState = UIButton::ButtonState::Off,
                     std::function<void(const UIButton::ButtonEvent &)> callback = nullptr)
            : id(id), label(label), type(type), initialState(initialState), callback(callback) {}
    };

    /**
     * @brief Konstruktor
     * @param tft TFT display referencia
     * @param bounds A gombsor pozíciója és mérete
     * @param buttonConfigs Gombok konfigurációja
     * @param buttonWidth Egyetlen gomb szélessége (alapértelmezett: 60px)
     * @param buttonHeight Egyetlen gomb magassága (alapértelmezett: 35px)
     * @param buttonGap Gombok közötti távolság (alapértelmezett: 3px)
     */
    UIVerticalButtonBar(TFT_eSPI &tft, const Rect &bounds, const std::vector<ButtonConfig> &buttonConfigs, uint16_t buttonWidth = 60, uint16_t buttonHeight = 35,
                        uint16_t buttonGap = 3);

    virtual ~UIVerticalButtonBar() = default;

    /**
     * @brief Gomb állapotának beállítása ID alapján
     * @param buttonId A gomb azonosítója
     * @param state Az új állapot
     */
    void setButtonState(uint8_t buttonId, UIButton::ButtonState state);

    /**
     * @brief Gomb állapotának lekérdezése ID alapján
     * @param buttonId A gomb azonosítója
     * @return A gomb aktuális állapota
     */
    UIButton::ButtonState getButtonState(uint8_t buttonId) const;

    /**
     * @brief Egy gomb referenciájának megszerzése ID alapján
     * @param buttonId A gomb azonosítója
     * @return A gomb shared_ptr-e, vagy nullptr ha nem található
     */
    std::shared_ptr<UIButton> getButton(uint8_t buttonId) const;

    /**
     * @brief Gomb hozzáadása futásidőben
     * @param config Az új gomb konfigurációja
     * @return true ha sikerült hozzáadni, false ha nincs hely
     */
    bool addButton(const ButtonConfig &config);

    /**
     * @brief Gomb eltávolítása ID alapján
     * @param buttonId Az eltávolítandó gomb azonosítója
     * @return true ha sikerült eltávolítani, false ha nem található
     */
    bool removeButton(uint8_t buttonId);

    /**
     * @brief Gomb láthatóságának beállítása
     * @param buttonId A gomb azonosítója
     * @param visible true = látható, false = rejtett
     */
    void setButtonVisible(uint8_t buttonId, bool visible);

    /**
     * @brief Gombok újrarendezése
     * @details Újraszámolja a pozíciókat a látható gombok alapján
     */
    void relayoutButtons();

  protected:
    /**
     * @brief Gombok létrehozása és elhelyezése
     */
    void createButtons(const std::vector<ButtonConfig> &buttonConfigs);

  private:
    uint16_t buttonWidth;
    uint16_t buttonHeight;
    uint16_t buttonGap;
    std::vector<std::shared_ptr<UIButton>> buttons;
};

#endif // __UI_VERTICAL_BUTTON_BAR_H
