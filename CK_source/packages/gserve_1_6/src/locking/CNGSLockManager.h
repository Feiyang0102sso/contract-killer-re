#ifndef CNGSLOCKMANAGER_H
#define CNGSLOCKMANAGER_H

#include "CNGSServerObject.h"
#include "CSingleton.h"

class CHash_gServe;

/** Keeps the server's shared-data locks for the current session. */
class CNGSLockManager : public CNGSServerObject, public CSingleton {
public:
    enum { ClassId = 0x4cda0bb7 };

    CNGSLockManager();
    virtual ~CNGSLockManager();

    static inline CNGSLockManager* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(ClassId, &found);
        CNGSLockManager* instance = static_cast<CNGSLockManager*>((CSingleton*)found);
        if (instance == NULL) {
            instance = new CNGSLockManager();
        }
        return instance;
    }

    void removeDeleteAllLocks();
    void suspend();
    void resume();
    void reset();

private:
    CHash_gServe* m_locks;  // 0x20
};

#endif
