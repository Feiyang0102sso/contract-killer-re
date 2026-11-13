#include "AnimationTransition.h"

#include "AnimatableSwerveObject.h"
#include "AnimationImpl.h"
#include "DGHelper.h"

AnimationTransition::AnimationTransition(AnimatableSwerveObject* object, AnimationListener* listener)
    : m_object(object), m_listener(listener)
{
    m_descriptors.ensureCapacity(5);
}

AnimationTransition::~AnimationTransition()
{
    shutDownAllAnims();
}

void AnimationTransition::shutDownAllAnims()
{
    for (int index = 0; index < m_descriptors.size(); ++index) {
        AnimationInstance* instance = m_descriptors[index].m_instance;
        // The owning actor tears down its animation list before destroying this transition.
        if (instance != NULL) {
            delete instance;
        }
    }
    m_descriptors.removeAllElements();
}

void AnimationTransition::update(int elapsed)
{
    updateWeights(elapsed);
}

void AnimationTransition::updateWeights(int elapsed)
{
    for (int index = 0; index < m_descriptors.size(); ++index) {
        AnimationDescriptor& descriptor = m_descriptors[index];
        float change = elapsed * descriptor.m_fadeSpeed;
        if (descriptor.m_state == FadeOut) {
            descriptor.m_weight -= change;
            if (descriptor.m_weight <= 0.0f) {
                descriptor.m_weight = 0.0f;
                descriptor.m_state = Finished;
            }
        } else if (descriptor.m_state == FadeIn) {
            descriptor.m_weight += change;
            if (descriptor.m_weight >= 1.0f) {
                descriptor.m_weight = 1.0f;
                descriptor.m_state = Playing;
            }
        }
        if (descriptor.m_instance != NULL) {
            descriptor.m_instance->setWeight(descriptor.m_weight);
        }
    }

    int remaining = m_descriptors.size();
    for (int index = 0; index < remaining; ++index) {
        AnimationDescriptor& descriptor = m_descriptors[index];
        if (descriptor.m_state != Finished) {
            continue;
        }
        if (descriptor.m_instance != NULL) {
            descriptor.m_instance->stop();
            if (m_descriptors[index].m_instance != NULL) {
                delete m_descriptors[index].m_instance;
                m_descriptors[index].m_instance = NULL;
            }
        }
        if (remaining >= 2) {
            m_descriptors[index] = m_descriptors[remaining - 1];
        }
        --remaining;
        --index;
    }
    m_descriptors.setSize(remaining);
}

void AnimationTransition::startAnimation(AnimationImpl* animation, float weight, bool looped,
                                         int fadeTime, float speed, float startTime)
{
    if (animation == NULL) {
        return;
    }
    AnimationDescriptor* existing = getAnimDesc(DGHelper::getSwerveID(animation->getName()));
    if (existing != NULL) {
        existing->m_state = FadeIn;
        existing->m_fadeSpeed = 1.0f / fadeTime;
        if (existing->m_instance != NULL) {
            existing->m_instance->setTime(0.0f);
            existing->m_instance->setSpeed(speed);
            existing->m_instance->setLooped(looped);
        }
        return;
    }
    AnimationInstance* instance = m_object->play(animation, weight, false);
    if (m_listener != NULL || looped) {
        instance->setListener(this);
    }
    instance->setSpeed(speed);
    instance->setLooped(looped);
    if (startTime != 0.0f) {
        int duration = static_cast<int>(instance->m_animation->getDuration());
        instance->setTime(static_cast<float>(static_cast<int>(startTime) % duration));
    }
    AnimationDescriptor descriptor = {looped, 1.0f / fadeTime, 0.0f, FadeIn, instance};
    m_descriptors.addElement(descriptor);
}

AnimationTransition::AnimationDescriptor* AnimationTransition::getAnimDesc(int id) const
{
    for (int index = 0; index < m_descriptors.size(); ++index) {
        AnimationDescriptor& descriptor = m_descriptors.m_items[index];
        const char* name = descriptor.m_instance->m_animation->getName();
        if (DGHelper::getSwerveID(name) == id) {
            return &m_descriptors.m_items[index];
        }
    }
    return NULL;
}

void AnimationTransition::eventListener(const AnimationEvent& event, AnimationInstance* instance)
{
    const AnimationDescriptor* found = NULL;
    for (int index = 0; index < m_descriptors.size(); ++index) {
        const AnimationDescriptor& descriptor = m_descriptors[index];
        if (descriptor.m_instance == instance) {
            found = &descriptor;
            break;
        }
    }
    if (!found->m_looped || (event.m_id != AnimationEvent::EVENT_END && event.m_id != AnimationEvent::EVENT_START)) {
        if (m_listener != NULL) {
            m_listener->eventListener(event, instance);
        }
    }
}

void AnimationTransition::stopAnimation(const char* name, int)
{
    AnimationDescriptor* descriptor = getAnimDesc(DGHelper::getSwerveID(name));
    if (descriptor != NULL) {
        descriptor->m_state = FadeOut;
    }
}

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertAnimationTransitionSize[sizeof(AnimationTransition) == 0x1c ? 1 : -1];
#endif
