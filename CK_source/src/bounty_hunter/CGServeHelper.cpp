// CGServeHelper.cpp - gServe lookups for the game UI (CK 1.5.2, written by game2).
#include "CGServeHelper.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"

XString CGServeHelper::GetFriendNickname(int userId)
{
    XString nickname;
    nickname = *XString::pEmpty;
    // m_remoteUserLists[0]: friends with an avatar.
    CNGSRemoteUser* user = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0]->findRemoteUserByID(userId);
    if (user != NULL) {
        nickname = user->GetNickName().GetWCharPtr();
    }
    return nickname;
}
