// CNGSJSONData.cpp - downloadable, device-cached JSON document (b025x).
#include "CNGSJSONData.h"
#include "CNGS.h"
#include "CNGSHeader.h"
#include "CNGSLocalUser.h"
#include "CNGSLoginFlow.h"
#include "CNGSSession.h"
#include "CNGSJSONParser.h"
#include "CNGSUtil.h"
#include "CNetMessageServer.h"
#include "CHttpTransport_gServe.h"
#include "CObjectMap.h"
#include "CObjectMapObject.h"
#include "CArrayOutputStream.h"
#include "CFileUtil.h"
#include "CFileInputStream.h"
#include "CCrc32.h"
#include "CRandGen.h"










CNGSJSONData::CNGSJSONData()
{
    m_responseData = NULL;
    m_transport = NULL;
    m_buffer64 = NULL;
    m_requestCallback = NULL;
    m_responseLength = 0;
    m_hasResponse = false;
    m_root = NULL;
    m_rawJSON = NULL;
    m_noSaveToDisk = false;
    m_busy = false;
    m_field6c = 0;
    m_field68 = 0;
    m_lastAccessTime = 0;
    m_saveFileName = L"gbsio.dat";
    m_overrideFileName = L"override.json";
    m_overrideName = L"override";
}

CNGSJSONData::~CNGSJSONData()
{
    if (m_root != NULL) {
        delete m_root;
        m_root = NULL;
    }
    if (m_requestCallback != NULL) {
        delete m_requestCallback;
        m_requestCallback = NULL;
    }
    if (m_responseData != NULL) {
        delete[] m_responseData;
        m_responseData = NULL;
    }
    if (m_rawJSON != NULL) {
        delete[] m_rawJSON;
        m_rawJSON = NULL;
    }
}

void CNGSJSONData::CheckForUpdate()
{
}

void CNGSJSONData::SetRequestCallback(CNGSServerRequestFunctor* callback)
{
    if (m_requestCallback != NULL) {
        delete m_requestCallback;
        m_requestCallback = NULL;
    }
    m_requestCallback = callback;
}

int CNGSJSONData::GetContent(const char* path, bool forceDownload)
{
    m_responseLength = 0;
    m_hasResponse = false;
    if (m_responseData != NULL) {
        delete[] m_responseData;
        m_responseData = NULL;
    }
    if (m_requestCallback == NULL) {
        m_requestCallback = new CNGSJSONDataRequestFunctor(this, &CNGSJSONData::HandleValidateCNGSJSONDataResponse);
    }
    if (m_baseURL.GetLength() == 0) {
        m_baseURL = "http://s3.amazonaws.com/gserve/";
    }

    CStrChar url(m_baseURL);
    url += path;
    CNetMessageServer server(url);
    server.m_value10 = 1;
    SwpDeviceInfo deviceInfo;
    deviceInfo.m_timeoutMillis = 60000;
    deviceInfo.m_connectTimeoutMillis = 60000;
    server.m_value14 = 0;
    server.m_method = CNetMessageServer::METHOD_GET;

    CHttpTransport_gServe* transport = new CHttpTransport_gServe();
    m_transport = transport;
    int result;
    if (m_lastAccessTime != 0 && !forceDownload) {
        // Conditional GET: only when changed since the cached copy.
        char modifiedSince[256];
        strftime(modifiedSince, 256, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&m_lastAccessTime));
        result = m_transport->Get(deviceInfo, CStrChar(server.m_url).GetCharPtr(), 0, NULL, modifiedSince);
    } else {
        result = transport->Get(deviceInfo, CStrChar(server.m_url).GetCharPtr(), 0, NULL, NULL);
    }
    return result;
}

void CNGSJSONData::HandleValidateCNGSJSONDataResponse(char* data, int length, bool notModified)
{
    if (data != NULL && length > 0 && !notModified) {
        char* json = new char[length + 1];
        if (json != NULL) {
            memcpy(json, data, length);
            json[length] = 0;
            CObjectMapValue* root = CNGSJSONParser::decodeValue(CStrWChar(json));
            if (root != NULL) {
                delete m_root;
                delete[] m_rawJSON;
                m_root = root;
                m_rawJSON = json;
                if (!m_noSaveToDisk) {
                    SaveToDisk();
                }
            } else {
                delete[] json;
            }
        }
    }
    m_busy = false;
    CNGSLoginFlow* login = CNGSLoginFlow::GetInstance();
    if (login->IsRunning()) {
        CNGSLoginStep* step = login->GetCurrentStep();
        if (step != NULL && step->GetClassId() == 0xd538bed3) {
            login->OnEvent(LoginFlowSuccess, "Override");
        }
    }
}

