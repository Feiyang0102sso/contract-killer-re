#ifndef CINTROSCENE_H
#define CINTROSCENE_H

#include "WindowBase.h"

/** Logo, licensor image and completion state of the CK intro. */
class CIntroScene : public Window {
public:
    CIntroScene();
    virtual ~CIntroScene();
    virtual void Init();
    virtual void OnCreateState(Window** window, int state);
    virtual void OnStateInterrupt(int state, Window* window);
};

#endif
