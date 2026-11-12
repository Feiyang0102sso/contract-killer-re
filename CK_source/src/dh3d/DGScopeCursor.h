#ifndef DGSCOPECURSOR_H
#define DGSCOPECURSOR_H

#include "DGCursor.h"

class ICRenderSurface;

/** Screen-center scope with a camera field of view driven by the HUD slider. */
class DGScopeCursor : public DGCursor {
public:
    DGScopeCursor();
    virtual ~DGScopeCursor();
    virtual void onConstraintReached(int constraint);
    virtual int GetAimX() const;
    virtual int GetAimY() const;
    virtual void onWeaponSwitch();
    virtual void onFire();
    virtual void onPointerMove(int x, int y, int deltaX, int deltaY);
    virtual void onPointerBreak();
    virtual void onPointerPressed(int x, int y);
    virtual void onPointerReleased();
    virtual void DrawImpl();
    virtual void UpdateImpl(int elapsedMs);
    virtual void InitImpl();
    virtual void DeInitImpl();

    void CreateResources();
    void ReleaseResources();
    void InitFovValues();

private:
    float m_wideFov;                      // 0x54
    float m_narrowFov;
    bool m_resourcesLoaded;
    int m_field60;
    ICRenderSurface* m_pin;
    ICRenderSurface* m_dot;
    ICRenderSurface* m_arrows;
    MovingEntity m_reticleMotion;        // 0x70
    bool m_pressed;
    bool m_moved;
    int m_motionDelayMs;                 // 0xb8
    int m_fieldsBC[4];
    float m_fov;                         // 0xcc
    bool m_fired;
};

#endif
