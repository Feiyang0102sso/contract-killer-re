// The CK root window owns the shared game resource bank.
#include "CDH_RootNode.h"
#include "CGameData.h"
#include "CGameScene.h"
#include "CIntroScene.h"
#include "ICCore.h"
#include "ICMediaPlayer.h"

namespace {
enum RootState { Intro = 1, GameFinished = 2, Game = 3, Exit = 4 };
}

CDH_RootNode::CDH_RootNode() : Window(false)
{
}

CDH_RootNode::~CDH_RootNode()
{
}

void CDH_RootNode::Init()
{
    CGameData::GetInstance()->m_resBank = &m_resBank;
    ChangeState(Game, STATE_PUSH);
}

void CDH_RootNode::Shutdown()
{
    m_resBank.removeAllKeysets();
}

void CDH_RootNode::OnCreateState(Window** window, int state)
{
    Window* created = NULL;
    switch (state) {
    case Intro:
        created = new CIntroScene();
        break;
    case Game:
        ICMediaPlayer::GetInstance()->Stop(0);
        ICMediaPlayer::GetInstance()->StopVibrate(0);
        created = new CGameScene();
        break;
    case Exit:
        CApplet::m_pCore->ExitApp(ICCore::ExitStatus_Normal);
        break;
    }
    *window = created;
}

void CDH_RootNode::OnStateInterrupt(int state, Window* window)
{
    if (state == Intro) {
        if (window->GetInterrupt() == 1) {
            ChangeState(Game, STATE_REPLACE);
        } else {
            ChangeState(Exit, STATE_REPLACE);
        }
    } else if (state == Game) {
        ChangeState(GameFinished, STATE_REPLACE);
    }
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertRootNodeSize[sizeof(CDH_RootNode) == 0xc8 ? 1 : -1];
#endif
