// Test program to determine actual display dimensions
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }

    tft.init();
    tft.setRotation(1); // Same rotation as main program

    Serial.println("=== DISPLAY SIZE INFORMATION ===");
    Serial.print("Display width: ");
    Serial.println(tft.width());
    Serial.print("Display height: ");
    Serial.println(tft.height());

    // Test all rotations
    for (int rot = 0; rot < 4; rot++) {
        tft.setRotation(rot);
        Serial.print("Rotation ");
        Serial.print(rot);
        Serial.print(" - Width: ");
        Serial.print(tft.width());
        Serial.print(", Height: ");
        Serial.println(tft.height());
    }

    // Set back to rotation 1 as used in main program
    tft.setRotation(1);
    Serial.println("=== END ===");
}

void loop() {
    // Empty loop
}
