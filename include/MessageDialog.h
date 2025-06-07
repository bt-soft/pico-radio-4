#ifndef __MESSAGE_DIALOG_H
#define __MESSAGE_DIALOG_H

#include "ButtonsGroupManager.h" // Hozzáadva
#include "UIDialogBase.h"
// A UIButton.h-t a UIDialogBase vagy a ButtonsGroupManager már tartalmazza

class MessageDialog : public UIDialogBase, public ButtonsGroupManager<MessageDialog> { // Módosítva az öröklés
  public:
    enum class ButtonsType { Ok, OkCancel, YesNo, YesNoCancel };

  protected:
    const char *message;
    ButtonsType buttonsType;

    std::vector<std::shared_ptr<UIButton>> _buttonsList; // Megmarad a létrehozott gombok tárolására és eltávolítására
    std::vector<ButtonGroupDefinition> _buttonDefs;      // Gombdefiníciók tárolására

    // A createDialogContent most csak a _buttonDefs-et készíti elő
    virtual void createDialogContent() override;
    virtual void layoutDialogContent() override;

    // Felülírjuk a drawSelf-et az üzenet kirajzolásához
    virtual void drawSelf() override;

  public:
    MessageDialog(UIScreen *parentScreen, TFT_eSPI &tft, const Rect &bounds, const char *title, const char *message, ButtonsType buttonsType = ButtonsType::Ok,
                  const ColorScheme &cs = ColorScheme::defaultScheme());

    virtual ~MessageDialog() override = default;
};

#endif // __MESSAGE_DIALOG_H