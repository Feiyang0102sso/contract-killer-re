#ifndef ANIMATIONINSTANCE_H
#define ANIMATIONINSTANCE_H

#include "Fixed.h"

class Animation;
class AnimatableObject;
class AnimationInstance;

/** Event passed to AnimationListener::eventListener. */
struct AnimationEvent {
    enum {
        EVENT_END = 0x7ffffffe,
        EVENT_START = 0x7fffffff
    };

    AnimationEvent(int id, const char* name) : m_id(id), m_name(name) {}

    int m_id;
    const char* m_name;
};

class AnimationListener {
public:
    virtual ~AnimationListener() {}
    virtual void eventListener(const AnimationEvent& event, AnimationInstance* instance) = 0;
};

/** The playback state of one animation on an object. */
class AnimationInstance {
public:
    AnimationInstance(Animation* animation, AnimatableObject* object);

    void advanceTime(int elapsed);
    void setWeight(float weight);
    void setTime(float time);
    void stop();
    void setLooped(bool looped);

    /** Playback speed factor (1.0: normal). */
    void setSpeed(float speed)
    {
        m_speed = Fixed::fromFloat(speed);
    }

    void setListener(AnimationListener* listener)
    {
        if (!m_stopped) {
            m_listener = listener;
        }
    }

    Animation* m_animation;             // 0x00
    AnimatableObject* m_object;         // 0x04
    int m_time;                         // 0x08 20.12
    int m_weight;                       // 0x0c 20.12
    int m_speed;                        // 0x10 20.12
    AnimationListener* m_listener;      // 0x14
    int m_nextEventIndex;               // 0x18
    bool m_stopped;                     // 0x1c
    bool m_looped;                      // 0x1d
};

#endif
