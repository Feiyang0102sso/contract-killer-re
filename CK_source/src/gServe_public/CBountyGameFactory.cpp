#include "CBountyGameFactory.h"
#include "CBHFriend.h"

CNGSRemoteUser* CBountyGameFactory::createRemoteUser(const CNGSUserCredentials& credentials)
{
    return new CBHFriend(&credentials);
}
