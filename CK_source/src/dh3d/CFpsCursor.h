#ifndef CFPSCURSOR_H
#define CFPSCURSOR_H

#include "DGCursor.h"

class SG_Presenter;

/** Fixed crosshair that turns the camera with the aim gesture. */
class CFPSCursor : public DGCursor {
public:
    CFPSCursor();
    virtual ~CFPSCursor();
    virtual int GetAimX() const;
    virtual int GetAimY() const;
    virtual void ShowCross(bool show);
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

    void HandleShot();
    void DrawCross();
    void DrawReloadingMessage();

private:
    int m_aimX;
    int m_aimY;
    SG_Presenter* m_cross;
    int m_downX;
    int m_downY;
    float m_angleV;
    float m_angleH;
    bool m_pressed;
    bool m_moved;
    bool m_shotRequested;
    bool m_shotAnimation;
    bool m_showCross;
};

#endif
