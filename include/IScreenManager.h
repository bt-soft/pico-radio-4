#ifndef __ISCREEN_MANAGER_H
#define __ISCREEN_MANAGER_H

class IScreenManager {

  public:
    virtual bool switchToScreen(const char *screenName, void *params = nullptr) = 0;

    virtual bool goBack() = 0;

    // MemoryScreen paraméter kezelés
    virtual void setMemoryScreenParams(bool autoAdd, const char *rdsName = nullptr) = 0;
    virtual void switchToMemoryScreen() = 0;
};

#endif // __ISCREEN_MANAGER_H