// CDH_Animation.h - controllers attached to one CK Swerve object.
#ifndef CDH_ANIMATION_H
#define CDH_ANIMATION_H

#include "tws.h"
#include "Vector_ss.h"
#include "xString.h"

class CDH_Animation {
public:
    typedef ::Object3D Object3D;
    typedef AnimationController Controller;
    typedef AnimationTrack Track;

    explicit CDH_Animation(Object3D object);
    virtual ~CDH_Animation();

    static bool HaveController(Object3D object, const char* name);
    int addController(const XString& name);
    int getControllerBaseDuration(int index);
    void setControllerDuration(int index, int duration);
    void setCurrentController(int index, bool reverse);
    void setCurrentController(int worldTime, int index, bool reverse);
    void stopAllAnimation();
    void animate();
    void animate(int worldTime);
    bool isAnimationComplete() const;
    bool isAnimationComplete(int worldTime) const;

private:
    Object3D m_object;                  // 0x04
    Vector<Controller> m_controllers;  // 0x08
    Vector<int> m_durations;            // 0x18
    int m_currentController;           // 0x28
    int m_startTime;
    float m_speed;
};

#endif
