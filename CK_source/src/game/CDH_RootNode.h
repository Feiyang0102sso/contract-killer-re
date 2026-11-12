// CDH_RootNode.h - owns the resource bank while the game screens are active.
#ifndef CDH_ROOTNODE_H
#define CDH_ROOTNODE_H

#include "WindowBase.h"
#include "CResBank.h"

class CDH_RootNode : public Window {
public:
    CDH_RootNode();
    virtual ~CDH_RootNode();

    virtual void Init();
    virtual void Shutdown();
    virtual void OnCreateState(Window** window, int state);
    virtual void OnStateInterrupt(int state, Window* window);

private:
    CResBank m_resBank;  // +0xa4
};

#endif
