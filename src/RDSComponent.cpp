#include "RDSComponent.h"
#include "utils.h"

// ===================================================================
// PTY (Program Type) tábla - Statikus definíciók
// ===================================================================

/**
 * @brief RDS Program Type (PTY) nevek táblája
 * @details Az RDS standard 32 különböző program típust definiál (0-31).
 */
const char *RDSComponent::RDS_PTY_NAMES[] = {
    "No defined",            // 0
    "News",                  // 1
    "Current affairs",       // 2
    "Information",           // 3
    "Sport",                 // 4
    "Education",             // 5
    "Drama",                 // 6
    "Culture",               // 7
    "Science",               // 8
    "Varied",                // 9
    "Pop Music",             // 10
    "Rock Music",            // 11
    "Easy Listening",        // 12
    "Light Classical",       // 13
    "Serious Classical",     // 14
    "Other Music",           // 15
    "Weather",               // 16
    "Finance",               // 17
    "Children's Programmes", // 18
    "Social Affairs",        // 19
    "Religion",              // 20
    "Phone-in",              // 21
    "Travel",                // 22
    "Leisure",               // 23
    "Jazz Music",            // 24
    "Country Music",         // 25
    "National Music",        // 26
    "Oldies Music",          // 27
    "Folk Music",            // 28
    "Documentary",           // 29
    "Alarm Test",            // 30
    "Alarm"                  // 31
};

const uint8_t RDSComponent::RDS_PTY_COUNT = sizeof(RDSComponent::RDS_PTY_NAMES) / sizeof(RDSComponent::RDS_PTY_NAMES[0]);

// ===================================================================
// PTY konverzió
// ===================================================================

/**
 * @brief PTY kód konvertálása szöveges leírássá
 * @param ptyCode A PTY kód (0-31)
 * @return String A PTY szöveges leírása
 */
String RDSComponent::convertPtyCodeToString(uint8_t ptyCode) {
    if (ptyCode < RDS_PTY_COUNT) {
        return String(RDS_PTY_NAMES[ptyCode]);
    }
    return "Unknown PTY";
}

// ===================================================================
// Konstruktor és inicializálás
// ===================================================================

/**
 * @brief RDSComponent konstruktor
 */
