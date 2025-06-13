#include "RDSComponent.h"
#include "utils.h"

// ===================================================================
// Konstruktor és inicializálás
// ===================================================================

/**
 * @brief RDSComponent konstruktor
 */
RDSComponent::RDSComponent(TFT_eSPI &tft, const Rect &bounds, Si4735Manager &manager)
    : UIComponent(tft, bounds, ColorScheme::defaultScheme()), si4735Manager(manager), lastRdsUpdate(0), lastScrollUpdate(0), dataChanged(false), scrollSprite(nullptr),
      scrollOffset(0), radioTextPixelWidth(0), needsScrolling(false), scrollSpriteCreated(false), rdsAvailable(false) {

    // Alapértelmezett színek beállítása
    stationNameColor = TFT_CYAN;   // Állomásnév - cián
    programTypeColor = TFT_ORANGE; // Program típus - narancs
    radioTextColor = TFT_WHITE;    // Radio text - fehér
    dateTimeColor = TFT_YELLOW;    // Dátum/idő - sárga
    backgroundColor = TFT_BLACK;   // Háttér - fekete

    // Alapértelmezett layout számítása
    calculateDefaultLayout();
}

/**
 * @brief Destruktor - erőforrások felszabadítása
 */
RDSComponent::~RDSComponent() { cleanupScrollSprite(); }

// ===================================================================
// Layout és konfigurálás
// ===================================================================

/**
 * @brief Alapértelmezett layout számítása
 */
void RDSComponent::calculateDefaultLayout() {
    const uint16_t margin = 4; // Keret miatti margin
    const uint16_t lineHeight = 18;
    const uint16_t dateTimeWidth = 85;
    const uint16_t stationNameWidth = 180; // Csökkentett szélesség

    // Rendelkezésre álló szélesség számítása
    uint16_t availableWidth = bounds.width - 2 * margin;
    uint16_t remainingWidth = availableWidth - stationNameWidth - dateTimeWidth - 2 * 10; // 10px spacing

    // Állomásnév - felső sor, bal oldal
    stationNameArea = Rect(bounds.x + margin, bounds.y + margin, stationNameWidth, lineHeight);

    // Program típus - felső sor, közép (dinamikus szélesség)
    uint16_t ptyX = bounds.x + margin + stationNameWidth + 10;
    programTypeArea = Rect(ptyX, bounds.y + margin, remainingWidth, lineHeight);

    // Dátum/idő - felső sor, jobb szél
    dateTimeArea = Rect(bounds.x + bounds.width - margin - dateTimeWidth, bounds.y + margin, dateTimeWidth, lineHeight);

    // Radio text - alsó sor, teljes szélesség (keret miatt csökkentett)
    radioTextArea = Rect(bounds.x + margin, bounds.y + margin + lineHeight + 4, bounds.width - 2 * margin, lineHeight * 2);
}

/**
 * @brief Állomásnév területének beállítása
 */
void RDSComponent::setStationNameArea(const Rect &area) { stationNameArea = area; }

/**
 * @brief Program típus területének beállítása
 */
void RDSComponent::setProgramTypeArea(const Rect &area) { programTypeArea = area; }

/**
 * @brief Radio text területének beállítása
 */
void RDSComponent::setRadioTextArea(const Rect &area) {
    radioTextArea = area;
    // Ha változott a terület, újra kell inicializálni a scroll sprite-ot
    if (scrollSpriteCreated) {
        cleanupScrollSprite();
        initializeScrollSprite();
    }
}

/**
 * @brief Dátum/idő területének beállítása
 */
void RDSComponent::setDateTimeArea(const Rect &area) { dateTimeArea = area; }

/**
 * @brief RDS színek testreszabása
 */
void RDSComponent::setRdsColors(uint16_t stationColor, uint16_t typeColor, uint16_t textColor, uint16_t timeColor, uint16_t bgColor) {
    stationNameColor = stationColor;
    programTypeColor = typeColor;
    radioTextColor = textColor;
    dateTimeColor = timeColor;
    backgroundColor = bgColor;

    // Ha már létezik scroll sprite, frissítjük a színeit
    if (scrollSprite && scrollSpriteCreated) {
        scrollSprite->setTextColor(radioTextColor, backgroundColor);
    }
}

