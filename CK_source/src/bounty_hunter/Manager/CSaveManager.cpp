// CSaveManager.cpp - CK ARMv7 XML save envelope and per-profile local storage.
#include "CSaveManager.h"

#include "App.h"
#include "AppSettings.h"
#include "CBH_RSACrypt.h"
#include "CCrc32.h"
#include "CFileData1.h"
#include "CGameApp.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSLoginFlow.h"
#include "CNGSUtil.h"
#include "CNetworkAvailability.h"
#include "CPrivateData1.h"
#include "CProfileManager.h"
#include "CPublicData1.h"
#include "CXmlHelper.h"
#include "DataInputStream.h"
#include "DataOutputStream.h"
#include "ICFile.h"
#include "ICFileMgr.h"
#include "IOSInterface.h"
#include "NPMem.h"
// Header statics of the original unit (static initializer: widths and heights only, then a class id).
#define AD_BANNER_SIZES_NO_MARGINS
#include "AdBannerSizes.h"
#include "ICInAppPurchase.h"

// Named original data: 11 and 15 UTF-32 code units, including their terminators.
extern const wchar_t SAVE_FILE_NAME[] = L"savebh.dat";
static const wchar_t TMP_FILE_NAME[] = L"savebh_tmp.dat";

typedef char SaveManagerSizeCheck[sizeof(CSaveManager) == 0x58 ? 1 : -1];

CSaveManager::CSaveManager()
{
    m_flags = 0;
    m_saveState = -1;
}

ESaveResult CSaveManager::Init()
{
    m_pending = false;
    m_force = 0;
    m_pauseGServe = 1;
    XString profile("default");
    XString profileCopy(profile);
    CNGS* ngs = CNGS::GetInstance();
    if (ngs->GetLocalUser()->GetClientID() != -1) {
        profileCopy = ngs->GetLocalUser()->m_credentials.getSubDirectoryPath().GetWCharPtr();
    }
    CStrWChar path(ICFileMgr::GetInstance()->GetApplicationDataPath());
    path = ICFileMgr::GetInstance()->GetApplicationDataPath();
    if (path.GetLength() > 0) {
        path += ICFileMgr::GetInstance()->GetPathSeparator() + CStrWChar(XString::AnsiString(profileCopy.c_str()));
    }
    // The original adds the filename only when this directory already exists.
    if (ICFileMgr::GetInstance()->Exists(XString(path.GetWCharPtr()).c_str())) {
        path += ICFileMgr::GetInstance()->GetPathSeparator() + CStrWChar(SAVE_FILE_NAME);
    }
    m_fileName = path.GetWCharPtr();

    CFileData1* fileData = static_cast<App*>(WindowApp::m_instance)->m_fileData;
    if (fileData->m_size > 0) {
        // The original copies the profile content but then parses the source buffer.
        unsigned char* copy = new unsigned char[fileData->m_size];
        np_memcpy(copy, fileData->m_data, fileData->m_size);
        ParseData(fileData->m_data, fileData->m_size, &m_document);
        delete[] copy;
        m_root = m_document.FirstChildElement();
    } else {
        ICFile* file = ICFileMgr::GetInstance()->Open(path.GetWCharPtr(), ICFileMgr::AccessRead);
        if (file != NULL) {
            unsigned int size = ICFileMgr::GetInstance()->Size(path.GetWCharPtr());
            unsigned char* data = new unsigned char[size];
            int read = file->Read(data, size);
            ParseData(data, read, &m_document);
            ICFileMgr::GetInstance()->Close(file);
            delete[] data;
            m_root = m_document.FirstChildElement();
        } else {
            TiXmlElement* root = new TiXmlElement("savefile");
            m_root = m_document.InsertEndChild(*root)->ToElement();
            delete root;
        }
    }
    // ParseData results (CRC / XML errors) are ignored by Init.
    return SAVE_SUCCESS;
}

void CSaveManager::PauseByGServe(unsigned char paused)
{
    m_pauseGServe = paused;
}

ESaveResult CSaveManager::ParseData(unsigned char* data, int size, TiXmlDocument* document)
{
    unsigned int expected = (unsigned int)data[0] << 24 | (unsigned int)data[1] << 16 |
                            (unsigned int)data[2] << 8 | data[3];
    unsigned int actual = CCrc32::GetInstance()->Crc32(data + 4, size - 4);
    if (actual != expected) {
        return SAVE_CRC_ERROR;
    }
    CBH_XorCrypt::Decypher((char*)data + 5, size - 5, data[4]);
    document->Clear();
    document->Parse((char*)data + 5);
    if (document->Error()) {
        return SAVE_XML_ERROR;
    }
    return SAVE_SUCCESS;
}

