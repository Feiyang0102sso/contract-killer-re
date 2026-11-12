#include "CPublicData1.h"

#include <stdlib.h>
#include <wchar.h>
#include "App.h"
#include "CHash.h"
#include "CNGSAttribute.h"
#include "CSaveManager.h"
#include "CStringToKey.h"
#include "ICStdUtil.h"

typedef char PublicDataSizeCheck[sizeof(CPublicData1) == 0x48 ? 1 : -1];

CPublicData1::CPublicData1()
    : CSaveRestoreInterface(1000, 1, BACKING_STORE_ATTRIBUTES)
{
    resetData();
}

void CPublicData1::initGuildFriendsList()
{
    m_guildFriends.free_all_items();
    const wchar_t* start = m_guildFriendList.GetWCharPtr();
    while (start != NULL && *start != L'\0') {
        const wchar_t* separator = wcschr(start, L';');
        if (separator == NULL || separator == start) {
            break;
        }
        XString token(start, (separator - start) * sizeof(wchar_t));
        int userId = token.ToInt();
        if (userId > 0) {
            m_guildFriends.addElement(userId);
        }
        start = separator + 1;
    }
    if (WindowApp::m_instance != NULL) {
        WindowApp::HandleTunnelCommand(4, 0, 0, 0);
    }
}

unsigned char CPublicData1::compareData(const CSaveRestoreInterface* other)
{
    const CPublicData1* data = static_cast<const CPublicData1*>(other);
    return m_level == data->m_level && m_successBounties == data->m_successBounties;
}

unsigned char CPublicData1::copyData(const CSaveRestoreInterface* other)
{
    const CPublicData1* data = static_cast<const CPublicData1*>(other);
    m_level = data->m_level;
    m_successBounties = data->m_successBounties;
    m_guildFriendList = data->m_guildFriendList;
    initGuildFriendsList();
    return 1;
}

void CPublicData1::resetData()
{
    m_level = 0;
    m_successBounties = 0;
    m_guildFriendList = L"";
    m_guildFriends.free_all_items();
}

void CPublicData1::getData(TCVector<CNGSAttribute*>& attributes)
{
    CStrWChar levelName = getwTokenNameFor("LEVEL");
    CStrWChar bountiesName = getwTokenNameFor("CONTRACT");
    CStrWChar friendsName = getwTokenNameFor("GUILD_FRIENDS");
    attributes.Add(new CNGSAttribute(levelName, m_level, m_collectionType));
    attributes.Add(new CNGSAttribute(bountiesName, m_successBounties, m_collectionType));
    PrepareGuildFriendListAsString();
    attributes.Add(new CNGSAttribute(friendsName, m_guildFriendList, false, m_collectionType));
}

void CPublicData1::PrepareGuildFriendListAsString()
{
    m_guildFriendList = L"";
    for (int index = 0; index < m_guildFriends.size(); index++) {
        XString userId = XString::Format(L"%d;", m_guildFriends[index]);
        m_guildFriendList.Concatenate(userId.c_str());
    }
}

void CPublicData1::setData(const CHash* attributes)
{
    resetData();
    CStrChar level = getTokenNameFor("LEVEL");
    CStrChar bounties = getTokenNameFor("CONTRACT");
    CStrChar friends = getTokenNameFor("GUILD_FRIENDS");
    void* attribute = NULL;
    if (attributes->Find(CStringToKey(level.GetCharPtr(), 0), &attribute)) {
        m_level = static_cast<CNGSAttribute*>(attribute)->getVal_int16();
    } else {
        dataNotFound();
    }
    if (attributes->Find(CStringToKey(bounties.GetCharPtr(), 0), &attribute)) {
        m_successBounties = static_cast<CNGSAttribute*>(attribute)->getVal_int32();
    } else {
        dataNotFound();
    }
    if (attributes->Find(CStringToKey(friends.GetCharPtr(), 0), &attribute)) {
        m_guildFriendList = static_cast<CNGSAttribute*>(attribute)->getVal_string();
        if (m_guildFriendList.GetLength() > 0) {
            initGuildFriendsList();
        }
    }
}

void CPublicData1::debugPrint()
{
    debugPrintBase();
}

CStrWChar CPublicData1::asString()
{
    char text[128] = { 0 };
    ICStdUtil::SPrintF(text, "level=%i contract=%i", m_level, m_successBounties);
    return CStrWChar(text);
}

bool CPublicData1::isInGuildFriendList(int userId)
{
    for (int index = 0; index < m_guildFriends.size(); index++) {
        if (m_guildFriends[index] == userId) {
            return true;
        }
    }
    return false;
}

void CPublicData1::addGuildFriend(int userId)
{
    if (userId > 0 && !isInGuildFriendList(userId)) {
        m_guildFriends.addElement(userId);
    }
    PrepareGuildFriendListAsString();
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app != NULL) {
        WindowApp::HandleTunnelCommand(4, 0, 0, 0);
        if (app->m_saveManager != NULL) {
            app->m_saveManager->SaveOnNextUpdate(0);
        }
    }
}
