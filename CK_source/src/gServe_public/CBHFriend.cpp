#include "CBHFriend.h"

CBHFriend::CBHFriend(const CNGSUserCredentials* credentials) : CNGSRemoteUser(credentials)
{
    setPopulated(0);
    registerSaveRestoreInterface(&m_publicData);
}

CBHFriend::~CBHFriend()
{
}

typedef char AssertBHFriendSize[sizeof(CBHFriend) == 0xc8 ? 1 : -1];
