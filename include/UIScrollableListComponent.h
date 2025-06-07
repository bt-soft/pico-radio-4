#ifndef __UI_SCROLLABLE_LIST_COMPONENT_H
#define __UI_SCROLLABLE_LIST_COMPONENT_H

#include "IScrollableListDataSource.h"
#include "UIComponent.h"
#include <vector>

/**
 * @brief Újrafelhasználható görgethető lista UI komponens.
 *
 * Ez a komponens egy listát jelenít meg, amely görgethető,
 * és az elemeket egy IScrollableListDataSource interfészen keresztül kapja.
 */
class UIScrollableListComponent : public UIComponent {
  public:
    static constexpr uint8_t DEFAULT_VISIBLE_ITEMS = 5;
    static constexpr uint8_t DEFAULT_ITEM_HEIGHT = 20; // Vagy számoljuk a font magasságból
    static constexpr uint8_t SCROLL_BAR_WIDTH = 8;
    static constexpr uint8_t ITEM_TEXT_PADDING_X = 5;

  private:
    IScrollableListDataSource *dataSource = nullptr;
    int topItemIndex = 0;      // A lista tetején látható elem indexe
    int selectedItemIndex = 0; // A kiválasztott elem indexe (abszolút)
    uint8_t visibleItemCount = DEFAULT_VISIBLE_ITEMS;
    uint8_t itemHeight = DEFAULT_ITEM_HEIGHT;

    // Színek
    uint16_t itemTextColor;
    uint16_t selectedItemTextColor;
    uint16_t selectedItemBackground;
    uint16_t scrollBarColor;
    uint16_t scrollBarBackgroundColor;

    void drawScrollBar() {
        // Ha nincs dataSource, vagy nincs elég elem a görgetéshez, ne rajzolj scrollbart.
        if (!dataSource || dataSource->getItemCount() <= visibleItemCount) {
            return; // Nincs szükség scrollbarra
        }

        int16_t scrollBarX = bounds.x + bounds.width - SCROLL_BAR_WIDTH;
        tft.fillRect(scrollBarX, bounds.y, SCROLL_BAR_WIDTH, bounds.height, scrollBarBackgroundColor);

        float ratio = (float)visibleItemCount / dataSource->getItemCount();
        uint16_t thumbHeight = std::max((int)(bounds.height * ratio), 10); // Minimális magasság

        // A thumbPosRatio számításának javítása, hogy elkerüljük a 0-val való osztást
        float thumbPosRatio = 0;
        int totalItems = dataSource->getItemCount();
        if (totalItems > visibleItemCount) { // Csak akkor van értelme a görgetésnek, ha több elem van, mint amennyi látható
            thumbPosRatio = (float)topItemIndex / (totalItems - visibleItemCount);
        } else {               // Ha minden elem látható, vagy kevesebb van, mint a látható hely
            thumbPosRatio = 0; // Ha minden látszik
        }
        // Biztosítjuk, hogy a thumbPosRatio 0 és 1 között legyen
        thumbPosRatio = std::max(0.0f, std::min(thumbPosRatio, 1.0f));

        uint16_t thumbY = bounds.y + (int)((bounds.height - thumbHeight) * thumbPosRatio);

        tft.fillRect(scrollBarX, thumbY, SCROLL_BAR_WIDTH, thumbHeight, scrollBarColor);
    }

  public:
    UIScrollableListComponent(TFT_eSPI &tft, const Rect &bounds, IScrollableListDataSource *ds, uint8_t visItems = DEFAULT_VISIBLE_ITEMS, uint8_t itmHeight = 0)
        : UIComponent(tft, bounds, ColorScheme::defaultScheme()), dataSource(ds) {

        if (itmHeight == 0) {
            // Item magasság számítása a font alapján, ha nincs megadva
            uint8_t prevSize = this->tft.textsize;     // Aktuális szövegméret mentése
            this->tft.setFreeFont(&FreeSansBold9pt7b); // Nagyobb font a magasság számításához
            this->tft.setTextSize(1);                  // Natív méret a FreeSansBold9pt7b-hez
            itemHeight = tft.fontHeight() + 6;         // Kis padding (lehet, hogy 4 is elég, teszteld)
            this->tft.setTextSize(prevSize);           // Eredeti szövegméret visszaállítása
        } else {
            itemHeight = itmHeight;
        }

        // Komponens háttérszínének beállítása feketére
        this->colors.background = TFT_COLOR_BACKGROUND; // UIComponent::colors.background

        // Listaelemek színeinek explicit beállítása
        itemTextColor = TFT_WHITE;          // Nem kiválasztott elem szövege
        selectedItemTextColor = TFT_BLACK;  // Kiválasztott elem szövege
        selectedItemBackground = TFT_GREEN; // Kiválasztott elem háttere

        // Görgetősáv színei
        scrollBarColor = TFT_LIGHTGREY;
        scrollBarBackgroundColor = TFT_DARKGREY;

        if (bounds.height > 0 && itemHeight > 0) {
            visibleItemCount = bounds.height / itemHeight;
        }
    }

