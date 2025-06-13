#include "MemoryScreen.h"
#include "Band.h"
#include "UIColorPalette.h"
#include "rtVars.h"
#include "utils.h"

// ===================================================================
// Vízszintes gombsor azonosítók - Képernyő-specifikus navigáció
// ===================================================================
namespace MemoryScreenHorizontalButtonIDs {
static constexpr uint8_t ADD_CURRENT_BUTTON = 30;
static constexpr uint8_t EDIT_BUTTON = 31;
static constexpr uint8_t DELETE_BUTTON = 32;
static constexpr uint8_t BACK_BUTTON = 33;
} // namespace MemoryScreenHorizontalButtonIDs

// ===================================================================
// Konstruktor és inicializálás
// ===================================================================

MemoryScreen::MemoryScreen(TFT_eSPI &tft, Si4735Manager &si4735Manager) : UIScreen(tft, SCREEN_NAME_MEMORY, &si4735Manager) {

    // Aktuális sáv típus meghatározása
    isFmMode = isCurrentBandFm();

    layoutComponents();
    loadStations();
}

MemoryScreen::~MemoryScreen() {}

// ===================================================================
// UI komponensek layout és elhelyezés
// ===================================================================

/**
 *
 */
void MemoryScreen::layoutComponents() {

    // Lista komponens létrehozása
    // Rect listBounds(5, 25, 400, 250);

    // Képernyő dimenzióinak és margóinak meghatározása
    const int16_t margin = 5;
    const int16_t buttonHeight = UIButton::DEFAULT_BUTTON_HEIGHT;
    const int16_t listTopMargin = 30;                            // Hely a címnek
    const int16_t listBottomPadding = buttonHeight + margin * 2; // Hely az Exit gombnak

    // Görgethető lista komponens létrehozása és hozzáadása a gyermek komponensekhez
    Rect listBounds(margin, listTopMargin, UIComponent::SCREEN_W - (2 * margin), UIComponent::SCREEN_H - listTopMargin - listBottomPadding);
    memoryList = std::make_shared<UIScrollableListComponent>(tft, listBounds, this, 6, 20);
    addChild(memoryList);

    // Vízszintes gombsor létrehozása
    createHorizontalButtonBar();
}

/**
 *
 */
void MemoryScreen::createHorizontalButtonBar() {
    using namespace MemoryScreenHorizontalButtonIDs; // Gombsor pozíció számítása (képernyő alján)
    constexpr int16_t margin = 5;
    uint16_t buttonY = UIComponent::SCREEN_H - UIButton::DEFAULT_BUTTON_HEIGHT - margin;
    uint16_t buttonWidth = 90;
    uint16_t buttonHeight = UIButton::DEFAULT_BUTTON_HEIGHT;
    uint16_t spacing = 5;

    // Csak az első 3 gombhoz számítjuk a szélességet (Back gomb külön lesz)
    uint16_t totalWidth = 3 * buttonWidth + 2 * spacing;
    uint16_t startX = margin;

    Rect buttonRect(startX, buttonY, totalWidth, buttonHeight);

    // Gomb konfigurációk létrehozása (Back gomb nélkül)
    std::vector<UIHorizontalButtonBar::ButtonConfig> buttonConfigs = {
        {ADD_CURRENT_BUTTON, "Add Curr", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) { handleAddCurrentButton(event); }},
        {EDIT_BUTTON, "Edit", UIButton::ButtonType::Pushable, UIButton::ButtonState::Disabled, [this](const UIButton::ButtonEvent &event) { handleEditButton(event); }},
        {DELETE_BUTTON, "Delete", UIButton::ButtonType::Pushable, UIButton::ButtonState::Disabled, [this](const UIButton::ButtonEvent &event) { handleDeleteButton(event); }}};

    horizontalButtonBar = std::make_shared<UIHorizontalButtonBar>(tft, buttonRect, buttonConfigs, buttonWidth, buttonHeight, spacing);
    addChild(horizontalButtonBar);

    // Back gomb külön, jobbra igazítva
    uint16_t backButtonWidth = 60;
    uint16_t backButtonX = UIComponent::SCREEN_W - backButtonWidth - margin;
    Rect backButtonRect(backButtonX, buttonY, backButtonWidth, buttonHeight);

    backButton = std::make_shared<UIButton>(tft, BACK_BUTTON, backButtonRect, "Back", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off,
                                            [this](const UIButton::ButtonEvent &event) { handleBackButton(event); });
    addChild(backButton);
}

