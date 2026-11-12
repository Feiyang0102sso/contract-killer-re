// COverrideManager.h - store overrides and a version mark for cached item data.
#ifndef COVERRIDEMANAGER_H
#define COVERRIDEMANAGER_H

#include "CNGSJSONData.h"

class COverrideManager : public CNGSJSONData {
public:
    COverrideManager();

    virtual void checkUpdates();
    virtual void HandleResponse(char* data, int length, bool notModified);
    bool LoadFromServer(bool forceDownload);
    int changesMark() const;

private:
    int m_changeMark;
};

#endif
