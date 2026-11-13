// AppMedia.cpp - scheduled music, sound effects and vibration.
#include "AppMedia.h"
#include "App.h"
#include "AppSettings.h"
#include "ICMediaPlayer.h"

void AppMedia::SoundChannel::Stop()
{
    if (m_eventId != 0) {
        ICMediaPlayer* player = ICMediaPlayer::GetInstance();
        if (player->IsPlaying(m_eventId)) {
            player->Stop(m_eventId);
        }
    }
    m_media.Release();
    m_startTime = 0;
    m_eventId = 0;
    m_loop = false;
}

void AppMedia::SoundChannel::Update()
{
    if (m_media.m_resource != NULL) {
        ICMediaPlayer* player = ICMediaPlayer::GetInstance();
        if (m_eventId == 0) {
            if (m_startTime <= WindowApp::m_instance->m_timeSeconds) {
                CMedia* data = NULL;
                if (m_media.m_resource != NULL) {
                    data = static_cast<CMedia*>(m_media.m_resource->GetData());
                }
                m_eventId = player->Play(data, m_loop, 0);
            }
        } else if (!player->IsPlaying(m_eventId)) {
            m_media.Release();
            m_startTime = 0;
            m_eventId = 0;
            m_loop = false;
        }
    }
}

AppMedia::AppMedia()
{
    m_activated = false;
    for (int index = 0; index < CHANNEL_MULTIPLE; ++index) {
        m_channels[index].m_owner = this;
    }
    m_vibrationStartTime = 0;
    m_vibrationDuration = 0;
    m_vibrationEvent = 0;
}

AppMedia::~AppMedia()
{
    StopAll();
}

void AppMedia::StopAll()
{
    Stop(CHANNEL_MULTIPLE);
    // The original loops over the two single channels (0 and 1).
    for (int channel = 0; channel < 2; channel++) {
        Stop((ESoundChannel)channel);
    }
    StopVibration();
}

void AppMedia::Init()
{
    UpdateMediaPlayerSettings();
}

void AppMedia::UpdateMediaPlayerSettings()
{
    AppSettings* settings = static_cast<App*>(WindowApp::m_instance)->m_settings;
    bool sound = m_activated && (settings->IsSoundEnabled() || settings->IsMusicEnabled());
    bool vibration = m_activated && settings->IsVibrationEnabled();
    int volume = settings->Volume();
    ICMediaPlayer* player = ICMediaPlayer::GetInstance();
    if (player->GetSoundEnabled() != sound) {
        player->SetSoundEnabled(sound);
        ResumeMusic();
    }
    if (player->GetVibrationEnabled() != vibration) {
        player->SetVibrationEnabled(vibration);
    }
    if (player->GetVolume() != volume) {
        player->SetVolume(volume);
    }
}

void AppMedia::Shutdown()
{
    StopAll();
}

void AppMedia::Pause()
{
    ICMediaPlayer* player = ICMediaPlayer::GetInstance();
    player->Pause(0);
    player->PauseVibrate(0);
}

void AppMedia::Resume()
{
    ICMediaPlayer* player = ICMediaPlayer::GetInstance();
    player->Resume(0);
    player->ResumeVibrate(0);

    AppSettings* settings = static_cast<App*>(WindowApp::m_instance)->m_settings;
    bool sound = settings->IsSoundEnabled();
    bool music = settings->IsMusicEnabled();
    bool vibration = settings->IsVibrationEnabled();
    int volume = settings->Volume();
    settings->SetSoundEnabled(false);
    settings->SetMusicEnabled(false);
    settings->SetVibrationEnabled(false);
    settings->SetVolume(0);
    Update();
    settings->SetSoundEnabled(sound);
    settings->SetMusicEnabled(music);
    settings->SetVibrationEnabled(vibration);
    settings->SetVolume(volume);
    Update();
}

