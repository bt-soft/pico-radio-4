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
    const int16_t listBottomPadding = buttonHeight + margin * 2; // Hely az Exit gombnak    // Görgethető lista komponens létrehozása és hozzáadása a gyermek komponensekhez
    Rect listBounds(margin, listTopMargin, UIComponent::SCREEN_W - (2 * margin), UIComponent::SCREEN_H - listTopMargin - listBottomPadding);
    memoryList = std::make_shared<UIScrollableListComponent>(tft, listBounds, this, 6, 27); // itemHeight megnövelve 20-ról 26-ra
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

    // Első elem automatikus kiválasztása, ha van állomás
    if (stations.size() > 0) {
        selectedIndex = 0;
        // A UIScrollableListComponent automatikusan 0-ra állítja a selectedItemIndex-et
    } else {
        selectedIndex = -1;
    }

    // Inicializáljuk a lastTunedIndex változót
    lastTunedIndex = -1;
    for (int i = 0; i < stations.size(); i++) {
        if (isStationCurrentlyTuned(stations[i])) {
            lastTunedIndex = i;
            break;
        }
    }
}

void MemoryScreen::refreshList() {
    loadStations();
    if (memoryList) {
        memoryList->markForRedraw();
    }
    updateHorizontalButtonStates();
}

void MemoryScreen::refreshCurrentTunedIndication() {
    // Csak a lista megjelenítését frissítjük, nem töltjük újra az adatokat
    if (memoryList) {
        memoryList->markForRedraw();
    }
    updateHorizontalButtonStates();
}

void MemoryScreen::refreshTunedIndicationOptimized() {
    if (!memoryList) {
        return;
    }

    // Megkeressük a jelenleg behangolt állomás indexét
    int currentTunedIndex = -1;
    for (int i = 0; i < stations.size(); i++) {
        if (isStationCurrentlyTuned(stations[i])) {
            currentTunedIndex = i;
            break;
        }
    }

    // Ha megváltozott a behangolt állomás, frissítjük az érintett elemeket
    if (currentTunedIndex != lastTunedIndex) {
        // Korábbi behangolt elem frissítése (ha volt)
        if (lastTunedIndex >= 0 && lastTunedIndex < stations.size()) {
            memoryList->redrawListItem(lastTunedIndex);
        }

        // Új behangolt elem frissítése (ha van)
        if (currentTunedIndex >= 0 && currentTunedIndex < stations.size()) {
            memoryList->redrawListItem(currentTunedIndex);
        }

        // Frissítjük a nyilvántartást
        lastTunedIndex = currentTunedIndex;
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

    // Fix formátum: mindig ugyanolyan pozícióban kezdődik a szöveg
    String label = "";
    if (const_cast<MemoryScreen *>(this)->isStationCurrentlyTuned(station)) {
        label = String(CURRENT_TUNED_ICON) + String(station.name);
    } else {
        // Szóközök ugyanolyan hosszban mint a CURRENT_TUNED_ICON ("> ")
        label = "   " + String(station.name); // 2 szóköz a "> " helyett
    }

    return label;
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
    // Optimalizált frissítés - csak az érintett elemek újrarajzolása
    refreshTunedIndicationOptimized();
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
    // Cím kirajzolása memória állapottal
    tft.setFreeFont(&FreeSansBold12pt7b);
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW, TFT_COLOR_BACKGROUND);
    tft.setTextDatum(TC_DATUM);

    // Memória állapot hozzáadása a címhez
    uint8_t currentCount = getCurrentStationCount();
    uint8_t maxCount = getMaxStationCount();
    String title = String(isFmMode ? "FM Memory" : "AM Memory") + " (" + String(currentCount) + "/" + String(maxCount) + ")";
    tft.drawString(title, UIComponent::SCREEN_W / 2, 5);

    // Komponensek már automatikusan kirajzolódnak
}

void MemoryScreen::activate() {
    DEBUG("MemoryScreen activated\n"); // Sáv típus frissítése
    bool newFmMode = isCurrentBandFm();
    if (newFmMode != isFmMode) {
        isFmMode = newFmMode;
        refreshList();
    } else {
        // Ha a sáv típus nem változott, csak a behangolt állomás jelzését frissítjük optimalizáltan
        refreshTunedIndicationOptimized();
    }

    updateHorizontalButtonStates();
    UIScreen::activate();
}

