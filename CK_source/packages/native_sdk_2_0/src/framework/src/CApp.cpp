// CApp.cpp - base application: subsystem ownership.
// (gfx 2026-11-10: CK's CApp.o only holds the constructor and the destructors; the port-era
// default configuration, Initialize/Destroy/OnTimer/OnSystemEvent and FrameworkHandleMessage
// are not in the CK unit - the game's CGameApp implements the pure virtuals.)
#include "CApp.h"

#include "CAppExecutor.h"
#include "CResourceManager.h"
#include "CHash.h"
#include "CSingleton.h"



CApp::CApp() : CClass(ClassId)
{
    m_executor = NULL;
    m_registry = NULL;
    m_resourceManager = NULL;
    // 0x14 is left alone, as in the original (memset starts at 0x18)
    m_object18 = NULL;
    m_singletons = NULL;
    m_handleFactory = NULL;
    m_fileMgr = NULL;
    m_cached38 = NULL;
    m_mediaPlayer = NULL;
    m_moviePlayer = NULL;
    m_graphics = NULL;
    m_graphics2d = NULL;
}

CApp::~CApp()
{
    if (m_executor != NULL) {
        delete m_executor;
        m_executor = NULL;
    }
    if (m_registry != NULL) {
        delete m_registry;
        m_registry = NULL;
    }
    if (m_resourceManager != NULL) {
        delete m_resourceManager;
        m_resourceManager = NULL;
    }
    if (m_object18 != NULL) {
        delete m_object18;
        m_object18 = NULL;
    }

    // delete every singleton still registered, then the table itself
    if (m_singletons != NULL) {
        int bucketCount = m_singletons->m_bucketCount;
        for (int bucket = 0; bucket < bucketCount; bucket++) {
            for (CHash::Entry* entry = m_singletons->m_buckets[bucket]; entry != NULL; entry = entry->m_next) {
                if (entry->m_value != NULL) {
                    delete (CSingleton*)entry->m_value;
                    entry->m_value = NULL;
                }
            }
        }
        // the original re-checks the pointer (safe-delete idiom) and skips the store when NULL
        CHash* singletons = m_singletons;
        if (singletons != NULL) {
            delete singletons;
            m_singletons = NULL;
        }
    }
}


