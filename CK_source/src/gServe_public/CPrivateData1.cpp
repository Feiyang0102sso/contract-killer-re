#include "CPrivateData1.h"

#include "CHash.h"
#include "CNGSAttribute.h"
#include "CStringToKey.h"
#include "ICStdUtil.h"

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char PrivateDataSizeCheck[sizeof(CPrivateData1) == 0x44 ? 1 : -1];
#endif

static const char* const TOKENS[] = {
    "CURRENCY", "HARD_CURRENCY", "EXPERIENCE", "CONTENT_CONFLICT_STATUS",
    "SOUND", "MUSIC", "FIREBTN", "NOTIFICATION"
};

CPrivateData1::CPrivateData1()
    : CSaveRestoreInterface(1001, 4, BACKING_STORE_ATTRIBUTES)
{
    // the original constructor does not set m_hasProgress
    resetData();
}

unsigned char CPrivateData1::compareData(const CSaveRestoreInterface* other)
{
    const CPrivateData1* data = static_cast<const CPrivateData1*>(other);
    return m_softCurrency == data->m_softCurrency && m_hardCurrency == data->m_hardCurrency &&
           m_experience == data->m_experience && m_musicSetting == data->m_musicSetting &&
           m_soundSetting == data->m_soundSetting && m_fireBtnSetting == data->m_fireBtnSetting &&
           m_notificationSetting == data->m_notificationSetting;
}

unsigned char CPrivateData1::copyData(const CSaveRestoreInterface* other)
{
    const CPrivateData1* data = static_cast<const CPrivateData1*>(other);
    m_softCurrency = data->m_softCurrency;
    m_hardCurrency = data->m_hardCurrency;
    m_experience = data->m_experience;
    m_conflictStatus = data->m_conflictStatus;
    m_musicSetting = data->m_musicSetting;
    m_soundSetting = data->m_soundSetting;
    m_fireBtnSetting = data->m_fireBtnSetting;
    m_notificationSetting = data->m_notificationSetting;
    return 1;
}

void CPrivateData1::resetData()
{
    m_softCurrency = 0;
    m_hardCurrency = 0;
    m_experience = 0;
    m_conflictStatus = CONTENT_FILE_CONFLICT_NONE;
    m_soundSetting = 1;
    m_musicSetting = 1;
    m_fireBtnSetting = 1;
    m_notificationSetting = 1;
}

void CPrivateData1::getData(TCVector<CNGSAttribute*>& attributes)
{
    CStrWChar softCurrency = getwTokenNameFor(TOKENS[0]);
    CStrWChar hardCurrency = getwTokenNameFor(TOKENS[1]);
    CStrWChar experience = getwTokenNameFor(TOKENS[2]);
    CStrWChar conflictStatus = getwTokenNameFor(TOKENS[3]);
    CStrWChar sound = getwTokenNameFor(TOKENS[4]);
    CStrWChar music = getwTokenNameFor(TOKENS[5]);
    CStrWChar fireButton = getwTokenNameFor(TOKENS[6]);
    CStrWChar notification = getwTokenNameFor(TOKENS[7]);

    CNGSAttribute* softCurrencyAttribute = new CNGSAttribute(softCurrency, m_softCurrency, m_collectionType);
    CNGSAttribute* hardCurrencyAttribute = new CNGSAttribute(hardCurrency, m_hardCurrency, m_collectionType);
    CNGSAttribute* experienceAttribute = new CNGSAttribute(experience, m_experience, m_collectionType);
    CNGSAttribute* conflictAttribute = new CNGSAttribute(conflictStatus, m_conflictStatus, m_collectionType);
    CNGSAttribute* soundAttribute = new CNGSAttribute(sound, m_soundSetting, m_collectionType);
    CNGSAttribute* musicAttribute = new CNGSAttribute(music, m_musicSetting, m_collectionType);
    CNGSAttribute* fireButtonAttribute = new CNGSAttribute(fireButton, m_fireBtnSetting, m_collectionType);
    CNGSAttribute* notificationAttribute = new CNGSAttribute(notification, m_notificationSetting, m_collectionType);
    attributes.Add(softCurrencyAttribute);
    attributes.Add(hardCurrencyAttribute);
    attributes.Add(experienceAttribute);
    attributes.Add(conflictAttribute);
    attributes.Add(soundAttribute);
    attributes.Add(musicAttribute);
    attributes.Add(fireButtonAttribute);
    attributes.Add(notificationAttribute);
}

