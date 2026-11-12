// CNGSFileMgr.mm - directory creation through NSFileManager.
#include "CNGSFileMgr.h"
#import <Foundation/Foundation.h>
#include <stdlib.h>

#include "CStrWChar.h"

unsigned char CNGSFileMgr::MkDir(const wchar_t* path)
{
    CStrWChar directoryPath(path);
    char narrowPath[1024] = { 0 };
    size_t length = wcstombs(narrowPath, directoryPath.GetWCharPtr(), sizeof(narrowPath));
    if (length >= sizeof(narrowPath)) {
        return 0;
    }
    NSString* nsPath = [[[NSString alloc] initWithUTF8String:narrowPath] autorelease];
    BOOL isDirectory = NO;
    BOOL result = YES;
    if (![[NSFileManager defaultManager] fileExistsAtPath:nsPath isDirectory:&isDirectory]) {
        result = [[NSFileManager defaultManager] createDirectoryAtPath:nsPath withIntermediateDirectories:YES
                                                            attributes:nil error:nil];
    }
    return result;
}

unsigned char CNGSFileMgr::RmDir(const wchar_t* path)
{
    CStrWChar directoryPath(path);
    char narrowPath[1024] = {};
    size_t length = wcstombs(narrowPath, directoryPath.GetWCharPtr(), sizeof(narrowPath));
    if (length >= sizeof(narrowPath)) {
        return 0;
    }
    NSString* nsPath = [[[NSString alloc] initWithUTF8String:narrowPath] autorelease];
    return [[NSFileManager defaultManager] removeItemAtPath:nsPath error:nil];
}
