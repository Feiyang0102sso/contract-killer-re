#include "CSubContractManager.h"

#include "App.h"
#include "CXmlHelper.h"
#include "ResMap.h"

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char CKSubContractManagerSize[sizeof(CSubContractManager) == 0x1c ? 1 : -1];
#endif

SSubContractInfo::SSubContractInfo(const TiXmlNode& node)
{
    m_appearSeconds = CXmlHelper::GetAttributeValue(&node, "appearTime").ToInt();
    m_availableSeconds = CXmlHelper::GetAttributeValue(&node, "disappearTime").ToInt();
    m_rewardFactor = CXmlHelper::GetAttributeValue(&node, "rewardPercent").ToFloat() * 0.01f;
}

CSubContractManager::CSubContractManager()
{
    ResMap* resources = static_cast<App*>(WindowApp::m_instance)->InitResMap();
    char* text = resources->loadText(XString(L"res/common/configs/sub_contracts.xml"));
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    const TiXmlNode* contracts = document.FirstChild("subcontracts");
    m_appearRadius = CXmlHelper::GetAttributeValue(contracts, "appearRadius").ToInt();
    m_disappearNotifyTimeLeft = CXmlHelper::GetAttributeValue(contracts, "disappearNotifyTimeLeft").ToInt();
    for (const TiXmlNode* item = contracts->FirstChild("subcontract"); item != NULL;
         item = item->NextSibling("subcontract")) {
        m_contracts.addElement(SSubContractInfo(*item));
    }
}

CSubContractManager::~CSubContractManager()
{
}
