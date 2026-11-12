// Fade and ownership of the animation instances used by one CK actor.
#ifndef ANIMATIONTRANSITION_H
#define ANIMATIONTRANSITION_H

#include "AnimationInstance.h"
#include "Vector_ss.h"

class AnimationImpl;
class AnimatableSwerveObject;

class AnimationTransition : public AnimationListener {
public:
    enum State { FadeIn, Playing, FadeOut, Finished };

    struct AnimationDescriptor {
        bool m_looped;
        float m_fadeSpeed;
        float m_weight;
        State m_state;
        AnimationInstance* m_instance;
    };

    AnimationTransition(AnimatableSwerveObject* object, AnimationListener* listener);
    virtual ~AnimationTransition();
    void shutDownAllAnims();
    void update(int elapsed);
    void updateWeights(int elapsed);
    void startAnimation(AnimationImpl* animation, float weight, bool looped, int fadeTime, float speed, float startTime);
    AnimationDescriptor* getAnimDesc(int id) const;
    virtual void eventListener(const AnimationEvent& event, AnimationInstance* instance);
    void stopAnimation(const char* name, int fadeTime);

private:
    Vector<AnimationDescriptor> m_descriptors;
    AnimatableSwerveObject* m_object;
    AnimationListener* m_listener;
};

#endif
