#pragma once
#import <UIKit/UIKit.h>

@interface UIDevice (Hardware)
- (NSString*)getSysInfoByName:(const char*)name;
- (unsigned int)getSysInfo:(int)selector;
- (NSString*)platform;
- (NSString*)hwmodel;
- (unsigned int)cpuFrequency;
- (unsigned int)busFrequency;
- (unsigned int)totalMemory;
- (unsigned int)userMemory;
- (unsigned int)maxSocketBufferSize;
- (NSNumber*)totalDiskSpace;
- (NSNumber*)freeDiskSpace;
- (int)platformType;
- (NSString*)platformString;
- (NSString*)platformCode;
@end
