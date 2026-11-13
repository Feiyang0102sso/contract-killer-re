// AppMedia.h - scheduled music, sound effects and vibration.
#ifndef APPMEDIA_H
#define APPMEDIA_H

#include "WindowBase.h"
#include "Vector_ss.h"

class AppMedia {
public:
    enum ESoundChannel {
        CHANNEL_MUSIC = 0,
        CHANNEL_EFFECT = 1,
        CHANNEL_MULTIPLE = 2
    };

    enum ESoundLoopType {
        PLAY_ONCE = 0,
        PLAY_LOOP = 1
    };

    enum EStartType {
        START_IF_IDLE = 0,
        START_REPLACE = 1,
        START_IF_DIFFERENT = 2
    };

    AppMedia();
    ~AppMedia();

    void Init();
    void Shutdown();
    void Pause();
    void Resume();
    void Update();
    void SetActivated(bool activated);
    bool ResumeMusic();
    void StopVibration();
    void Stop(ESoundChannel channel);
    void StopAll();
    void Play(ESoundChannel channel, const MediaRes& media, float delay,
              ESoundLoopType loop, EStartType start);

private:
    struct SoundChannel {
        SoundChannel()
        {
            m_owner = NULL;
            m_startTime = 0;
            m_eventId = 0;
            m_loop = false;
        }
        SoundChannel(AppMedia* owner, const MediaRes& media, float startTime)
            : m_owner(owner), m_media(media), m_startTime(startTime), m_eventId(0), m_loop(false)
        {
        }
        void Stop();
        void Update();

        AppMedia* m_owner;
        MediaRes m_media;
        float m_startTime;
        unsigned int m_eventId;
        bool m_loop;
    };

    void UpdateMediaPlayerSettings();

    bool m_activated;
    SoundChannel m_channels[2];
    Vector<SoundChannel> m_effects;
    MediaRes m_music;
    float m_vibrationStartTime;
    unsigned int m_vibrationDuration;
    unsigned int m_vibrationEvent;
};

#endif
