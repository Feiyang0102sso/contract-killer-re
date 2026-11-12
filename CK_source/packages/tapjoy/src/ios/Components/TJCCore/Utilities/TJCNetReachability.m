#import "TJCNetReachability.h"

@implementation TJCNetReachability
+ (BOOL)isUsingWifi
{
    return [[self reachabilityForLocalWiFi] currentReachabilityStatus] != NotReachable;
}

+ (BOOL)isUsingInternet
{
    return [[self reachabilityForInternetConnection] currentReachabilityStatus] != NotReachable;
}

+ (NSString*)getReachibilityType
{
    if ([self isUsingWifi]) {
        return @"wifi";
    }
    if ([self isUsingInternet]) {
        return @"mobile";
    }
    return @"none";
}
@end
