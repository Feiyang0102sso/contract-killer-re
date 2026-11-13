// ResMap.cpp - resource name map read from res_map.dat.
#include "ResMap.h"
#include <string.h>
#include <wchar.h>
#include "CStrWChar.h"
#include "CFileUtil.h"
#include "CFileInputStream.h"
#include "CApplet.h"
#include "CApp.h"
#include "CResource.h"
#include "CResourceManager.h"





const wchar_t* ResMap::RESMAP_FILENAME = L"res_map.dat";
const char* ResMap::SEPARATOR = "=";
const char* ResMap::RESMAP_NEWLINE_SEQ = "\n";

ResMap::ResMap()
    : m_fileNames(8), m_resourceIds(8)
{
}

ResMap::~ResMap()
{
}

void ResMap::init()
{
    CStrWChar path;
    CFileUtil::GetApplicationPathForFile(path, RESMAP_FILENAME);
    CFileInputStream stream;
    stream.Open(path.m_string);
    unsigned int size = stream.m_length;
    char* data = new char[size + 1];
    data[size] = 0;
    stream.Read((unsigned char*)data, size);
    stream.Close();
    parseMapData(data, size);
    delete[] data;
    m_fileNames.SetTableSize(1000);
}

void ResMap::parseMapData(const char* data, unsigned int size)
{
    unsigned int newlineSize = strlen(RESMAP_NEWLINE_SEQ);
    const char* line = data;
    while (static_cast<unsigned int>(line - data) < size) {
        const char* end = strstr(line, RESMAP_NEWLINE_SEQ);
        if (end == NULL) {
            break;
        }
        parseMapStr(line, end - line);
        line = end + newlineSize;
    }
}

void ResMap::parseMapStr(const char* line, unsigned int length)
{
    const char* separator = strstr(line, SEPARATOR);
    int keyLength = separator - line;
    char* resourceId = new char[keyLength + 1];
    strncpy(resourceId, line, keyLength + 1);
    resourceId[keyLength] = 0;
    int valueLength = line + length - separator;
    char* fileName = new char[valueLength];
    strncpy(fileName, separator + 1, valueLength);
    fileName[valueLength - 1] = 0;
    addMapRecord(resourceId, fileName);
    delete[] resourceId;
    delete[] fileName;
}

void ResMap::addMapRecord(const char* resourceId, const char* fileName)
{
    CStrChar id(resourceId);
    m_fileNames.Add(XString(resourceId), XString(fileName));
    m_resourceIds.Add(XString(fileName), id);
}

XString ResMap::collapsePathToString(const XString& path)
{
    XString copy = path;
    return copy.Replace(L"/", L"\\", true);
}

XString ResMap::getFilename(const char* name) const
{
    XString* fileName = m_fileNames.Get(XString(name));
    if (fileName != NULL) {
        return *fileName;
    }
    return XString("");
}

const char* ResMap::getResourceID(const XString& fileName) const
{
    // the original neither checks the name nor the lookup result
    CStrChar* id = m_resourceIds.Get(fileName);
    if (id->m_length == 0) {
        return NULL;
    }
    return id->m_string;
}

char* ResMap::loadText(const XString& name) const
{
    CFileInputStream stream;
    // Prefer a saved copy in the application data folder.
    CStrWChar path(name.m_data);
    CFileUtil::GetApplicationDataPathForFile(path, collapsePathToString(name).m_data);
    char* text = NULL;
    if (stream.Open(path.m_string)) {
        unsigned int size = stream.m_length;
        text = new char[size + 1];
        stream.Read(reinterpret_cast<unsigned char*>(text), size);
        text[size] = 0;
        stream.Close();
    } else {
        // Fall back to the bundled resource.
        CResourceManager* resources = CApplet::m_pApp->m_resourceManager;
        CResource* resource = NULL;
        resources->CreateResource(getResourceID(name), &resource);
        if (resource != NULL) {
            unsigned int size = static_cast<CStrChar*>(resource->GetData())->m_length;
            text = new char[size + 1];
            memcpy(text, static_cast<CStrChar*>(resource->GetData())->m_string, size);
            text[size] = 0;
            resources->ReleaseResource(getResourceID(name));
        }
    }
    return text;
}
