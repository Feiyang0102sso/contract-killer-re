// CK AdColony bridge reconstructed from AdColony_Facade.o.
#include "AdColony_Facade.h"
#include "CKAdsCompat.h"

#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>

// These selectors belong to the SDK version used by CK.
@interface AdColony : NSObject
+ (void)initAdColonyWithDelegate:(id)delegate;
+ (int)zoneStatusForZone:(NSString*)zone;
+ (BOOL)virtualCurrencyAwardAvailableForZone:(NSString*)zone;
+ (int)getVirtualCurrencyRewardAmountForZone:(NSString*)zone;
+ (void)playVideoAdForZone:(NSString*)zone withDelegate:(id)delegate
         withV4VCPrePopup:(BOOL)prePopup andV4VCPostPopup:(BOOL)postPopup;
@end

@interface AdColony_Internal : NSObject
{
@public
    NSMutableDictionary* zones;
    NSString* appID;
    AdColony_Facade::IVideoEndedCallbackDelegate* m_delegate;
    CFTimeInterval videoStartTime;
    Class backend;
    NSString* playingZone;
}
- (void)startWithAppID:(NSString*)value;
- (NSString*)adColonyApplicationID;
- (NSDictionary*)adColonyAdZoneNumberAssociation;
- (NSMutableDictionary*)zones;
- (void)adColonyTakeoverBeganForZone:(NSString*)zone;
- (void)adColonyVideoAdNotServedForZone:(NSString*)zone;
- (void)adColonyTakeoverEndedForZone:(NSString*)zone withVC:(BOOL)awarded;
@end

@implementation AdColony_Internal

- (id)init
{
    self = [super init];
    if (self != nil) {
        zones = [[NSMutableDictionary alloc] initWithCapacity:1];
    }
    return self;
}

- (void)dealloc
{
    [zones release];
    [appID release];
    [playingZone release];
    [super dealloc];
}

- (void)startWithAppID:(NSString*)value
{
    if (appID != nil) {
        return;
    }
    appID = [value retain];
    backend = NSClassFromString(@"AdColony");
    if (![backend respondsToSelector:@selector(initAdColonyWithDelegate:)]
        || ![backend respondsToSelector:@selector(zoneStatusForZone:)]
        || ![backend respondsToSelector:@selector(virtualCurrencyAwardAvailableForZone:)]
        || ![backend respondsToSelector:@selector(getVirtualCurrencyRewardAmountForZone:)]
        || ![backend respondsToSelector:@selector(playVideoAdForZone:withDelegate:withV4VCPrePopup:andV4VCPostPopup:)]) {
        backend = Nil;
        return;
    }
    [backend initAdColonyWithDelegate:self];
}

- (NSString*)adColonyApplicationID
{
    return appID;
}

- (NSDictionary*)adColonyAdZoneNumberAssociation
{
    return zones;
}

- (NSMutableDictionary*)zones
{
    return zones;
}

- (void)adColonyTakeoverBeganForZone:(NSString*)zone
{
    NSLog(@"Video ad or fullscreen banner launched for zone %@", zone);
}

- (void)finishVideoForZone:(NSString*)zone completed:(BOOL)completed
{
    if (playingZone == nil || ![playingZone isEqualToString:zone]) {
        return;
    }
    AdColony_Facade::IVideoEndedCallbackDelegate* delegate = m_delegate;
    m_delegate = NULL;
    [playingZone release];
    playingZone = nil;
    if (delegate != NULL) {
        delegate->OnVideoEnded(completed, [zone cStringUsingEncoding:NSASCIIStringEncoding]);
    }
}

- (void)adColonyVideoAdNotServedForZone:(NSString*)zone
{
    [self finishVideoForZone:zone completed:NO];
}

- (void)adColonyTakeoverEndedForZone:(NSString*)zone withVC:(BOOL)awarded
{
    [self finishVideoForZone:zone completed:awarded && CACurrentMediaTime() - videoStartTime > 1.0];
}

@end

static AdColony_Internal* m_Internal;

