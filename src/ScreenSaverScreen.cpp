#include "ScreenSaverScreen.h"
#include "PicoSensorUtils.h"
#include "UIColorPalette.h"
#include "rtVars.h"

/**
 * @file ScreenSaverScreen.cpp
 * @brief Képernyővédő osztály implementációja
 * @details Animált kerettel és frekvencia kijelzéssel rendelkező képernyővédő
 */

/**
 * @brief Konstruktor
 * @param tft_ref TFT kijelző referencia
 * @param band_ref Sáv objektum referencia
 * @param config_ref Konfiguráció objektum referencia
 * @details Inicializálja az animációs színeket és a frekvencia kijelző komponenst
 */
ScreenSaverScreen::ScreenSaverScreen(TFT_eSPI &tft_ref, Band &band_ref, Config &config_ref)
    : UIScreen(tft_ref, SCREEN_NAME_SCREENSAVER), activationTime(0), lastAnimationUpdateTime(0), animationBorderX(0), animationBorderY(0), currentFrequencyValue(0), posSaver(0),
      band(band_ref), config(config_ref), lastFullUpdateSaverTime(0) {

    // Animációs vonal színeinek előszámítása
    for (uint8_t i = 0; i < ScreenSaverConstants::SAVER_ANIMATION_LINE_LENGTH; i++) {
        // A képlet (31 - abs(i - 31)) értékeket generál 0-tól 31-ig és vissza 0-ig.
        saverLineColors[i] = (ScreenSaverConstants::SAVER_LINE_CENTER - std::abs(static_cast<int>(i) - ScreenSaverConstants::SAVER_LINE_CENTER));
    } // FreqDisplay inicializálása
    // A kezdeti határok helyőrzők, frissítésre kerülnek az updateFrequencyAndBatteryDisplay-ben
    using namespace ScreenSaverConstants;
    Rect initialFreqBounds(0, 0, FREQ_DISPLAY_WIDTH, FREQ_DISPLAY_HEIGHT); // Helyőrző
    freqDisplayComp = std::make_shared<FreqDisplay>(tft, initialFreqBounds, band, config);
    addChild(freqDisplayComp);
}

/**
 * @brief Képernyővédő aktiválása
 * @details Beállítja a képernyővédő módot: kék színek, rejtett aláhúzás, kezdeti pozicionálás
 */
void ScreenSaverScreen::activate() {
    UIScreen::activate(); // Szülő osztály activate hívása
    DEBUG("ScreenSaverScreen activated.\n");
    activationTime = millis();
    lastAnimationUpdateTime = millis();
    lastFullUpdateSaverTime = millis(); // Teljes frissítés időzítő nullázása

    // FreqDisplay konfigurálása képernyővédő módra: kék színek és rejtett aláhúzás
    freqDisplayComp->setCustomColors(UIColorPalette::createScreenSaverFreqColors());
    freqDisplayComp->setHideUnderline(true);

    // Frekvencia és akkumulátor kezdeti elhelyezése
    updateFrequencyAndBatteryDisplay(); // Ez törli a képernyőt és beállítja a kezdeti pozíciókat
    // markForRedraw() hívása az updateFrequencyAndBatteryDisplay-ben történik
}

/**
 * @brief Képernyővédő deaktiválása
 * @details Visszaállítja a FreqDisplay normál módját: alapértelmezett színek és látható aláhúzás
 */
void ScreenSaverScreen::deactivate() {
    UIScreen::deactivate(); // Szülő osztály deactivate hívása
    DEBUG("ScreenSaverScreen deactivated.\n");

    // FreqDisplay visszaállítása normál módra: alapértelmezett színek és látható aláhúzás
    freqDisplayComp->resetToDefaultColors();
    freqDisplayComp->setHideUnderline(false);
}

/**
 * @brief Saját loop kezelése
 * @details Animáció pozíció frissítése és 15 másodpercenkénti teljes frissítés kezelése
 */
void ScreenSaverScreen::handleOwnLoop() {
    uint32_t currentTime = millis();

    // Animáció pozíció számláló növelése
    posSaver++;
    if (posSaver >= ScreenSaverConstants::SAVER_ANIMATION_STEPS) {
        posSaver = 0;
    }

    // 15 másodpercenként teljes frissítés (új pozíció és akkumulátor info)
    if (currentTime - lastFullUpdateSaverTime >= ScreenSaverConstants::SAVER_NEW_POS_INTERVAL_MSEC) {
        lastFullUpdateSaverTime = currentTime;
        updateFrequencyAndBatteryDisplay();
        // updateFrequencyAndBatteryDisplay hívja a markForRedraw()-t
    }

    // Az animáció minden frame-nél újrarajzolást igényel
    // A flag-et a loop végén állítjuk be, hogy biztosítsuk az animáció folytonosságát
    needsRedraw = true;
}

/**
 * @brief Tartalom rajzolása
 * @details FreqDisplay gyermek komponensként rajzolódik, mi csak az animált keretet rajzoljuk
 * Az akkumulátor info csak teljes frissítéskor rajzolódik a folyamatos újrarajzolás elkerülésére
 */
