#include "MiniAudioDisplay.h"
#include "Core1Logic.h"
#include "UIColorPalette.h"
#include <TFT_eSPI.h>

MiniAudioDisplay::MiniAudioDisplay(TFT_eSPI &tft, const Rect &bounds, const ColorScheme &colors)
    : UIComponent(tft, static_cast<const Rect &>(bounds), colors), primaryColor_(UIColorPalette::audioSpectrumPrimary), secondaryColor_(UIColorPalette::audioSpectrumSecondary),
      backgroundColor_(UIColorPalette::audioSpectrumBackground), lastUpdateTime_(0), showingModeDisplay_(false), modeDisplayStartTime_(0) {}

void MiniAudioDisplay::draw() {
    if (isDisabled()) {
        return;
    }

    if (isRedrawNeeded()) {
        const Rect &bounds = getBounds();

        // Sprite inicializálás vagy méretváltás
        if (!spriteHolder.sprite || spriteHolder.width != bounds.width || spriteHolder.height != bounds.height) {
            if (spriteHolder.sprite) {
                spriteHolder.sprite->deleteSprite();
                delete spriteHolder.sprite;
            }
            spriteHolder.sprite = new TFT_eSprite(&tft);
            spriteHolder.sprite->setColorDepth(16);
            spriteHolder.sprite->createSprite(bounds.width, bounds.height);
            spriteHolder.width = bounds.width;
            spriteHolder.height = bounds.height;
        }

        // Sprite törlése háttérszínnel
        spriteHolder.sprite->fillSprite(backgroundColor_);

        // Minden tartalom sprite-ra rajzolódik
        drawContentToSprite(spriteHolder.sprite);

        // Sprite kirajzolása a kijelzőre
        spriteHolder.sprite->pushSprite(bounds.x, bounds.y);

        // Ha aktív a mód kijelzés, akkor a feliratot a kijelző alatt jelenítjük meg (közvetlenül tft-re)
        if (showingModeDisplay_) {
            drawModeDisplay();
        }

        needsRedraw = false;
    }
}

void MiniAudioDisplay::update() {
    if (isDisabled()) {
        return;
    }

    uint32_t currentTime = millis();

    // Mód kijelzés időzítés ellenőrzése
    if (showingModeDisplay_) {
        if (currentTime - modeDisplayStartTime_ >= MODE_DISPLAY_DURATION_MS) {
            showingModeDisplay_ = false;
            markForRedraw();
        }
    } else {
        // Normál frissítés csak ha nem jelenik meg a mód kijelzés
        if (currentTime - lastUpdateTime_ >= UPDATE_INTERVAL_MS) {
            markForRedraw();
            lastUpdateTime_ = currentTime;
        }
    }
}

void MiniAudioDisplay::setEnabled(bool enabled) {
    setDisabled(!enabled);

    if (enabled) {
        markForRedraw();
    } else {
        // Terület törlése ha letiltjuk
        const Rect &bounds = getBounds();
        tft.fillRect(bounds.x, bounds.y, bounds.width, bounds.height, backgroundColor_);
    }
}

void MiniAudioDisplay::setColorScheme(uint16_t primary, uint16_t secondary, uint16_t background) {
    primaryColor_ = primary;
    secondaryColor_ = secondary;
    backgroundColor_ = background;
    markForRedraw();
}

AudioProcessor *MiniAudioDisplay::getAudioProcessor() {
    return ::getAudioProcessor(); // Globális függvény hívása a Core1Logic.h-ból
}

bool MiniAudioDisplay::handleTouch(const TouchEvent &event) {
    // Ha érintés történt a területen és van callback
    if (event.pressed && modeChangeCallback_) {
        modeChangeCallback_();
        return true;
    }
    return UIComponent::handleTouch(event);
}

void MiniAudioDisplay::setModeChangeCallback(std::function<void()> callback) { modeChangeCallback_ = callback; }

void MiniAudioDisplay::showModeDisplay(const String &modeName) {
    currentModeDisplayText_ = modeName;
    showingModeDisplay_ = true;
    modeDisplayStartTime_ = millis();
    markForRedraw();
}

void MiniAudioDisplay::drawModeDisplay() {
    const Rect &bounds = getBounds();
    // A felirat a kijelző alatt jelenjen meg
    int16_t textY = bounds.y + bounds.height + 8;             // 8px-el alatta
    int16_t textWidth = currentModeDisplayText_.length() * 7; // Durva becslés
    int16_t textX = bounds.x + (bounds.width - textWidth) / 2;

    // Háttér a szöveg mögé (fekete sáv, 16px magas)
    tft.fillRect(bounds.x, bounds.y + bounds.height + 2, bounds.width, 16, TFT_BLACK);

    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(textX, textY);
    tft.print(currentModeDisplayText_);
}

static constexpr uint32_t MODE_DISPLAY_DURATION_MS = 20000; // 20 másodperc
