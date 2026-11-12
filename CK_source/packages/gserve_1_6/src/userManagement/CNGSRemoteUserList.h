// PROVISIONAL (created by game2): owner packages/gserve_1_6/src/userManagement/CNGSRemoteUserList.cpp not written yet
// CNGSRemoteUserList.h - a list of remote users kept by the local user; only what the game UI uses.
#ifndef CNGSREMOTEUSERLIST_H
#define CNGSREMOTEUSERLIST_H

#include "CNGSServerObject.h"
#include "CHash.h"
#include "CStrWChar.h"
#include "TCVector.h"

class CNGSRemoteUser;

class CNGSRemoteUserList : public CNGSServerObject {
public:
    CNGSRemoteUserList();
    void reset();
    virtual ~CNGSRemoteUserList();

    void addRemoteUserIfNotAlreadyInList(CNGSRemoteUser* user);
    void removeRemoteUser(int index);
    /** The listed user with id `userId`, NULL when not in the list. */
    CNGSRemoteUser* findRemoteUserByID(int userId);
    CNGSRemoteUser* findRemoteUserBySNID(int network, const CStrWChar& identity);

    int m_field18;
    TCVector<CNGSRemoteUser*> m_users;
    CHash m_usersById;
    CHash m_usersByNetworkId[6];
};

#endif