ESaveResult CSaveManager::Save(bool force)
{
    CNGSUtil::DebugLog("CSaveManager::Save %d, %d, %d, %d, %d", m_pauseSaving, m_pauseGServe,
                       static_cast<App*>(WindowApp::m_instance)->m_field3a0,
                       CNGSLoginFlow::GetInstance()->IsRunning(), force);
    if (m_pauseSaving) {
        return SAVE_PAUSED;
    }
    if (m_pauseGServe != 0) {
        return SAVE_PAUSED;
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_field3a0) {
        return SAVE_PAUSED;
    }
    if (CNGSLoginFlow::GetInstance()->IsRunning()) {
        return SAVE_PAUSED;
    }
    // The original repeats the busy checks.
    if (static_cast<App*>(WindowApp::m_instance)->m_field3a0) {
        return SAVE_PAUSED;
    }
    if (CNGSLoginFlow::GetInstance()->IsRunning()) {
        return SAVE_PAUSED;
    }

    XString profile("default");
    profile = CNGS::GetInstance()->GetLocalUser()->m_credentials.getSubDirectoryPath().GetWCharPtr();
    CStrWChar path(ICFileMgr::GetInstance()->GetApplicationDataPath());
    if (path.GetLength() > 0) {
        path += CStrWChar(ICFileMgr::GetInstance()->GetPathSeparator()) + CStrWChar(profile.c_str());
    }
    if (!ICFileMgr::GetInstance()->Exists(XString(path.GetWCharPtr()).c_str())) {
        ICFileMgr::GetInstance()->MkDir(path.GetWCharPtr());
    }
    path += ICFileMgr::GetInstance()->GetPathSeparator() + CStrWChar(TMP_FILE_NAME);
    ICFile* file = ICFileMgr::GetInstance()->Open(path.GetWCharPtr(), ICFileMgr::AccessWrite);
    DataOutputStream unusedStream; // Constructed in the original despite being unused.
    unsigned char* buffer = NULL;
    int size = 0;
    if (file != NULL) {
        size = GetData(&buffer);
        if (file->Write(buffer, size) != size) {
            ICFileMgr::GetInstance()->Close(file);
            // Original write-failure branch leaves buffer allocated.
            ShowSaveErrorMessage(SAVE_WRITE_ERROR);
            return SAVE_WRITE_ERROR;
        }
    }
    ICFileMgr::GetInstance()->Close(file);
    bool valid = IsSaveValid(path);
    ICFileMgr* fileManager = ICFileMgr::GetInstance();
    if (valid == true) {
        {
            CStrWChar finalPath(fileManager->GetApplicationDataPath());
            if (finalPath.GetLength() > 0) {
                finalPath += ICFileMgr::GetInstance()->GetPathSeparator() + CStrWChar(XString::AnsiString(profile.c_str()));
            }
            ICFileMgr::GetInstance()->Exists(XString(finalPath.GetWCharPtr()).c_str());
            finalPath += ICFileMgr::GetInstance()->GetPathSeparator() + CStrWChar(SAVE_FILE_NAME);
            ICFileMgr::GetInstance()->Delete(finalPath.GetWCharPtr());
            ICFileMgr::GetInstance()->Rename(path.GetWCharPtr(), finalPath.GetWCharPtr());
        }
        m_saveState = 1;
        if (force && !CGameApp::syncServerFlag && !CGameApp::gserveDataConflict) {
            App* app = static_cast<App*>(WindowApp::m_instance);
            CPrivateData1* privateData;
            CPublicData1* publicData;
            if (app != NULL && (privateData = app->m_privateData) != NULL &&
                (publicData = app->m_publicData) != NULL) {
                CFileData1* fileData = app->m_fileData;
                TiXmlElement* player = app->m_saveManager->GetRootElement()->FirstChildElement("player");
                int credits = CXmlHelper::GetAttributeValue(player, "hunt_credits").ToInt();
                int experience = CXmlHelper::GetAttributeValue(player, "xp").ToInt();
                int points = CXmlHelper::GetAttributeValue(player, "points").ToInt();
                privateData->setExperience(experience);
                privateData->setSoftCurrency(points);
                privateData->setHardCurrency(credits);
                privateData->m_hasProgress = 1;
                privateData->setSoundSetting(static_cast<App*>(WindowApp::m_instance)->m_settings->IsSoundEnabled());
                privateData->setMusicSetting(static_cast<App*>(WindowApp::m_instance)->m_settings->IsMusicEnabled());
                privateData->setFireBtnSetting(static_cast<App*>(WindowApp::m_instance)->m_settings->IsFireButtonEnabled());
                privateData->setNotificationSetting(static_cast<App*>(WindowApp::m_instance)->m_settings->IsLocalPushEnabled());
                publicData->m_level = CXmlHelper::GetAttributeValue(player, "level").ToInt();
                TiXmlElement* stats = player->FirstChildElement("game_stats");
                publicData->m_successBounties = CXmlHelper::GetAttributeValue(stats, "m_successBounties").ToInt();
                fileData->setData(buffer, size);

                TCVector<int> collections;
                static_cast<App*>(WindowApp::m_instance)->m_profileManager->getAllRegisteredCollections(collections);
                static_cast<App*>(WindowApp::m_instance)->m_profileManager->save(
                    collections, static_cast<App*>(WindowApp::m_instance)->m_networkAvailability->IsAvailable());
                CGameApp::syncServerFlag = true;
            }
        }
        delete[] buffer;
        return SAVE_SUCCESS;
    }
    // Original validation-failure branch leaves buffer allocated.
    fileManager->Delete(path.GetWCharPtr());
    ShowSaveErrorMessage(SAVE_WRITE_ERROR);
    return SAVE_WRITE_ERROR;
}

