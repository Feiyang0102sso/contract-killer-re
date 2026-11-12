// CSwerve.h - application singleton owning the Swerve (M3G) runtime and its renderer.
// (created by b040x, the owner of swerve/code/src/CSwerve.cpp)
#ifndef CSWERVE_H
#define CSWERVE_H

#include <stddef.h>
#include "CSingleton.h"
#include "CStrCharBuffer.h"
#include "SwerveApi.h"

class CSwerveGraphics3D;

class CSwerve : public CSingleton {
public:
    /** Key in the application singleton table. */
    enum { SingletonId = 0x36412505 };

    CSwerve();
    virtual ~CSwerve();

    /** The application's instance, created on first use. */
    static inline CSwerve* GetInstance()
    {
        if (m_pSwerve == NULL) {
            CSwerve* found = NULL;
            CApplet::m_pApp->m_singletons->Find(SingletonId, (void**)&found);
            CSwerve* swerve = found;
            if (swerve == NULL) {
                swerve = new CSwerve();
            }
            m_pSwerve = swerve;
        }
        return m_pSwerve;
    }

    /** Create the runtime and the renderer (OpenGL ES 1.x only). Returns 1 on success. */
    unsigned char Init();
    /** Destroy the renderer and the runtime. */
    void Free();
    /** Log `error` of the runtime together with its error text; `context` names the caller.
        (CKZ only: not in CK's CSwerve.o, declaration kept for the ported game code) */
    static void LogError(int error, const char* context);
    /** Decode an M3G file with `loader` and return its first object (NULL on failure). (CK, added by ggn) */
    IObject3D* LoadM3GRoot(ILoader* loader, unsigned int size, const unsigned char* data);
    /** Load the M3G resource `name` and return its first root object (NULL on failure). */
    IObject3D* LoadM3GResource(const char* name);
    /** Hash of `text` used as M3G user id (30 bits). (CKZ only: not in CK's binary) */
    static int GenerateUserID(const char* text);

    static CSwerve* m_pSwerve;      // the instance (also in the singleton table)

    ISwerve* m_swerve;                                  // 0x08 runtime root object
    CSwerveGraphics3D* m_graphics3D;                    // 0x0c
    // CK: the object is 0x10 bytes (np_malloc(0x10) in swvGLBindImage); CKZ's m_buffer at 0x10 removed by ggn
};

#endif
