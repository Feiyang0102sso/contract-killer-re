#ifndef CACHIEVEMENTMANAGER_H
#define CACHIEVEMENTMANAGER_H

#include "HashTable.h"
#include "Vector_ss.h"
#include "xString.h"

class TiXmlNode;
class TiXmlElement;

class CAchievement {
public:
    // Inline in the original header (weak C2 at 0x1098b8); m_value and m_hidden stay uninitialized.
    CAchievement()
    {
        m_progress = 0;
        m_steps = 1;
        m_synced = true;
    }

    XString m_name;
    int m_value;
    int m_progress;
    int m_steps;
    XString m_imageId;
    XString m_screenName;
    XString m_description;
    XString m_completedDescription;
    bool m_hidden;
    bool m_synced;
};

class CAchievementManager {
public:
    CAchievementManager();
    ~CAchievementManager();

    void ParseAchievements(TiXmlNode* node);
    void Save(TiXmlElement* root);
    void Load(TiXmlElement* root);
    void SetAchievementProgress(XString name, int progress, bool synced);
    void CheckAllAchievements();
    void CheckAchievement(XString name, int value);
    // Inline bool overload: the original copies the name into a second by-value slot (0x1086bc).
    void CheckAchievement(XString name, bool value)
    {
        CheckAchievement(name, (int)value);
    }
    bool GiveAchievement(XString name, int progress);
    void Update();

    unsigned int m_lastCheckTime;
    unsigned int m_lastPostingTime;
    int m_weaponAvailabilityStamp;  // 0x08 last CDH_Weapon::g_availabilityChangesCount seen
    HashTable<XString, CAchievement*> m_achievements;  // 0x0c
    Vector<XString> m_names;                          // 0x18
};

#endif
