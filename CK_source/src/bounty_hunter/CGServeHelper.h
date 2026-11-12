// CGServeHelper.h - gServe lookups used by the game UI. Owner: src/bounty_hunter/CGServeHelper.cpp (game2).
#ifndef CGSERVEHELPER_H
#define CGSERVEHELPER_H

#include "xString.h"

class CGServeHelper {
public:
    /** Nickname of friend `userId` (empty when the user is not in the friend list). */
    static XString GetFriendNickname(int userId);
};

#endif
