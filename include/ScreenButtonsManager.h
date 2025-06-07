#ifndef __SCREEN_BUTTONS_MANAGER_H
#define __SCREEN_BUTTONS_MANAGER_H

#include <functional>
#include <memory>
#include <vector>

#include "UIButton.h"
#include "UIContainerComponent.h"
#include "UIScreen.h" // Szükséges lehet, ha a DerivedScreen mindig UIScreen leszármazott
#include "defines.h"  // A DEBUG makróhoz

// Struktúra a gombok definíciójához a csoportos elrendezéshez
// Ezt a struktúrát használják a layout metódusok.
struct ButtonDefinition {
    uint8_t id;                                                      // Gomb egyedi azonosítója
    const char *label;                                               // Gomb felirata
    UIButton::ButtonType type;                                       // Gomb típusa (Pushable, Toggleable)
    std::function<void(const UIButton::ButtonEvent &)> callback;     // Visszahívási függvény eseménykezeléshez
    UIButton::ButtonState initialState = UIButton::ButtonState::Off; // Kezdeti állapot (alapértelmezetten Off)
    uint16_t width = 0;                                              // Egyedi szélesség (0 = UIButton::DEFAULT_BUTTON_WIDTH)
    uint16_t height = 0;                                             // Egyedi magasság (0 = UIButton::DEFAULT_BUTTON_HEIGHT)
};

template <typename DerivedScreen> // Curiously Recurring Template Pattern (CRTP): A DerivedScreen lesz a konkrét képernyő osztály (pl. TestScreen)
class ScreenButtonsManager {
  protected:
    // A konstruktor és destruktor lehet default, mivel ez az osztály
    // elsősorban metódusokat biztosít, nem saját állapotot.
    ScreenButtonsManager() = default;
    virtual ~ScreenButtonsManager() = default;

