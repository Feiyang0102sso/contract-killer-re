#ifndef CBOUNTYGAMEFACTORY_H
#define CBOUNTYGAMEFACTORY_H

#include "CNGSFactory.h"

/** Adds CK's public profile to gServe remote users. */
class CBountyGameFactory : public CNGSFactory {
public:
    virtual CNGSRemoteUser* createRemoteUser(const CNGSUserCredentials& credentials);
    virtual ~CBountyGameFactory() {}
};

#endif