void CPrivateData1::setData(const CHash* attributes)
{
    resetData();
    CNGSAttribute* softCurrency = NULL;
    CNGSAttribute* hardCurrency = NULL;
    CNGSAttribute* experience = NULL;
    CNGSAttribute* conflictStatus = NULL;
    CNGSAttribute* sound = NULL;
    CNGSAttribute* music = NULL;
    CNGSAttribute* fireButton = NULL;
    CNGSAttribute* notification = NULL;

    CStrChar softCurrencyName = getTokenNameFor(TOKENS[0]);
    CStrChar hardCurrencyName = getTokenNameFor(TOKENS[1]);
    CStrChar experienceName = getTokenNameFor(TOKENS[2]);
    CStrChar conflictStatusName = getTokenNameFor(TOKENS[3]);
    CStrChar soundName = getTokenNameFor(TOKENS[4]);
    CStrChar musicName = getTokenNameFor(TOKENS[5]);
    CStrChar fireButtonName = getTokenNameFor(TOKENS[6]);
    CStrChar notificationName = getTokenNameFor(TOKENS[7]);

    if (attributes->Find(CStringToKey(softCurrencyName.GetCharPtr(), 0), (void**)&softCurrency)) {
        m_softCurrency = softCurrency->getVal_uint64();
    } else {
        dataNotFound();
    }
    if (attributes->Find(CStringToKey(hardCurrencyName.GetCharPtr(), 0), (void**)&hardCurrency)) {
        m_hardCurrency = hardCurrency->getVal_uint64();
    } else {
        dataNotFound();
    }
    if (attributes->Find(CStringToKey(experienceName.GetCharPtr(), 0), (void**)&experience)) {
        m_experience = experience->getVal_uint64();
    } else {
        dataNotFound();
    }
    if (attributes->Find(CStringToKey(conflictStatusName.GetCharPtr(), 0), (void**)&conflictStatus)) {
        m_conflictStatus = conflictStatus->getVal_int32();
    } else {
        dataNotFound();
    }
    if (attributes->Find(CStringToKey(soundName.GetCharPtr(), 0), (void**)&sound)) {
        m_soundSetting = sound->getVal_uint8();
    } else {
        dataNotFound();
    }
    if (attributes->Find(CStringToKey(musicName.GetCharPtr(), 0), (void**)&music)) {
        m_musicSetting = music->getVal_uint8();
    } else {
        dataNotFound();
    }
    if (attributes->Find(CStringToKey(fireButtonName.GetCharPtr(), 0), (void**)&fireButton)) {
        m_fireBtnSetting = fireButton->getVal_uint8();
    } else {
        dataNotFound();
    }
    if (attributes->Find(CStringToKey(notificationName.GetCharPtr(), 0), (void**)&notification)) {
        m_notificationSetting = notification->getVal_uint8();
    } else {
        dataNotFound();
    }
}

void CPrivateData1::debugPrint()
{
    debugPrintBase();
}

CStrWChar CPrivateData1::asString()
{
    char text[128] = { 0 };
    ICStdUtil::SPrintF(text, "soft=%i hard=%i xp=%i", (int)m_softCurrency, (int)m_hardCurrency, (int)m_experience);
    return CStrWChar(text);
}

unsigned long long CPrivateData1::getSoftCurrency() { return m_softCurrency; }
unsigned long long CPrivateData1::getHardCurrency() { return m_hardCurrency; }
unsigned long long CPrivateData1::getExperience() { return m_experience; }
void CPrivateData1::setSoftCurrency(unsigned long long amount) { m_softCurrency = amount; }
void CPrivateData1::setHardCurrency(unsigned long long amount) { m_hardCurrency = amount; }
void CPrivateData1::setExperience(unsigned long long experience) { m_experience = experience; }
void CPrivateData1::setContentFileConflictStatus(ContentManagerConflict status) { m_conflictStatus = status; }
unsigned char CPrivateData1::getSoundSetting() { return m_soundSetting; }
unsigned char CPrivateData1::getMusicSetting() { return m_musicSetting; }
unsigned char CPrivateData1::getFireBtnSetting() { return m_fireBtnSetting; }
unsigned char CPrivateData1::getNotificationSetting() { return m_notificationSetting; }
void CPrivateData1::setSoundSetting(unsigned char enabled) { m_soundSetting = enabled; }
void CPrivateData1::setMusicSetting(unsigned char enabled) { m_musicSetting = enabled; }
void CPrivateData1::setFireBtnSetting(unsigned char enabled) { m_fireBtnSetting = enabled; }
void CPrivateData1::setNotificationSetting(unsigned char enabled) { m_notificationSetting = enabled; }
