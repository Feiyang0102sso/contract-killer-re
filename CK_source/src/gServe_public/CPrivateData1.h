// CPrivateData1.h - wallet, experience and user settings in the private profile collection.
#ifndef CPRIVATEDATA1_H
#define CPRIVATEDATA1_H

#include "CSaveRestoreInterface.h"

enum ContentManagerConflict {
    CONTENT_FILE_CONFLICT_NONE = 0,
    CONTENT_FILE_CONFLICT_RESOLVED = 1
};

class CPrivateData1 : public CSaveRestoreInterface {
public:
    CPrivateData1();
    virtual void resetData();
    virtual unsigned char compareData(const CSaveRestoreInterface* other);
    virtual unsigned char copyData(const CSaveRestoreInterface* other);
    virtual void getData(TCVector<CNGSAttribute*>& attributes);
    virtual void setData(const CHash* attributes);
    virtual CStrWChar asString();
    virtual void debugPrint();

    unsigned long long getSoftCurrency();
    unsigned long long getHardCurrency();
    unsigned long long getExperience();
    void setSoftCurrency(unsigned long long amount);
    void setHardCurrency(unsigned long long amount);
    void setExperience(unsigned long long experience);
    void setContentFileConflictStatus(ContentManagerConflict status);
    unsigned char getSoundSetting();
    unsigned char getMusicSetting();
    unsigned char getFireBtnSetting();
    unsigned char getNotificationSetting();
    void setSoundSetting(unsigned char enabled);
    void setMusicSetting(unsigned char enabled);
    void setFireBtnSetting(unsigned char enabled);
    void setNotificationSetting(unsigned char enabled);

    unsigned char m_hasProgress;       // +0x20 local marker
    unsigned long long m_softCurrency; // +0x24
    unsigned long long m_hardCurrency; // +0x2c
    unsigned long long m_experience;   // +0x34
    int m_conflictStatus;               // +0x3c
    unsigned char m_soundSetting;      // +0x40
    unsigned char m_musicSetting;      // +0x41
    unsigned char m_fireBtnSetting;    // +0x42
    unsigned char m_notificationSetting; // +0x43
};

#endif
