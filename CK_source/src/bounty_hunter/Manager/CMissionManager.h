// CMissionManager.h - loads and owns the CK story missions.
#ifndef CMISSIONMANAGER_H
#define CMISSIONMANAGER_H

#include "CMission.h"

class CMissionManager {
public:
    CMissionManager();
    virtual ~CMissionManager();
    CMission* GetMissionById(int id) const;

    Vector<CMission*> m_missions; // 0x04
};

#endif
