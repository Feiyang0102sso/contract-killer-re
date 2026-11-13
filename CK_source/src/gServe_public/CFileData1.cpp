#include "CFileData1.h"

#include <string.h>
#include "CHash.h"
#include "CNGSAttribute.h"
#include "CStringToKey.h"
#include "ICStdUtil.h"

typedef char FileDataSizeCheck[sizeof(CFileData1) == 0x28 ? 1 : -1];

CFileData1::CFileData1()
    : CSaveRestoreInterface(1002, 1, BACKING_STORE_CONTENT_FILE), m_data(NULL), m_size(0)
{
}

CFileData1::~CFileData1()
{
    delete[] m_data;
}

void CFileData1::setData(const unsigned char* data, int size)
{
    m_size = size;
    if (m_data == data) {
        return;
    }
    if (m_data != NULL) {
        delete[] m_data;
        m_data = NULL;
    }
    m_data = new unsigned char[m_size];
    memcpy(m_data, data, m_size);
}

unsigned char CFileData1::compareData(const CSaveRestoreInterface* other)
{
    return m_size == static_cast<const CFileData1*>(other)->m_size;
}

unsigned char CFileData1::copyData(const CSaveRestoreInterface* other)
{
    const CFileData1* file = static_cast<const CFileData1*>(other);
    if (file != this) {
        setData(file->m_data, file->m_size);
    }
    return 1;
}

void CFileData1::resetData()
{
    if (m_data != NULL) {
        delete[] m_data;
        m_data = NULL;
    }
    m_size = 0;
}

void CFileData1::getData(TCVector<CNGSAttribute*>& attributes)
{
    CStrWChar name = getwTokenNameFor("DATA");
    attributes.Add(new CNGSAttribute(name, m_data, m_size, m_collectionType));
}

void CFileData1::setData(const CHash* attributes)
{
    resetData();
    void* attribute = NULL;
    CStrChar name = getTokenNameFor("DATA");
    unsigned int key = CStringToKey(name.GetCharPtr(), 0);
    int found = attributes->Find(key, &attribute);
    if (found != 0) {
        m_data = static_cast<CNGSAttribute*>(attribute)->getVal_base64(m_size);
    } else {
        dataNotFound();
    }
}

void CFileData1::debugPrint()
{
    debugPrintBase();
}

CStrWChar CFileData1::asString()
{
    char text[128] = { 0 };
    ICStdUtil::SPrintF(text, "size=%i", m_size);
    return CStrWChar(text);
}
