// CSwerveGraphics3D.h - binds Glu render surfaces as targets of the Swerve renderer.
// (created by b040x, the owner of swerve/code/src/CSwerveGraphics3D.cpp)
#ifndef CSWERVEGRAPHICS3D_H
#define CSWERVEGRAPHICS3D_H

#include <stddef.h>
#include "CClass.h"
#include "CVector.h"
#include "CRectangle.h"
#include "SwerveApi.h"
#include "ICGraphics.h"
#include "ICRenderSurface.h"
#include "SwvPort.h"

class ICRenderSurface;

/**
 * Render state of one off-screen image, kept in swvImage::m_glRuntime (created by swvGLBindImage).
 * Defined in this header in the original (N_SOL of swvglruntime.o); moved here from
 * malij297_glruntime.cpp by b104. CK (ggn): the destructor binds the previous target again.
 */
struct GLRuntime {
    ICRenderSurface* m_surface;           // 0x00 target surface of the image
    ICRenderSurface* m_previousTarget;    // 0x04 graphics target before the image was bound

    GLRuntime()
    {
        m_surface = NULL;
        m_previousTarget = NULL;
    }

    /** Give the device its previous target back. */
    ~GLRuntime()
    {
        if (m_previousTarget != NULL) {
            m_previousTarget->Bind();
        }
    }
};

/**
 * CK layout (ggn, from the CK constructor / Free / BindTarget / ReleaseTarget / GetMutableSurface
 * code): a plain polymorphic class (no CClass base, vtable = the two destructors), 0x30 bytes.
 * CKZ's class id, environment block and context-lost listener do not exist in CK.
 * CK's m_mutableSurfaces is the 0x14-byte CVector of native_sdk_2_0. Its constructor,
 * Size() and Add(int) are defined out of line.
 */
class CSwerveGraphics3D {
public:
    /** A render surface made for mutable images of one size / format (a resource named m_name). */
    struct MutableSurface {
        char m_name[0x10];                    // 0x00 "SWV_%08X"
        ICRenderSurface* m_surface;           // 0x10
    };

    CSwerveGraphics3D();
    virtual ~CSwerveGraphics3D();

    /** Create the renderer. Returns 1 on success. */
    unsigned char Init();
    /** Release the mutable surfaces and the renderer. */
    void Free();
    /** A render surface at least as large as `image` (created as a resource when there is none). */
    ICRenderSurface* GetMutableSurface(swvImage* image);  // changed by ggn (CK: swvImage)
    /**
     * Make `target` the target of the renderer (`viewport` NULL: the whole surface); `clear` and
     * `flags` go to the renderer's slot 0x44 unless clear is set without flags.
     * The default for `flags` only keeps the three-argument call of WindowBase compiling
     * (CK's Window::RunPaint3D passes 1, 0).
     */
    void BindTarget(ICRenderSurface* target, const CRectangle* viewport, unsigned char clear,
                    int flags = 0);
    /** Release the current target (give the graphics device its previous target back). */
    void ReleaseTarget();

    IGraphics3D* m_graphics3D;                  // 0x04
    ICRenderSurface* m_target;                  // 0x08
    ICRenderSurface* m_previousTarget;          // 0x0c target of the graphics device before BindTarget
    IObject3D* m_boundImage;                    // 0x10 image target being rendered into
    CRectangle m_viewport;                      // 0x14
    CVector m_mutableSurfaces;                 // 0x1c ARMv7 pointer values
};

#endif
