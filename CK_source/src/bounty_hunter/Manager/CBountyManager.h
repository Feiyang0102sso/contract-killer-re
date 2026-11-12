#ifndef CBOUNTYMANAGER_H
#define CBOUNTYMANAGER_H

#include "Vector_ss.h"
#include "xString.h"

class TiXmlNode;

enum EBoutnyMessageType { BountyMessageDefault, BountyMessageWeaponGated, BountyMessageMoving, BountyMessageRandomWeapon };

class CBountyManager {
public:
    struct SMessage {
        // By-value parameters: callers copy each string into a parameter slot first (original 0x10c6d8).
        SMessage(XString goal, XString title, XString text)
            : m_goal(goal), m_title(title), m_text(text), m_resourceIds(false)
        {
        }

        XString m_goal;
        XString m_title;
        XString m_text;
        bool m_resourceIds;
    };

    struct HauntInfo {
        XString m_name;
        Vector<SMessage> m_messages;
    };

    struct SBountyGeneralInfo {
        int m_id;
        bool m_usedForAmbush;
        XString m_name;
        float m_difficultyMin;
        float m_difficultyMax;
        int m_money;
        int m_xp;
        int m_moneyPenalty;
        int m_xpPenalty;
        int m_moneyRefusal;
        int m_xpRefusal;
        Vector<SMessage> m_messages;
        Vector<SMessage> m_weaponGatedMessages;
        Vector<SMessage> m_movingMessages;
        Vector<SMessage> m_randomWeaponMessages;
        Vector<HauntInfo> m_haunts;
        Vector<XString> m_scripts;
        int m_energy;
        int m_alertSeconds;
        int m_escapeSeconds;
        bool m_bossFirst;
        bool m_bossWins;
        bool m_tranquillize;
        bool m_bossSpawnsLast;
        bool m_silentKill;
        float m_noScriptProbability;
        float m_minWeaponRange;
    };

    CBountyManager();
    ~CBountyManager();

    SBountyGeneralInfo* GetInfo(int id);
    SMessage* GetRandomMessageWithSeed(int id, const XString& haunt, unsigned int seed, EBoutnyMessageType type);
    // Static in the original: the caller passes the info in r1 and no manager (CGPSBounty 0x12733c).
    static XString GetRandomScriptWithSeed(const SBountyGeneralInfo& info, int id, const XString& location,
                                           int camera, int playerLevel, unsigned int seed);
    int GetRandomAmbushTypeId();
    SMessage* GetRandomTargettingMessage();

private:
    void ParseBountyConfig();
    void ParseTargettingConfig();
    void ParseBounty(TiXmlNode* node);
    void ParseInfo(TiXmlNode* node, SBountyGeneralInfo& info);
    void ParseReward(TiXmlNode* node, SBountyGeneralInfo& info);
    void ParsePunishment(TiXmlNode* node, SBountyGeneralInfo& info);
    void ParseRefuse(TiXmlNode* node, SBountyGeneralInfo& info);
    void ParsePossibleScripts(TiXmlNode* node, SBountyGeneralInfo& info);
    void ParseHaunts(TiXmlNode* node, SBountyGeneralInfo& info);
    void ParseMessagesImpl(TiXmlNode* node, const char* name, Vector<SMessage>& messages);

    Vector<SBountyGeneralInfo> m_types;
    Vector<SMessage*> m_targetingMessages;
};

#endif