    /**
     * @brief Gombokat rendez el függőlegesen, a képernyő jobb széléhez igazítva.
     * A metódus a leszármazott képernyő (ami UIScreen-ből is származik)
     * tft referenciáját és addChild metódusát használja.
     */
    void layoutVerticalButtonGroup(const std::vector<ButtonDefinition> &buttonDefs, std::vector<std::shared_ptr<UIButton>> *out_createdButtons = nullptr, int16_t marginRight = 5,
                                   int16_t marginTop = 5, int16_t marginBottom = 5, int16_t defaultButtonWidthRef = UIButton::DEFAULT_BUTTON_WIDTH,
                                   int16_t defaultButtonHeightRef = UIButton::DEFAULT_BUTTON_HEIGHT, int16_t columnGap = 3, int16_t buttonGap = 3) {

        DerivedScreen *self = static_cast<DerivedScreen *>(this);

        if (buttonDefs.empty()) {
            return;
        }

        // A 'self->tft' a DerivedScreen (pl. TestScreen) UIScreen ősosztályából
        // (pontosabban UIComponent-ből) örökölt 'tft' referenciája lesz.
        // Az 'self->addChild' pedig az UIContainerComponent public metódusa.
        const int16_t screenHeight = self->getTft().height();
        const int16_t maxColumnHeight = screenHeight - marginTop - marginBottom;
        const int16_t screenWidth = self->getTft().width();

        if (out_createdButtons) {
            out_createdButtons->clear();
        }

        // --- Előfeldolgozási fázis: Oszlopok struktúrájának és méreteinek meghatározása ---
        std::vector<std::vector<ButtonDefinition>> colsOfButtons;
        std::vector<int16_t> colMaxWidhtsList;
        std::vector<ButtonDefinition> currentBuildingColButtons;
        int16_t currentY_build = marginTop;
        int16_t currentBuildingColMaxW = 0;

        for (const auto &def : buttonDefs) {
            int16_t btnW = (def.width > 0) ? def.width : defaultButtonWidthRef;
            int16_t btnH = (def.height > 0) ? def.height : defaultButtonHeightRef;

            if (currentY_build == marginTop && btnH > maxColumnHeight) {
                DEBUG("ScreenButtonsManager::layoutVerticalButtonGroup: Button %d ('%s') too tall for column (%d > %d), skipping.\n", def.id, def.label, btnH, maxColumnHeight);
                if (!currentBuildingColButtons.empty()) { // Ha az előző oszlopban voltak gombok
                    colsOfButtons.push_back(currentBuildingColButtons);
                    colMaxWidhtsList.push_back(currentBuildingColMaxW);
                    currentBuildingColButtons.clear();
                    currentBuildingColMaxW = 0;
                }
                currentY_build = marginTop; // Marad a jelenlegi oszlop tetején a következő gombnak
                continue;
            }

            if (currentY_build + btnH > maxColumnHeight && currentY_build != marginTop) { // Új oszlopot kell kezdeni
                colsOfButtons.push_back(currentBuildingColButtons);
                colMaxWidhtsList.push_back(currentBuildingColMaxW);
                currentBuildingColButtons.clear();
                currentY_build = marginTop;
                currentBuildingColMaxW = 0;

                if (btnH > maxColumnHeight) { // Ellenőrizzük, hogy az új oszlopot kezdő gomb nem túl magas-e
                    DEBUG("ScreenButtonsManager::layoutVerticalButtonGroup: Button %d ('%s') too tall for new column (%d > %d), skipping.\n", def.id, def.label, btnH,
                          maxColumnHeight);
                    continue;
                }
            }
            currentBuildingColButtons.push_back(def);
            currentBuildingColMaxW = std::max(currentBuildingColMaxW, btnW);
            currentY_build += btnH + buttonGap;
        }
        if (!currentBuildingColButtons.empty()) {
            colsOfButtons.push_back(currentBuildingColButtons);
            colMaxWidhtsList.push_back(currentBuildingColMaxW);
        }

        if (colsOfButtons.empty())
            return;

        uint8_t numCols = colMaxWidhtsList.size();
        int16_t totalColsEffectiveWidth = 0;
        for (int16_t w : colMaxWidhtsList) {
            totalColsEffectiveWidth += w;
        }
        int16_t totalColsWidthWithGaps = totalColsEffectiveWidth + (numCols > 1 ? (numCols - 1) * columnGap : 0);

        // --- Elrendezési fázis ---
        int16_t startX = screenWidth - marginRight - totalColsWidthWithGaps;
        int16_t currentLayoutX = startX;

        for (size_t colIndex = 0; colIndex < colsOfButtons.size(); ++colIndex) {
            int16_t currentLayoutY = marginTop;
            const auto &currentCol = colsOfButtons[colIndex];
            for (const auto &def : currentCol) {
                int16_t btnWidth = (def.width > 0) ? def.width : defaultButtonWidthRef;
                int16_t btnHeight = (def.height > 0) ? def.height : defaultButtonHeightRef;

                Rect bounds(currentLayoutX, currentLayoutY, btnWidth, btnHeight);
                auto button = std::make_shared<UIButton>(self->getTft(), def.id, bounds, def.label, def.type, def.initialState, def.callback);
                self->addChild(button);
                if (out_createdButtons) {
                    out_createdButtons->push_back(button);
                }
                currentLayoutY += btnHeight + buttonGap;
            }
            if (colIndex < colsOfButtons.size() - 1) {
                currentLayoutX += colMaxWidhtsList[colIndex] + columnGap;
            }
        }
    }

