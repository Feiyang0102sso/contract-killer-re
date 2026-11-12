// CUtil.h - small Foundation helpers of gserve (array sorting / logging, wchar_t <-> NSString).
#ifndef CUTIL_H
#define CUTIL_H

#import <Foundation/Foundation.h>

@interface CUtil : NSObject

/** The strings of `array` sorted case-insensitively. */
+ (NSArray*)sortByName:(NSArray*)array;
/** NSLogs every element of `array` with its index. */
+ (void)debugArray:(NSArray*)array;
/** Autoreleased NSString from a UTF-32 (wchar_t) C string. */
+ (NSString*)WcharToNSString:(const wchar_t*)text;
/** UTF-32 (wchar_t) C string of `string` (owned by the string). */
+ (const wchar_t*)NSStringToWchar:(NSString*)string;

@end

#ifdef __cplusplus
/** C++ side helpers of CUtil.mm. */
class CUtil_Cpp {
public:
    /** Main screen size in pixels (points * scale). */
    static void getDeviceScreen(int* width, int* height);
};
#endif

#endif
