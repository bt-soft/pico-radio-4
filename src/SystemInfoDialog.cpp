#include "SystemInfoDialog.h"

SystemInfoDialog::SystemInfoDialog(UIScreen *parentScreen, TFT_eSPI &tft, const Rect &bounds, const ColorScheme &cs)
    : MessageDialog(parentScreen, tft, bounds, "System Information", "", ButtonsType::Ok, cs) {
    // Üres message stringet adunk át, mert saját drawSelf-fel rajzoljuk a tartalmat
}

String SystemInfoDialog::buildSystemInfoText() {
    String info = "";
    info += formatProgramInfo();
    info += formatMemoryInfo();
    info += formatHardwareInfo();
    info += formatSi4735Info();
    return info;
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
    info += "Display: TFT 320x240\n\n"; // Hardkódolt érték, mivel a TFT közvetlenül nem érhető el
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

void SystemInfoDialog::drawSelf() {
    // Előbb rajzoljuk az alapértelmezett dialógus hátteret és fejlécet (de nem a message-t)
    UIDialogBase::drawSelf();

    // Rendszerinformációk összeállítása
    String infoContent = buildSystemInfoText();

    // Szöveg tulajdonságok beállítása
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
}