void MemoryScreen::onDialogClosed(UIDialogBase *closedDialog) {
    UIScreen::onDialogClosed(closedDialog);

    // Dialógus állapot resetelése
    currentDialogState = DialogState::None;

    // Gombok állapotának frissítése és explicit újrarajzolás
    updateHorizontalButtonStates();

    // A gombsor konténer teljes újrarajzolása, hogy biztosan megjelenjenek a gombok
    if (horizontalButtonBar) {
        horizontalButtonBar->markForCompleteRedraw();
    }
}

// ===================================================================
// Gomb eseménykezelők
// ===================================================================

void MemoryScreen::handleAddCurrentButton(const UIButton::ButtonEvent &event) {
    if (event.state == UIButton::EventButtonState::Clicked) {
        // Ellenőrizzük, hogy az aktuális állomás már létezik-e
        if (isCurrentStationInMemory()) {
            // Figyelmeztető dialógus megjelenítése
            showStationExistsDialog();
        } else {
            showAddStationDialog();
        }
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
            getScreenManager()->goBack();
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

void MemoryScreen::showStationExistsDialog() {
    StationData currentStation = getCurrentStationData();
    String freqStr = formatFrequency(currentStation.frequency, isFmMode);
    String modStr = getModulationName(currentStation.modulation);

    String message = "Station already exists in memory:\n" + freqStr + " " + modStr;

    auto infoDialog = std::make_shared<MessageDialog>(this, tft, Rect(-1, -1, 280, 0), "Station Exists", message.c_str(), MessageDialog::ButtonsType::Ok);

    infoDialog->setDialogCallback([this](UIDialogBase *dialog, UIDialogBase::DialogResult result) {
        // Nincs teendő, csak tájékoztatás
    });

    showDialog(infoDialog);
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
    // Memória telített ellenőrzése
    if (isMemoryFull()) {
        auto dialog = std::make_shared<MessageDialog>(this, tft, Rect(-1, -1, 300, 120), "Hiba", "Memória megtelt!\nTöröljön állomásokat.");
        showDialog(dialog);
        return;
    }

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
    horizontalButtonBar->setButtonState(
        DELETE_BUTTON, hasSelection ? UIButton::ButtonState::Off
                                    : UIButton::ButtonState::Disabled); // Add Current gomb állapota - letiltva, ha az aktuális állomás már a memóriában van VAGY a memória tele van
    bool currentStationExists = isCurrentStationInMemory();
    bool memoryFull = isMemoryFull();
    horizontalButtonBar->setButtonState(ADD_CURRENT_BUTTON, (currentStationExists || memoryFull) ? UIButton::ButtonState::Disabled : UIButton::ButtonState::Off);
}

bool MemoryScreen::isCurrentStationInMemory() {
    StationData currentStation = getCurrentStationData();

    // Ellenőrizzük, hogy az aktuális frekvencia és moduláció már létezik-e a memóriában
    for (const StationData &station : stations) {
        if (station.frequency == currentStation.frequency && station.modulation == currentStation.modulation) {
            return true;
        }
    }

    return false;
}

bool MemoryScreen::isStationCurrentlyTuned(const StationData &station) {
    StationData currentStation = getCurrentStationData();

    // Alapvető összehasonlítás: frekvencia és moduláció
    bool basicMatch = (station.frequency == currentStation.frequency && station.modulation == currentStation.modulation);

    if (!basicMatch) {
        return false;
    }

    // SSB/CW módoknál a BFO offset is egyeznie kell
    if (station.modulation == LSB || station.modulation == USB || station.modulation == CW) {
        return (station.bfoOffset == currentStation.bfoOffset);
    }
    // AM/FM módoknál csak frekvencia és moduláció kell egyezzen
    return true;
}

uint8_t MemoryScreen::getCurrentStationCount() const { return isFmMode ? fmStationStore.getStationCount() : amStationStore.getStationCount(); }

uint8_t MemoryScreen::getMaxStationCount() const { return isFmMode ? MAX_FM_STATIONS : MAX_AM_STATIONS; }

bool MemoryScreen::isMemoryFull() const { return getCurrentStationCount() >= getMaxStationCount(); }