// ===================================================================
// Scroll sprite kezelés
// ===================================================================

/**
 * @brief Scroll sprite inicializálása
 */
void RDSComponent::initializeScrollSprite() {
    if (scrollSprite || scrollSpriteCreated) {
        cleanupScrollSprite();
    }

    if (radioTextArea.width > 0 && radioTextArea.height > 0) {
        scrollSprite = new TFT_eSprite(&tft);
        if (scrollSprite->createSprite(radioTextArea.width, radioTextArea.height)) {
            scrollSprite->setFreeFont(); // Alapértelmezett font
            scrollSprite->setTextSize(1);
            scrollSprite->setTextColor(radioTextColor, backgroundColor);
            scrollSprite->setTextDatum(TL_DATUM);
            scrollSpriteCreated = true;
        } else {
            delete scrollSprite;
            scrollSprite = nullptr;
            scrollSpriteCreated = false;
        }
    }
}

/**
 * @brief Scroll sprite felszabadítása
 */
void RDSComponent::cleanupScrollSprite() {
    if (scrollSprite) {
        if (scrollSpriteCreated) {
            scrollSprite->deleteSprite();
        }
        delete scrollSprite;
        scrollSprite = nullptr;
        scrollSpriteCreated = false;
    }
}

// ===================================================================
// RDS adatok kezelése
// ===================================================================

/**
 * @brief RDS adatok frissítése a Si4735Manager-től
 */
void RDSComponent::updateRdsData() {
    uint32_t currentTime = millis();

    // Időzített frissítés
    if (currentTime - lastRdsUpdate < RDS_UPDATE_INTERVAL_MS) {
        return;
    }
    lastRdsUpdate = currentTime;

    // TESZT RDS ADATOK - Ideiglenesen az RDS funkció tesztelésére
    static bool testDataSet = false;
    if (!testDataSet) {
        cachedStationName = "TEST FM";
        cachedProgramType = "Rock Music";
        cachedRadioText = "Ez egy teszt radio text üzenet amely hosszabb mint a kijelző szélessége";
        cachedDateTime = "12:34 2025-06-13";
        rdsAvailable = true;
        dataChanged = true;
        testDataSet = true;
        return;
    }

    // RDS elérhetőség ellenőrzése - eredeti kód
    bool newRdsAvailable = si4735Manager.isRdsAvailable();

    if (!newRdsAvailable) {
        // Nincs RDS - töröljük az adatokat ha voltak
        if (rdsAvailable || !cachedStationName.isEmpty() || !cachedProgramType.isEmpty() || !cachedRadioText.isEmpty() || !cachedDateTime.isEmpty()) {

            cachedStationName = "";
            cachedProgramType = "";
            cachedRadioText = "";
            cachedDateTime = "";
            rdsAvailable = false;
            dataChanged = true;
        }
        return;
    }

    rdsAvailable = true;
    dataChanged = false;

    // Állomásnév frissítése
    String newStationName = si4735Manager.getRdsStationName();
    if (newStationName != cachedStationName) {
        cachedStationName = newStationName;
        dataChanged = true;
    }

    // Program típus frissítése
    String newProgramType = si4735Manager.getRdsProgramType();
    if (newProgramType != cachedProgramType) {
        cachedProgramType = newProgramType;
        dataChanged = true;
    }

    // Radio text frissítése
    String newRadioText = si4735Manager.getRdsRadioText();
    if (newRadioText != cachedRadioText) {
        cachedRadioText = newRadioText;
        dataChanged = true;

        // Radio text változott - scroll újraszámítás
        if (!cachedRadioText.isEmpty()) {
            tft.setFreeFont();
            tft.setTextSize(1);
            radioTextPixelWidth = tft.textWidth(cachedRadioText);
            needsScrolling = (radioTextPixelWidth > radioTextArea.width);
            scrollOffset = 0; // Scroll restart
        } else {
            needsScrolling = false;
        }
    }

    // Dátum/idő frissítése
    uint16_t year, month, day, hour, minute;
    if (si4735Manager.getRdsDateTime(year, month, day, hour, minute)) {
        String newDateTime = String(hour < 10 ? "0" : "") + String(hour) + ":" + String(minute < 10 ? "0" : "") + String(minute);
        if (newDateTime != cachedDateTime) {
            cachedDateTime = newDateTime;
            dataChanged = true;
        }
    }
}

