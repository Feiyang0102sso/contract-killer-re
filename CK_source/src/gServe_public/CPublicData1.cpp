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

// This constructor calls the original weak instantiation out of line.
template void Vector<int>::free_all_items();
extern void FreeGuildFriendItems(Vector<int>*) __asm__("__ZN6VectorIiE14free_all_itemsEv");

CPublicData1::CPublicData1()
    : CSaveRestoreInterface(1000, 1, BACKING_STORE_ATTRIBUTES)
{
    m_level = 0;
    m_successBounties = 0;
    m_guildFriendList.ReleaseMemory();
    m_guildFriendList.Concatenate(L"");
    FreeGuildFriendItems(&m_guildFriends);
    m_guildFriends.m_size = 0;
    m_guildFriends.m_capacity = 0;
}

void CPublicData1::initGuildFriendsList()
{
    if (m_guildFriends.m_items != NULL) {
        free(m_guildFriends.m_items);
        m_guildFriends.m_items = NULL;
    }
    m_guildFriends.m_size = 0;
    m_guildFriends.m_capacity = 0;
    CStrWChar remaining = m_guildFriendList.GetSubString(0, m_guildFriendList.GetLength() - 1);
    while (remaining.GetWCharPtr() != NULL && remaining.GetLength() > 0) {
        int separator = remaining.GetCharIndex(1, L';');
        if (separator < 1) {
            remaining = "";
            continue;
        }

        CStrWChar token = remaining.GetSubString(0, separator - 1);
        XString text(token.GetWCharPtr());
        int userId = text.ToInt();
        if (userId > 0) {
            m_guildFriends.addElement(userId);
        }
        if (separator + 1 < remaining.GetLength() - 1) {
            remaining = remaining.GetSubString(separator + 1, remaining.GetLength() - 1);
        } else {
            remaining = "";
        }
    }
    WindowApp::HandleTunnelCommand(4, 0, 0, 0);
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
    if (m_guildFriends.m_items != NULL) {
        free(m_guildFriends.m_items);
        m_guildFriends.m_items = NULL;
    }
    m_guildFriends.m_size = 0;
    m_guildFriends.m_capacity = 0;
}

void CPublicData1::getData(TCVector<CNGSAttribute*>& attributes)
{
    CStrWChar levelName = getwTokenNameFor("LEVEL");
    CStrWChar bountiesName = getwTokenNameFor("CONTRACT");
    CStrWChar friendsName = getwTokenNameFor("GUILD_FRIENDS");
    CNGSAttribute* level = new CNGSAttribute(levelName, m_level, m_collectionType);
    CNGSAttribute* bounties = new CNGSAttribute(bountiesName, m_successBounties, m_collectionType);
    PrepareGuildFriendListAsString();
    CNGSAttribute* friends = new CNGSAttribute(friendsName, m_guildFriendList, false, m_collectionType);
    attributes.Add(level);
    attributes.Add(bounties);
    attributes.Add(friends);
}

void CPublicData1::PrepareGuildFriendListAsString()
{
    m_guildFriendList = "";
    for (int index = 0; index < m_guildFriends.size(); index++) {
        XString userId = XString::Format(L"%d;", m_guildFriends[index]);
        m_guildFriendList.Concatenate(userId.c_str());
    }
}

void CPublicData1::setData(const CHash* attributes)
{
    resetData();
    CNGSAttribute* level = NULL;
    CNGSAttribute* bounties = NULL;
    CNGSAttribute* friends = NULL;
    CStrChar levelName = getTokenNameFor("LEVEL");
    CStrChar bountiesName = getTokenNameFor("CONTRACT");
    CStrChar friendsName = getTokenNameFor("GUILD_FRIENDS");
    if (attributes->Find(CStringToKey(levelName.GetCharPtr(), 0), (void**)&level)) {
        m_level = level->getVal_int16();
    } else {
        dataNotFound();
    }
    if (attributes->Find(CStringToKey(bountiesName.GetCharPtr(), 0), (void**)&bounties)) {
        m_successBounties = bounties->getVal_int32();
    } else {
        dataNotFound();
    }
    if (attributes->Find(CStringToKey(friendsName.GetCharPtr(), 0), (void**)&friends)) {
        m_guildFriendList = friends->getVal_string();
        if (m_guildFriendList.GetWCharPtr() != NULL && m_guildFriendList.GetLength() > 0) {
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
    bool found = false;
    for (int index = 0; index < m_guildFriends.size(); index++) {
        found |= m_guildFriends[index] == userId;
    }
    return found;
}

void CPublicData1::addGuildFriend(int userId)
{
    if (userId > 0 && !isInGuildFriendList(userId)) {
        m_guildFriends.addElement(userId);
    }
    PrepareGuildFriendListAsString();
    WindowApp::HandleTunnelCommand(4, 0, 0, 0);
    static_cast<App*>(WindowApp::m_instance)->m_saveManager->SaveOnNextUpdate(0);
}
