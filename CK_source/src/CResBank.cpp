// CK ARMv7 resource bank and time-sliced preload queue.
#include "CResBank.h"
#include "App.h"
#include "CDH_ResourceManager.h"
#include "CStdUtil_Cocoa.h"

CResBank::CResBank()
{
    m_keyIndex = 0;
    m_loadedCount = 0;
    m_totalCount = 0;
    m_state = Idle;
}

CResBank::~CResBank()
{
    removeAllKeysets();
}

void CResBank::removeAllKeysets()
{
    for (int index = 0; index < m_keysets.size(); index++) {
        const TKeyset& entry = m_keysets[index];
        if (entry.m_ownedData != NULL) {
            delete entry.m_object;
            free(entry.m_ownedData);
        } else if (entry.m_object != NULL) {
            ((App*)WindowApp::m_instance)->m_resourceManager->releaseResource(entry.m_resourceName);
        }
    }
    m_keysets.removeAllElements();
    m_keyIndex = 0;
    m_loadedCount = 0;
    m_totalCount = 0;
    m_state = Idle;
}

int CResBank::GetState()
{
    return m_state;
}

unsigned int CResBank::GetProgressPercent()
{
    if (m_state == Loading && m_totalCount != 0) {
        return m_loadedCount * 100 / m_totalCount;
    }
    return 100;
}

void CResBank::HandleUpdate(int elapsed)
{
    unsigned int start = CStdUtil_Cocoa::GetUpTimeMS();
    while (m_state == Loading && CStdUtil_Cocoa::GetUpTimeMS() - start < 100) {
        if (m_keysets.size() == 0) {
            m_state = Loaded;
            return;
        }
        if (m_keyIndex >= m_keysets[0].m_object->m_count) {
            startNextKeyset();
        } else if (!loadNextResource()) {
            m_state = Failed;
        }
    }
}

bool CResBank::loadNextResource()
{
    const char* name = m_keysets[0].m_object->GetKey(m_keyIndex++);
    if (name == NULL) {
        return false;
    }
    CDH_ResourceManager* resources = ((App*)WindowApp::m_instance)->m_resourceManager;
    if (resources->getResourceData(name, false) == NULL) {
        return false;
    }
    m_loadedCount++;
    return true;
}

void CResBank::startNextKeyset()
{
    if (m_keysets[0].m_object != NULL) {
        ((App*)WindowApp::m_instance)->m_resourceManager->releaseResource(m_keysets[0].m_resourceName);
    }
    m_keysets.removeElementAt(0);
    m_keyIndex = 0;
}

void CResBank::addKeyset(const char* name)
{
    CDH_ResourceManager* resources = ((App*)WindowApp::m_instance)->m_resourceManager;
    CKeysetResource* keyset = (CKeysetResource*)resources->getResourceData(name, false);
    if (keyset == NULL) {
        m_state = Failed;
        return;
    }
    TKeyset entry = { name, keyset, NULL };
    m_keysets.addElement(entry);
    m_totalCount += keyset->m_count;
    if (m_state == Idle) {
        m_state = Loading;
    }
}

void CResBank::load(const char* name)
{
    removeAllKeysets();
    addKeyset(name);
}

typedef char AssertResBankSize[sizeof(CResBank) == 0x24 ? 1 : -1];