// ===================================================================
// Rajzolási metódusok
// ===================================================================

/**
 * @brief Állomásnév kirajzolása
 */
void RDSComponent::drawStationName() {
    if (cachedStationName.isEmpty()) {
        // Terület törlése ha nincs adat
        tft.fillRect(stationNameArea.x, stationNameArea.y, stationNameArea.width, stationNameArea.height, backgroundColor);
        return;
    }

    tft.setFreeFont();
    tft.setTextSize(1);
    tft.setTextColor(stationNameColor, backgroundColor);
    tft.setTextDatum(TL_DATUM);

    // Háttér törlése
    tft.fillRect(stationNameArea.x, stationNameArea.y, stationNameArea.width, stationNameArea.height, backgroundColor);

    // Szöveg kirajzolása
    tft.drawString(cachedStationName, stationNameArea.x, stationNameArea.y);
}

/**
 * @brief Program típus kirajzolása
 */
void RDSComponent::drawProgramType() {
    if (cachedProgramType.isEmpty()) {
        // Terület törlése ha nincs adat
        tft.fillRect(programTypeArea.x, programTypeArea.y, programTypeArea.width, programTypeArea.height, backgroundColor);
        return;
    }

    tft.setFreeFont();
    tft.setTextSize(1);
    tft.setTextColor(programTypeColor, backgroundColor);
    tft.setTextDatum(TL_DATUM);

    // Háttér törlése
    tft.fillRect(programTypeArea.x, programTypeArea.y, programTypeArea.width, programTypeArea.height, backgroundColor);

    // Szöveg kirajzolása
    tft.drawString(cachedProgramType, programTypeArea.x, programTypeArea.y);
}

/**
 * @brief Radio text kirajzolása (scroll támogatással)
 */
void RDSComponent::drawRadioText() {
    if (cachedRadioText.isEmpty()) {
        // Terület törlése ha nincs adat
        tft.fillRect(radioTextArea.x, radioTextArea.y, radioTextArea.width, radioTextArea.height, backgroundColor);
        return;
    }

    if (!needsScrolling) {
        // Egyszerű megjelenítés, ha elfér
        tft.setFreeFont();
        tft.setTextSize(1);
        tft.setTextColor(radioTextColor, backgroundColor);
        tft.setTextDatum(TL_DATUM);

        // Háttér törlése
        tft.fillRect(radioTextArea.x, radioTextArea.y, radioTextArea.width, radioTextArea.height, backgroundColor);

        // Szöveg kirajzolása
        tft.drawString(cachedRadioText, radioTextArea.x, radioTextArea.y);
    } else {
        // Scroll esetén sprite használata
        if (!scrollSpriteCreated) {
            initializeScrollSprite();
        }

        if (scrollSprite && scrollSpriteCreated) {
            handleRadioTextScroll();
        }
    }
}

/**
 * @brief Dátum és idő kirajzolása
 */
void RDSComponent::drawDateTime() {
    if (cachedDateTime.isEmpty()) {
        // Terület törlése ha nincs adat
        tft.fillRect(dateTimeArea.x, dateTimeArea.y, dateTimeArea.width, dateTimeArea.height, backgroundColor);
        return;
    }

    tft.setFreeFont();
    tft.setTextSize(1);
    tft.setTextColor(dateTimeColor, backgroundColor);
    tft.setTextDatum(TR_DATUM); // Jobbra igazított

    // Háttér törlése
    tft.fillRect(dateTimeArea.x, dateTimeArea.y, dateTimeArea.width, dateTimeArea.height, backgroundColor);

    // Szöveg kirajzolása (jobb oldali igazítás)
    tft.drawString(cachedDateTime, dateTimeArea.x + dateTimeArea.width, dateTimeArea.y);
}

