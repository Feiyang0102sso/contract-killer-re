#import "FlurryAnalytics.h"
#import <UIKit/UIKit.h>
#include "CKAdsCompat.h"

#if !CK_ADS_COMPAT

// Replacement layer switched off: Flurry calls are accepted and dropped (not original code).
@implementation FlurryAnalytics
+ (void)startSession:(NSString*)apiKey
{
}

+ (void)setUserID:(NSString*)userID
{
}

+ (void)setSessionReportsOnCloseEnabled:(BOOL)enabled
{
}

+ (void)setSessionReportsOnPauseEnabled:(BOOL)enabled
{
}

+ (void)logEvent:(NSString*)eventName withParameters:(NSDictionary*)parameters
{
}

+ (NSDictionary*)sessionSnapshot
{
    return nil;
}
@end

#else

static NSString* SessionID;
static NSDate* SessionStarted;
static NSMutableArray* SessionEvents;
static NSUInteger SessionEventCount;
static BOOL ReportOnClose;
static BOOL ReportOnPause;
static BOOL UserAssigned;
static const NSUInteger EventLimit = 256;
static const NSUInteger ParameterByteLimit = 16 * 1024;

@implementation FlurryAnalytics
+ (void)initialize
{
    if (self == [FlurryAnalytics class]) {
        NSNotificationCenter* notifications = [NSNotificationCenter defaultCenter];
        [notifications addObserver:self selector:@selector(applicationClosed:)
                               name:UIApplicationWillTerminateNotification object:nil];
        [notifications addObserver:self selector:@selector(applicationPaused:)
                               name:UIApplicationDidEnterBackgroundNotification object:nil];
    }
}

+ (void)startSession:(NSString*)apiKey
{
    if ([apiKey length] == 0) {
        return;
    }
    @synchronized(self) {
        [SessionStarted release];
        [SessionID release];
        [SessionEvents release];
        CFUUIDRef identifier = CFUUIDCreate(kCFAllocatorDefault);
        SessionID = (NSString*)CFUUIDCreateString(kCFAllocatorDefault, identifier);
        CFRelease(identifier);
        SessionStarted = [[NSDate alloc] init];
        SessionEvents = [[NSMutableArray alloc] init];
        SessionEventCount = 0;
    }
}

+ (void)setUserID:(NSString*)userID
{
    @synchronized(self) {
        UserAssigned = [userID length] != 0;
    }
}

+ (void)setSessionReportsOnCloseEnabled:(BOOL)enabled
{
    @synchronized(self) {
        ReportOnClose = enabled;
    }
}

+ (void)setSessionReportsOnPauseEnabled:(BOOL)enabled
{
    @synchronized(self) {
        ReportOnPause = enabled;
    }
}

+ (void)logEvent:(NSString*)eventName withParameters:(NSDictionary*)parameters
{
    if ([eventName length] == 0) {
        return;
    }
    @synchronized(self) {
        if (SessionEvents == nil) {
            return;
        }
        NSMutableDictionary* event = [NSMutableDictionary dictionaryWithObjectsAndKeys:
            [[eventName copy] autorelease], @"name", [NSDate date], @"date", nil];
        if ([parameters count] != 0 && [NSPropertyListSerialization propertyList:parameters
            isValidForFormat:NSPropertyListBinaryFormat_v1_0]) {
            NSData* data = [NSPropertyListSerialization dataFromPropertyList:parameters
                format:NSPropertyListBinaryFormat_v1_0 errorDescription:NULL];
            if ([data length] != 0 && [data length] <= ParameterByteLimit) {
                id snapshot = [NSPropertyListSerialization propertyListFromData:data mutabilityOption:NSPropertyListImmutable
                    format:NULL errorDescription:NULL];
                [event setValue:snapshot forKey:@"parameters"];
            }
        }
        if ([SessionEvents count] == EventLimit) {
            [SessionEvents removeObjectAtIndex:0];
        }
        [SessionEvents addObject:event];
        ++SessionEventCount;
    }
}

+ (NSDictionary*)sessionSnapshot
{
    @synchronized(self) {
        if (SessionEvents == nil) {
            return nil;
        }
        NSArray* events = [[[NSArray alloc] initWithArray:SessionEvents copyItems:YES] autorelease];
        return [NSDictionary dictionaryWithObjectsAndKeys:SessionID, @"id", SessionStarted, @"started",
            [NSNumber numberWithBool:UserAssigned], @"hasUser",
            [NSNumber numberWithUnsignedInteger:SessionEventCount], @"eventCount", events, @"events", nil];
    }
}

+ (void)saveSession
{
    NSDictionary* report = [self sessionSnapshot];
    NSArray* directories = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    if (report != nil && [directories count] != 0) {
        NSString* file = [[directories objectAtIndex:0] stringByAppendingPathComponent:@"CKFlurrySession.plist"];
        [report writeToFile:file atomically:YES];
    }
}

+ (void)applicationClosed:(NSNotification*)notification
{
    @synchronized(self) {
        if (ReportOnClose) {
            [self saveSession];
        }
    }
}

+ (void)applicationPaused:(NSNotification*)notification
{
    @synchronized(self) {
        if (ReportOnPause) {
            [self saveSession];
        }
    }
}
@end

#endif
