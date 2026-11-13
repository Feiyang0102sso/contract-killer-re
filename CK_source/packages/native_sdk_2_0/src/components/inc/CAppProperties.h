// CAppProperties.h - application settings singleton (created by b000x, header-only).
// Wraps the CProperties loaded from the properties file and remembers which keys a later
// merge overrode. The destructor is the key function and is defined inline below, so the
// vtable and destructors are emitted (weak) in every file that includes this header.
#ifndef CAPPPROPERTIES_H
#define CAPPPROPERTIES_H

#include "CSingleton.h"
#include "CProperties.h"
#include "CStrChar.h"
#include "CVector.h"

                                                                           

class CAppProperties : public CSingleton {
public:
    enum { SingletonId = 0x8c535a57 };

    // CDemoMgr::Read/GetDemoString and CUtility::RegisterUISoftkeys inline the full constructor.
    inline CAppProperties() : CSingleton(SingletonId)
    {
    }

    virtual ~CAppProperties();

    static inline CAppProperties* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(SingletonId, &found);
        CAppProperties* instance = static_cast<CAppProperties*>(found);
        if (instance == NULL) {
            instance = new CAppProperties();
        }
        return instance;
    }

    CProperties m_properties;                       // 0x08
    CVector m_overriddenKeys;                       // 0x28 keys replaced by merged property files
};

inline CAppProperties::~CAppProperties()
{
    for (int index = 0; index < m_overriddenKeys.Size(); ++index) {
        delete reinterpret_cast<CStrChar*>(m_overriddenKeys.m_data[index]);
    }
}

       

#endif