void CNGSJSONData::HandleUpdate(int elapsedMs)
{
    if (m_transport != NULL) {
        m_transport->HandleUpdate(elapsedMs);
        int progress;
        if (m_transport->GetStatus(&progress) == 6) {
            int responseCode = m_transport->GetResponseCode();
            int length = m_transport->GetResponseLen();
            unsigned char* data = NULL;
            if (length != 0) {
                data = new unsigned char[length];
                memcpy(data, m_transport->GetResponsePtr(), length);
            }
            if (m_requestCallback != NULL) {
                ((CNGSDataRequestFunctor*)m_requestCallback)->requestCompleteCallback((char*)data, length, responseCode == 304);
                if (m_requestCallback != NULL) {
                    delete m_requestCallback;
                    m_requestCallback = NULL;
                }
                m_requestCallback = NULL;
                m_responseData = new unsigned char[length];
                memcpy(m_responseData, data, length);
                m_responseLength = length;
                m_hasResponse = true;
            }
            if (data != NULL) {
                delete[] data;
            }
            m_transport->ClearResponse();
            m_transport->Cancel();
            if (m_buffer64 != NULL) {
                delete[] m_buffer64;
                m_buffer64 = NULL;
            }
            m_buffer64 = NULL;
            if (m_transport != NULL) {
                delete m_transport;
                m_transport = NULL;
            }
            m_transport = NULL;
        }
    }
}

void CNGSJSONData::SetLastAccessTime(int time)
{
    m_lastAccessTime = time;
}

unsigned char CNGSJSONData::IsBusy()
{
    return m_busy && m_transport != NULL;
}

unsigned char CNGSJSONData::LoadFromDisk(const CStrWChar& fileName)
{
    CStrWChar path;
    CFileUtil::GetApplicationDataPathForFile(path, fileName.GetWCharPtr());
    CFileInputStream stream;
    if (!stream.Open(path.GetWCharPtr()) || stream.Available() < 20) {
        return 1;
    }

    time_t savedTime = stream.ReadUInt32();
    unsigned int version = stream.ReadUInt32();
    unsigned int length = stream.ReadUInt32();
    unsigned int crc = stream.ReadUInt32();
    unsigned int padding = stream.ReadUInt32();
    if (difftime(getCurrentTime(), savedTime) < 0.0 || version != 0x100 ||
        padding > stream.Available() || length < 9) {
        return 1;
    }
    stream.Skip(padding);
    if (length - 1 > stream.Available()) {
        return 1;
    }
    unsigned int textLength = stream.ReadUInt32();
    unsigned int keyLength = stream.ReadUInt32();
    unsigned int wordCount = (length - 9) / sizeof(wchar_t);
    if (textLength == 0 || keyLength == 0 || textLength > wordCount ||
        keyLength > wordCount - textLength) {
        return 1;
    }

    unsigned int obfuscatedLength = textLength + keyLength;
    wchar_t* obfuscated = new wchar_t[obfuscatedLength];
    if (obfuscated == NULL) {
        return 1;
    }
    stream.Read(reinterpret_cast<unsigned char*>(obfuscated), obfuscatedLength * sizeof(wchar_t));
    sDataBlock block;
    unsigned char result = CNGSUtil::DeObfuscateString(block, obfuscated, L"x0x0x0x0", textLength, keyLength);
    delete[] obfuscated;
    if (result != 0 || block.m_data == NULL) {
        return 1;
    }

    wchar_t* wide = new wchar_t[textLength + 1];
    if (wide == NULL) {
        free(block.m_data);
        return 1;
    }
    memcpy(wide, static_cast<const wchar_t*>(block.m_data) + keyLength, textLength * sizeof(wchar_t));
    wide[textLength] = 0;
    free(block.m_data);
    size_t bytes = wcstombs(NULL, wide, 0);
    if (bytes == (size_t)-1) {
        delete[] wide;
        return 1;
    }
    char* json = new char[bytes + 1];
    if (json == NULL) {
        delete[] wide;
        return 1;
    }
    wcstombs(json, wide, bytes + 1);
    if (CCrc32::GetInstance()->Crc32(reinterpret_cast<unsigned char*>(json), bytes) != crc) {
        delete[] wide;
        delete[] json;
        return 1;
    }
    CObjectMapValue* root = CNGSJSONParser::decodeValue(CStrWChar(wide));
    delete[] wide;
    if (root == NULL) {
        delete[] json;
        return 1;
    }

    delete m_root;
    delete[] m_rawJSON;
    m_root = root;
    m_rawJSON = json;
    m_lastAccessTime = savedTime;
    m_field68 = version;
    m_field6c = 0;
    return 0;
}

