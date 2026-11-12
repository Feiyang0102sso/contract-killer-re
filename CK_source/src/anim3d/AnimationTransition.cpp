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
        if (instance != NULL) {
            instance->setWeight(0.0f);
            instance->stop();
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
        if (descriptor.m_state == FadeIn) {
            descriptor.m_weight += change;
            if (descriptor.m_weight >= 1.0f) {
                descriptor.m_weight = 1.0f;
                descriptor.m_state = Playing;
            }
        } else if (descriptor.m_state == FadeOut) {
            descriptor.m_weight -= change;
            if (descriptor.m_weight <= 0.0f) {
                descriptor.m_weight = 0.0f;
                descriptor.m_state = Finished;
            }
        }
        if (descriptor.m_instance != NULL) {
            descriptor.m_instance->setWeight(descriptor.m_weight);
        }
    }

    int remaining = m_descriptors.size();
    int index = 0;
    while (index < remaining) {
        AnimationDescriptor& descriptor = m_descriptors[index];
        if (descriptor.m_state != Finished) {
            ++index;
            continue;
        }
        if (descriptor.m_instance != NULL) {
            descriptor.m_instance->stop();
            delete descriptor.m_instance;
        }
        --remaining;
        if (index != remaining) {
            descriptor = m_descriptors[remaining];
        }
    }
    m_descriptors.setSize(remaining);
}

void AnimationTransition::startAnimation(AnimationImpl* animation, float weight, bool looped,
                                         int fadeTime, float speed, float startTime)
{
    if (animation == NULL) {
        return;
    }
    float fadeSpeed = 1.0f / fadeTime;
    AnimationDescriptor* existing = getAnimDesc(DGHelper::getSwerveID(animation->getName()));
    if (existing != NULL) {
        existing->m_state = FadeIn;
        existing->m_fadeSpeed = fadeSpeed;
        existing->m_looped = looped;
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
        int duration = static_cast<int>(animation->getDuration());
        if (duration > 0) {
            instance->setTime(static_cast<float>(static_cast<int>(startTime) % duration));
        }
    }
    AnimationDescriptor descriptor = {looped, fadeSpeed, 0.0f, FadeIn, instance};
    m_descriptors.addElement(descriptor);
}

AnimationTransition::AnimationDescriptor* AnimationTransition::getAnimDesc(int id) const
{
    for (int index = 0; index < m_descriptors.size(); ++index) {
        AnimationDescriptor& descriptor = m_descriptors.m_items[index];
        if (descriptor.m_instance != NULL &&
            DGHelper::getSwerveID(descriptor.m_instance->m_animation->getName()) == id) {
            return &descriptor;
        }
    }
    return NULL;
}

void AnimationTransition::eventListener(const AnimationEvent& event, AnimationInstance* instance)
{
    if (m_listener == NULL) {
        return;
    }
    for (int index = 0; index < m_descriptors.size(); ++index) {
        AnimationDescriptor& descriptor = m_descriptors[index];
        if (descriptor.m_instance == instance) {
            if (!descriptor.m_looped || (event.m_id != AnimationEvent::EVENT_END && event.m_id != AnimationEvent::EVENT_START)) {
                m_listener->eventListener(event, instance);
            }
            return;
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

typedef char AssertAnimationTransitionSize[sizeof(AnimationTransition) == 0x1c ? 1 : -1];
