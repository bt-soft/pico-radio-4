#ifndef __MULTI_BUTTON_DIALOG_H
#define __MULTI_BUTTON_DIALOG_H

#include "ButtonsGroupManager.h"
#include "UIDialogBase.h"

/**
 * @brief MultiButtonDialog osztály felhasználó által definiált gombok kezelésére.
 *
 * Ez az osztály a MessageDialog UserDefined funkcionalitását veszi át,
 * és specializált kezelést biztosít több gomb megjelenítésére és kezelésére.
 */
class MultiButtonDialog : public UIDialogBase, public ButtonsGroupManager<MultiButtonDialog> {
  public:
    /**
     * @brief Callback típus gomb kattintásokhoz
     * @param buttonIndex A kattintott gomb indexe a gombok tömbben
     * @param buttonLabel A kattintott gomb felirata
     * @param dialog A dialógus referenciája
     */
    using ButtonClickCallback = std::function<void(int buttonIndex, const char *buttonLabel, MultiButtonDialog *dialog)>;

  protected:
    const char *message;
    std::vector<std::shared_ptr<UIButton>> _buttonsList; // Létrehozott gombok tárolására
    std::vector<ButtonGroupDefinition> _buttonDefs;      // Gombdefiníciók tárolására

    // Felhasználó által definiált gombok
    const char *const *_userOptions = nullptr;
    uint8_t _numUserOptions = 0;
    int _clickedUserButtonIndex = -1;                   // A kattintott gomb indexe
    const char *_clickedUserButtonLabel = nullptr;      // A kattintott gomb felirata
    ButtonClickCallback _buttonClickCallback = nullptr; // Gomb kattintás callback
    bool _autoCloseOnButtonClick = true;                // Automatikusan bezárja-e a dialógust gomb kattintáskor    // Default choice gomb támogatás
    int _defaultButtonIndex = -1;                       // Az alapértelmezett gomb indexe (-1 = nincs)
    bool _disableDefaultButton = true;                  // Ha true, az alapértelmezett gomb le van tiltva

    // Virtuális metódusok felülírása
    virtual void createDialogContent() override;
    virtual void layoutDialogContent() override;
    virtual void drawSelf() override;

  public: /**
           * @brief MultiButtonDialog konstruktor.
           *
           * @param parentScreen A szülő UIScreen.
           * @param tft TFT_eSPI referencia.
           * @param title A dialógus címe.
           * @param message Az üzenet szövege.
           * @param options Gombok feliratainak tömbje.
           * @param numOptions A gombok száma.
           * @param buttonClickCb Gomb kattintás callback (opcionális).
           * @param autoClose Automatikusan bezárja-e a dialógust gomb kattintáskor (alapértelmezett: true).           * @param defaultButtonIndex Az alapértelmezett (jelenlegi)
           * gomb indexe, amely kiemelve jelenik meg (-1 = nincs, alapértelmezett).
           * @param disableDefaultButton Ha true, az alapértelmezett gomb le van tiltva; ha false, csak vizuálisan kiemelve (alapértelmezett: true).
           * @param bounds A dialógus határai (opcionális, automatikus méret és középre igazítás).
           * @param cs Színséma (opcionális).
           */
    MultiButtonDialog(UIScreen *parentScreen, TFT_eSPI &tft, const char *title, const char *message, const char *const *options, uint8_t numOptions,
                      ButtonClickCallback buttonClickCb = nullptr, bool autoClose = true, int defaultButtonIndex = -1, bool disableDefaultButton = true,
                      const Rect &bounds = {-1, -1, 0, 0}, const ColorScheme &cs = ColorScheme::defaultScheme());

    virtual ~MultiButtonDialog() override = default; // Getter metódusok
    int getClickedUserButtonIndex() const { return _clickedUserButtonIndex; }
    const char *getClickedUserButtonLabel() const { return _clickedUserButtonLabel; }
    bool getAutoCloseOnButtonClick() const { return _autoCloseOnButtonClick; }
    int getDefaultButtonIndex() const { return _defaultButtonIndex; }
    bool getDisableDefaultButton() const { return _disableDefaultButton; } // Setter metódusok
    void setAutoCloseOnButtonClick(bool autoClose) { _autoCloseOnButtonClick = autoClose; }
    void setButtonClickCallback(ButtonClickCallback callback) { _buttonClickCallback = callback; }
    void setDefaultButtonIndex(int defaultIndex); // Implementáció a .cpp fájlban
    void setDisableDefaultButton(bool disable);   // Implementáció a .cpp fájlban

    /**
     * @brief Manuálisan bezárja a dialógust a megadott eredménnyel.
     * @param result A dialógus eredménye
     */
    void closeDialog(DialogResult result = DialogResult::Accepted);
};

#endif // __MULTI_BUTTON_DIALOG_H
