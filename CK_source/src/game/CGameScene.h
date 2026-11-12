#ifndef CGAMESCENE_H
#define CGAMESCENE_H

#include "WindowBase.h"
#include "CResBank.h"

class CGluGamesNetwork;

/** Owns the game and pause state stack and its preload bank. */
class CGameScene : public Window {
public:
    enum State { Demo = 1, Loading = 7, Game = 8, Pause = 9, PauseWithSoundOff = 10 };

    CGameScene();
    virtual ~CGameScene();

    virtual void Init();
    virtual void Shutdown();
    virtual void Update();
    virtual void OnPause();
    virtual void OnCreateState(Window** window, int state);
    virtual void OnStateInterrupt(int state, Window* window);
    virtual bool HandleEvent(unsigned int type, unsigned int param1, unsigned int param2);

    static void AtExitGGN(void* context);
    static void ShowGGN();
    static bool IsGGNShowed();

private:
    static bool m_bGGNShowed;
    static CGluGamesNetwork* m_pGGN;

    CResBank m_resBank; // 0xa4
};

#endif