RDSComponent::RDSComponent(TFT_eSPI &tft, const Rect &bounds, Si4735Manager &manager)
    : UIComponent(tft, bounds, ColorScheme::defaultScheme()), si4735Manager(manager), lastRdsUpdate(0), lastScrollUpdate(0), lastValidRdsData(0), dataChanged(false),
      scrollSprite(nullptr), scrollOffset(0), radioTextPixelWidth(0), needsScrolling(false), scrollSpriteCreated(false), rdsAvailable(false) {

    // Alapértelmezett színek beállítása
    stationNameColor = TFT_CYAN;   // Állomásnév - cián
    programTypeColor = TFT_ORANGE; // Program típus - narancs
    radioTextColor = TFT_WHITE;    // Radio text - fehér
    dateTimeColor = TFT_YELLOW;    // Dátum/idő - sárga
    backgroundColor = TFT_BLACK;   // Háttér - fekete

    // Alapértelmezett layout számítása
    calculateDefaultLayout();

    // Kezdetben nincs RDS adat
    rdsAvailable = false;
    dataChanged = false;

    // Inicializáljuk az időzítőt az aktuális időre, hogy ne legyen azonnali timeout
    lastValidRdsData = millis();
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

    // Adaptív frissítési időköz:
    // - Ha nincs RDS adat -> 1 másodperc (gyors keresés)
    // - Ha van stabil RDS adat -> 3 másodperc (takarékos)
    uint32_t adaptiveInterval = RDS_UPDATE_INTERVAL_MS;
    if (cachedStationName.isEmpty() && cachedProgramType.isEmpty()) {
        adaptiveInterval = 1000; // 1 másodperc új állomás esetén
    } else {
        adaptiveInterval = 3000; // 3 másodperc stabil állomás esetén
    }

    // Időzített frissítés
    if (currentTime - lastRdsUpdate < adaptiveInterval) {
        return;
    }
    lastRdsUpdate = currentTime;

    bool newRdsAvailable = si4735Manager.isRdsAvailable();

    // Az RDS elérhetőség csak informatív - NE töröljük a cached adatokat!
    // A timeout logika kezelje az adatok törlését
    rdsAvailable = newRdsAvailable;

    dataChanged = false;

    // Ha bármelyik RDS adat nem üres, frissítjük a "last valid" időt
    bool hasValidData = false;

    // Állomásnév frissítése - csak ha nem üres vagy ha explicit törölni kell
    String newStationName = si4735Manager.getRdsStationName();
    if (!newStationName.isEmpty() && newStationName != cachedStationName) {
        // Új, nem üres állomásnév érkezett
        cachedStationName = newStationName;
        dataChanged = true;
        hasValidData = true;
    }
    if (!newStationName.isEmpty())
        hasValidData = true;

    // Program típus frissítése - PTY kód alapján
    uint8_t newPtyCode = si4735Manager.getRdsProgramTypeCode();
    if (newPtyCode != 255) { // 255 = nincs RDS
        String newProgramType = convertPtyCodeToString(newPtyCode);
        if (!newProgramType.isEmpty() && newProgramType != cachedProgramType) {
            cachedProgramType = newProgramType;
            dataChanged = true;
            hasValidData = true;
        }
        if (!newProgramType.isEmpty())
            hasValidData = true;
    }

    // Radio text frissítése - csak ha nem üres
    String newRadioText = si4735Manager.getRdsRadioText();
    if (!newRadioText.isEmpty() && newRadioText != cachedRadioText) {
        cachedRadioText = newRadioText;
        dataChanged = true;
        hasValidData = true;

        // Radio text változott - scroll újraszámítás
        tft.setFreeFont();
        tft.setTextSize(1);
        radioTextPixelWidth = tft.textWidth(cachedRadioText);
        needsScrolling = (radioTextPixelWidth > radioTextArea.width);
        scrollOffset = 0; // Scroll restart
    }
    if (!newRadioText.isEmpty())
        hasValidData = true;

    // Dátum/idő frissítése
    uint16_t year, month, day, hour, minute;
    if (si4735Manager.getRdsDateTime(year, month, day, hour, minute)) {
        String newDateTime = String(hour < 10 ? "0" : "") + String(hour) + ":" + String(minute < 10 ? "0" : "") + String(minute);
        if (newDateTime != cachedDateTime) {
            cachedDateTime = newDateTime;
            dataChanged = true;
            hasValidData = true;
        }
    }

    // Ha volt valid adat, frissítsük az időzítőt
    if (hasValidData) {
        lastValidRdsData = currentTime;
    }

    // Timeout ellenőrzés - különböző időzítés a különböző RDS adatokhoz
    const uint32_t STATION_INFO_TIMEOUT = 90000; // 90 másodperc az állomásnév és programtípus számára
    const uint32_t RADIO_TEXT_TIMEOUT = 60000;   // 60 másodperc a radio text számára
    const uint32_t DATETIME_TIMEOUT = 30000;     // 30 másodperc a dátum/idő számára (ez a leginstabilabb)

    if (currentTime - lastValidRdsData > STATION_INFO_TIMEOUT) {
        // Hosszú timeout után töröljük az állomásnevet és programtípust
        if (!cachedStationName.isEmpty() || !cachedProgramType.isEmpty()) {
            cachedStationName = "";
            cachedProgramType = "";
            dataChanged = true;
        }
    }

    if (currentTime - lastValidRdsData > RADIO_TEXT_TIMEOUT) {
        // Közepes timeout után töröljük a radio textet
        if (!cachedRadioText.isEmpty()) {
            cachedRadioText = "";
            dataChanged = true;
            needsScrolling = false;
        }
    }

    if (currentTime - lastValidRdsData > DATETIME_TIMEOUT) {
        // Rövid timeout után töröljük a dátum/időt
        if (!cachedDateTime.isEmpty()) {
            cachedDateTime = "";
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
 * @brief RDS cache törlése frekvencia változáskor
 * @details Azonnal törli az összes RDS adatot és reseteli az időzítőket.
 * Használatos frekvencia váltáskor, amikor az RDS adatok már nem érvényesek.
 */
void RDSComponent::clearRdsOnFrequencyChange() {
    // Cache törlése
    cachedStationName = "";
    cachedProgramType = "";
    cachedRadioText = "";
    cachedDateTime = "";

    // Állapot resetelés
    rdsAvailable = false;
    dataChanged = true;
    needsScrolling = false;
    scrollOffset = 0;

    // Időzítők resetelése - új frekvencián kezdjük mérni az időt
    lastValidRdsData = millis();
    lastRdsUpdate = 0; // Azonnal frissítsen    // Sprite tisztítás
    cleanupScrollSprite();

    // Képernyő frissítés
    markForRedraw(false);
}

/**
 * @brief Ellenőrzi, hogy van-e érvényes RDS adat
 */
bool RDSComponent::hasValidRDS() const {
    return rdsAvailable && (!cachedStationName.isEmpty() || !cachedProgramType.isEmpty() || !cachedRadioText.isEmpty() || !cachedDateTime.isEmpty());
}

// ===================================================================