unsigned char CNGSJSONData::LoadFromServer(bool forceDownload)
{
    CNGSHeader header;
    CStrWChar gameId;
    CNGSUtil::IntegerToString(header.m_gameId, gameId);
    CStrWChar carrier(header.m_carrier);
    if (carrier.GetLength() == 0) {
        carrier = L"att";
    }
    return LoadFromServer(carrier, gameId, header.m_platformName, forceDownload);
}

unsigned char CNGSJSONData::LoadFromServer(const CStrWChar& carrier, const CStrWChar& gameId,
                                          const CStrWChar& platform, bool forceDownload)
{
    CStrWChar path(m_overrideName);
    if (gameId.GetLength() != 0) {
        path.Concatenate("/");
        path.Concatenate(gameId.GetWCharPtr());
    }
    if (platform.GetLength() != 0) {
        path.Concatenate("/");
        path.Concatenate(platform.GetWCharPtr());
    }
    if (carrier.GetLength() != 0) {
        path.Concatenate("/");
        path.Concatenate(carrier.GetWCharPtr());
    }
    path.Concatenate("/");
    path.Concatenate(m_overrideFileName.GetWCharPtr());
    return LoadFromServer(path, forceDownload);
}

unsigned char CNGSJSONData::LoadFromServer(const CStrWChar& path, bool forceDownload)
{
    int length = path.GetLength();
    char* text = (char*)malloc(length + 1);
    if (wcstombs(text, path.GetWCharPtr(), length + 1) == (size_t)-1) {
        text[0] = 0;
    } else {
        text[length] = 0;
    }
    m_busy = true;
    int result = GetContent(text, forceDownload);
    free(text);
    m_busy = (result == 0);
    return m_busy;
}

unsigned long long CNGSJSONData::getCurrentTime()
{
    CNGSSession* session = CNGS::GetInstance()->GetLocalUser()->m_session;
    unsigned long long now = 0;
    if (session != NULL) {
        now = session->getNetworkCurrentTimeSeconds();
    }
    if (now == 0) {
        now = time(NULL);
    }
    return now;
}

unsigned char CNGSJSONData::SaveToDisk()
{
    unsigned char saved = 0;
    if (m_rawJSON != NULL) {
        _CNGSJSONSaveData saveData;
        saveData.m_time = (int)getCurrentTime();
        saveData.m_version = 0x100;
        saveData.m_length = 0;
        saveData.m_crc = 0;
        saveData.m_paddingSize = 0;
        saveData.m_data = NULL;
        saveData.m_totalSize = 0;
        saveData.m_key = "x0x0x0x0";
        sDataBlock block;
        CStrWChar text(m_rawJSON);
        if (CNGSUtil::ObfuscateString(block, text.GetWCharPtr(), saveData.m_key.GetWCharPtr()) == 0) {
            // Pad the file to whole 512 byte blocks plus the header; the random bytes sit before the data.
            saveData.m_length = block.m_size + 1;
            int blockBytes = saveData.m_length + (512 - saveData.m_length % 512);
            saveData.m_totalSize = blockBytes + 0x14;
            saveData.m_paddingSize = (blockBytes >> 1) - (saveData.m_length >> 1);
            size_t jsonLength = strlen(m_rawJSON);
            saveData.m_crc = CCrc32::GetInstance()->Crc32((unsigned char*)m_rawJSON, jsonLength);
            saveData.m_data = block.m_data;
            saved = SaveToDisk(&saveData);
            if (block.m_data != NULL) {
                free(block.m_data);
                block.m_data = NULL;
            }
        }
    }
    return saved;
}

unsigned char CNGSJSONData::SaveToDisk(_CNGSJSONSaveData* saveData)
{
    unsigned char* buffer = new unsigned char[saveData->m_totalSize];
    CArrayOutputStream stream;
    int index = 0;
    stream.Open(buffer, saveData->m_totalSize);
    stream.WriteInt32(saveData->m_time);
    stream.WriteInt32(saveData->m_version);
    stream.WriteInt32(saveData->m_length);
    stream.WriteUInt32(saveData->m_crc);
    stream.WriteInt32(saveData->m_paddingSize);
    CRandGen* random = CRandGen::GetInstance();
    unsigned int paddingWords = saveData->m_paddingSize / 4;
    for (index = 0; index < paddingWords; index++) {
        stream.WriteInt32(random->GetRand(0x7fff));
    }
    for (index = 0; index < saveData->m_paddingSize - paddingWords * 4; index++) {
        stream.WriteUInt8(0);
    }
    stream.Write((const unsigned char*)saveData->m_data, saveData->m_length - 1);
    unsigned char written = CFileUtil::WriteApplicationDataFile(m_saveFileName.GetWCharPtr(), buffer, saveData->m_totalSize);
    if (buffer != NULL) {
        delete[] buffer;
    }
    return written;
}

