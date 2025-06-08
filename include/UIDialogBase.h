#ifndef __UI_DIALOG_BASE_H
#define __UI_DIALOG_BASE_H

#include <functional>

#include "UIButton.h"
#include "UIContainerComponent.h"

// Előre deklarációk
class UIScreen;

class UIDialogBase : public UIContainerComponent {
  public:
    enum class DialogResult {
        None,
        Accepted, // OK, Yes, stb.
        Rejected, // Cancel, No, stb.
        Dismissed // 'X' gombbal vagy programatikusan bezárva
    };

    using DialogCallback = std::function<void(DialogResult)>;

    static constexpr const uint8_t DIALOG_DEFAULT_CLOSE_BUTTON_ID = 111; // Alapértelmezett bezáró gomb ID

  protected:
    UIScreen *parentScreen; // A képernyő, amely megjelenítette ezt a dialógust
    const char *title;
    DialogCallback callback = nullptr;
    bool veilDrawn = false;                          // Fátyol rajzolásának állapota
    bool autoClose = true;                           // Automatikus bezárás a gombok megnyomásakor
    std::shared_ptr<UIButton> closeButton = nullptr; // Bezáró gomb

    // Dialógus elrendezési konstansok
    static constexpr uint16_t HEADER_HEIGHT = 28;                                  // Fejléc magassága
    static constexpr uint16_t PADDING = 5;                                         // Belső margó
    static constexpr uint16_t BORDER_RADIUS = 8;                                   // Saroklekerekítés
    static constexpr uint16_t CLOSE_BUTTON_SIZE = HEADER_HEIGHT - 2 * PADDING - 2; // Bezáró gomb mérete
    static constexpr uint16_t VEIL_COLOR = TFT_DARKGREY;                           // Fátyol színe (lehetne tft.color565(30,30,30) egy sötétebbért)

    // MultiButtonDialog lefagyás debughoz
    static constexpr uint16_t DEFAULT_HEADER_HEIGHT = HEADER_HEIGHT;
    static constexpr uint16_t DEFAULT_HEADER_HEIGHT_WITH_TITLE = HEADER_HEIGHT;
    static constexpr uint16_t DEFAULT_HEADER_HEIGHT_NO_TITLE = HEADER_HEIGHT; // Leszármazottaknak felüldefiniálható metódusok a tartalom és lábléc létrehozásához
    virtual void createDialogContent() {}
    virtual void layoutDialogContent() {} // Elrendezéshez, ha szükséges

    // Belső segéd metódusok
    void createCloseButton();
    void drawVeil();

    bool topDialog = false; // Jelzi, hogy ez a dialógus a legfelső (legutolsó) a stackben

  public:
    UIDialogBase(UIScreen *parentScreen, TFT_eSPI &tft, const Rect &bounds, const char *title, const ColorScheme &cs = ColorScheme::defaultScheme());
    virtual ~UIDialogBase() override = default;

    virtual void show();
    virtual void close(DialogResult result = DialogResult::Dismissed);

    // Header magasság lekérése (tartalom pozicionálásához)
    uint16_t getHeaderHeight() const { return HEADER_HEIGHT; }

    void setDialogCallback(DialogCallback cb) { callback = cb; }

    void setAutoClose(bool autoClose) { autoClose = autoClose; }
    bool getAutoClose() const { return autoClose; }

    // UICompositeComponent metódusok felülírása

    virtual void draw() override;

    // Legyen virtual, hogy a leszármazottak is felülírhassák helyesen
    virtual void markForRedraw() override;

    // A veilDrawn flag resetelése
    void resetVeilDrawnFlag() { veilDrawn = false; }

    /**
     * @brief Touch esemény kezelése, amely először a gyerek komponenseken próbálkozik,
     * majd ha egyik sem, akkor maga a UIDialogBase kezeli.
     * @param event A touch esemény, amely tartalmazza a koordinátákat és a lenyomás állapotát
     */
    virtual bool handleTouch(const TouchEvent &event) override;

    /**
     * @brief Kezeli a rotary eseményeket, amelyek a dialógusra vonatkoznak.
     * @param event A rotary esemény, amely tartalmazza az irányt és a gomb állapotát
     * @return true, ha a rotary eseményt a dialógus kezelte, false egyébként
     * @details A handleRotary és loop alapértelmezetten a UIContainerComponent-től öröklődik,
     *  ami továbbítja a gyerekeknek. Ha a dialógusnak magának kellene kezelnie, felülírható.
     */
    virtual bool handleRotary(const RotaryEvent &event) override;

    /**
     * @brief Rajzolja a dialógus hátterét és fejlécét.
     */
    virtual void drawSelf() override;

    /**
     * @brief Ellenőrzi, hogy ez a dialógus a legfelső (legutolsó) a stackben.
     * @return true, ha ez a dialógus a legfelső, false ha nem
     */
    inline bool isTopDialog() const { return topDialog; }

    /**
     * @brief Beállítja, hogy ez a dialógus a legfelső legyen a stackben.
     * @param isTop true, ha ez a dialógus a legfelső, false ha nem
     */
    inline void setTopDialog(bool isTop) { topDialog = isTop; }
};

#endif //__UI_DIALOG_BASE_H