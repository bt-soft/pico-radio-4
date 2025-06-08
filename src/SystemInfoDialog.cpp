#include "SystemInfoDialog.h"

SystemInfoDialog::SystemInfoDialog(UIScreen *parentScreen, TFT_eSPI &tft, const Rect &bounds, const ColorScheme &cs)
    : MessageDialog(parentScreen, tft, bounds, "System Information", "", ButtonsType::Ok, cs), currentPage(0) {
    // Üres message stringet adunk át, mert saját drawSelf-fel rajzoljuk a tartalmat
    // Lapozós navigáció gombokkal történik
    // A navigációs gombokat a layoutDialogContent()-ben hozzuk létre

    // WICHTIG: Explicit call to layoutDialogContent() needed because the virtual call
    // in MessageDialog constructor called MessageDialog::layoutDialogContent(), not our override
    DEBUG("SystemInfoDialog constructor - calling layoutDialogContent() explicitly\n");
    layoutDialogContent();
}

String SystemInfoDialog::getCurrentPageContent() {
    DEBUG("SystemInfoDialog::getCurrentPageContent() - Getting content for page %d\n", currentPage);
    switch (currentPage) {
        case 0:
            return formatProgramInfo();
        case 1:
            return formatMemoryInfo();
        case 2:
            return formatHardwareInfo();
        case 3:
            return formatSi4735Info();
        default:
            return "Invalid page";
    }
}

String SystemInfoDialog::formatProgramInfo() {
    String info = "";
    info += "=== Program Information ===\n";
    info += "Name: " PROGRAM_NAME "\n";
    info += "Version: " PROGRAM_VERSION "\n";
    info += "Author: " PROGRAM_AUTHOR "\n";
    info += "Built: " __DATE__ " " __TIME__ "\n\n";
    return info;
}

String SystemInfoDialog::formatMemoryInfo() {
    // Aktuális memória állapot lekérése
    PicoMemoryInfo::MemoryStatus_t memStatus = PicoMemoryInfo::getMemoryStatus();

    // EEPROM használat számítása
    uint32_t eepromUsed = EEPROM_AM_STATIONS_ADDR + AM_STATIONS_REQUIRED_SIZE;
    uint32_t eepromFree = EEPROM_SIZE - eepromUsed;
    float eepromUsedPercent = (eepromUsed * 100.0f) / EEPROM_SIZE;

    String info = "";
    info += "=== Memory Information ===\n\n";

    info += "Flash Memory:\n";
    info += "  Total: " + String(FULL_FLASH_SIZE / 1024) + " kB\n";
    info += "  Used: " + String(memStatus.programSize / 1024) + " kB (" + String(memStatus.programPercent, 1) + "%)\n";
    info += "  Free: " + String(memStatus.freeFlash / 1024) + " kB\n\n";

    info += "RAM (Heap):\n";
    info += "  Total: " + String(memStatus.heapSize / 1024) + " kB\n";
    info += "  Used: " + String(memStatus.usedHeap / 1024) + " kB (" + String(memStatus.usedHeapPercent, 1) + "%)\n";
    info += "  Free: " + String(memStatus.freeHeap / 1024) + " kB\n\n";

    info += "EEPROM Storage:\n";
    info += "  Total: " + String(EEPROM_SIZE) + " B\n";
    info += "  Used: " + String(eepromUsed) + " B (" + String(eepromUsedPercent, 1) + "%)\n";
    info += "  Free: " + String(eepromFree) + " B";

    return info;
}

String SystemInfoDialog::formatHardwareInfo() {
    String info = "";
    info += "=== Hardware Information ===\n";
    info += "MCU: RP2040 @ 133MHz\n";
    info += "Display: TFT 320x240\n\n";
    return info;
}

String SystemInfoDialog::formatSi4735Info() {
    String info = "";
    info += "=== Radio Information ===\n";
    info += "Chip: Si4735\n";
    info += "Status: (To be implemented)\n";
    return info;
}