    /**
     * @brief Gombokat rendez el vízszintesen, a képernyő aljához igazítva.
     * A metódus a leszármazott képernyő (ami UIScreen-ből is származik)
     * tft referenciáját és addChild metódusát használja.
     */
    void layoutHorizontalButtonGroup(const std::vector<ButtonDefinition> &buttonDefs, std::vector<std::shared_ptr<UIButton>> *out_createdButtons = nullptr, int16_t marginLeft = 5,
                                     int16_t marginRight = 5, int16_t marginBottom = 5, int16_t defaultButtonWidthRef = UIButton::DEFAULT_BUTTON_WIDTH,
                                     int16_t defaultButtonHeightRef = UIButton::DEFAULT_BUTTON_HEIGHT, int16_t rowGap = 3, int16_t buttonGap = 3) {

        DerivedScreen *self = static_cast<DerivedScreen *>(this);

        if (buttonDefs.empty()) {
            return;
        }

        const int16_t screenHeight = self->getTft().height();
        const int16_t screenWidth = self->getTft().width();
        const int16_t maxRowWidth = screenWidth - marginLeft - marginRight;

        if (out_createdButtons) {
            out_createdButtons->clear();
        }

        // --- Előfeldolgozási fázis: Sorok struktúrájának és méreteinek meghatározása ---
        std::vector<std::vector<ButtonDefinition>> rowsOfButtons;
        std::vector<int16_t> rowMaxHeightsList;
        std::vector<ButtonDefinition> currentBuildingRowButtons;
        int16_t currentX_build = marginLeft;
        int16_t currentBuildingRowMaxH = 0;

        for (const auto &def : buttonDefs) {
            int16_t btnWidth = (def.width > 0) ? def.width : defaultButtonWidthRef;
            int16_t btnHeight = (def.height > 0) ? def.height : defaultButtonHeightRef;

            if (currentX_build == marginLeft && btnWidth > maxRowWidth) {
                DEBUG("ScreenButtonsManager::layoutHorizontalButtonGroup: Button %d ('%s') too wide for row (%d > %d), skipping.\n", def.id, def.label, btnWidth, maxRowWidth);
                if (!currentBuildingRowButtons.empty()) { // Ha az előző sorban voltak gombok
                    rowsOfButtons.push_back(currentBuildingRowButtons);
                    rowMaxHeightsList.push_back(currentBuildingRowMaxH);
                    currentBuildingRowButtons.clear();
                    currentBuildingRowMaxH = 0;
                }
                currentX_build = marginLeft; // Marad a jelenlegi sor elején a következő gombnak
                continue;
            }

            if (currentX_build + btnWidth > maxRowWidth && currentX_build != marginLeft) { // Új sort kell kezdeni
                rowsOfButtons.push_back(currentBuildingRowButtons);
                rowMaxHeightsList.push_back(currentBuildingRowMaxH);
                currentBuildingRowButtons.clear();
                currentX_build = marginLeft;
                currentBuildingRowMaxH = 0;

                if (btnWidth > maxRowWidth) { // Ellenőrizzük, hogy az új sort kezdő gomb nem túl széles-e
                    DEBUG("ScreenButtonsManager::layoutHorizontalButtonGroup: Button %d ('%s') too wide for new row (%d > %d), skipping.\n", def.id, def.label, btnWidth,
                          maxRowWidth);
                    continue;
                }
            }
            currentBuildingRowButtons.push_back(def);
            currentBuildingRowMaxH = std::max(currentBuildingRowMaxH, btnHeight);
            currentX_build += btnWidth + buttonGap;
        }
        if (!currentBuildingRowButtons.empty()) {
            rowsOfButtons.push_back(currentBuildingRowButtons);
            rowMaxHeightsList.push_back(currentBuildingRowMaxH);
        }

        if (rowsOfButtons.empty())
            return;

        uint8_t numRows = rowMaxHeightsList.size();
        int16_t totalRowsEffectiveHeight = 0;
        for (int16_t h : rowMaxHeightsList) {
            totalRowsEffectiveHeight += h;
        }
        int16_t totalRowsHeightWithGaps = totalRowsEffectiveHeight + (numRows > 1 ? (numRows - 1) * rowGap : 0);

        // --- Elrendezési fázis ---
        int16_t startY = screenHeight - marginBottom - totalRowsHeightWithGaps;
        int16_t currentLayoutY = startY;

        for (size_t rowIndex = 0; rowIndex < rowsOfButtons.size(); ++rowIndex) {
            int16_t currentLayoutX = marginLeft;
            const auto &currentRow = rowsOfButtons[rowIndex];
            for (const auto &def : currentRow) {
                int16_t btnWidth = (def.width > 0) ? def.width : defaultButtonWidthRef;
                int16_t btnHeight = (def.height > 0) ? def.height : defaultButtonHeightRef;

                Rect bounds(currentLayoutX, currentLayoutY, btnWidth, btnHeight);
                auto button = std::make_shared<UIButton>(self->getTft(), def.id, bounds, def.label, def.type, def.initialState, def.callback);
                self->addChild(button);
                if (out_createdButtons) {
                    out_createdButtons->push_back(button);
                }
                currentLayoutX += btnWidth + buttonGap;
            }
            if (rowIndex < rowsOfButtons.size() - 1) {
                currentLayoutY += rowMaxHeightsList[rowIndex] + rowGap;
            }
        }
    }
};

#endif // __SCREEN_BUTTONS_MANAGER_H