// CFriendGatedMissionManager.h - the missions of res/common/configs/friends_gated_missions.xml.
// Layout (0x10, no vtable) from the constructor and GetMissionById.
#ifndef CFRIENDGATEDMISSIONMANAGER_H
#define CFRIENDGATEDMISSIONMANAGER_H

#include <stddef.h>
#include "Vector_ss.h"

class CFriendGatedMission;

class CFriendGatedMissionManager {
public:
    CFriendGatedMissionManager();
    ~CFriendGatedMissionManager();

    /** NULL when `id` is out of range. */
    CFriendGatedMission* GetMissionById(int id) const;
    int MissionCount() const;

    Vector<CFriendGatedMission*> m_missions;    // 0x00
};

#endif
