#ifndef CSUBCONTRACTMANAGER_H
#define CSUBCONTRACTMANAGER_H

#include "Vector_ss.h"

class TiXmlNode;

struct SSubContractInfo {
    explicit SSubContractInfo(const TiXmlNode& node);

    int m_appearSeconds;
    int m_availableSeconds;
    float m_rewardFactor;
};

class CSubContractManager {
public:
    CSubContractManager();
    virtual ~CSubContractManager();

    Vector<SSubContractInfo> m_contracts;       // 0x04
    int m_appearRadius;                         // 0x14
    int m_disappearNotifyTimeLeft;
};

#endif
