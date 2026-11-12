// CMission.h - CK 1.5.2 story mission settings and dialog sequences.
#ifndef CMISSION_H
#define CMISSION_H

#include "Vector_ss.h"
#include "xString.h"

class TiXmlNode;

class CMission {
public:
    // These enums also appear in the pre-existing shared map and analytics interfaces.
    enum EMissionDifficulty { DIFFICULTY_ANY, DIFFICULTY_EASY, DIFFICULTY_MEDIUM, DIFFICULTY_HARD };
    enum EMissionType { MISSION_ANY, MISSION_KA, MISSION_IC, MISSION_CB, MISSION_ST, MISSION_QU, MISSION_EM };

    struct Dialog {
        int m_id;
        const char* m_image;
        XString m_text;
        XString m_title;
        int m_condition;
    };

    explicit CMission(TiXmlNode* node);
    void Parse(TiXmlNode* node);
    void ParseInfo(TiXmlNode* node);
    void ParseReward(TiXmlNode* node);
    void ParseBoss(TiXmlNode* node);
    void ParseFakeBoss(TiXmlNode* node);
    void ParseDialogs(TiXmlNode* node, XString elementName, Vector<Dialog>* dialogs);
    void ParsePermittedWeapon(TiXmlNode* node);
    void ParseEstimatedWeapon(TiXmlNode* node);

    int m_moneyReward;                 // 0x00
    int m_experienceReward;
    int m_weaponGiftId;
    int m_id;
    XString m_screenName;              // 0x10
    XString m_objectiveTitle;
    XString m_objectiveGoal;
    float m_minDifficulty;
    float m_maxDifficulty;
    int m_energyCost;
    int m_civiliansCount;
    int m_alertSeconds;
    int m_escapeSeconds;
    bool m_warTimer;
    bool m_bossFirst;
    bool m_bossWins;
    bool m_tranquillize;
    bool m_bossSpawnsLast;
    bool m_silentKill;
    float m_minWeaponRange;            // 0x3c
    XString m_scriptName;
    XString m_boss;
    XString m_bossName;
    XString m_fakeBoss;
    XString m_fakeBossName;
    Vector<Dialog> m_startDialogs;     // 0x54
    Vector<Dialog> m_priorDialogs;
    Vector<Dialog> m_inMissionDialogs;
    Vector<Dialog> m_finalDialogs;
    Vector<int> m_permittedWeapons;
    int m_estimatedWeaponId;
    int m_estimatedWeaponAmmoCount;
};

#endif
