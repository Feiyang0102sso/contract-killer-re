#ifndef CGAMEDATA_H
#define CGAMEDATA_H

#include "CSingleton.h"

class CResBank;

/** Game-wide data registered in the application singleton table. */
class CGameData : public CSingleton {
public:
    enum { SingletonId = 0x17b126f6 };

    CGameData();
    virtual ~CGameData();

    static inline CGameData* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, &found);
        CGameData* instance = (CGameData*)found;
        if (instance == NULL) {
            instance = new CGameData();
        }
        return instance;
    }

    CResBank* m_resBank;        // 0x08 resource bank of the active scene
    bool m_field0c;             // 0x0c
    bool m_field0d;             // 0x0d
    int m_field10;              // 0x10
};

#endif