    void setDataSource(IScrollableListDataSource *ds) {
        dataSource = ds;
        topItemIndex = 0;
        selectedItemIndex = 0;
        markForRedraw();
    }

  private:
    /**
     * @brief Egyetlen listaelemet rajzol újra a megadott abszolút index alapján.
     * @param absoluteIndex A lista teljes hosszában vett indexe az újrarajzolandó elemnek.
     */
    void redrawListItem(int absoluteIndex) {
        if (!dataSource)
            return;

        // Ellenőrizzük, hogy az elem látható-e
        if (absoluteIndex < topItemIndex || absoluteIndex >= topItemIndex + visibleItemCount) {
            return; // Nem látható, nincs teendő
        }

        int visibleItemSlot = absoluteIndex - topItemIndex; // 0-tól (visibleItemCount-1)-ig
        int16_t itemY = bounds.y + visibleItemSlot * itemHeight;

        // Biztosítjuk, hogy ne rajzoljunk a komponens határain kívülre
        if (itemY < bounds.y || itemY + itemHeight > bounds.y + bounds.height + 1) { // +1 a kerekítési hibák miatt
            return;
        }

        Rect itemBounds(bounds.x, itemY, bounds.width - SCROLL_BAR_WIDTH, itemHeight);

        // Szövegbeállítások mentése és visszaállítása
        uint8_t prevDatum = tft.getTextDatum();
        uint8_t prevSize = tft.textsize;
        // const GFXfont* prevFont = tft.gfxFont; // Ha egyedi fontot használnánk

        tft.setTextDatum(ML_DATUM);
        // A fontot és méretet a label/value részeknél külön állítjuk

        if (absoluteIndex == selectedItemIndex) {
            tft.fillRect(itemBounds.x, itemBounds.y, itemBounds.width, itemBounds.height, selectedItemBackground);
            tft.setTextColor(selectedItemTextColor, selectedItemBackground);
        } else {
            tft.fillRect(itemBounds.x, itemBounds.y, itemBounds.width, itemBounds.height, TFT_COLOR_BACKGROUND); // Fekete háttér
            tft.setTextColor(itemTextColor, TFT_COLOR_BACKGROUND);
        }

        String fullItemText = dataSource->getItemAt(absoluteIndex);
        int tabPosition = fullItemText.indexOf('\t');
        String labelPart = fullItemText;
        String valuePart = "";

        if (tabPosition != -1) {
            labelPart = fullItemText.substring(0, tabPosition);
            valuePart = fullItemText.substring(tabPosition + 1);
        }

        // Label rész rajzolása (nagyobb, balra igazított)
        tft.setFreeFont(&FreeSansBold9pt7b); // Nagyobb font a labelnek
        tft.setTextSize(1);                  // Natív méret
        tft.drawString(labelPart, itemBounds.x + ITEM_TEXT_PADDING_X, itemBounds.y + itemHeight / 2);

        // Value rész rajzolása (kisebb, jobbra igazított)
        if (valuePart.length() > 0) {
            tft.setFreeFont(); // Kisebb, alapértelmezett font
            tft.setTextSize(1);
            tft.setTextDatum(MR_DATUM); // Middle Right
            tft.drawString(valuePart, itemBounds.x + itemBounds.width - ITEM_TEXT_PADDING_X, itemBounds.y + itemHeight / 2);
            tft.setTextDatum(ML_DATUM); // Visszaállítás ML_DATUM-ra a következő elemhez/állapothoz
        }

        // Szövegbeállítások visszaállítása
        tft.setTextDatum(prevDatum);
        tft.setTextSize(prevSize);
        // tft.setFreeFont(prevFont); // Ha egyedi fontot használnánk
    }

