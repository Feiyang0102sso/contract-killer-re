#include "CUltrasoundEmitter.h"

#include "CBH_Player.h"
#include "CXmlHelper.h"

CUltrasoundEmitter::CUltrasoundEmitter(const TiXmlElement& config)
    : CItem(0)
{
    // the original leaves m_count for ParseStats() and calls Parse directly
    Parse(config);
}

void CUltrasoundEmitter::setAvailable(bool available)
{
    // the original only acts on true
    if (available) {
        CBH_Player::GetInstance()->AddUltrasoundEmitterAttempt(m_count);
    }
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

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertCUltrasoundEmitterSize[sizeof(CUltrasoundEmitter) == 0x60 ? 1 : -1];
#endif
