#ifndef __ISCROLLABLE_LIST_DATA_SOURCE_H
#define __ISCROLLABLE_LIST_DATA_SOURCE_H

#include <Arduino.h>

/**
 * @brief Interfész a UIScrollableListComponent adatforrásához.
 *
 * Ez az interfész definiálja azokat a metódusokat, amelyeket egy
 * adatforrásnak implementálnia kell, hogy a UIScrollableListComponent
 * meg tudja jeleníteni az elemeit.
 */
class IScrollableListDataSource {
  public:
    virtual ~IScrollableListDataSource() = default;

    /** @brief Visszaadja a lista elemeinek teljes számát. */
    virtual int getItemCount() const = 0;

    /** @brief Visszaadja a megadott indexű elem szövegét. */
    virtual String getItemAt(int index) const = 0;

    /** @brief Akkor hívódik meg, amikor egy elemre kattintanak (kiválasztják). */
    virtual void onItemClicked(int index) = 0;
};

#endif // __ISCROLLABLE_LIST_DATA_SOURCE_H