static NSString* GetZone(const char* zoneId)
{
    if (zoneId == NULL) {
        return [[m_Internal zones] objectForKey:[NSNumber numberWithInt:1]];
    }
    return [NSString stringWithCString:zoneId encoding:NSASCIIStringEncoding];
}

void AdColony_Facade::addZoneId(const char* zoneId)
{
    if (m_Internal == nil) {
        m_Internal = [[AdColony_Internal alloc] init];
    }
    if (zoneId != NULL) {
        NSMutableDictionary* zones = [m_Internal zones];
        NSString* value = [NSString stringWithCString:zoneId encoding:NSASCIIStringEncoding];
        if ([value length] != 0) {
            [zones setObject:value forKey:[NSNumber numberWithInt:[zones count] + 1]];
        }
    }
}

void AdColony_Facade::init(const char* appId)
{
    if (m_Internal != nil && appId != NULL) {
        [m_Internal startWithAppID:[NSString stringWithCString:appId encoding:NSASCIIStringEncoding]];
    }
}

void AdColony_Facade::initFromBundle()
{
#if !CK_ADS_COMPAT
    // Replacement layer switched off: AdColony is never started, isReady() stays false.
    return;
#endif
    NSBundle* bundle = [NSBundle mainBundle];
    NSString* appID = [bundle objectForInfoDictionaryKey:@"CKAdColonyAppID"];
    NSArray* zoneIDs = [bundle objectForInfoDictionaryKey:@"CKAdColonyZoneIDs"];
    if (m_Internal != nil || ![appID isKindOfClass:[NSString class]] || [appID length] == 0
        || ![appID canBeConvertedToEncoding:NSASCIIStringEncoding]
        || ![zoneIDs isKindOfClass:[NSArray class]]) {
        return;
    }
    NSMutableSet* registeredZones = [NSMutableSet set];
    for (NSString* zoneID in zoneIDs) {
        if ([zoneID isKindOfClass:[NSString class]] && [zoneID length] != 0
            && [zoneID canBeConvertedToEncoding:NSASCIIStringEncoding]
            && ![registeredZones containsObject:zoneID]) {
            addZoneId([zoneID UTF8String]);
            [registeredZones addObject:zoneID];
        }
    }
    if ([registeredZones count] != 0) {
        init([appID UTF8String]);
    }
}

void AdColony_Facade::release()
{
    if (m_Internal != nil) {
        m_Internal->m_delegate = NULL;
        [m_Internal release];
        m_Internal = nil;
    }
}

bool AdColony_Facade::isReady(const char* zoneId)
{
    NSString* zone = GetZone(zoneId);
    return m_Internal != nil && m_Internal->backend != Nil && m_Internal->playingZone == nil
        && zone != nil && [m_Internal->backend zoneStatusForZone:zone] == 3
        && [m_Internal->backend virtualCurrencyAwardAvailableForZone:zone];
}

void AdColony_Facade::showVideo(IVideoEndedCallbackDelegate* delegate, const char* zoneId,
                               bool showPrePopup, bool showPostPopup)
{
    if (!isReady(zoneId)) {
        if (delegate != NULL) {
            delegate->OnVideoEnded(false, zoneId);
        }
        return;
    }
    NSString* zone = GetZone(zoneId);
    m_Internal->videoStartTime = CACurrentMediaTime();
    m_Internal->m_delegate = delegate;
    m_Internal->playingZone = [zone retain];
    [m_Internal->backend playVideoAdForZone:zone withDelegate:m_Internal
                     withV4VCPrePopup:showPrePopup andV4VCPostPopup:showPostPopup];
}

int AdColony_Facade::getAwardAmount(const char* zoneId)
{
    NSString* zone = GetZone(zoneId);
    if (m_Internal == nil || m_Internal->backend == Nil || zone == nil) {
        return 0;
    }
    return [m_Internal->backend getVirtualCurrencyRewardAmountForZone:zone];
}

void AdColony_Facade::cancelVideoCallback(IVideoEndedCallbackDelegate* delegate)
{
    if (m_Internal != nil && m_Internal->m_delegate == delegate) {
        m_Internal->m_delegate = NULL;
    }
}
