// CAvatarManager.h - the avatars of res/common/configs/avatars.xml.
// Layout (0x10, no vtable) from the constructor.
#ifndef CAVATARMANAGER_H
#define CAVATARMANAGER_H

#include <stddef.h>
#include "Vector_ss.h"

class CAvatar;

class CAvatarManager {
public:
    CAvatarManager();
    ~CAvatarManager();

    /** Avatar with m_id == id (NULL when none). */
    CAvatar* GetAvatar(int id);
    /** Does any avatar unlock at `level`? */
    bool IsNewAvatarUnlocked(int level);

    Vector<CAvatar*> m_avatars;     // 0x00
};

#endif
