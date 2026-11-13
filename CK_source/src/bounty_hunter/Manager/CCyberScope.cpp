#include "CCyberScope.h"

#include "CBH_Player.h"
#include "CXmlHelper.h"

CCyberScope::CCyberScope(const TiXmlElement& config)
    : CItem(0), m_count(0)
{
    CItem::Parse(config);
}

void CCyberScope::setAvailable(bool available)
{
    // the original only acts on true
    if (available) {
        CBH_Player::GetInstance()->AddCyberScopeCount(m_count);
    }
}

void CCyberScope::ParseStats(const TiXmlElement& stats)
{
    CItem::ParseStats(stats);
    m_count = CXmlHelper::GetAttributeValueOrUseDefault(stats, "count", XString("1")).ToInt();
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
