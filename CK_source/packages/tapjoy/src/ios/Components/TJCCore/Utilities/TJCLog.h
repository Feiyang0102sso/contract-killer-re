#pragma once
#import <Foundation/Foundation.h>

@interface TJCLog : NSObject
+ (void)setLogThreshold:(int)threshold;
+ (void)logWithLevel:(int)level format:(NSString*)format, ... NS_FORMAT_FUNCTION(2, 3);
@end
