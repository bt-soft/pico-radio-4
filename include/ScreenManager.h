#ifndef __SCREEN_MANAGER_H
#define __SCREEN_MANAGER_H

#include <TFT_eSPI.h>
#include <functional>
#include <map>   // Tartalmazva volt, de explicit jobb
#include <queue> // Tartalmazva volt, de explicit jobb

#include "IScreenManager.h"
#include "UIScreen.h"
// Szükséges include-ok a screen factory-khoz és a config-hoz
#include "Config.h" // Config &cfg miatt és configRef.data eléréséhez
#include "EmptyScreen.h"
#include "FMScreen.h"
#include "ScreenSaverScreen.h" // Az új képernyővédő
#include "TestScreen.h"
#include "defines.h" // Képernyőnevekhez

// Deferred action struktúra - biztonságos képernyőváltáshoz
struct DeferredAction {
    enum Type { SwitchScreen, GoBack };

    Type type;
    const char *screenName;
    void *params;

    DeferredAction(Type t, const char *name = nullptr, void *p = nullptr) : type(t), screenName(name), params(p) {}
};

// Képernyő factory típus
using ScreenFactory = std::function<std::shared_ptr<UIScreen>(TFT_eSPI &)>;

// Képernyőkezelő
class ScreenManager : public IScreenManager {

  private:
    TFT_eSPI &tft;
    std::map<String, ScreenFactory> screenFactories;
    std::shared_ptr<UIScreen> currentScreen;
    const char *previousScreenName;
    uint32_t lastActivityTime;

    Config &configRef; // Referencia a globális config objektumra

    // Deferred action queue - biztonságos képernyőváltáshoz
    std::queue<DeferredAction> deferredActions;
    bool processingEvents = false;

    // Aktuális képernyő lekérdezése
    std::shared_ptr<UIScreen> getCurrentScreen() const { return currentScreen; }

    // Előző képernyő neve
    String getPreviousScreenName() const { return previousScreenName; }

  public:
    ScreenManager(TFT_eSPI &tft, Config &cfg) : tft(tft), configRef(cfg), previousScreenName(nullptr), lastActivityTime(millis()) { registerDefaultScreenFactories(); }

    // Képernyő factory regisztrálása
    void registerScreenFactory(const char *screenName, ScreenFactory factory) { screenFactories[screenName] = factory; }

    // Deferred képernyő váltás - biztonságos váltás eseménykezelés közben
    void deferSwitchToScreen(const char *screenName, void *params = nullptr) {
        DEBUG("ScreenManager: Deferring switch to screen '%s'\n", screenName);
        deferredActions.push(DeferredAction(DeferredAction::SwitchScreen, screenName, params));
    }

    // Deferred vissza váltás
    void deferGoBack() {
        DEBUG("ScreenManager: Deferring go back\n");
        deferredActions.push(DeferredAction(DeferredAction::GoBack));
    }

    // Deferred actions feldolgozása - a main loop-ban hívandó
    void processDeferredActions() {
        while (!deferredActions.empty()) {
            const DeferredAction &action = deferredActions.front();

            DEBUG("ScreenManager: Processing deferred action type=%d\n", (int)action.type);

            if (action.type == DeferredAction::SwitchScreen) {
                immediateSwitch(action.screenName, action.params);
            } else if (action.type == DeferredAction::GoBack) {
                immediateGoBack();
            }

            deferredActions.pop();
        }
    }

    // Képernyő váltás név alapján - biztonságos verzió - IScreenManager
    bool switchToScreen(const char *screenName, void *params = nullptr) override {
        if (processingEvents) {
            // Eseménykezelés közben - halasztott váltás
            deferSwitchToScreen(screenName, params);
            return true;
        } else {
            // Biztonságos - azonnali váltás
            return immediateSwitch(screenName, params);
        }
    }

