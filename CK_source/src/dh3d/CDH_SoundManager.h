// CDH_SoundManager.h - sounds and music of the game (media player events per resource).
#ifndef CDH_SOUNDMANAGER_H
#define CDH_SOUNDMANAGER_H

#include "CClass.h"
#include "HashTable.h"
#include "Vector_ss.h"

class CMedia;

class CDH_SoundManager : public CClass {
public:
    enum { ClassId = 0x7ba2290e };

    CDH_SoundManager();
    virtual ~CDH_SoundManager();

    void StopAllSoundsAndRelease();
    void ReleaseResources();
    void StopAllSounds();
    void StopAllMusic();
    void StopAll();

    /** Play a named resource or a random sound from the named key set. */
    void PlaySound(const char* name, bool loop, bool exclusive);
    void PlayMusic(const char* name, bool loop, bool exclusive);
    void PlayRandomSound(const char* keySet, bool exclusive);
    void PrecacheSound(const char* name);
    void PrecacheKeyset(const char* keySet);
    bool IsPlaying(const char* name);
    unsigned char IsPaused(const char* name);
    void Stop(const char* name);
    void Pause(const char* name);
    void Resume(const char* name);

private:
    struct SSound {
        const char* m_name;
        unsigned int m_eventId;
        CMedia* m_media;
        bool m_music;
    };

    void ReleaseSoundImpl(SSound* sound);
    void RemoveFromPlayingSounds(const char* name);
    SSound* GetFromPlayingSounds(const char* name, int start);
    SSound* PrecacheAndGetSound(const char* name);
    SSound* CreateSound(const char* name);
    void Play(const char* name, bool loop, bool music, bool exclusive);

    HashTable<HashTableKey<const char*>, SSound*> m_sounds;  // +0x08
    Vector<SSound*> m_playing;                              // +0x14
};

#endif