void ScreenSaverScreen::drawContent() {
    // FreqDisplay gyermek komponens és az UIScreen::draw() rajzolja ha needsRedraw true.
    // Mi csak az animált keretet rajzoljuk itt.
    // Az akkumulátor info csak teljes frissítéskor rajzolódik az updateFrequencyAndBatteryDisplay()-ben.

    drawAnimatedBorder();

    // Az akkumulátor info csak teljes frissítéskor rajzolódik a folyamatos újrarajzolás elkerülésére
    // Az updateFrequencyAndBatteryDisplay()-ben kerül kirajzolásra pozícióváltáskor
}

/**
 * @brief Frekvencia és akkumulátor kijelző frissítése
 * @details 15 másodpercenként új véletlenszerű pozícióba helyezi az animált keretet
 * A FreqDisplay és akkumulátor a kerethez relatívan pozícionálódik
 */
void ScreenSaverScreen::updateFrequencyAndBatteryDisplay() {
    tft.fillScreen(TFT_COLOR_BACKGROUND);

    using namespace ScreenSaverConstants;

    // Animált keret véletlenszerű pozíciójának meghatározása
    // Biztosítjuk, hogy a teljes keret a képernyőn maradjon
    uint16_t maxBorderX = tft.width() - ANIMATION_BORDER_WIDTH;
    uint16_t maxBorderY = tft.height() - ANIMATION_BORDER_HEIGHT;

    if (maxBorderX <= 0)
        maxBorderX = 1;
    if (maxBorderY <= 0)
        maxBorderY = 1;

    // Új véletlenszerű pozíció az animált keretnek
    animationBorderX = random(maxBorderX);
    animationBorderY = random(maxBorderY);

    // FreqDisplay pozícionálása a keret bal felső sarkához képest
    uint16_t freqDisplayX = animationBorderX + FREQ_DISPLAY_X_OFFSET;
    uint16_t freqDisplayY = animationBorderY + FREQ_DISPLAY_Y_OFFSET;

    // Aktuális frekvencia beállítása és FreqDisplay frissítése
    currentFrequencyValue = band.getCurrentBand().currFreq;
    if (freqDisplayComp) {
        freqDisplayComp->setBounds(Rect(freqDisplayX, freqDisplayY, FREQ_DISPLAY_WIDTH, FREQ_DISPLAY_HEIGHT));
        freqDisplayComp->setFrequency(currentFrequencyValue);
        freqDisplayComp->markForRedraw(); // FreqDisplay újrarajzolásának biztosítása
    }

    // Akkumulátor info rajzolása (a keret pozíciójához relatívan)
    drawBatteryInfo();

    markForRedraw();
}

/**
 * @brief Animált keret rajzolása
 * @details Fix méretű téglalap keret rajzolása az animationBorderX, animationBorderY pozícióban
 * A keret mérete konstans, és a teljes képernyőn bárhova pozícionálható
 */
void ScreenSaverScreen::drawAnimatedBorder() {
    using namespace ScreenSaverConstants;
    uint16_t t = posSaver;

    // Képernyő méret ellenőrzése
    uint16_t screenW = tft.width();
    uint16_t screenH = tft.height();

    // Animált keret koordinátái
    int16_t rectLeft = animationBorderX;
    int16_t rectRight = animationBorderX + ANIMATION_BORDER_WIDTH;
    int16_t rectTop = animationBorderY;
    int16_t rectBottom = animationBorderY + ANIMATION_BORDER_HEIGHT;

    // Keret képernyőn maradásának biztosítása (csak ha valamiért túlnyúlna)
    if (rectLeft < 0)
        rectLeft = 0;
    if (rectRight >= screenW)
        rectRight = screenW - 1;
    if (rectTop < 0)
        rectTop = 0;
    if (rectBottom >= screenH)
        rectBottom = screenH - 1;

    int16_t rectWidth = rectRight - rectLeft;
    int16_t rectHeight = rectBottom - rectTop;

    // Animációs vonal minden pixelének rajzolása
    for (uint8_t i = 0; i < SAVER_ANIMATION_LINE_LENGTH; i++) {
        uint8_t c_val = saverLineColors[i];
        uint16_t pixel_color = (c_val * SAVER_COLOR_FACTOR) + c_val;

        // Pixel pozíciók kiszámítása téglalap keret animációhoz
        int16_t pixelX, pixelY;

        // Teljes kerület lépések: felső + jobb + alsó + bal
        uint16_t totalSteps = SAVER_ANIMATION_STEPS;
        uint16_t topSteps = totalSteps / 4;    // Felső oldal lépések
        uint16_t rightSteps = totalSteps / 4;  // Jobb oldal lépések
        uint16_t bottomSteps = totalSteps / 4; // Alsó oldal lépések
        uint16_t leftSteps = totalSteps / 4;   // Bal oldal lépések

        if (t < topSteps) {
            // Felső oldal: balról jobbra mozgás
            pixelX = rectLeft + (t * rectWidth) / topSteps;
            pixelY = rectTop;
        } else if (t < topSteps + rightSteps) {
            // Jobb oldal: fentről lefelé mozgás
            pixelX = rectRight;
            pixelY = rectTop + ((t - topSteps) * rectHeight) / rightSteps;
        } else if (t < topSteps + rightSteps + bottomSteps) {
            // Alsó oldal: jobbról balra mozgás
            pixelX = rectRight - ((t - topSteps - rightSteps) * rectWidth) / bottomSteps;
            pixelY = rectBottom;
        } else {
            // Bal oldal: lentről felfelé mozgás
            pixelX = rectLeft;
            pixelY = rectBottom - ((t - topSteps - rightSteps - bottomSteps) * rectHeight) / leftSteps;
        }

        // Pixel rajzolása csak ha a képernyő határain belül van
        if (pixelX >= 0 && pixelX < screenW && pixelY >= 0 && pixelY < screenH) {
            tft.drawPixel(pixelX, pixelY, pixel_color);
        }

        // Következő pixel pozíció kiszámítása
        t += SAVER_ANIMATION_STEP_JUMP;
        if (t >= SAVER_ANIMATION_STEPS) {
            t -= SAVER_ANIMATION_STEPS;
        }
    }
}

