#ifndef CDH_LEVELUPSCREEN_H
#define CDH_LEVELUPSCREEN_H

#include "CDH_BaseScreen.h"

class CDH_LevelUpScreen : public CDH_BaseScreen {
public:
    CDH_LevelUpScreen();
    virtual ~CDH_LevelUpScreen();
    virtual void Init();
    virtual void OnStartupEasy();
    virtual void OnCommand(Event& event);
};

#endif
