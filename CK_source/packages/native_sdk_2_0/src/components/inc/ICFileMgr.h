// (first drafted by b005 as PROVISIONAL; completed by b002 with the implementation CFileMgr_Cocoa;
// class_owners.json lists packages/gserve_1_6_0/src/gServe_platform/CFileUtil_gServe.cpp)
// ICFileMgr.h - platform file system interface; one instance per application (singleton).
#ifndef ICFILEMGR_H
#define ICFILEMGR_H

#include <wchar.h>
#include "CSingleton.h"

                                                                           

class ICFile;

class ICFileMgr : public CSingleton {
public:
    enum { SingletonId = 0x70fa1bdf };

    enum eFileAccessMode {
        AccessRead = 0,     // "rb"
        AccessWrite = 1,    // "wb"
        AccessAppend = 2
    };

    ICFileMgr() : CSingleton(SingletonId) {}

    inline virtual ~ICFileMgr()
    {
        if (CApplet::m_pApp != NULL) {
            CApplet::m_pApp->m_fileMgr = NULL;
        }
    }

    /** Platform factory (CFileMgr_Cocoa). */
    static ICFileMgr* CreateInstance();

    /** The application's file manager: cached in CApp, created on first use. */
    static inline ICFileMgr* GetInstance()
    {
        CApp* app = CApplet::m_pApp;
        ICFileMgr* instance;
        if (app != NULL) {
            instance = app->m_fileMgr;
            if (instance == NULL) {
                ICFileMgr* found = NULL;
                app->m_singletons->Find(SingletonId, (void**)&found);
                instance = found;
                if (instance == NULL) {
                    instance = CreateInstance();
                }
                app->m_fileMgr = instance;
            }
        } else {
            instance = NULL;
        }
        return instance;
    }

    virtual const wchar_t* GetApplicationPath() = 0;
    virtual const wchar_t* GetApplicationDataPath() = 0;
    /** Inline in the original: the data directory unless a platform overrides it. */
    virtual const wchar_t* GetApplicationTempPath() { return GetApplicationDataPath(); }
    /** Inline in the original: the data directory unless a platform overrides it. */
    virtual const wchar_t* GetApplicationCachePath() { return GetApplicationDataPath(); }
    virtual const wchar_t* GetPathSeparator() = 0;
    // plat: CK 1.5.2 has no EnumInit/EnumNext slots (CFileMgr_Cocoa vtable 0x33a4b0)
    /** NULL when the file cannot be opened. */
    virtual ICFile* Open(const wchar_t* path, eFileAccessMode mode) = 0;
    virtual void Close(ICFile* file) = 0;
    virtual unsigned char Exists(const wchar_t* path) = 0;
    /** File size in bytes, 0 when missing. */
    virtual unsigned int Size(const wchar_t* path) = 0;
    virtual unsigned char Rename(const wchar_t* from, const wchar_t* to) = 0;
    virtual unsigned char Delete(const wchar_t* path) = 0;
    virtual unsigned char MkDir(const wchar_t* path) = 0;
    virtual unsigned char RmDir(const wchar_t* path) = 0;
    virtual unsigned char DeleteFilesInDir(const wchar_t* path) = 0;
    virtual unsigned int GetFreeDiskSpace() = 0;
};


       

#endif