    // Azonnali képernyő váltás - csak biztonságos kontextusban hívható
    bool immediateSwitch(const char *screenName, void *params = nullptr) {

        // Ha már ez a képernyő aktív, nem csinálunk semmit
        if (currentScreen && STREQ(currentScreen->getName(), screenName)) {
            return true;
        }

        // Factory keresése
        auto it = screenFactories.find(screenName);
        if (it == screenFactories.end()) {
            DEBUG("ScreenManager: Screen factory not found for '%s'\n", screenName);
            return false;
        }

        // Jelenlegi képernyő törlése
        if (currentScreen) {
            previousScreenName = currentScreen->getName();
            currentScreen->deactivate();
            currentScreen.reset(); // Memória felszabadítása
            DEBUG("ScreenManager: Destroyed screen '%s'\n", previousScreenName);
        }

        // TFT display törlése a képernyőváltás előtt
        tft.fillScreen(TFT_BLACK);
        DEBUG("ScreenManager: Display cleared for screen switch\n");

        // Új képernyő létrehozása
        currentScreen = it->second(tft);
        if (currentScreen) {
            currentScreen->setManager(this);
            if (params) {
                currentScreen->setParameters(params);
            }
            // Fontos: Az activate() hívása *előtt* állítjuk be a lastActivityTime-ot,
            // ha nem a képernyővédőre váltunk, hogy az activate() felülírhassa, ha akarja.
            if (!STREQ(screenName, SCREEN_NAME_SCREENSAVER)) {
                lastActivityTime = millis();
            }
            currentScreen->activate();
            DEBUG("ScreenManager: Created and activated screen '%s'\n", screenName);
            return true;
        } else {
            DEBUG("ScreenManager: Failed to create screen '%s'\n", screenName);
        }
        return false;
    }

    // Vissza az előző képernyőre - biztonságos verzió - IScreenManager
    bool goBack() override {
        if (processingEvents) {
            // Eseménykezelés közben - halasztott váltás
            deferGoBack();
            return true;
        } else {
            // Biztonságos - azonnali váltás
            return immediateGoBack();
        }
    }

    // Azonnali visszaváltás - csak biztonságos kontextusban hívható
    bool immediateGoBack() {
        if (previousScreenName != nullptr) {
            return immediateSwitch(previousScreenName);
        }
        return false;
    }

    // Touch esemény kezelése
    bool handleTouch(const TouchEvent &event) {
        if (currentScreen) {
            if (!STREQ(currentScreen->getName(), SCREEN_NAME_SCREENSAVER)) {
                lastActivityTime = millis();
            }
            processingEvents = true;
            bool result = currentScreen->handleTouch(event);
            processingEvents = false;
            return result;
        }
        return false;
    }

    // Rotary encoder esemény kezelése
    bool handleRotary(const RotaryEvent &event) {
        if (currentScreen) {
            if (!STREQ(currentScreen->getName(), SCREEN_NAME_SCREENSAVER)) {
                lastActivityTime = millis();
            }
            processingEvents = true;
            bool result = currentScreen->handleRotary(event);
            processingEvents = false;
            return result;
        }
        return false;
    }

    // Loop hívás
    void loop() {
        // Először a halasztott műveletek feldolgozása
        processDeferredActions();

        if (currentScreen) {
            // Képernyővédő időzítő ellenőrzése
            uint32_t screenSaverTimeoutMs = configRef.data.screenSaverTimeoutMinutes * 60000UL;

            if (screenSaverTimeoutMs > 0 &&                                  // Ha a képernyővédő engedélyezve van (idő > 0)
                !STREQ(currentScreen->getName(), SCREEN_NAME_SCREENSAVER) && // És nem vagyunk már a képernyővédőn
                lastActivityTime != 0 &&                                     // És volt már aktivitás
                (millis() - lastActivityTime > screenSaverTimeoutMs)) {      // És lejárt az idő

                DEBUG("ScreenManager: Screen saver timeout. Switching to %s\n", SCREEN_NAME_SCREENSAVER);
                switchToScreen(SCREEN_NAME_SCREENSAVER);
                // lastActivityTime frissül, amikor a felhasználó újra interakcióba lép a képernyővédőn,
                // és visszaváltáskor az immediateSwitch-ben.
            }
            currentScreen->loop();
        }
    }

    // Rajzolás
    void draw() {
        if (currentScreen) {
            // Csak akkor rajzolunk, ha valóban szükséges
            if (currentScreen->isRedrawNeeded()) {
                currentScreen->draw();
            }
        }
    }

  private:
    void registerDefaultScreenFactories();
};
#endif // __SCREEN_MANAGER_H