void SystemInfoDialog::layoutDialogContent() {
    // Először hívjuk meg a szülő osztály layout metódusát, amely létrehozza az OK gombot
    MessageDialog::layoutDialogContent();

    // Most, hogy az OK gomb létezik, kiszámíthatjuk a navigációs gombok helyes pozícióját
    const auto &buttonsList = getButtonsList();

    // Debug: Dialógus bounds információ
    DEBUG("SystemInfoDialog::layoutDialogContent() - Dialog bounds: x=%d, y=%d, w=%d, h=%d\n", bounds.x, bounds.y, bounds.width, bounds.height);
    DEBUG("SystemInfoDialog::layoutDialogContent() - OK buttons count: %zu\n", buttonsList.size());

    // Töröljük a régi navigációs gombokat, ha léteznek
    if (prevButton) {
        removeChild(prevButton);
        prevButton = nullptr;
    }
    if (nextButton) {
        removeChild(nextButton);
        nextButton = nullptr;
    }

    // Navigációs gombok létrehozása a dialógus alján
    const int16_t buttonWidth = 80;
    const int16_t buttonHeight = UIButton::DEFAULT_BUTTON_HEIGHT;
    const int16_t buttonY = bounds.y + bounds.height - buttonHeight - 10;
    const int16_t spacing = 10;
    DEBUG("SystemInfoDialog::layoutDialogContent() - Navigation buttons: buttonY=%d, buttonHeight=%d\n", buttonY, buttonHeight);

    // Previous button pozíció kiszámítása
    int16_t prevButtonX = bounds.x + spacing;
    DEBUG("SystemInfoDialog::layoutDialogContent() - Prev button position: prevButtonX=%d\n", prevButtonX);

    // Navigációs gombok színsémája - ButtonColorScheme létrehozása
    ButtonColorScheme navButtonScheme = UIColorPalette::createDefaultButtonScheme();
    navButtonScheme.background = UIColorPalette::BUTTON_DEFAULT_BACKGROUND;
    navButtonScheme.foreground = UIColorPalette::BUTTON_DEFAULT_TEXT;
    navButtonScheme.border = UIColorPalette::BUTTON_DEFAULT_BORDER; // Previous gomb (bal oldal)
    prevButton = std::make_shared<UIButton>(
        tft, 200, // ID = 200
        Rect(prevButtonX, buttonY, buttonWidth, buttonHeight), "< Prev", UIButton::ButtonType::Pushable,
        [this](const UIButton::ButtonEvent &event) {
            DEBUG("SystemInfoDialog - Prev button event: state=%d, currentPage=%d\n", (int)event.state, currentPage);
            if (event.state == UIButton::EventButtonState::Clicked && currentPage > 0) {
                DEBUG("SystemInfoDialog - Prev button clicked: going from page %d to %d\n", currentPage, currentPage - 1);
                currentPage--;
                updateNavigationButtons();
                markForRedraw();
                drawSelf(); // Explicit redraw to update page content
                DEBUG("SystemInfoDialog - Prev button: explicit drawSelf() called, page now %d\n", currentPage);
            }
        },
        navButtonScheme); // Next gomb pozicionálása a dialógus jobb szélétől
    int16_t nextButtonX = bounds.x + bounds.width - spacing - buttonWidth;

    DEBUG("SystemInfoDialog::layoutDialogContent() - Next button position: nextButtonX=%d (dialog right edge)\n", nextButtonX);
    DEBUG("SystemInfoDialog::layoutDialogContent() - Button placement check: dialog_right=%d, button_right=%d\n", bounds.x + bounds.width, nextButtonX + buttonWidth);
    nextButton = std::make_shared<UIButton>(
        tft, 201, // ID = 201
        Rect(nextButtonX, buttonY, buttonWidth, buttonHeight), "Next >", UIButton::ButtonType::Pushable,
        [this](const UIButton::ButtonEvent &event) {
            DEBUG("SystemInfoDialog - Next button event: state=%d, currentPage=%d, maxPages=%d\n", (int)event.state, currentPage, TOTAL_PAGES - 1);
            DEBUG("SystemInfoDialog - Next button bounds: x=%d, y=%d, w=%d, h=%d\n", nextButton->getBounds().x, nextButton->getBounds().y, nextButton->getBounds().width,
                  nextButton->getBounds().height);
            DEBUG("SystemInfoDialog - Next button disabled: %s\n", nextButton->isDisabled() ? "true" : "false");
            if (event.state == UIButton::EventButtonState::Clicked) {
                DEBUG("SystemInfoDialog - Next button clicked! Checking conditions...\n");
                if (currentPage < TOTAL_PAGES - 1) {
                    DEBUG("SystemInfoDialog - Next button: valid click, going from page %d to %d\n", currentPage, currentPage + 1);
                    currentPage++;
                    updateNavigationButtons();
                    markForRedraw();
                    drawSelf(); // Explicit redraw to update page content
                    DEBUG("SystemInfoDialog - Next button: explicit drawSelf() called, page now %d\n", currentPage);
                } else {
                    DEBUG("SystemInfoDialog - Next button: click ignored, already at last page %d\n", currentPage);
                }
            } else {
                DEBUG("SystemInfoDialog - Next button: non-click event, state=%d\n", (int)event.state);
            }
        },
        navButtonScheme); // Gombok hozzáadása a komponens gyűjteményhez
    DEBUG("SystemInfoDialog::layoutDialogContent() - Adding navigation buttons to dialog\n");
    addChild(prevButton);
    addChild(nextButton);

    // Biztosítjuk, hogy a gombok újrarajzolásra kerüljenek
    if (prevButton) {
        prevButton->markForRedraw();
        DEBUG("SystemInfoDialog::layoutDialogContent() - Prev button marked for redraw, bounds: x=%d, y=%d, w=%d, h=%d\n", prevButton->getBounds().x, prevButton->getBounds().y,
              prevButton->getBounds().width, prevButton->getBounds().height);
    }
    if (nextButton) {
        nextButton->markForRedraw();
        DEBUG("SystemInfoDialog::layoutDialogContent() - Next button marked for redraw, bounds: x=%d, y=%d, w=%d, h=%d\n", nextButton->getBounds().x, nextButton->getBounds().y,
              nextButton->getBounds().width, nextButton->getBounds().height);
        DEBUG("SystemInfoDialog::layoutDialogContent() - Next button initially disabled: %s\n", nextButton->isDisabled() ? "true" : "false");
    }

    updateNavigationButtons();
}

