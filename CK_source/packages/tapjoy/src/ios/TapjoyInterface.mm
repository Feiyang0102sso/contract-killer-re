// TapjoyInterface.mm - C++ entry points of the Tapjoy integration; everything forwards to TapjoyManager.
#include "TapjoyInterface.h"

#import "TapjoyManager.h"

TapjoyManager* TapjoyInterface::m_pTapjoy;
TapjoyInterface::FeaturedAppInfo* TapjoyInterface::m_pFeaturedAppInfo;
bool TapjoyInterface::m_FeaturedAppQueryInProgress;

void TapjoyInterface::initialize(const char* appId, const char* padAppId, const char* secretKey, bool enableVideoAds)
{
    m_pTapjoy = [[TapjoyManager alloc] init:appId :padAppId :secretKey :enableVideoAds];
    m_pFeaturedAppInfo = NULL;
}

void TapjoyInterface::initializeFromBundle()
{
    NSBundle* bundle = [NSBundle mainBundle];
    id appId = [bundle objectForInfoDictionaryKey:@"CKTapjoyAppID"];
    id padAppId = [bundle objectForInfoDictionaryKey:@"CKTapjoyPadAppID"];
    id secret = [bundle objectForInfoDictionaryKey:@"CKTapjoySecret"];
    id videoAds = [bundle objectForInfoDictionaryKey:@"CKTapjoyVideoEnabled"];
    if (![appId isKindOfClass:[NSString class]] || ![secret isKindOfClass:[NSString class]]) {
        initialize("", "", "", false);
        return;
    }
    if (![padAppId isKindOfClass:[NSString class]]) {
        padAppId = appId;
    }
    bool enableVideoAds = [videoAds respondsToSelector:@selector(boolValue)] && [videoAds boolValue];
    initialize([appId UTF8String], [padAppId UTF8String], [secret UTF8String], enableVideoAds);
}

void TapjoyInterface::prepareToDestroy()
{
    if (m_pTapjoy != nil) {
        [[NSNotificationCenter defaultCenter] removeObserver:m_pTapjoy];
    }
    [TapjoyConnect disconnect];
}

void TapjoyInterface::destroy()
{
    [m_pTapjoy release];
    m_pTapjoy = nil;
    delete m_pFeaturedAppInfo;
}

bool TapjoyInterface::getFeaturedAppQueryState()
{
    return m_FeaturedAppQueryInProgress;
}

TapjoyInterface::FeaturedAppInfo* TapjoyInterface::getFeaturedApp()
{
    return m_pFeaturedAppInfo;
}

void TapjoyInterface::openTapjoyInterface()
{
    [m_pTapjoy displayInterface];
}

void TapjoyInterface::tick(int elapsedMillis)
{
    [m_pTapjoy tick:elapsedMillis];
}

unsigned char TapjoyInterface::interfaceIsOpen()
{
    return [m_pTapjoy interfaceIsOpen];
}

unsigned char TapjoyInterface::canDisplayInterface()
{
    return [m_pTapjoy canDisplayInterface];
}

unsigned int TapjoyInterface::getRemainingTapjoyPoints()
{
    return [m_pTapjoy getRemainingTapjoyPoints];
}

unsigned char TapjoyInterface::consumeTapjoyPoints(unsigned int points)
{
    return [m_pTapjoy consumeTapjoyPoints:points];
}

unsigned int TapjoyInterface::convertPointsToGameCurrency(unsigned int points)
{
    return [m_pTapjoy convertPointsToGameCurrency:points];
}

void TapjoyInterface::onResume()
{
    [m_pTapjoy onResume];
}

void TapjoyInterface::registerVideoCallback(void (*callback)(unsigned char, void*), void* context)
{
    [m_pTapjoy registerVideoCallback:callback :context];
}
