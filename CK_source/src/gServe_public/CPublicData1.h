// CPublicData1.h - the public profile fields and the list of guild friends.
#ifndef CPUBLICDATA1_H
#define CPUBLICDATA1_H

#include "CSaveRestoreInterface.h"
#include "Vector_ss.h"

class CPublicData1 : public CSaveRestoreInterface {
public:
    CPublicData1();
    virtual void resetData();
    virtual unsigned char compareData(const CSaveRestoreInterface* other);
    virtual unsigned char copyData(const CSaveRestoreInterface* other);
    virtual void getData(TCVector<CNGSAttribute*>& attributes);
    virtual void setData(const CHash* attributes);
    virtual CStrWChar asString();
    virtual void debugPrint();

    void initGuildFriendsList();
    void PrepareGuildFriendListAsString();
    bool isInGuildFriendList(int userId);
    void addGuildFriend(int userId);

    short m_level;                  // +0x20
    int m_successBounties;           // +0x24
    CStrWChar m_guildFriendList;     // +0x28 semicolon-terminated IDs
    Vector<int> m_guildFriends;      // +0x38
};

#endif
