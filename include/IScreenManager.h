#ifndef __ISCREEN_MANAGER_H
#define __ISCREEN_MANAGER_H

class IScreenManager {

  public:
    virtual bool switchToScreen(const char *screenName, void *params = nullptr) = 0;

    virtual bool goBack() = 0;
};

#endif // __ISCREEN_MANAGER_H