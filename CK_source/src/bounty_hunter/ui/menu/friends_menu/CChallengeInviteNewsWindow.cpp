#include "CChallengeInviteNewsWindow.h"

#include "App.h"
#include "BaseDialog.h"
#include "CChallenge.h"
#include "CChallengeManager.h"
#include "CFontMgr.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CNewsFeedManager.h"
#include "ICSystemFont.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

CChallengeInviteNewsWindow::CChallengeInviteNewsWindow(const SMissionInviteMessage* message)
    : Window(false), m_sender(NULL)
{
    m_senderId = message->m_senderId;
    m_message = message;
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    m_sender = local->m_remoteUserLists[0]->findRemoteUserByID(m_senderId);
    CreateUserInterface();
}

void CChallengeInviteNewsWindow::CreateUserInterface()
{
    SetHeightByContent(0, 0);
    XString sender;
    if (m_sender != NULL) {
        CStrWChar name = m_sender->GetNickName();
        sender = name.GetWCharPtr();
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CChallenge* challenge = app->m_challengeManager->GetChallengeById(m_message->m_missionId);
    XString challengeName(L"");
    if (challenge != NULL) {
        challengeName = challenge->m_name;
    }
    XString text = XString::Format(ResString("IDS_CHALLENGE_INVITE_NEWS_FEED").c_str(),
                                  sender.c_str(), challengeName.c_str());
    TextWindow* label = new TextWindow(text, CFontMgr::GetInstance()->GetFont(15));
    label->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    label->SetPercentWidth(100, 0, 0);
    ICSystemFont* font = ICSystemFont::CreateInstance();
    font->Initialize(SScreenAdaptingConstant(12, 25)(), NULL, 1);
    label->SetFont(font);
    AddToFront(label, 0, 0);
    SeparateLineRed* line = new SeparateLineRed();
    line->SetOutsetSpacing(SScreenAdaptingConstant(5, 10)(), 0, 0, 0);
    line->SetDesiredHeight(SScreenAdaptingConstant(5, 10)());
    line->SetCellPos(0, 1, 2, 1);
    AddToFront(line);
}