// ===================================================================
// Adatok kezelése
// ===================================================================

void MemoryScreen::loadStations() {
    stations.clear();

    if (isFmMode) {
        uint8_t count = fmStationStore.getStationCount();
        for (uint8_t i = 0; i < count; i++) {
            const StationData *stationData = fmStationStore.getStationByIndex(i);
            if (stationData) {
                stations.push_back(*stationData);
            }
        }
    } else {
        uint8_t count = amStationStore.getStationCount();
        for (uint8_t i = 0; i < count; i++) {
            const StationData *stationData = amStationStore.getStationByIndex(i);
            if (stationData) {
                stations.push_back(*stationData);
            }
        }
    }

    DEBUG("Loaded %d stations for %s mode\n", stations.size(), isFmMode ? "FM" : "AM");
}

void MemoryScreen::refreshList() {
    loadStations();
    if (memoryList) {
        memoryList->markForRedraw();
    }
    updateHorizontalButtonStates();
}

// ===================================================================
// IScrollableListDataSource interface
// ===================================================================

int MemoryScreen::getItemCount() const { return stations.size(); }

String MemoryScreen::getItemLabelAt(int index) const {
    if (index < 0 || index >= stations.size()) {
        return "";
    }

    const StationData &station = stations[index];
    return String(station.name);
}

String MemoryScreen::getItemValueAt(int index) const {
    if (index < 0 || index >= stations.size()) {
        return "";
    }

    const StationData &station = stations[index];
    String freq = formatFrequency(station.frequency, isFmMode);
    String mod = getModulationName(station.modulation);

    return freq + " " + mod;
}

bool MemoryScreen::onItemClicked(int index) {
    selectedIndex = index;
    tuneToStation(index);
    updateHorizontalButtonStates();
    return false; // Nincs szükség teljes újrarajzolásra
}

// ===================================================================
// UIScreen interface
// ===================================================================

bool MemoryScreen::handleRotary(const RotaryEvent &event) {
    // Ha van aktív dialógus, továbbítjuk neki
    if (isDialogActive()) {
        return UIScreen::handleRotary(event);
    }

    // Lista scrollozás
    if (memoryList) {
        return memoryList->handleRotary(event);
    }

    return false;
}

void MemoryScreen::handleOwnLoop() {
    // Nincs speciális loop logika szükséges
}

void MemoryScreen::drawContent() {
    // Cím kirajzolása
    tft.setFreeFont(&FreeSansBold12pt7b);
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW, TFT_COLOR_BACKGROUND);
    tft.setTextDatum(TC_DATUM);
    String title = isFmMode ? "FM Memory" : "AM Memory";
    tft.drawString(title, UIComponent::SCREEN_W / 2, 5);

    // Komponensek már automatikusan kirajzolódnak
}

void MemoryScreen::activate() {
    DEBUG("MemoryScreen activated\n");

    // Sáv típus frissítése
    bool newFmMode = isCurrentBandFm();
    if (newFmMode != isFmMode) {
        isFmMode = newFmMode;
        refreshList();
    }

    updateHorizontalButtonStates();
    UIScreen::activate();
}

void MemoryScreen::onDialogClosed(UIDialogBase *closedDialog) {
    UIScreen::onDialogClosed(closedDialog);

    // Dialógus állapot resetelése
    currentDialogState = DialogState::None;

    // Gombok állapotának frissítése
    updateHorizontalButtonStates();
}

// ===================================================================
// Gomb eseménykezelők
// ===================================================================

void MemoryScreen::handleAddCurrentButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        showAddStationDialog();
    }
}

void MemoryScreen::handleEditButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked && selectedIndex >= 0) {
        showEditStationDialog();
    }
}

void MemoryScreen::handleDeleteButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked && selectedIndex >= 0) {
        showDeleteConfirmDialog();
    }
}

