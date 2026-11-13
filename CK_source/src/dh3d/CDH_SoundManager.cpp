// CDH_SoundManager.cpp - CK sound resources and active playback events.
#include "CDH_SoundManager.h"
#include "App.h"
#include "AppSettings.h"
#include "CDH_ResourceManager.h"
#include "CRandGen.h"
#include "CResource.h"
#include "ICMediaPlayer.h"
#include "CResourceManager.h"

CDH_SoundManager::CDH_SoundManager()
    : CClass(0x0275a643), m_sounds(8)
{
    m_classId = ClassId;
}

CDH_SoundManager::~CDH_SoundManager()
{
    ReleaseResources();
}

void CDH_SoundManager::StopAllSoundsAndRelease()
{
    ReleaseResources();
}

void CDH_SoundManager::ReleaseResources()
{
    StopAllSounds();
    StopAllMusic();
    for (int index = 0; index < m_sounds.m_count; ++index) {
        ReleaseSoundImpl(*m_sounds.GetAt(index, NULL));
    }
    m_sounds.DeleteContents();
}

void CDH_SoundManager::StopAllSounds()
{
    Vector<SSound*> playing(m_playing);
    for (int index = 0; index < playing.size(); ++index) {
        SSound* sound = playing[index];
        if (!sound->m_music) {
            ICMediaPlayer::GetInstance()->Stop(sound->m_eventId);
            int position = m_playing.indexOf(sound);
            if (position >= 0) {
                m_playing.removeElementAt(position);
            }
        }
    }
}

void CDH_SoundManager::StopAllMusic()
{
    Vector<SSound*> playing(m_playing);
    for (int index = 0; index < playing.size(); ++index) {
        SSound* sound = playing[index];
        if (sound->m_music) {
            ICMediaPlayer::GetInstance()->Stop(sound->m_eventId);
            int position = m_playing.indexOf(sound);
            if (position >= 0) {
                m_playing.removeElementAt(position);
            }
        }
    }
}

void CDH_SoundManager::ReleaseSoundImpl(SSound* sound)
{
    if (sound != NULL) {
        if (sound->m_name != NULL) {
            RemoveFromPlayingSounds(sound->m_name);
            CApplet::m_pApp->m_resourceManager->ReleaseResource(sound->m_name);
            sound->m_name = NULL;
        }
        sound->m_media = NULL;
    }
}

void CDH_SoundManager::RemoveFromPlayingSounds(const char* name)
{
    for (int index = 0; index < m_playing.size(); ++index) {
        if (strcmp(m_playing[index]->m_name, name) == 0) {
            m_playing.removeElementAt(index);
            return;
        }
    }
}

unsigned char CDH_SoundManager::IsPaused(const char* name)
{
    SSound* sound = GetFromPlayingSounds(name, 0);
    if (sound != NULL) {
        return ICMediaPlayer::GetInstance()->IsPaused(sound->m_eventId);
    }
    return false;
}

CDH_SoundManager::SSound* CDH_SoundManager::GetFromPlayingSounds(const char* name, int)
{
    for (int index = 0; index < m_playing.size(); ++index) {
        if (strcmp(m_playing[index]->m_name, name) == 0) {
            return m_playing[index];
        }
    }
    return NULL;
}

void CDH_SoundManager::PlayRandomSound(const char* keySet, bool exclusive)
{
    CDH_KeySet* keys = static_cast<CDH_KeySet*>(
        static_cast<App*>(WindowApp::m_instance)->m_resourceManager->getResourceData(keySet, false));
    CRandGen* random = CRandGen::GetInstance();
    int index = random->GetRandRange(0, keys->m_count - 1);
    Play(keys->GetKey(index), false, false, exclusive);
#ifndef CK_PC
    // PORT: the sound cache keeps pointers into the key set's strings; on Windows the released key
    // set is freed at once and the next lookup reads freed memory, so the PC build keeps it loaded.
    static_cast<App*>(WindowApp::m_instance)->m_resourceManager->releaseResource(keySet);
#endif
}

