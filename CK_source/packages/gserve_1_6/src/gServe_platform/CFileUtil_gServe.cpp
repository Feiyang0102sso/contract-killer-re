// CFileUtil_gServe.cpp - application data files.
#include "CFileUtil_gServe.h"

#include <stdlib.h>
#include "ICFile.h"
#include "ICFileMgr.h"


void CFileUtil_gServe::GetApplicationDataPathForFile(CStrWChar& outPath, const wchar_t* fileName, const wchar_t* subFolder)
{
    outPath = ICFileMgr::GetInstance()->GetApplicationDataPath();
    if (outPath.m_length > 0) {
        outPath.Concatenate(ICFileMgr::GetInstance()->GetPathSeparator());
    }
    if (subFolder != NULL) {
        outPath.Concatenate(subFolder);
        outPath.Concatenate(ICFileMgr::GetInstance()->GetPathSeparator());
    }
    outPath.Concatenate(fileName);
}

unsigned char CFileUtil_gServe::ReadFile(const wchar_t* path, unsigned char* buffer, unsigned int size)
{
    unsigned char result = 1;
    if (buffer != NULL && size != 0) {
        result = 0;
        ICFile* file = ICFileMgr::GetInstance()->Open(path, ICFileMgr::AccessRead);
        if (file != NULL) {
            unsigned int readSize = file->Read(buffer, size);
            result = (readSize == size);
            ICFileMgr::GetInstance()->Close(file);
        }
    }
    return result;
}

unsigned char CFileUtil_gServe::WriteFile(const wchar_t* path, const unsigned char* data, unsigned int size)
{
    unsigned char result = 0;
    if (data != NULL && size != 0) {
        result = 0;
        ICFile* file = ICFileMgr::GetInstance()->Open(path, ICFileMgr::AccessWrite);
        if (file != NULL) {
            unsigned int writtenSize = file->Write(data, size);
            result = (writtenSize == size);
            ICFileMgr::GetInstance()->Close(file);
        }
    }
    return result;
}

unsigned char CFileUtil_gServe::ReadApplicationDataFile(const wchar_t* fileName, unsigned char* buffer, unsigned int size,
                                                        const wchar_t* subFolder)
{
    CStrWChar path;
    GetApplicationDataPathForFile(path, fileName, subFolder);
    return ReadFile(path.m_string, buffer, size);
}

unsigned char CFileUtil_gServe::WriteApplicationDataFile(const wchar_t* fileName, const unsigned char* data,
                                                         unsigned int size, const wchar_t* subFolder)
{
    CStrWChar path;
    GetApplicationDataPathForFile(path, fileName, subFolder);
    return WriteFile(path.m_string, data, size);
}

unsigned int CFileUtil_gServe::GetFilesize(const wchar_t* subFolder, const wchar_t* fileName)
{
    CStrWChar path;
    GetApplicationDataPathForFile(path, fileName, subFolder);
    return ICFileMgr::GetInstance()->Size(path.m_string);
}