void MemoryScreen::handleBackButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        // Visszatérés az előző képernyőre
        if (getScreenManager()) {
            String prevScreen = isFmMode ? SCREEN_NAME_FM : SCREEN_NAME_AM;
            getScreenManager()->switchToScreen(prevScreen.c_str());
        }
    }
}

// ===================================================================
// Dialógus kezelők
// ===================================================================

void MemoryScreen::showAddStationDialog() {
    currentDialogState = DialogState::AddingStation;
    pendingStation = getCurrentStationData();

    auto keyboardDialog = std::make_shared<VirtualKeyboardDialog>(this, tft, "Add Station", "", MAX_STATION_NAME_LEN, [this](const String &newText) {
        // Szöveg változás callback - itt nem csinálunk semmit
    });

    // Dialog bezárás kezelése
    keyboardDialog->setDialogCallback([this, keyboardDialog](UIDialogBase *dialog, UIDialogBase::DialogResult result) {
        if (result == UIDialogBase::DialogResult::Accepted && currentDialogState == DialogState::AddingStation) {
            String stationName = keyboardDialog->getCurrentText();
            if (stationName.length() > 0) {
                addCurrentStation(stationName);
            }
        }
        currentDialogState = DialogState::None;
    });

    showDialog(keyboardDialog);
}

void MemoryScreen::showEditStationDialog() {
    if (selectedIndex < 0 || selectedIndex >= stations.size()) {
        return;
    }

    currentDialogState = DialogState::EditingStationName;
    String currentName = stations[selectedIndex].name;

    auto keyboardDialog = std::make_shared<VirtualKeyboardDialog>(this, tft, "Edit Name", currentName, MAX_STATION_NAME_LEN, [this](const String &newText) {
        // Szöveg változás callback
    });

    keyboardDialog->setDialogCallback([this, keyboardDialog](UIDialogBase *dialog, UIDialogBase::DialogResult result) {
        if (result == UIDialogBase::DialogResult::Accepted && currentDialogState == DialogState::EditingStationName && selectedIndex >= 0) {
            String newName = keyboardDialog->getCurrentText();
            if (newName.length() > 0) {
                updateStationName(selectedIndex, newName);
            }
        }
        currentDialogState = DialogState::None;
    });

    showDialog(keyboardDialog);
}

void MemoryScreen::showDeleteConfirmDialog() {
    if (selectedIndex < 0 || selectedIndex >= stations.size()) {
        return;
    }

    currentDialogState = DialogState::ConfirmingDelete;
    String message = "Delete station:\n" + String(stations[selectedIndex].name) + "\n" + formatFrequency(stations[selectedIndex].frequency, isFmMode) + "?";

    auto confirmDialog = std::make_shared<MessageDialog>(this, tft, Rect(-1, -1, 250, 0), "Confirm Delete", message.c_str(), MessageDialog::ButtonsType::YesNo);

    confirmDialog->setDialogCallback([this](UIDialogBase *dialog, UIDialogBase::DialogResult result) {
        if (result == UIDialogBase::DialogResult::Accepted && currentDialogState == DialogState::ConfirmingDelete && selectedIndex >= 0) {
            deleteStation(selectedIndex);
            selectedIndex = -1;
        }
        currentDialogState = DialogState::None;
    });

    showDialog(confirmDialog);
}

// ===================================================================
// Állomás műveletek
// ===================================================================

void MemoryScreen::tuneToStation(int index) {
    if (index < 0 || index >= stations.size()) {
        return;
    }

    const StationData &station = stations[index];

    DEBUG("Tuning to station: %s, freq: %d\n", station.name, station.frequency);

    // Si4735Manager::tuneMemoryStation használata (öröklés Si4735Band-ből)
    if (pSi4735Manager) {
        pSi4735Manager->tuneMemoryStation(station.frequency, station.bfoOffset, station.bandIndex, station.modulation, station.bandwidthIndex);
    }
}

