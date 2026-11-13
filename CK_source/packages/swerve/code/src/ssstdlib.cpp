// ssstdlib.cpp - standard library services of the Swerve porting layer: files (through the Glu file
// manager), strings, memory and debug output. The engine core calls them with C linkage.
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "CApplet.h"
#include "CApp.h"
#include "CHash.h"
#include "ICFileMgr.h"
#include "ICFile.h"
#include "CStrWChar.h"
#include "CFileUtil.h"
#include "CStdUtil_Cocoa.h"
#include "ICDebug.h"
#include "NPMalloc.h"
#include "NPMem.h"
#include "SwvPort.h"

// ---- constants ----
/** Seek origins of the engine (swvFileSeek). */
enum {
    SWV_SEEK_CUR = 0,
    SWV_SEEK_SET = 1,
    SWV_SEEK_END = 2
};
/** Size of the swvDbgPrintf output buffer. */
static const int DEBUG_MESSAGE_SIZE = 1024;

extern "C" {

/** The application's file manager. */
ICFileMgr* swvFileManagerGetInstance()
{
    return ICFileMgr::GetInstance();
}

/** Nothing to release: the file manager belongs to the application. */
void swvFileManagerRelease(ICFileMgr* fileManager)
{
}

/**
 * Open `name` for reading: as given when it already starts with the application or data path,
 * otherwise from the application directory and then from the data directory.
 * Returns 1 and the file in `*file` on success.
 */
int swvFileManagerOpenFileForReading(ICFileMgr* fileManager, const char* name, ICFile** file)
{
    if (fileManager == NULL) {
        return 0;
    }

    int result = 0;
    CStrWChar path(name);
    ICFile* opened;
    if (path.Find(fileManager->GetApplicationPath(), 0) == 0 ||
        path.Find(fileManager->GetApplicationDataPath(), 0) == 0) {
        // already a full path
        opened = fileManager->Open(path.GetWCharPtr(), ICFileMgr::AccessRead);
    } else {
        // relative name: try the application bundle first, then the data directory
        CStrWChar fullPath;
        CFileUtil::GetApplicationPathForFile(fullPath, path.GetWCharPtr());
        opened = fileManager->Open(fullPath.GetWCharPtr(), ICFileMgr::AccessRead);
        if (opened == NULL) {
            CFileUtil::GetApplicationDataPathForFile(fullPath, path.GetWCharPtr());
            opened = fileManager->Open(fullPath.GetWCharPtr(), ICFileMgr::AccessRead);
        }
    }
    if (opened != NULL) {
        *file = opened;
        result = 1;
    }
    return result;
}

/** Close a file opened by swvFileManagerOpenFileForReading. */
void swvFileRelease(ICFile* file)
{
    if (file != NULL) {
        delete file;
    }
}

/** Size of the file in bytes (the position is kept); 0 on failure. */
unsigned int swvFileSize(ICFile* file)
{
    unsigned int size = 0;
    if (file != NULL) {
        unsigned int position = file->Tell();
        if (file->Seek(0, ICFile::SeekEnd)) {
            size = file->Tell();
            file->Seek(position, ICFile::SeekSet);
        }
    }
    return size;
}

/** Move the file position; returns 1 on success. */
int swvFileSeek(ICFile* file, int origin, int offset)
{
    int result = 0;
    if (file != NULL) {
        ICFile::eFileSeekMode mode;
        switch (origin) {
        case SWV_SEEK_END:
            mode = ICFile::SeekEnd;
            break;
        case SWV_SEEK_CUR:
            mode = ICFile::SeekCurrent;
            break;
        case SWV_SEEK_SET:
            mode = ICFile::SeekSet;
            break;
        default:
            return result;
        }
        result = file->Seek(offset, mode) != 0;
    }
    return result;
}

/** Read up to `size` bytes; returns the bytes read. */
unsigned int swvFileReadBytes(ICFile* file, unsigned char* buffer, unsigned int size)
{
    unsigned int read = 0;
    if (file != NULL) {
        read = file->Read(buffer, size);
    }
    return read;
}

size_t swvStrLen(const char* text)
{
    return strlen(text);
}

/** Copy of `text` (NULL for NULL or empty text). */
char* swvStrDup(const char* text)
{
    if (text == NULL) {
        return NULL;
    }
    int length = strlen(text);
    if (length < 1) {
        return NULL;
    }
    char* copy = (char*)np_malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }
    np_memcpy(copy, text, length + 1);
    return copy;
}

/** Lower-case `text` in place. */
char* swvStrLwr(char* text)
{
    if (text != NULL) {
        for (char* current = text; *current != 0; current++) {
            *current = tolower(*current);
        }
    }
    return text;
}

int swvStrNCmp(const char* left, const char* right, size_t count)
{
    return strncmp(left, right, count);
}

char* swvStrChr(const char* text, int character)
{
    return const_cast<char*>(strchr(text, (char)character)); // PORT: preserve the original C return type.
}

void* swvSystemAllocate(size_t size)
{
    return np_malloc(size);
}

void swvSystemFree(void* pointer)
{
    np_free(pointer);
}

void swvMemSet(void* destination, int value, size_t size)
{
    np_memset(destination, (unsigned char)value, size);
}

void swvMemCpy(void* destination, const void* source, size_t size)
{
    np_memcpy(destination, source, size);
}

/** The engine may not end the application. */
void swvExit()
{
}

/** printf-style debug message. */
void swvDbgPrintf(const char* format, ...)
{
    va_list arguments;
    char message[DEBUG_MESSAGE_SIZE];
    va_start(arguments, format);
    CStdUtil_Cocoa::VSPrintF(message, format, arguments);
    ICDebug::LogMessage(message);
    va_end(arguments);
}

}
