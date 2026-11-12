// CNGSFriendsManager.cpp - (added by gserve2) the original unit has no own functions in CK 1.5.2:
// only header inlines and the static initializer that computes the friends manager class id
// (and the CNGSURLMgr key of the included header).
#include "CStringToKey.h"

/**
 * Singleton key of the friends manager, computed at static initialization. The original unit read it
 * in code that the linker dead-stripped; `used` keeps the store without that code.
 */
static unsigned int ClassId_CFriendsManager __attribute__((used)) = CStringToKey("CFriendsManager", 0);

#include "CNGSURLMgr.h"