void MemoryScreen::addCurrentStation(const String &name) {
    StationData newStation = getCurrentStationData();
    strncpy(newStation.name, name.c_str(), MAX_STATION_NAME_LEN);
    newStation.name[MAX_STATION_NAME_LEN] = '\0'; // Store-ba mentés
    if (isFmMode) {
        if (fmStationStore.addStation(newStation)) {
            DEBUG("FM station added: %s\n", newStation.name);
        } else {
            DEBUG("Failed to add FM station\n");
        }
    } else {
        if (amStationStore.addStation(newStation)) {
            DEBUG("AM station added: %s\n", newStation.name);
        } else {
            DEBUG("Failed to add AM station\n");
        }
    }

    refreshList();
}

void MemoryScreen::updateStationName(int index, const String &newName) {
    if (index < 0 || index >= stations.size()) {
        return;
    }

    StationData updatedStation = stations[index];
    strncpy(updatedStation.name, newName.c_str(), MAX_STATION_NAME_LEN);
    updatedStation.name[MAX_STATION_NAME_LEN] = '\0'; // Store-ban frissítés
    if (isFmMode) {
        if (fmStationStore.updateStation(index, updatedStation)) {
            DEBUG("FM station updated: %s\n", updatedStation.name);
        } else {
            DEBUG("Failed to update FM station\n");
        }
    } else {
        if (amStationStore.updateStation(index, updatedStation)) {
            DEBUG("AM station updated: %s\n", updatedStation.name);
        } else {
            DEBUG("Failed to update AM station\n");
        }
    }

    refreshList();
}

void MemoryScreen::deleteStation(int index) {
    if (index < 0 || index >= stations.size()) {
        return;
    } // Store-ból törlés
    if (isFmMode) {
        if (fmStationStore.deleteStation(index)) {
            DEBUG("FM station deleted at index %d\n", index);
        } else {
            DEBUG("Failed to delete FM station\n");
        }
    } else {
        if (amStationStore.deleteStation(index)) {
            DEBUG("AM station deleted at index %d\n", index);
        } else {
            DEBUG("Failed to delete AM station\n");
        }
    }

    refreshList();
}

// ===================================================================
// Segéd metódusok
// ===================================================================

StationData MemoryScreen::getCurrentStationData() {
    StationData station = {0};

    if (pSi4735Manager) {
        // Si4735Manager közvetlenül örökli a Band metódusokat
        auto &currentBand = pSi4735Manager->getCurrentBand();

        station.frequency = currentBand.currFreq;
        station.bfoOffset = currentBand.lastBFO;
        station.bandIndex = config.data.currentBandIdx; // Band index a config-ból
        station.modulation = currentBand.currMod;
        station.bandwidthIndex = 0; // TODO: Ha van bandwidth index tárolás
    }

    return station;
}

String MemoryScreen::formatFrequency(uint16_t frequency, bool isFm) const {
    if (isFm) {
        // FM: 10kHz egységben tárolt, MHz-ben megjelenítve
        return String(frequency / 100.0, 1) + " MHz";
    } else {
        // AM: kHz-ben tárolt és megjelenített
        return String(frequency) + " kHz";
    }
}

String MemoryScreen::getModulationName(uint8_t modulation) const {
    switch (modulation) {
        case 0:
            return "FM";
        case 1:
            return "AM";
        case 2:
            return "LSB";
        case 3:
            return "USB";
        case 4:
            return "CW";
        default:
            return "???";
    }
}

bool MemoryScreen::isCurrentBandFm() {
    if (pSi4735Manager) {
        return pSi4735Manager->isCurrentBandFM();
    }
    return true; // Default FM
}

void MemoryScreen::updateHorizontalButtonStates() {
    if (!horizontalButtonBar) {
        return;
    }

    using namespace MemoryScreenHorizontalButtonIDs;

    // Edit és Delete gombok csak akkor aktívak, ha van kiválasztott elem
    bool hasSelection = (selectedIndex >= 0 && selectedIndex < stations.size());

    horizontalButtonBar->setButtonState(EDIT_BUTTON, hasSelection ? UIButton::ButtonState::Off : UIButton::ButtonState::Disabled);

    horizontalButtonBar->setButtonState(DELETE_BUTTON, hasSelection ? UIButton::ButtonState::Off : UIButton::ButtonState::Disabled);
}
