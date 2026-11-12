#include "CSubContractManager.h"

#include "CXmlHelper.h"

typedef char CKSubContractManagerSize[sizeof(CSubContractManager) == 0x1c ? 1 : -1];

SSubContractInfo::SSubContractInfo(const TiXmlNode& node)
{
    m_appearSeconds = CXmlHelper::GetAttributeValue(&node, "appearTime").ToInt();
    m_availableSeconds = CXmlHelper::GetAttributeValue(&node, "disappearTime").ToInt();
    m_rewardFactor = CXmlHelper::GetAttributeValue(&node, "rewardPercent").ToFloat() * 0.01f;
}

CSubContractManager::CSubContractManager() : m_appearRadius(0), m_disappearNotifyTimeLeft(0)
{
    TiXmlDocument document = CXmlHelper::Parse(L"res/common/configs/sub_contracts.xml");
    const TiXmlElement* contracts = document.FirstChildElement("subcontracts");
    if (contracts != NULL) {
        m_appearRadius = CXmlHelper::GetAttributeValue(contracts, "appearRadius").ToInt();
        m_disappearNotifyTimeLeft = CXmlHelper::GetAttributeValue(contracts, "disappearNotifyTimeLeft").ToInt();
        for (const TiXmlElement* item = contracts->FirstChildElement("subcontract"); item != NULL;
             item = item->NextSiblingElement("subcontract")) {
            m_contracts.addElement(SSubContractInfo(*item));
        }
    }
}

CSubContractManager::~CSubContractManager()
{
}
