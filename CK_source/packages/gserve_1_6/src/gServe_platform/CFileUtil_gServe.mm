// CFileUtil_gServe.mm - filesystem query through Foundation.
#include "CFileUtil_gServe.h"
#include <stdlib.h>
#include <string.h>

#undef memset

unsigned char CFileUtil_gServe::ExistsDir(const wchar_t* path)
{
    char utf8Path[1024];
    memset(utf8Path, 0, sizeof(utf8Path));
    wcstombs(utf8Path, path, wcslen(path) + 1);
    NSString* filePath = [[[NSString alloc] initWithUTF8String:utf8Path] autorelease];
    BOOL isDirectory = YES;
    return [[NSFileManager defaultManager] fileExistsAtPath:filePath isDirectory:&isDirectory];
}
