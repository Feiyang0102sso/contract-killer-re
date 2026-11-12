#ifndef CBHFRIEND_H
#define CBHFRIEND_H

#include "CNGSRemoteUser.h"
#include "CPublicData1.h"

/** Remote player with CK's public profile collection. */
class CBHFriend : public CNGSRemoteUser {
public:
    explicit CBHFriend(const CNGSUserCredentials* credentials);
    virtual ~CBHFriend();

    virtual unsigned char isPopulated() const { return m_populated; }
    virtual void setPopulated(unsigned char value) { m_populated = value; }

    CPublicData1 m_publicData; // 0x7c
    unsigned char m_populated; // 0xc4
};

#endif
