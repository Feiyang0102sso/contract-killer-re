// CApp.h - base application: owns the executor, the registry, the resource manager and the
// singleton table, and caches the platform interfaces. The game derives from it (CGameApp) and
// supplies OnInit()/OnFree()/OnTimer()/OnSystemEvent() and CApp::CreateInstance().
// (gfx 2026-11-10: layout and vtable rebuilt from the CK constructor / destructor and the
// GetInstance readers: CK has no CAppConfig member and only the four pure virtuals below.)
#ifndef CAPP_H
#define CAPP_H

#include "CClass.h"




class CHash;
class ICFileMgr;
class ICMediaPlayer;
class ICMoviePlayer;  // added by fx2
class ICGraphics;
class ICGraphics2d;



class CMessage;




class CAppExecutor;
class CResourceManager;

class CApp : public CClass {
public:
    enum { ClassId = 0x0275a643 };

    /** System event ids passed to OnSystemEvent() by the platform layer. */
    enum SystemEvent {
        SystemEvent_Start = 0x67542658,
        SystemEvent_Stop = 0x36754280,
        SystemEvent_Suspend = 0x54246de3,
        SystemEvent_Resume = 0x75424363
    };

    /** The game's application object (src/CGameApp.cpp). */
    static CApp* CreateInstance();

    CApp();
    virtual ~CApp();

    /** Game start-up; 0 on success (CApplet::Initialize calls it directly). */
    virtual int OnInit() = 0;
    virtual void OnFree() = 0;
    /** Platform timer tick. */
    virtual void OnTimer() = 0;
    /** A system event queued by the platform layer. */
    virtual void OnSystemEvent(unsigned int eventId, unsigned int param1, unsigned int param2) = 0;

    // PORT COMPAT (gfx): CApplet_mm.mm (plat) still calls these port-era entry points; the CK
    // CApplet calls OnInit()/OnFree() through the vtable. Remove when CApplet is converted.
    int Initialize() { return OnInit(); }
    void Destroy() { OnFree(); }

    CAppExecutor* GetExecutor() const { return m_executor; }

    CAppExecutor* m_executor;                       // 0x08 owned
    union {
        CClass* m_registry;                         // 0x0c owned; CRegistry created by CGameApp::InitRegistry
        CClass* m_subsystem14;                      // PORT COMPAT name (CEvent / CEventListener of plat)
    };
    CResourceManager* m_resourceManager;            // 0x10 owned
    void* m_field14;                                // 0x14 not touched by CApp (never initialized)
    CClass* m_object18;                             // 0x18 owned, deleted after the resource manager
    CHash* m_singletons;                // 0x1c CSingleton id -> instance, owned
    void* m_handleFactory;                         // 0x20 CHandleFactory* cache
    ICFileMgr* m_fileMgr;               // 0x24 ICFileMgr* cache
    void* m_cached38;                               // 0x28 ICLicenseMgr* cache (port name kept for ICLicenseMgr.h)
    ICMediaPlayer* m_mediaPlayer;       // 0x2c ICMediaPlayer* cache
    ICMoviePlayer* m_moviePlayer;       // 0x30 ICMoviePlayer* cache  // changed by fx2 (was void*)
    union {
        ICGraphics* m_graphics;                     // 0x34 ICGraphics* cache
        void* m_graphicsAbstractionManager;         // PORT COMPAT name (ICGraphicsAbstractionManager.h)
    };
    ICGraphics2d* m_graphics2d;                     // 0x38 ICGraphics2d* cache
};



#endif
