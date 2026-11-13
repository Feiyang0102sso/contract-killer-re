// CFileMgr_Cocoa.cpp - iOS file system: stdio files and POSIX directories.
// getcwd() hands back a C library allocation: free() must reach the C library
#undef free
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "CFileMgr_Cocoa.h"
#include "CFile_Cocoa.h"
#include "CUtf.h"
#include "ICDebug.h"
#include "ICDebug_inl.h"
#include <errno.h>




static const wchar_t kwcsPathSeparator[] = L"/";

ICFileMgr* ICFileMgr::CreateInstance()
{
    return new CFileMgr_Cocoa();
}

CFileMgr_Cocoa::CFileMgr_Cocoa()
{
    ReadPathInfo();
    ICDebug::LogMessage("File system online");
    ICDebug::LogMessage("PlatformGlu version %i.%i.%i", 2, 0, 30);
}

CFileMgr_Cocoa::~CFileMgr_Cocoa()
{
}

const wchar_t* CFileMgr_Cocoa::GetApplicationPath()
{
    return m_applicationPath;
}

const wchar_t* CFileMgr_Cocoa::GetApplicationDataPath()
{
    return m_dataPath;
}

const wchar_t* CFileMgr_Cocoa::GetApplicationTempPath()
{
    return m_tempPath;
}

const wchar_t* CFileMgr_Cocoa::GetApplicationCachePath()
{
    return m_cachePath;
}

const wchar_t* CFileMgr_Cocoa::GetPathSeparator()
{
    return kwcsPathSeparator;
}

ICFile* CFileMgr_Cocoa::Open(const wchar_t* path, eFileAccessMode mode)
{
    if (path == NULL) {
        return NULL;
    }
    unsigned int pathLength = wcslen(path);
    if (pathLength >= PathLength || CUtf::GetUtfLen(path, pathLength) >= PathLength) {
        return NULL;
    }
    CFile_Cocoa* file = new CFile_Cocoa();
    if (file == NULL) {
        return NULL;
    }

    int length = CUtf::WcsToUtf8(path, pathLength, (unsigned char*)m_utf8Path, PathLength);
    m_utf8Path[length] = 0;

    const char* fileMode;
    switch (mode) {
    case AccessRead:
        fileMode = "rb";
        break;
    case AccessWrite:
        fileMode = "wb";
        break;
    case AccessAppend:
        fileMode = "ab";
        break;
    default:
        delete file;
        return NULL;
    }
    file->m_file = fopen(m_utf8Path, fileMode);
    if (file->m_file == NULL) {
        delete file;
        file = NULL;
    }
    return file;
}

void CFileMgr_Cocoa::Close(ICFile* file)
{
    delete file;
}

unsigned char CFileMgr_Cocoa::Exists(const wchar_t* path)
{
    if (path == NULL) {
        return 0;
    }
    ICFile* file = Open(path, AccessRead);
    if (file == NULL) {
        return 0;
    }
    Close(file);
    return 1;
}

unsigned int CFileMgr_Cocoa::Size(const wchar_t* path)
{
    if (path == NULL) {
        return 0;
    }
    ICFile* file = Open(path, AccessRead);
    if (file == NULL) {
        return 0;
    }
    file->Seek(0, ICFile::SeekEnd);
    unsigned int size = file->Tell();
    Close(file);
    return size;
}

unsigned char CFileMgr_Cocoa::Rename(const wchar_t* from, const wchar_t* to)
{
    if (from == NULL || to == NULL) {
        return 0;
    }
    int length = CUtf::WcsToUtf8(from, wcslen(from), (unsigned char*)m_utf8Path, PathLength);
    m_utf8Path[length] = 0;
    length = CUtf::WcsToUtf8(to, wcslen(to), (unsigned char*)m_utf8Path2, PathLength);
    m_utf8Path2[length] = 0;
    return rename(m_utf8Path, m_utf8Path2) == 0;
}

unsigned char CFileMgr_Cocoa::Delete(const wchar_t* path)
{
    if (path == NULL) {
        return 0;
    }
    int length = CUtf::WcsToUtf8(path, wcslen(path), (unsigned char*)m_utf8Path, PathLength);
    m_utf8Path[length] = 0;
    return remove(m_utf8Path) == 0;
}

unsigned char CFileMgr_Cocoa::MkDir(const wchar_t* path)
{
    if (path == NULL) {
        return 0;
    }
    char directory[1000];
    unsigned int pathLength = wcslen(path);
    if (pathLength >= sizeof(directory) || CUtf::GetUtfLen(path, pathLength) >= sizeof(directory)) {
        return 0;
    }
    CUtf::WcsToUtf8(path, pathLength, (unsigned char*)directory, sizeof(directory));
    int result = mkdir(directory, 0777);
    if (result == 0) {
        return 1;
    }
    if (errno == EEXIST) {
        // an existing directory counts as created
        struct stat info;
        return stat(directory, &info) == 0 && S_ISDIR(info.st_mode);
    }
    return 0;
}

unsigned char CFileMgr_Cocoa::RmDir(const wchar_t* path)
{
    return 0;
}

unsigned char CFileMgr_Cocoa::DeleteFilesInDir(const wchar_t* path)
{
    return 0;
}

unsigned int CFileMgr_Cocoa::GetFreeDiskSpace()
{
    return 0x7fffffff;
}
