// CAppInfo.cpp - application identity.
#include "CAppInfo.h"
#include "ICStdUtil.h"
#include "AppGluStrings.h"

                                                                           

CAppInfo* CAppInfo::GetInstance()
{
    void* found = NULL;
    CApplet::m_pApp->m_singletons->Find(SingletonId, &found);
    CAppInfo* instance = (CAppInfo*)found;
    if (instance == NULL) {
        instance = new CAppInfo();
    }
    return instance;
}

CAppInfo::CAppInfo() : CSingleton(SingletonId)
{
    m_versionMajor = 0;
    m_versionMinor = 0;
    m_versionMicro = 0;
    m_versionMajor = atoi(VERSION_MAJOR);
    m_versionMinor = atoi(VERSION_MINOR);
    m_versionMicro = atoi(VERSION_MICRO);
    m_name = APP_GLU_NAME_STR;
    m_variant = APP_GLU_VARIANT_STR;

    // build tag line plus build date and time; the original passed __DATE__ / __TIME__ of its
    // build machine, written out here so the rebuilt binary carries the original values
    char buildInfo[100];
    ICStdUtil::SPrintF(buildInfo, APP_TIMESTAMP_FORMAT, "Aug 17 2012", "13:56:36");
    m_buildTag = buildInfo;
}

int CAppInfo::GetVersionMajor() const
{
    return m_versionMajor;
}

int CAppInfo::GetVersionMinor() const
{
    return m_versionMinor;
}

int CAppInfo::GetVersionMicro() const
{
    return m_versionMicro;
}

       