void AppMedia::Update()
{
    UpdateMediaPlayerSettings();
    AppSettings* settings = static_cast<App*>(WindowApp::m_instance)->m_settings;
    bool music = m_activated && settings->IsMusicEnabled();
    bool sound = m_activated && settings->IsSoundEnabled();
    bool vibration = m_activated && settings->IsVibrationEnabled();
    float now = WindowApp::m_instance->m_timeSeconds;

    for (int index = 0; index < CHANNEL_MULTIPLE; ++index) {
        bool enabled = sound;
        if (index == CHANNEL_MUSIC) {
            enabled = music;
        }
        if (enabled) {
            m_channels[index].Update();
        } else {
            m_channels[index].Stop();
        }
    }

    for (int index = m_effects.size() - 1; index >= 0; --index) {
        SoundChannel& effect = m_effects[index];
        if (sound) {
            effect.Update();
        } else {
            effect.Stop();
        }
        if (effect.m_media.m_resource == NULL) {
            m_effects.removeElementAt(index);
        }
    }

    if (m_vibrationDuration != 0) {
        if (!vibration) {
            StopVibration();
        } else {
            ICMediaPlayer* player = ICMediaPlayer::GetInstance();
            if (m_vibrationEvent == 0) {
                if (now >= m_vibrationStartTime) {
                    m_vibrationEvent = player->Vibrate(m_vibrationDuration, 0);
                }
            } else if (!player->IsVibrating(m_vibrationEvent)) {
                m_vibrationDuration = 0;
                m_vibrationEvent = 0;
            }
        }
    }
}

void AppMedia::SetActivated(bool activated)
{
    m_activated = activated;
    UpdateMediaPlayerSettings();
}

bool AppMedia::ResumeMusic()
{
    if (m_music.m_resource != NULL && m_activated) {
        AppSettings* settings = static_cast<App*>(WindowApp::m_instance)->m_settings;
        if (settings->IsMusicEnabled()) {
            Play(CHANNEL_MUSIC, m_music, 0.1f, PLAY_LOOP, START_IF_DIFFERENT);
            return true;
        }
    }
    return false;
}

void AppMedia::StopVibration()
{
    if (m_vibrationEvent != 0) {
        ICMediaPlayer::GetInstance()->StopVibrate(m_vibrationEvent);
        m_vibrationStartTime = 0;
        m_vibrationDuration = 0;
        m_vibrationEvent = 0;
    }
}

void AppMedia::Stop(ESoundChannel channel)
{
    if (channel < CHANNEL_MULTIPLE) {
        m_channels[channel].Stop();
        if (channel == CHANNEL_MUSIC) {
            m_music.Release();
        }
        return;
    }
    for (int index = 0; index < m_effects.size(); ++index) {
        m_effects[index].Stop();
    }
    m_effects.setSize(0);
}

void AppMedia::Play(ESoundChannel channel, const MediaRes& media, float delay,
                    ESoundLoopType loop, EStartType start)
{
    AppSettings* settings = static_cast<App*>(WindowApp::m_instance)->m_settings;
    if (channel == CHANNEL_MUSIC) {
        if (!m_activated || !settings->IsMusicEnabled()) {
            m_music = media;
            return;
        }
    } else if (!m_activated || !settings->IsSoundEnabled()) {
        return;
    }

    float when = WindowApp::m_instance->m_timeSeconds + delay;
    if (channel >= CHANNEL_MULTIPLE) {
        for (int index = 0; index < m_effects.size(); ++index) {
            SoundChannel& effect = m_effects[index];
            if (!(effect.m_media == media)) {
                continue;
            }
            if (effect.m_media.m_resource != NULL) {
                if (start == START_REPLACE || when < effect.m_startTime) {
                    effect.Stop();
                }
                if (effect.m_media.m_resource != NULL) {
                    return;
                }
            }
            if (media.m_resource != NULL) {
                effect.m_startTime = when;
                effect.m_media = media;
                effect.m_loop = loop == PLAY_LOOP;
            }
            return;
        }
        if (media.m_resource != NULL) {
            SoundChannel effect(this, media, when);
            m_effects.addElement(effect);
        }
        return;
    }

    SoundChannel& effect = m_channels[channel];
    if (effect.m_media.m_resource != NULL) {
        if (start == START_IF_DIFFERENT) {
            void* previousData = effect.m_media.m_resource->GetData();
            void* data = NULL;
            if (media.m_resource != NULL) {
                data = media.m_resource->GetData();
            }
            if (previousData != data) {
                effect.Stop();
            }
        } else if (start != START_IF_IDLE) {
            effect.Stop();
        }
    }
    if (effect.m_media.m_resource != NULL) {
        if (effect.m_startTime <= when) {
            return;
        }
        effect.Stop();
        if (effect.m_media.m_resource != NULL) {
            return;
        }
    }
    if (media.m_resource != NULL) {
        effect.m_startTime = when;
        effect.m_media = media;
        effect.m_loop = loop == PLAY_LOOP;
        if (channel == CHANNEL_MUSIC) {
            m_music = media;
        }
    }
}
