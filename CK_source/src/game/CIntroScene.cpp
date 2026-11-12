#include "CIntroScene.h"
#include "CDH_ImageScreen.h"
#include "GluLogoScreen.h"

namespace {
enum IntroState { Logo = 3, Done = 4, Licensor = 5, Alternate = 6 };
}

CIntroScene::CIntroScene() : Window(false)
{
}

CIntroScene::~CIntroScene()
{
}

void CIntroScene::Init()
{
    ChangeState(Logo, STATE_REPLACE);
}

void CIntroScene::OnCreateState(Window** window, int state)
{
    Window* created = NULL;
    switch (state) {
    case Logo:
        created = new CGluLogoScreen();
        break;
    case Done:
        SetInterrupt(1);
        break;
    case Licensor:
        created = new CDH_ImageScreen(0xff000000, "SUR_LICENSOR", 2000);
        break;
    }
    *window = created;
}

void CIntroScene::OnStateInterrupt(int state, Window* window)
{
    switch (state) {
    case Logo:
    case Licensor:
        if (window->GetInterrupt() == 1) {
            ChangeState(Done, STATE_REPLACE);
        } else {
            SetInterrupt(2);
        }
        break;
    case Done:
    case Alternate:
        if (window->GetInterrupt() == 1) {
            SetInterrupt(1);
        } else {
            SetInterrupt(2);
        }
        break;
    }
}
