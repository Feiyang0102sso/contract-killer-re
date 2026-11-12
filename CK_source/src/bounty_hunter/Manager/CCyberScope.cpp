#include "CCyberScope.h"

#include "CBH_Player.h"
#include "CXmlHelper.h"

CCyberScope::CCyberScope(const TiXmlElement& config)
    : CItem(0), m_count(0)
{
    init(config);
}

void CCyberScope::setAvailable(bool)
{
    CBH_Player::GetInstance()->AddCyberScopeCount(m_count);
}

void CCyberScope::ParseStats(const TiXmlElement& stats)
{
    CItem::ParseStats(stats);
    m_count = CXmlHelper::GetAttributeValue(stats, "count").ToInt();
}

XString CCyberScope::getUIDString() const
{
    return XString::Format(L"misc_%d", m_id);
}

int CCyberScope::getCurrentAmount()
{
    return CBH_Player::GetInstance()->m_cyberScopesCount;
}

typedef char AssertCCyberScopeSize[sizeof(CCyberScope) == 0x60 ? 1 : -1];