/**
 * @brief Radio text scroll kezelése
 */
void RDSComponent::handleRadioTextScroll() {
    if (!scrollSprite || !scrollSpriteCreated || !needsScrolling) {
        return;
    }

    uint32_t currentTime = millis();
    if (currentTime - lastScrollUpdate < SCROLL_INTERVAL_MS) {
        return;
    }
    lastScrollUpdate = currentTime;

    // Sprite törlése
    scrollSprite->fillScreen(backgroundColor);

    // Fő szöveg rajzolása (balra mozog)
    scrollSprite->drawString(cachedRadioText, -scrollOffset, 0);

    // Ha szükséges, "újra beúszó" szöveg rajzolása
    const int gapPixels = radioTextArea.width; // Szóköz a szöveg vége és újrakezdés között
    int secondTextX = -scrollOffset + radioTextPixelWidth + gapPixels;

    if (secondTextX < radioTextArea.width) {
        scrollSprite->drawString(cachedRadioText, secondTextX, 0);
    }

    // Sprite kirakása a képernyőre
    scrollSprite->pushSprite(radioTextArea.x, radioTextArea.y);

    // Scroll pozíció frissítése
    scrollOffset += SCROLL_STEP_PIXELS;

    // Ciklus újraindítása
    if (scrollOffset >= radioTextPixelWidth + gapPixels) {
        scrollOffset = 0;
    }
}

// ===================================================================
// UIComponent interface implementáció
// ===================================================================

/**
 * @brief Komponens teljes újrarajzolása
 */
void RDSComponent::draw() {
    // Teljes háttér törlése
    tft.fillRect(bounds.x, bounds.y, bounds.width, bounds.height, backgroundColor);

    // Keret rajzolása az RDS komponens köré
    tft.drawRect(bounds.x, bounds.y, bounds.width, bounds.height, TFT_DARKGREY);
    // Második vonal a szebb hatás érdekében
    tft.drawRect(bounds.x + 1, bounds.y + 1, bounds.width - 2, bounds.height - 2, TFT_LIGHTGREY);

    // Minden elem újrarajzolása
    drawStationName();
    drawProgramType();
    drawRadioText();
    drawDateTime();
}

/**
 * @brief Újrarajzolásra jelölés
 */
void RDSComponent::markForRedraw(bool markChildren) {
    UIComponent::markForRedraw(markChildren);
    // Nem állítjuk be a dataChanged-et automatikusan
    // Az csak akkor legyen true, ha tényleg változtak az RDS adatok
}

// ===================================================================
// Publikus interface
// ===================================================================

/**
 * @brief RDS adatok frissítése (loop-ban hívandó)
 */
void RDSComponent::updateRDS() {
    updateRdsData();

    // Ha a UIComponent szintjén újrarajzolás szükséges, akkor teljes újrarajzolás
    if (isRedrawNeeded()) {
        draw();
        needsRedraw = false; // Fontos: töröljük a flag-et
        return;
    }

    // Egyébként csak akkor rajzoljuk újra, ha változtak az adatok
    if (dataChanged) {
        // Csak az érintett részek újrarajzolása (nem a keret!)
        drawStationName();
        drawProgramType();
        drawRadioText();
        drawDateTime();
        dataChanged = false;
    }

    // Scroll kezelése még akkor is, ha nincs adatváltozás
    if (needsScrolling) {
        handleRadioTextScroll();
    }
}

/**
 * @brief RDS adatok törlése
 */
void RDSComponent::clearRDS() {
    cachedStationName = "";
    cachedProgramType = "";
    cachedRadioText = "";
    cachedDateTime = "";
    rdsAvailable = false;
    needsScrolling = false;
    scrollOffset = 0;

    draw(); // Teljes törlés

    cleanupScrollSprite();
}

/**
 * @brief Ellenőrzi, hogy van-e érvényes RDS adat
 */
bool RDSComponent::hasValidRDS() const {
    return rdsAvailable && (!cachedStationName.isEmpty() || !cachedProgramType.isEmpty() || !cachedRadioText.isEmpty() || !cachedDateTime.isEmpty());
}