/**
 * @brief Akkumulátor információ rajzolása
 * @details Akkumulátor szimbólum (téglalap + dudor) és töltöttségi százalék kirajzolása
 * Az animált keret pozíciójához relatívan pozícionálva
 */
void ScreenSaverScreen::drawBatteryInfo() {
    using namespace ScreenSaverConstants;

    // Akkumulátor feszültség olvasása és százalék kiszámítása
    float vSupply = PicoSensorUtils::readVBus();
    uint8_t bat_percent = map(static_cast<int>(vSupply * 100), MIN_BATTERY_VOLTAGE, MAX_BATTERY_VOLTAGE, 0, 100);
    bat_percent = constrain(bat_percent, 0, 100);

    // Akkumulátor szín meghatározása töltöttség alapján
    uint16_t colorBatt = TFT_DARKCYAN;
    if (bat_percent < 5) {
        colorBatt = UIColorPalette::TFT_COLOR_DRAINED_BATTERY; // Lemerült: vörös
    } else if (bat_percent < 15) {
        colorBatt = UIColorPalette::TFT_COLOR_SUBMERSIBLE_BATTERY; // Alacsony: sárga
    }

    // Akkumulátor pozicionálása az animált keret pozíciójához relatívan
    uint16_t batteryX = animationBorderX + BATTERY_BASE_X_OFFSET;
    uint16_t batteryY = animationBorderY + BATTERY_BASE_Y_OFFSET;

    // Akkumulátor szimbólum rajzolása
    tft.fillRect(batteryX, batteryY, BATTERY_RECT_W, BATTERY_RECT_H, TFT_BLACK); // Terület törlése
    tft.drawRect(batteryX, batteryY, BATTERY_RECT_W, BATTERY_RECT_H, colorBatt);
    tft.drawRect(batteryX + BATTERY_RECT_W, batteryY + (BATTERY_RECT_H - BATTERY_NUB_H) / 2, BATTERY_NUB_W, BATTERY_NUB_H,
                 colorBatt); // Töltöttségi százalék szöveg kiírása az akkumulátor belsejében
    tft.setFreeFont();
    tft.setTextSize(1);
    tft.setTextColor(colorBatt, TFT_BLACK); // Fekete háttér az akkumulátor belsejében
    tft.setTextDatum(MC_DATUM);             // Középre igazított szöveg
    tft.drawString(String(bat_percent) + "%", batteryX + BATTERY_RECT_W / 2, batteryY + BATTERY_RECT_H / 2);
}

/**
 * @brief Érintés esemény kezelése
 * @param event Érintés esemény adatok
 * @return true ha kezelte az eseményt (mindig), false egyébként
 * @details Bármilyen érintés ébreszti a képernyővédőt és visszatér az előző képernyőre
 */
bool ScreenSaverScreen::handleTouch(const TouchEvent &event) {
    if (event.pressed) {
        DEBUG("ScreenSaverScreen: Touch event, waking up.\n");
        if (getManager()) {
            getManager()->goBack(); // Visszatérés az előző képernyőre
        }
        return true;
    }
    return false;
}

/**
 * @brief Forgó encoder esemény kezelése
 * @param event Forgó encoder esemény adatok
 * @return true ha kezelte az eseményt (mindig), false egyébként
 * @details Bármilyen forgó encoder esemény (forgatás vagy kattintás) ébreszti a képernyővédőt
 */
bool ScreenSaverScreen::handleRotary(const RotaryEvent &event) {
    // Bármilyen forgó encoder esemény (forgatás vagy kattintás) ébresztő hatású
    DEBUG("ScreenSaverScreen: Rotary event, waking up.\n");
    if (getManager()) {
        getManager()->goBack(); // Visszatérés az előző képernyőre
    }
    return true;
}