unsigned int CSaveManager::GetData(unsigned char** data)
{
    TiXmlPrinter printer;
    m_document.Accept(&printer);
    // The characters start at Rep+8 of the printer buffer.
    const char* serialized = printer.CStr();
    int size = printer.Size();
    unsigned char* payload = new unsigned char[size];
    for (int index = 0; index < size; ++index) {
        payload[index] = serialized[index];
    }
    unsigned char key = CBH_XorCrypt::Cipher((char*)payload, size);
    DataOutputStream output;
    DataOutputStream body;
    body.writeByte(key);
    body.write(payload, size);
    unsigned int crc = CCrc32::GetInstance()->Crc32(body.GetData(), body.m_buffer.size());
    unsigned char* crcBytes = new unsigned char[4];
    crcBytes[0] = crc >> 24;
    crcBytes[1] = crc >> 16;
    crcBytes[2] = crc >> 8;
    crcBytes[3] = crc;
    output.write(crcBytes, 4);
    delete[] crcBytes;
    output.write(body.GetData(), body.m_buffer.size());
    unsigned char* bytes = output.GetData();
    unsigned char* result = new unsigned char[(unsigned int)output.m_buffer.size()];
    for (unsigned int index = 0; index < (unsigned int)output.m_buffer.size(); ++index) {
        result[index] = bytes[index];
    }
    *data = result;
    delete[] payload;
    return output.m_buffer.size();
}

void CSaveManager::ShowSaveErrorMessage(ESaveResult result)
{
    XString title = Window::ResString("IDS_SAVE_ERROR_TITLE");
    title = XString::Format(title.c_str(), result);
    XString text = Window::ResString("IDS_SAVE_ERROR_TEXT");
    IOSInterface::showMessage(XString::AnsiString(title.c_str()), XString::AnsiString(text.c_str()));
}

bool CSaveManager::IsSaveValid(const CStrWChar& path)
{
    ICFile* file = ICFileMgr::GetInstance()->Open(path.GetWCharPtr(), ICFileMgr::AccessRead);
    ICFileMgr* fileManager = ICFileMgr::GetInstance();
    if (file != NULL) {
        unsigned int size = fileManager->Size(path.GetWCharPtr());
        unsigned char* buffer = new unsigned char[size];
        file->Read(buffer, size);
        JDataInputStream stream((char*)buffer, size);
        unsigned int expected = stream.readInt();
        unsigned int actual = CCrc32::GetInstance()->Crc32(buffer + 4, size - 4);
        ICFileMgr::GetInstance()->Close(file);
        delete[] buffer;
        return actual == expected;
    }
    fileManager->Close(file);
    return false;
}

TiXmlElement* CSaveManager::GetRootElement()
{
    return m_root;
}

void CSaveManager::Update()
{
    if (m_pending) {
        Save(true);
        m_pending = false;
        m_force = 0;
    }
}

void CSaveManager::SaveOnNextUpdate(unsigned char force)
{
    m_pending = true;
    m_force = force;
}

void CSaveManager::Reset()
{
    ICFileMgr::GetInstance()->Delete(m_fileName.c_str());
    m_document.Clear();
    TiXmlElement* root = new TiXmlElement("savefile");
    m_root = m_document.InsertEndChild(*root)->ToElement();
    delete root;
}

ESaveResult CSaveManager::SetData(unsigned char* data, int size)
{
    ESaveResult result = ParseData(data, size, &m_document);
    if (result == SAVE_SUCCESS) {
        m_root = m_document.FirstChildElement();
    }
    return result;
}

void CSaveManager::PauseSaving()
{
    Save(true);
    m_pauseSaving = true;
}

void CSaveManager::ResumeSaving()
{
    m_pauseSaving = false;
    Save(true);
}
