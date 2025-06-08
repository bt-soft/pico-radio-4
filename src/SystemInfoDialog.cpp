#include "SystemInfoDialog.h"

SystemInfoDialog::SystemInfoDialog(UIScreen *parentScreen, TFT_eSPI &tft, const Rect &bounds) : UIDialogBase(parentScreen, tft, bounds, "System Information") {

    // Rendszer információk összegyűjtése
    collectSystemInfo();

    // Dialógus tartalmának létrehozása
    createDialogContent();
}

void SystemInfoDialog::collectSystemInfo() {
    infoContent = "";
    infoContent += formatProgramInfo();
    infoContent += formatMemoryInfo();
    infoContent += formatHardwareInfo();
    infoContent += formatSi4735Info();
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
    info += "=== Memory Information ===\n";

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
    info += "  Free: " + String(eepromFree) + " B\n\n";

    return info;
}

String SystemInfoDialog::formatHardwareInfo() {
    String info = "";
    info += "=== Hardware Information ===\n";
    info += "MCU: RP2040 @ 133MHz\n";
    info += "Flash: 2MB\n";
    info += "RAM: 256kB\n";
    info += "Display: TFT " + String(tft.width()) + "x" + String(tft.height()) + "\n\n";
    return info;
}

String SystemInfoDialog::formatSi4735Info() {
    String info = "";
    info += "=== Radio Information ===\n";
    info += "Chip: Si4735\n";
    info += "Status: (To be implemented)\n";
    info += "Firmware: (To be implemented)\n";
    info += "Frequency: (To be implemented)\n\n";
    return info;
}

void SystemInfoDialog::createDialogContent() {
    // Tartalom területének számítása
    const int16_t margin = 10;
    const int16_t buttonHeight = UIButton::DEFAULT_BUTTON_HEIGHT;
    const int16_t buttonWidth = 80;

    // OK gomb létrehozása - dialógus jobb alsó sarkában
    Rect buttonBounds(bounds.width - buttonWidth - margin, bounds.height - buttonHeight - margin, buttonWidth, buttonHeight);

    okButton = std::make_shared<UIButton>(tft, 0, buttonBounds, "OK", UIButton::ButtonType::Pushable, UIButton::ButtonState::Off, [this](const UIButton::ButtonEvent &event) {
        if (event.state == UIButton::EventButtonState::Clicked) {
            onOkButtonClicked();
        }
    });

    addChild(okButton);
}

void SystemInfoDialog::layoutDialogContent() {
    // Az elrendezés a createDialogContent-ben történik
    // Ez a metódus dinamikus elrendezés frissítésekhez használható, ha szükséges
}

void SystemInfoDialog::drawSelf() {
    // Előbb rajzoljuk az alapértelmezett dialógus hátteret és fejlécet
    UIDialogBase::drawSelf();

    // Szöveg tulajdonságok beállítása
    tft.setTextColor(TFT_WHITE, colors.background);
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);

    // Kisebb font használata jobb információsűrűség érdekében
    tft.setFreeFont(); // Alapértelmezett font a jobb kompatibilitás érdekében

    // Szöveges terület számítása (fejléc és gomb terület nélkül)
    const int16_t margin = 8;
    const int16_t buttonHeight = UIButton::DEFAULT_BUTTON_HEIGHT;
    const int16_t textStartX = bounds.x + margin;
    const int16_t textStartY = bounds.y + getHeaderHeight() + margin;
    const int16_t textWidth = bounds.width - (2 * margin);
    const int16_t textHeight = bounds.height - getHeaderHeight() - buttonHeight - (3 * margin);

    // Tartalom sorokra bontása és kirajzolása
    int16_t currentY = textStartY;
    const int16_t lineHeight = 12; // Sűrűbb sorok a jobb kihasználás érdekében    // Egyszerű soronkénti kirajzolás
    String remainingText = infoContent;
    int lineStart = 0;

    while (lineStart < remainingText.length() && currentY < (textStartY + textHeight - lineHeight)) {
        int lineEnd = remainingText.indexOf('\n', lineStart);
        if (lineEnd == -1) {
            lineEnd = remainingText.length();
        }

        String line = remainingText.substring(lineStart, lineEnd);

        // Egyszerű sortörés - ha a sor túl hosszú, csonkítjuk "..."-tal
        int16_t textWidth_px = tft.textWidth(line.c_str());
        if (textWidth_px > textWidth) {
            // Jó törési pont keresése
            while (line.length() > 3 && tft.textWidth(line.c_str()) > textWidth - 20) {
                line = line.substring(0, line.length() - 1);
            }
            line += "...";
        }

        tft.drawString(line.c_str(), textStartX, currentY);
        currentY += lineHeight;
        lineStart = lineEnd + 1;
    }
}

void SystemInfoDialog::onOkButtonClicked() {
    // Dialógus bezárása
    close(DialogResult::Accepted);
}
