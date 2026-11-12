// CFileMgr_Cocoa_mm.mm - Foundation part of the iOS file system: standard paths and directory
// enumeration (see CFileMgr_Cocoa.h).
#include <string.h>
#include "CFileMgr_Cocoa.h"
#include "CUtf.h"




void CFileMgr_Cocoa::ReadPathInfo()
{
    // application: the bundle resources
    m_applicationPath[0] = 0;
    NSBundle* bundle = [NSBundle mainBundle];
    if (bundle != nil) {
        NSString* resourcePath = [bundle resourcePath];
        if (resourcePath != nil) {
            const char* text = [resourcePath UTF8String];
            if (text != NULL) {
                CUtf::Utf8ToWcs((const unsigned char*)text, strlen(text), m_applicationPath, PathLength);
            }
        }
    }

    // data: Documents
    m_dataPath[0] = 0;
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    if ([paths count] != 0) {
        NSString* first = [paths objectAtIndex:0];
        if (first != nil) {
            const char* text = [first UTF8String];
            if (text != NULL) {
                CUtf::Utf8ToWcs((const unsigned char*)text, strlen(text), m_dataPath, PathLength);
            }
        }
    }

    // temporary files: Caches
    m_tempPath[0] = 0;
    paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    if ([paths count] != 0) {
        NSString* first = [paths objectAtIndex:0];
        if (first != nil) {
            const char* text = [first UTF8String];
            if (text != NULL) {
                CUtf::Utf8ToWcs((const unsigned char*)text, strlen(text), m_tempPath, PathLength);
            }
        }
    }

    // cache: Caches
    m_cachePath[0] = 0;
    paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    if ([paths count] != 0) {
        NSString* first = [paths objectAtIndex:0];
        if (first != nil) {
            const char* text = [first UTF8String];
            if (text != NULL) {
                CUtf::Utf8ToWcs((const unsigned char*)text, strlen(text), m_cachePath, PathLength);
            }
        }
    }
}
