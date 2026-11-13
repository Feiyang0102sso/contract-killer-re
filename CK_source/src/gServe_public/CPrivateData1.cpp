#include "CPrivateData1.h"

#include "CHash.h"
#include "CNGSAttribute.h"
#include "CStringToKey.h"
#include "ICStdUtil.h"

typedef char PrivateDataSizeCheck[sizeof(CPrivateData1) == 0x44 ? 1 : -1];

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

    attributes.Add(new CNGSAttribute(softCurrency, m_softCurrency, m_collectionType));
    attributes.Add(new CNGSAttribute(hardCurrency, m_hardCurrency, m_collectionType));
    attributes.Add(new CNGSAttribute(experience, m_experience, m_collectionType));
    attributes.Add(new CNGSAttribute(conflictStatus, m_conflictStatus, m_collectionType));
    attributes.Add(new CNGSAttribute(sound, m_soundSetting, m_collectionType));
    attributes.Add(new CNGSAttribute(music, m_musicSetting, m_collectionType));
    attributes.Add(new CNGSAttribute(fireButton, m_fireBtnSetting, m_collectionType));
    attributes.Add(new CNGSAttribute(notification, m_notificationSetting, m_collectionType));
}

void CPrivateData1::setData(const CHash* attributes)
{
    resetData();
    unsigned long long* counters[] = { &m_softCurrency, &m_hardCurrency, &m_experience };
    unsigned char* switches[] = { &m_soundSetting, &m_musicSetting, &m_fireBtnSetting, &m_notificationSetting };
    for (int index = 0; index < 8; index++) {
        CStrChar name = getTokenNameFor(TOKENS[index]);
        void* value = NULL;
        if (!attributes->Find(CStringToKey(name.GetCharPtr(), 0), &value)) {
            dataNotFound();
            continue;
        }
        CNGSAttribute* attribute = static_cast<CNGSAttribute*>(value);
        if (index < 3) {
            *counters[index] = attribute->getVal_uint64();
        } else if (index == 3) {
            m_conflictStatus = attribute->getVal_int32();
        } else {
            *switches[index - 4] = attribute->getVal_uint8();
        }
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
