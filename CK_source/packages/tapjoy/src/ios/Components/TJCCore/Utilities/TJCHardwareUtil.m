#import "TJCHardwareUtil.h"
#include <sys/sysctl.h>

static const char* const device_string_names[] = {
    "Unknown iOS device", "iPhone Simulator", "iPhone Simulator", "iPad Simulator",
    "iPhone 1G", "iPhone 3G", "iPhone 3GS", "iPhone 4", "iPhone 5",
    "iPod touch 1G", "iPod touch 2G", "iPod touch 3G", "iPod touch 4G",
    "iPad 1G", "iPad 2G", "Unknown iPhone", "Unknown iPod", "Unknown iPad", "iFPGA"
};

@implementation UIDevice (Hardware)
- (NSString*)getSysInfoByName:(const char*)name
{
    char value[128];
    size_t length = sizeof(value);
    if (sysctlbyname(name, value, &length, NULL, 0) != 0 || length == 0 || length > sizeof(value)) {
        return nil;
    }
    value[length - 1] = '\0';
    return [NSString stringWithUTF8String:value];
}

- (NSString*)platform
{
    return [self getSysInfoByName:"hw.machine"];
}

- (NSString*)hwmodel
{
    return [self getSysInfoByName:"hw.model"];
}

- (unsigned int)getSysInfo:(int)selector
{
    int path[] = {CTL_HW, selector};
    unsigned int value = 0;
    size_t length = sizeof(value);
    if (sysctl(path, 2, &value, &length, NULL, 0) != 0) {
        return 0;
    }
    return value;
}

- (unsigned int)cpuFrequency
{
    return [self getSysInfo:HW_CPU_FREQ];
}

- (unsigned int)busFrequency
{
    return [self getSysInfo:HW_BUS_FREQ];
}

- (unsigned int)totalMemory
{
    return [self getSysInfo:HW_PHYSMEM];
}

- (unsigned int)userMemory
{
    return [self getSysInfo:HW_USERMEM];
}

- (unsigned int)maxSocketBufferSize
{
    unsigned int value = 0;
    size_t length = sizeof(value);
    if (sysctlbyname("kern.ipc.maxsockbuf", &value, &length, NULL, 0) != 0) {
        return 0;
    }
    return value;
}

- (NSNumber*)totalDiskSpace
{
    NSDictionary* attributes = [[NSFileManager defaultManager] attributesOfFileSystemForPath:NSHomeDirectory() error:NULL];
    return [attributes objectForKey:NSFileSystemSize];
}

- (NSNumber*)freeDiskSpace
{
    NSDictionary* attributes = [[NSFileManager defaultManager] attributesOfFileSystemForPath:NSHomeDirectory() error:NULL];
    return [attributes objectForKey:NSFileSystemFreeSize];
}

- (int)platformType
{
    NSString* machine = [self platform];
    if ([machine isEqualToString:@"iFPGA"]) {
        return 18;
    }
    NSString* const devices[] = {@"iPhone1,1", @"iPhone1,2", @"iPhone2", @"iPhone3", @"iPhone4",
        @"iPod1,1", @"iPod2,1", @"iPod3,1", @"iPod4,1", @"iPad1,1", @"iPad2,1"};
    for (int index = 0; index < 11; ++index) {
        if ([machine isEqualToString:devices[index]]) {
            return index + 4;
        }
    }
    if ([machine hasPrefix:@"iPhone2,"]) {
        return 6;
    }
    if ([machine hasPrefix:@"iPhone3,"]) {
        return 7;
    }
    if ([machine hasPrefix:@"iPhone4,"]) {
        return 8;
    }
    if ([machine hasPrefix:@"iPhone"]) {
        return 15;
    }
    if ([machine hasPrefix:@"iPod"]) {
        return 16;
    }
    if ([machine hasPrefix:@"iPad"]) {
        return 17;
    }
    if ([machine hasSuffix:@"86"] || [machine isEqualToString:@"x86_64"]) {
        if ([[UIScreen mainScreen] bounds].size.height >= 768.0f) {
            return 3;
        }
        return 2;
    }
    return 0;
}

- (NSString*)platformString
{
    return [NSString stringWithUTF8String:device_string_names[[self platformType]]];
}

- (NSString*)platformCode
{
    NSString* const codes[] = {@"Unknown iOS device", @"iPhone Simulator", @"Unknown iOS device",
        @"Unknown iOS device", @"M68", @"N82", @"N88", @"N89", @"Unknown iPhone",
        @"N45", @"N72", @"N18", @"N80", @"K48", @"Unknown iPad",
        @"Unknown iPhone", @"Unknown iPod", @"Unknown iPad", @"Unknown iOS device"};
    return codes[[self platformType]];
}
@end
