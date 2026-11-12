// COverrideManager.cpp - refresh CK item overrides from the cache and gServe.
#include "COverrideManager.h"

typedef char COverrideManagerSizeCheck[sizeof(COverrideManager) == 0x8c ? 1 : -1];

COverrideManager::COverrideManager()
    : m_changeMark(0)
{
    m_overrideName = "override";
    m_overrideFileName = L"override.json";
    m_saveFileName = L"../Library/Caches/store_override.dat";
}

void COverrideManager::checkUpdates()
{
    bool forceDownload = false;
    if (m_root == NULL) {
        if (LoadFromDisk(m_saveFileName) == 0) {
            ++m_changeMark;
        } else {
            forceDownload = true;
        }
    }
    if (!IsBusy()) {
        LoadFromServer(forceDownload);
    }
}

bool COverrideManager::LoadFromServer(bool forceDownload)
{
    CNGSJSONDataRequestFunctor::Callback callback =
        static_cast<CNGSJSONDataRequestFunctor::Callback>(&COverrideManager::HandleResponse);
    SetRequestCallback(new CNGSJSONDataRequestFunctor(this, callback));
    return CNGSJSONData::LoadFromServer(forceDownload) != 0;
}

void COverrideManager::HandleResponse(char* data, int length, bool notModified)
{
    CObjectMapValue* previous = m_root;
    HandleValidateCNGSJSONDataResponse(data, length, notModified);
    if (m_root != previous && m_root != NULL) {
        ++m_changeMark;
    }
}

int COverrideManager::changesMark() const
{
    return m_changeMark;
}
