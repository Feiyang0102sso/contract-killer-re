// CVirtualFriendManager.h - friends stored in the save file (virtual_friend_list).
#ifndef CVIRTUALFRIENDMANAGER_H
#define CVIRTUALFRIENDMANAGER_H

#include "Vector_ss.h"
#include "xString.h"

/** One virtual friend (12 bytes). */
struct SVirtualFriendInfo {
    XString m_name;     // 0x00
    int m_level;        // 0x04
    XString m_avatarId; // 0x08
};

class CVirtualFriendManager {
public:
    CVirtualFriendManager();
    void Load();

    Vector<SVirtualFriendInfo> m_friends;   // 0x00
};

#endif
