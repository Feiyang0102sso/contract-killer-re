// CNGSLockManager.cpp - lifecycle of shared-data locks.
#include "CNGSLockManager.h"
#include "CHash_gServe.h"
#include "CNGSUtil.h"
#include "CNGSAttributeManager.h"

/** Lock state and owned strings identified by the lock manager. */
struct CNGSLock {
    enum {
        StateInitial = 0,
        StateWaiting = 1,
        StateSuspended = 4,
        StateResumed = 5
    };

    unsigned char m_unknown[0x10];
    int m_state;          // 0x10
    CStrChar m_text14;
    unsigned char m_unknown24[8];
    CStrWChar m_text2c;
    CStrWChar m_text3c;
    CStrChar m_text4c;
};

CNGSLockManager::CNGSLockManager()
    : CSingleton(ClassId)
{
    CHash_gServe* locks = new CHash_gServe();
    locks->Init(100, 20, 1);
    m_locks = locks;
}

CNGSLockManager::~CNGSLockManager()
{
    removeDeleteAllLocks();
    // the original clears the pointer twice (inside the check and after it)
    if (m_locks != NULL) {
        delete m_locks;
        m_locks = NULL;
    }
    m_locks = NULL;
}

void CNGSLockManager::removeDeleteAllLocks()
{
    // The original frees the lock at the head of each bucket.
    for (int bucket = 0; bucket < m_locks->m_bucketCount; bucket++) {
        CHash_gServe::Entry* entry = m_locks->m_buckets[bucket];
        if (entry != NULL) {
            delete (CNGSLock*)entry->m_value;
        }
    }
    m_locks->RemoveAll();
}

void CNGSLockManager::suspend()
{
    CNGSUtil::DebugLog("CNGSLockManager::suspend()");
    for (int bucket = 0; bucket < m_locks->m_bucketCount; bucket++) {
        CHash_gServe::Entry* entry = m_locks->m_buckets[bucket];
        if (entry != NULL) {
            CNGSLock* lock = (CNGSLock*)entry->m_value;
            switch (lock->m_state) {
            case CNGSLock::StateInitial:
            case CNGSLock::StateWaiting:
            case CNGSLock::StateResumed:
                lock->m_state = CNGSLock::StateSuspended;
                break;
            default:
                break;
            }
            CNGSUtil::DebugLog("---------------------------");
        }
    }
}

void CNGSLockManager::resume()
{
    CNGSUtil::DebugLog("CNGSLockManager::resume()");
    for (int bucket = 0; bucket < m_locks->m_bucketCount; bucket++) {
        CHash_gServe::Entry* entry = m_locks->m_buckets[bucket];
        if (entry != NULL) {
            CNGSLock* lock = (CNGSLock*)entry->m_value;
            if (lock->m_state == CNGSLock::StateSuspended) {
                lock->m_state = CNGSLock::StateResumed;
            }
            CNGSUtil::DebugLog("---------------------------");
        }
    }
}

void CNGSLockManager::reset()
{
    CNGSUtil::DebugLog("CNGSLockManager::reset()");
    removeDeleteAllLocks();
}
