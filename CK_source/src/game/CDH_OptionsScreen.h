// CDH_OptionsScreen.h - game settings and the reset confirmation state.
#ifndef CDH_OPTIONSSCREEN_H
#define CDH_OPTIONSSCREEN_H

#include "WindowBase.h"

class CDH_BaseScreen;
class CDH_RadioGroupWidget;

class CDH_OptionsScreen : public Window {
public:
    CDH_OptionsScreen();
    virtual ~CDH_OptionsScreen();
    virtual void Init();
    virtual void OnCommand(Event& event);
    virtual void OnCreateState(Window** window, int state);
    virtual void OnStateInterrupt(int state, Window* window);

private:
    enum State { OptionItems = 1, ConfirmReset = 2 };

    Window* AddButtonItem(XString label, int row);
    CDH_RadioGroupWidget* AddSelectorItem(int row, int selected);
    CDH_BaseScreen* createOptionItemsScreen();

    CDH_RadioGroupWidget* m_sound;
    CDH_RadioGroupWidget* m_music;
    CDH_RadioGroupWidget* m_fireButton;
    int m_reservedB0;  // tail word within the original 0xb4-byte allocation
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char OptionsScreenSizeCheck[sizeof(CDH_OptionsScreen) == 0xb4 ? 1 : -1];
#endif

#endif
