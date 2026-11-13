// CK ARMv7 resource bank and time-sliced preload queue.
#include "CResBank.h"
#include "App.h"
#include "CDH_ResourceManager.h"
#include "CStdUtil_Cocoa.h"

extern "C" unsigned int __udivsi3(unsigned int dividend, unsigned int divisor);

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
        void* ownedData = entry.m_ownedData;
        if (ownedData != NULL) {
            delete entry.m_object;
            free(ownedData);
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
    if (m_state == Loading) {
        // The original uses the ARM runtime helper even for an empty keyset.
        return __udivsi3(m_loadedCount * 100, m_totalCount);
    }
    return 100;
}

void CResBank::HandleUpdate(int)
{
    if (m_keysets.size() < 1) {
        return;
    }
    unsigned int start = CStdUtil_Cocoa::GetUpTimeMS();
    // The original keeps this limit until the next frame, even if the keyset changes.
    unsigned int keyCount = m_keysets[0].m_object->m_count;
    while (m_state == Loading && CStdUtil_Cocoa::GetUpTimeMS() - start < 100) {
        if (loadNextResource() == true) {
            if (m_keyIndex >= keyCount) {
                startNextKeyset();
                if (m_keysets.size() < 1) {
                    m_state = Loaded;
                    return;
                }
            }
        } else {
            m_state = Failed;
            return;
        }
    }
}

bool CResBank::loadNextResource()
{
    CKeysetResource* keyset = m_keysets[0].m_object;
    CDH_ResourceManager* resources = ((App*)WindowApp::m_instance)->m_resourceManager;
    const char* name = keyset->GetKey(m_keyIndex++);
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
    TKeyset entry;
    entry.m_resourceName = name;
    entry.m_ownedData = NULL;
    CDH_ResourceManager* resources = ((App*)WindowApp::m_instance)->m_resourceManager;
    entry.m_object = (CKeysetResource*)resources->getResourceData(name, false);
    m_keysets.addElement(entry);
    m_totalCount += entry.m_object->m_count;
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
