#pragma once
#import "Reachability.h"

@interface TJCNetReachability : Reachability
+ (BOOL)isUsingWifi;
+ (BOOL)isUsingInternet;
+ (NSString*)getReachibilityType;
@end