unsigned char CNGSJSONData::FindIntAttribute(const CStrWChar& path, const CStrWChar& name, unsigned long long* value)
{
    if (m_root == NULL) {
        return 0;
    }
    CObjectMap document(m_root);
    if (m_root->getType() == CObjectMapValue::TYPE_OBJECT) {
        CObjectMapObject* entry = (CObjectMapObject*)document.getDataAt(path, CStrWChar(L"/"));
        if (entry != NULL && entry->getType() == CObjectMapValue::TYPE_OBJECT) {
            if (entryIsExpired(entry)) {
                return 0;
            }
            if (entryHasActiveSale(entry)) {
                entry = (CObjectMapObject*)entry->getEntry(CStrWChar("sale"));
            }
            CObjectMapValue* attribute = entry->getEntry(name);
            if (attribute == NULL || attribute->getType() != CObjectMapValue::TYPE_INT) {
                return 0;
            }
            *value = ((CObjectMapInt*)attribute)->m_value;
            return 1;
        }
    }
    return 0;
}

unsigned char CNGSJSONData::entryIsExpired(CObjectMapObject* object)
{
    static const char* entry[6] = { "expiration", "start", "end" };
    for (int index = 0; index < 3; index++) {
        CObjectMapValue* date = object->getEntry(CStrWChar(entry[index]));
        if (date != NULL && date->getType() == CObjectMapValue::TYPE_STRING) {
            double seconds = convertDateToSecondsFromNow(CStrWChar(((CObjectMapString*)date)->m_value));
            if (index != 1) {
                // "expiration" / "end": expired once the date passed.
                return seconds > 0.0;
            }
            // "start" still ahead: not active yet.
            if (seconds < 0.0) {
                return 1;
            }
        }
    }
    return 0;
}

unsigned char CNGSJSONData::entryHasActiveSale(CObjectMapObject* object)
{
    unsigned char active = 0;
    CObjectMapObject* sale = (CObjectMapObject*)object->getEntry(CStrWChar("sale"));
    if (sale != NULL) {
        active = sale->getType() == CObjectMapValue::TYPE_OBJECT && !entryIsExpired(sale);
    }
    return active;
}

double CNGSJSONData::convertDateToSecondsFromNow(const CStrWChar& date)
{
    time_t now = (time_t)getCurrentTime();
    struct tm dateTime;
    char text[256];
    if (wcstombs(text, date.GetWCharPtr(), 256) == (size_t)-1) {
        text[0] = 0;
    } else {
        text[255] = 0;
    }
    if (strptime(text, "%a, %d %b %Y %H:%M:%S %Z", &dateTime) == NULL) {
        return 0.0;
    }
    return difftime(now, mktime(&dateTime));
}

unsigned char CNGSJSONData::FindStringAttribute(const CStrWChar& path, const CStrWChar& name, CStrWChar* value)
{
    if (m_root != NULL) {
        CObjectMap document(m_root);
        if (m_root->getType() == CObjectMapValue::TYPE_OBJECT) {
            CObjectMapValue* entry = document.getDataAt(path, CStrWChar(L"/"));
            if (entry != NULL) {
                if (entry->getType() == CObjectMapValue::TYPE_OBJECT) {
                    CObjectMapValue* attribute = ((CObjectMapObject*)entry)->getEntry(name);
                    if (attribute != NULL && attribute->getType() == CObjectMapValue::TYPE_STRING) {
                        *value = CStrWChar(((CObjectMapString*)attribute)->m_value);
                        return 1;
                    }
                } else if (entry->getType() == CObjectMapValue::TYPE_STRING) {
                    *value = CStrWChar(((CObjectMapString*)entry)->m_value);
                    return 1;
                } else if (entry->getType() == CObjectMapValue::TYPE_ARRAY) {
                    // The original reads an array like a string as well (+0x0c is not a string there).
                    *value = CStrWChar(((CObjectMapString*)entry)->m_value);
                    return 1;
                }
            }
        }
    }
    return 0;
}

void CNGSJSONDataRequestFunctor::requestCompleteCallback(char* data, int length, bool notModified)
{
    if (m_callback) {
        (m_target->*m_callback)(data, length, notModified);
    }
}

void CNGSJSONDataRequestFunctor::requestCompleteCallback(CObjectMap* response)
{
}
