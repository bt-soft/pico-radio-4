#ifndef __MEMORY_SCREEN_H
#define __MEMORY_SCREEN_H

#include "IScrollableListDataSource.h"
#include "MessageDialog.h"
#include "StationData.h"
#include "StationStore.h"
#include "UIButton.h"
#include "UIHorizontalButtonBar.h"
#include "UIScreen.h"
#include "UIScrollableListComponent.h"
#include "VirtualKeyboardDialog.h"
#include <vector>

/**
 * @brief Memória képernyő állomások kezeléséhez
 * @details Listázza, szerkeszti és kezeli a tárolt állomásokat
 */
class MemoryScreen : public UIScreen, public IScrollableListDataSource {
  public:
    /**
     * @brief Konstruktor
     * @param tft TFT display referencia
     * @param si4735Manager Si4735 rádió chip kezelő referencia
     */
    MemoryScreen(TFT_eSPI &tft, Si4735Manager &si4735Manager);
    virtual ~MemoryScreen();

    // UIScreen interface
    virtual bool handleRotary(const RotaryEvent &event) override;
    virtual void handleOwnLoop() override;
    virtual void drawContent() override;
    virtual void activate() override;
    virtual void onDialogClosed(UIDialogBase *closedDialog) override;

    // IScrollableListDataSource interface
    virtual int getItemCount() const override;
    virtual String getItemLabelAt(int index) const override;
    virtual String getItemValueAt(int index) const override;
    virtual bool onItemClicked(int index) override;

  private:
    // Vízszintes gombsor azonosítók
    static constexpr uint8_t ADD_CURRENT_BUTTON = 30;
    static constexpr uint8_t EDIT_BUTTON = 31;
    static constexpr uint8_t DELETE_BUTTON = 32;
    static constexpr uint8_t BACK_BUTTON = 33; // UI komponensek
    std::shared_ptr<UIScrollableListComponent> memoryList;
    std::shared_ptr<UIHorizontalButtonBar> horizontalButtonBar;
    std::shared_ptr<UIButton> backButton;

    // Adatok
    std::vector<StationData> stations;
    int selectedIndex = -1;
    bool isFmMode = true;

    // Dialógus állapotok
    enum class DialogState { None, AddingStation, EditingStationName, ConfirmingDelete } currentDialogState = DialogState::None;

    StationData pendingStation; // Új állomás hozzáadásakor

    // Metódusok
    void layoutComponents();
    void createHorizontalButtonBar();
    void updateHorizontalButtonStates();
    void loadStations();
    void refreshList();

    // Gomb eseménykezelők
    void handleAddCurrentButton(const UIButton::ButtonEvent &event);
    void handleEditButton(const UIButton::ButtonEvent &event);
    void handleDeleteButton(const UIButton::ButtonEvent &event);
    void handleBackButton(const UIButton::ButtonEvent &event);

    // Dialógus kezelők
    void showAddStationDialog();
    void showEditStationDialog();
    void showDeleteConfirmDialog();

    // Állomás műveletek
    void tuneToStation(int index);
    void addCurrentStation(const String &name);
    void updateStationName(int index, const String &newName);
    void deleteStation(int index); // Segéd metódusok
    StationData getCurrentStationData();
    String formatFrequency(uint16_t frequency, bool isFm) const;
    String getModulationName(uint8_t modulation) const;
    bool isCurrentBandFm();
};

#endif // __MEMORY_SCREEN_H
