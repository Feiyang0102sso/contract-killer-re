// CNGSDataIncentive.cpp - values attached to gServe offers and rewards.
#include "CNGSDataIncentive.h"

CDataIncentive::CDataIncentive()
    : m_quantity(0), m_userGameRewardId(0), m_incentiveCount(1), m_offerCategoryId(0)
{
    m_offerCategoryName = "";
}

CDataIncentive::CDataIncentive(const CDataIncentive& other)
{
    copy(other);
}

CDataIncentive& CDataIncentive::operator=(const CDataIncentive& other)
{
    copy(other);
    return *this;
}

void CDataIncentive::copy(const CDataIncentive& other)
{
    m_quantity = other.m_quantity;
    m_userGameRewardId = other.m_userGameRewardId;
    m_unitOfMeasurement = other.m_unitOfMeasurement;
    m_offeredGameName = other.m_offeredGameName;
    m_incentiveCount = other.m_incentiveCount;
    m_offerCategoryId = other.m_offerCategoryId;
    m_offerCategoryName = other.m_offerCategoryName;
}
