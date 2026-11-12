// CUtil.mm - small Foundation helpers of gserve.
#include "CUtil.h"
#include <wchar.h>
#import <UIKit/UIKit.h>

@implementation CUtil

+ (NSArray*)sortByName:(NSArray*)array
{
    return [array sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];
}

+ (void)debugArray:(NSArray*)array
{
    int count = [array count];
    for (int i = 0; i < count; i++) {
        NSLog(@"item %i: %@\n", i, [array objectAtIndex:i]);
    }
}

+ (NSString*)WcharToNSString:(const wchar_t*)text
{
    return [[[NSString alloc] initWithBytes:text
                                     length:wcslen(text) * sizeof(wchar_t)
                                   encoding:NSUTF32LittleEndianStringEncoding] autorelease];
}

+ (const wchar_t*)NSStringToWchar:(NSString*)string
{
    return (const wchar_t*)[string cStringUsingEncoding:NSUTF32LittleEndianStringEncoding];
}

@end

void CUtil_Cpp::getDeviceScreen(int* width, int* height)
{
    if (width == NULL || height == NULL) {
        return;
    }
    CGRect bounds = [[UIScreen mainScreen] bounds];
    float scale = [[UIScreen mainScreen] scale];
    *width = (int)(bounds.size.width * scale);
    *height = (int)(bounds.size.height * scale);
}