void CDH_SoundManager::PlaySound(const char* name, bool loop, bool exclusive)
{
    Play(name, loop, false, exclusive);
}

void CDH_SoundManager::Stop(const char* name)
{
    SSound* sound = GetFromPlayingSounds(name, 0);
    if (sound != NULL) {
        ICMediaPlayer::GetInstance()->Stop(sound->m_eventId);
        RemoveFromPlayingSounds(name);
    }
}

void CDH_SoundManager::PrecacheSound(const char* name)
{
    PrecacheAndGetSound(name);
}

CDH_SoundManager::SSound* CDH_SoundManager::PrecacheAndGetSound(const char* name)
{
    // A keyset owns its strings; the original cache uses the caller's pointer as the key.
    SSound** cached = m_sounds.Get(HashTableKey<const char*>(name));
    SSound* sound;
    if (cached == NULL) {
        sound = CreateSound(name);
        cached = &sound;
        m_sounds.Add(HashTableKey<const char*>(name), *cached);
    }
    return *cached;
}

void CDH_SoundManager::PrecacheKeyset(const char* keySet)
{
    CDH_KeySet* keys = static_cast<CDH_KeySet*>(
        static_cast<App*>(WindowApp::m_instance)->m_resourceManager->getResourceData(keySet, false));
    if (keys != NULL) {
        int count = keys->m_count;
        for (int index = 0; index < count; ++index) {
            PrecacheAndGetSound(keys->GetKey(index));
        }
#ifndef CK_PC
        // PORT: kept loaded on PC, see PlayRandomSound.
        static_cast<App*>(WindowApp::m_instance)->m_resourceManager->releaseResource(keySet);
#endif
    }
}

CDH_SoundManager::SSound* CDH_SoundManager::CreateSound(const char* name)
{
    SSound* sound = new SSound;
    sound->m_name = NULL;
    CResource* resource = NULL;
    CApplet::m_pApp->m_resourceManager->CreateResource(name, &resource);
    CMedia* media = static_cast<CMedia*>(resource->GetData());
    sound->m_name = name;
    sound->m_eventId = 0;
    sound->m_media = media;
    return sound;
}

void CDH_SoundManager::Play(const char* name, bool loop, bool music, bool exclusive)
{
    ICMediaPlayer* player = ICMediaPlayer::GetInstance();
    if (!player->GetSoundEnabled()) {
        return;
    }
    AppSettings* settings = static_cast<App*>(WindowApp::m_instance)->m_settings;
    if (music) {
        if (!settings->IsMusicEnabled()) {
            return;
        }
    } else if (!settings->IsSoundEnabled()) {
        return;
    }
    if (exclusive && IsPlaying(name)) {
        return;
    }
    SSound* sound = PrecacheAndGetSound(name);
    sound->m_music = music;
    sound->m_eventId = ICMediaPlayer::GetInstance()->Play(sound->m_media, loop, 0);
    if (m_playing.indexOf(sound) == -1) {
        m_playing.addElement(sound);
    }
}

bool CDH_SoundManager::IsPlaying(const char* name)
{
    SSound* sound = GetFromPlayingSounds(name, 0);
    if (sound != NULL) {
        if (ICMediaPlayer::GetInstance()->IsPlaying(sound->m_eventId)) {
            return true;
        }
        RemoveFromPlayingSounds(name);
    }
    return false;
}

void CDH_SoundManager::PlayMusic(const char* name, bool loop, bool exclusive)
{
    Play(name, loop, true, exclusive);
}

void CDH_SoundManager::Pause(const char* name)
{
    SSound* sound = GetFromPlayingSounds(name, 0);
    if (sound != NULL) {
        ICMediaPlayer::GetInstance()->Pause(sound->m_eventId);
    }
}

void CDH_SoundManager::Resume(const char* name)
{
    SSound* sound = GetFromPlayingSounds(name, 0);
    if (sound != NULL) {
        ICMediaPlayer::GetInstance()->Resume(sound->m_eventId);
    }
}

void CDH_SoundManager::StopAll()
{
    StopAllSounds();
    StopAllMusic();
}
