// CProfileDataDescriptor.cpp - A/B copy selector of a content file collection (b025x).
#include "CProfileDataDescriptor.h"
#include "CNGSAttribute.h"
#include "CHash.h"
#include "CStringToKey.h"
#include "ICStdUtil.h"

CProfileDataDescriptor::CProfileDataDescriptor()
    : CSaveRestoreInterface(-1, 1, BACKING_STORE_ATTRIBUTES)
    , m_ab('A')
{
}

unsigned char CProfileDataDescriptor::compareData(const CSaveRestoreInterface* other)
{
    return m_ab == static_cast<const CProfileDataDescriptor*>(other)->m_ab;
}

unsigned char CProfileDataDescriptor::copyData(const CSaveRestoreInterface* other)
{
    m_ab = static_cast<const CProfileDataDescriptor*>(other)->m_ab;
    return 1;
}

void CProfileDataDescriptor::resetData()
{
    m_ab = 'A';
}

void CProfileDataDescriptor::getData(TCVector<CNGSAttribute*>& attributes)
{
    CStrWChar name = getwTokenNameFor("ABFLAG");
    attributes.Add(new CNGSAttribute(name, m_ab, m_collectionType));
}

void CProfileDataDescriptor::setData(const CHash* attributes)
{
    resetData();
    void* found = NULL;
    CStrChar name = getTokenNameFor("ABFLAG");
    if (attributes->Find(CStringToKey(name.GetCharPtr(), 0), &found)) {
        m_ab = static_cast<CNGSAttribute*>(found)->getVal_int32();
    } else {
        dataNotFound();
    }
}

void CProfileDataDescriptor::debugPrint()
{
    debugPrintBase();
}

CStrWChar CProfileDataDescriptor::asString()
{
    char text[128] = {};
    ICStdUtil::SPrintF(text, "abFlag=%c", m_ab);
    return CStrWChar(text);
}

CStrChar CProfileDataDescriptor::getBaseName()
{
    CSaveRestoreInterface* file = getFile();
    if (file != NULL) {
        return file->getBaseName() + "DD";
    }
    return CStrChar("DATADESCRIPTOR");
}

void CProfileDataDescriptor::setCollectionType(int collectionType)
{
    m_collectionType = collectionType;
}

int CProfileDataDescriptor::generateCollectionType(int collectionType)
{
    return 1999 - collectionType;
}

int CProfileDataDescriptor::getAB()
{
    return m_ab;
}

void CProfileDataDescriptor::switchAB()
{
    // ?: in this order reproduces the original select ('B' preset, 'A' when not 'A')
    m_ab = (m_ab != 'A') ? 'A' : 'B';
}

int CProfileDataDescriptor::targetedCollectionType(int collectionType)
{
    return 1999 - collectionType;
}