  public:
    virtual void draw() override {
        if (!needsRedraw || !dataSource)
            return;

        tft.fillRect(bounds.x, bounds.y, bounds.width, bounds.height, TFT_COLOR_BACKGROUND); // Háttér törlése feketére
        // tft.drawRect(bounds.x, bounds.y, bounds.width, bounds.height, colors.border); // Opcionális keret

        // Szövegbeállítások mentése és visszaállítása a teljes lista rajzolásához
        uint8_t prevDatum = tft.getTextDatum();
        uint8_t prevSize = tft.textsize;
        // const GFXfont* prevFont = tft.gfxFont;

        // A fontot és méretet a label/value részeknél külön állítjuk minden itemnél

        int itemCount = dataSource->getItemCount();
        for (int i = 0; i < visibleItemCount; ++i) {
            int currentItemIndex = topItemIndex + i;
            if (currentItemIndex >= itemCount)
                break;

            int16_t itemY = bounds.y + i * itemHeight;
            // Biztosítjuk, hogy ne rajzoljunk a komponens határain kívülre
            if (itemY < bounds.y || itemY + itemHeight > bounds.y + bounds.height + 1) { // +1 a kerekítési hibák miatt
                continue;
            }
            Rect itemBounds(bounds.x, itemY, bounds.width - SCROLL_BAR_WIDTH, itemHeight);

            if (currentItemIndex == selectedItemIndex) {
                tft.fillRect(itemBounds.x, itemBounds.y, itemBounds.width, itemBounds.height, selectedItemBackground);
                tft.setTextColor(selectedItemTextColor, selectedItemBackground);
            } else {
                // A háttér már fekete a fő fillRect miatt
                tft.setTextColor(itemTextColor, TFT_COLOR_BACKGROUND);
            }

            String fullItemText = dataSource->getItemAt(currentItemIndex);
            int tabPosition = fullItemText.indexOf('\t');
            String labelPart = fullItemText;
            String valuePart = "";

            if (tabPosition != -1) {
                labelPart = fullItemText.substring(0, tabPosition);
                valuePart = fullItemText.substring(tabPosition + 1);
            }

            // Label rész rajzolása
            tft.setTextDatum(ML_DATUM);
            tft.setFreeFont(&FreeSansBold9pt7b);
            tft.setTextSize(1);
            tft.drawString(labelPart, itemBounds.x + ITEM_TEXT_PADDING_X, itemBounds.y + itemHeight / 2);

            // Value rész rajzolása
            if (valuePart.length() > 0) {
                tft.setTextDatum(MR_DATUM);
                tft.setFreeFont(); // Kisebb font
                tft.setTextSize(1);
                tft.drawString(valuePart, itemBounds.x + itemBounds.width - ITEM_TEXT_PADDING_X, itemBounds.y + itemHeight / 2);
            }
        }
        // Szövegbeállítások visszaállítása
        tft.setTextDatum(prevDatum);
        tft.setTextSize(prevSize);
        // tft.setFreeFont(prevFont);
        drawScrollBar();
        needsRedraw = false;
    }

    virtual bool handleRotary(const RotaryEvent &event) override {
        if (disabled || !dataSource || dataSource->getItemCount() == 0)
            return false;

        bool handled = false;
        int oldSelectedIndex = selectedItemIndex;
        int oldTopItemIndex = topItemIndex;

        if (event.direction == RotaryEvent::Direction::Up) {
            selectedItemIndex--;
            if (selectedItemIndex < 0)
                selectedItemIndex = 0;
            handled = true;
        } else if (event.direction == RotaryEvent::Direction::Down) {
            selectedItemIndex++;
            if (selectedItemIndex >= dataSource->getItemCount())
                selectedItemIndex = dataSource->getItemCount() - 1;
            handled = true;
        }

        if (event.buttonState == RotaryEvent::ButtonState::Clicked) {
            dataSource->onItemClicked(selectedItemIndex);
            // Az onItemClicked megváltoztathatja a lista tartalmát, ezért teljes újrarajzolás javasolt.
            markForRedraw();
            handled = true;
            return handled; // Kilépés a kattintás után
        }

        if (oldSelectedIndex != selectedItemIndex) {
            // Görgetés, ha a kiválasztás kilóg a látható részből
            if (selectedItemIndex < topItemIndex) {
                topItemIndex = selectedItemIndex;
            } else if (selectedItemIndex >= topItemIndex + visibleItemCount) {
                topItemIndex = selectedItemIndex - visibleItemCount + 1;
            }

            if (oldTopItemIndex != topItemIndex) {
                // A látható elemek megváltoztak, teljes újrarajzolás szükséges
                markForRedraw();
            } else {
                // Csak a kiválasztás változott a látható elemeken belül
                redrawListItem(oldSelectedIndex);  // Régi kiválasztott normál stílussal
                redrawListItem(selectedItemIndex); // Új kiválasztott kiemelt stílussal
                drawScrollBar();                   // Görgetősáv frissítése
            }
        }
        return handled;
    }

    virtual bool handleTouch(const TouchEvent &event) override {
        if (disabled || !dataSource || !bounds.contains(event.x, event.y) || dataSource->getItemCount() == 0) {
            return false;
        }

        if (event.pressed) { // Csak a lenyomásra reagálunk itt, a kattintást a UIComponent kezeli
            int touchedItemOffset = (event.y - bounds.y) / itemHeight;
            if (touchedItemOffset >= 0 && touchedItemOffset < visibleItemCount) {
                int newSelectedItemIndex = topItemIndex + touchedItemOffset;
                if (newSelectedItemIndex < dataSource->getItemCount()) {
                    if (selectedItemIndex != newSelectedItemIndex) {
                        selectedItemIndex = newSelectedItemIndex;
                        markForRedraw();
                    }
                    // A tényleges onItemClicked hívást az UIComponent::onClick-re bízzuk,
                    // miután a debounce és egyéb ellenőrzések lefutottak.
                }
            }
        }
        return UIComponent::handleTouch(event); // Átadjuk az ősosztálynak a kattintáskezeléshez
    }

  protected:
    virtual void onClick(const TouchEvent &event) override {
        if (dataSource && selectedItemIndex >= 0 && selectedItemIndex < dataSource->getItemCount()) {
            dataSource->onItemClicked(selectedItemIndex);
        }
        UIComponent::onClick(event); // Hívjuk az ősosztályt is, ha van benne logika
    }
};

#endif // __UI_SCROLLABLE_LIST_COMPONENT_H