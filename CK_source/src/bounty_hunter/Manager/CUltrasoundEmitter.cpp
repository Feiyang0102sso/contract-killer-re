#include "CUltrasoundEmitter.h"

#include "CBH_Player.h"
#include "CXmlHelper.h"

CUltrasoundEmitter::CUltrasoundEmitter(const TiXmlElement& config)
    : CItem(0), m_count(0)
{
    init(config);
}

void CUltrasoundEmitter::setAvailable(bool)
{
    CBH_Player::GetInstance()->AddUltrasoundEmitterAttempt(m_count);
}

void CUltrasoundEmitter::ParseStats(const TiXmlElement& stats)
{
    CItem::ParseStats(stats);
    m_count = CXmlHelper::GetAttributeValue(stats, "emitter_ccount").ToInt();
}

XString CUltrasoundEmitter::getUIDString() const
{
    return XString::Format(L"misc_%d", m_id);
}

int CUltrasoundEmitter::getCurrentAmount()
{
    return CBH_Player::GetInstance()->m_ultrasoundAttemptCount;
}

typedef char AssertCUltrasoundEmitterSize[sizeof(CUltrasoundEmitter) == 0x60 ? 1 : -1];