void SystemInfoDialog::updateNavigationButtons() {
    DEBUG("SystemInfoDialog::updateNavigationButtons() - currentPage=%d, TOTAL_PAGES=%d\n", currentPage, TOTAL_PAGES);

    // Gombok aktiválása/deaktiválása az aktuális oldal alapján
    if (prevButton) {
        bool prevEnabled = (currentPage > 0);
        prevButton->setEnabled(prevEnabled);
        DEBUG("SystemInfoDialog::updateNavigationButtons() - Prev button enabled: %s\n", prevEnabled ? "true" : "false");
    }
    if (nextButton) {
        bool nextEnabled = (currentPage < TOTAL_PAGES - 1);
        nextButton->setEnabled(nextEnabled);
        DEBUG("SystemInfoDialog::updateNavigationButtons() - Next button enabled: %s (page %d < max %d)\n", nextEnabled ? "true" : "false", currentPage, TOTAL_PAGES - 1);
        DEBUG("SystemInfoDialog::updateNavigationButtons() - Next button bounds after update: x=%d, y=%d, w=%d, h=%d\n", nextButton->getBounds().x, nextButton->getBounds().y,
              nextButton->getBounds().width, nextButton->getBounds().height);
    }
}

void SystemInfoDialog::drawSelf() {
    DEBUG("SystemInfoDialog::drawSelf() - Drawing page %d/%d\n", currentPage + 1, TOTAL_PAGES);
    // Előbb rajzoljuk az alapértelmezett dialógus hátteret és fejlécet (de nem a message-t)
    UIDialogBase::drawSelf();

    // Aktuális oldal tartalmának lekérése
    String infoContent = getCurrentPageContent();

    // Oldalszám megjelenítése a fejlécben - megfelelő pozícióban és háttérrel
    String pageInfo = "(Page " + String(currentPage + 1) + "/" + String(TOTAL_PAGES) + ")"; // Oldalszám kirajzolása a fejlécbe, megfelelő színekkel és pozícióval
    tft.setTextColor(UIColorPalette::DIALOG_HEADER_TEXT, UIColorPalette::DIALOG_HEADER_BACKGROUND);
    tft.setFreeFont(); // Kisebb alapértelmezett font a lapoinfo számára
    tft.setTextSize(1);
    tft.setTextDatum(MR_DATUM); // Middle-Right pozicionálás

    // Pozíció: jobb oldal, fejléc közepén, X gomb előtt
    int16_t pageInfoX = bounds.x + bounds.width - 30;       // X gomb előtt 30 pixel
    int16_t pageInfoY = bounds.y + (getHeaderHeight() / 2); // Fejléc közepén, mint a cím
    tft.drawString(pageInfo.c_str(), pageInfoX, pageInfoY);

    // Szöveg tulajdonságok visszaállítása a tartalomhoz
    tft.setTextColor(colors.foreground, colors.background);
    tft.setTextDatum(TL_DATUM); // Top-Left pozicionálás
    tft.setTextSize(1);
    tft.setFreeFont(); // Alapértelmezett kisebb font a jobb olvashatósághoz

    // Szöveges terület számítása (fejléc és gomb terület nélkül)
    const int16_t margin = 8;
    const int16_t buttonHeight = UIButton::DEFAULT_BUTTON_HEIGHT;
    const int16_t textStartX = bounds.x + margin;
    const int16_t textStartY = bounds.y + getHeaderHeight() + margin;
    const int16_t textWidth = bounds.width - (2 * margin);
    const int16_t textHeight = bounds.height - getHeaderHeight() - buttonHeight - (3 * margin);

    // Tartalom sorokra bontása és kirajzolása
    int16_t currentY = textStartY;
    const int16_t lineHeight = tft.fontHeight() + 2; // Kis extra hely a sorok között

    // Egyszerű soronkénti kirajzolás
    String remainingText = infoContent;
    int lineStart = 0;

    while (lineStart < remainingText.length() && currentY < (textStartY + textHeight - lineHeight)) {
        int lineEnd = remainingText.indexOf('\n', lineStart);
        if (lineEnd == -1) {
            lineEnd = remainingText.length();
        }

        String line = remainingText.substring(lineStart, lineEnd);

        // Hosszú sorok csonkítása, ha szükséges
        int16_t textWidth_px = tft.textWidth(line.c_str());
        if (textWidth_px > textWidth) {
            // Jó törési pont keresése - csonkítjuk "..."-tal
            while (line.length() > 3 && tft.textWidth(line.c_str()) > textWidth - 20) {
                line = line.substring(0, line.length() - 1);
            }
            line += "...";
        }
        tft.drawString(line.c_str(), textStartX, currentY);
        currentY += lineHeight;
        lineStart = lineEnd + 1;
    }

    // FONTOS: Összes gomb újrarajzolása, mert a UIDialogBase::drawSelf() felülírja őket

    // MessageDialog OK gombjai újrarajzolása
    const auto &buttonsList = getButtonsList();
    for (const auto &button : buttonsList) {
        if (button) {
            button->draw();
            DEBUG("SystemInfoDialog::drawSelf() - Redrawn MessageDialog button after content\n");
        }
    }

    // Navigációs gombok újrarajzolása
    if (prevButton) {
        prevButton->draw();
        DEBUG("SystemInfoDialog::drawSelf() - Redrawn Previous button after content\n");
    }
    if (nextButton) {
        nextButton->draw();
        DEBUG("SystemInfoDialog::drawSelf() - Redrawn Next button after content\n");
    }
}
