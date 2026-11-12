// CAppInfo.h - application identity: name, variant, build tag and version (singleton).
#ifndef CAPPINFO_H
#define CAPPINFO_H

#include "CSingleton.h"
#include "CStrWChar.h"

                                                                           

class CAppInfo : public CSingleton {
public:
    enum { SingletonId = 0x7674f803 };

    /** The application's instance, created on first use. */
    static CAppInfo* GetInstance();

    CAppInfo();

    virtual inline ~CAppInfo();

    int GetVersionMajor() const;
    int GetVersionMinor() const;
    int GetVersionMicro() const;

    CStrWChar m_name;               // 0x08 APP_GLU_NAME_STR
    CStrWChar m_variant;            // 0x18 APP_GLU_VARIANT_STR
    CStrWChar m_buildTag;           // 0x28 APP_TIMESTAMP_FORMAT: "$Name: ... $" tag plus build date/time
    int m_versionMajor;             // 0x38 VERSION_MAJOR
    int m_versionMinor;             // 0x3c VERSION_MINOR
    int m_versionMicro;             // 0x40 VERSION_MICRO
};

inline CAppInfo::~CAppInfo()
{
}

       

#endif
