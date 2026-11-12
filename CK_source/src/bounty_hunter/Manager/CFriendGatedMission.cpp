// CFriendGatedMission.cpp - a mission unlocked by the number of friends.
#include "CFriendGatedMission.h"
#include "CXmlHelper.h"

CFriendGatedMission::CFriendGatedMission(TiXmlNode* node)
{
    m_friendCount = 0;
    m_bountyType = 0;
    m_rewardMoney = 0;
    m_rewardXp = 0;
    ParseNode(node);
}

void CFriendGatedMission::ParseNode(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    ParseInfo(node->FirstChild("info"));
}

void CFriendGatedMission::ParseInfo(TiXmlNode* node)
{
    if (node == NULL) {
        return;
    }
    m_friendCount = CXmlHelper::GetAttributeValue(node, "friend_count").ToInt();
    m_bountyType = CXmlHelper::GetAttributeValue(node, "bounty_type").ToInt();
    m_rewardMoney = CXmlHelper::GetAttributeValue(node, "reward_money").ToInt();
    m_rewardXp = CXmlHelper::GetAttributeValue(node, "reward_xp").ToInt();
}
