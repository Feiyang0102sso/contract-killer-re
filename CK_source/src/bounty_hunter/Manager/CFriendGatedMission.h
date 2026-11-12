// CFriendGatedMission.h - one mission unlocked by the number of friends (friends_gated_missions.xml).
// Layout (0x10) from CFriendGatedMission::CFriendGatedMission and ParseInfo.
#ifndef CFRIENDGATEDMISSION_H
#define CFRIENDGATEDMISSION_H

class TiXmlNode;

class CFriendGatedMission {
public:
    CFriendGatedMission(TiXmlNode* node);

    /** Parse the <info> child of a <mission> node. */
    void ParseNode(TiXmlNode* node);
    /** Read the attributes of an <info> node. */
    void ParseInfo(TiXmlNode* node);

    int m_friendCount;      // 0x00 friends needed to unlock
    int m_bountyType;       // 0x04
    int m_rewardMoney;      // 0x08
    int m_rewardXp;         // 0x0c
};

#endif
