#import "TapjoyConnect.h"
#import "TJCHardwareUtil.h"
#import "TJCNetReachability.h"
#import "TJCCoreFetcher.h"
#import "TJCFeaturedAppManager.h"
#import "TJCFeaturedAppView.h"
#import "TJCOffersWebView.h"
#import "TJCUserAccountManager.h"
#import "TJCVideoManager.h"
#import "TBXML.h"
#import <CommonCrypto/CommonDigest.h>

static TapjoyConnect* SharedTapjoyConnect;

@interface TapjoyConnect ()
- (void)startConnectIfNeeded;
- (void)connectWithParam:(NSDictionary*)parameters;
- (void)connectFinished:(TJCCoreFetcher*)fetcher;
- (NSString*)requestPath:(NSString*)path relativeToService:(NSURL*)base;
@end

@implementation TapjoyConnect
@synthesize appID = appID_, secretKey = secretKey_, userID = userID_, isInitialConnect = isInitialConnect_;

+ (TapjoyConnect*)sharedTapjoyConnect
{
    if (SharedTapjoyConnect == nil) {
        SharedTapjoyConnect = [[self alloc] init];
    }
    return SharedTapjoyConnect;
}

+ (TapjoyConnect*)requestTapjoyConnect:(NSString*)appID secretKey:(NSString*)secretKey
{
    TapjoyConnect* connection = [self sharedTapjoyConnect];
    if ([connection appID] == nil) {
        [connection initConnectWithAppID:appID withSecretKey:secretKey];
    }
    [connection startConnectIfNeeded];
    return connection;
}

- (void)initConnectWithAppID:(NSString*)appID withSecretKey:(NSString*)secretKey
{
    self.appID = appID;
    self.secretKey = secretKey;
    currencyMultiplier_ = 1.0f;
    isInitialConnect_ = YES;

    NSBundle* bundle = [NSBundle mainBundle];
    id installation = [bundle objectForInfoDictionaryKey:@"CKTapjoyInstallationID"];
    if ([installation isKindOfClass:[NSString class]]) {
        installationID_ = [[installation lowercaseString] copy];
    }
    id userID = [bundle objectForInfoDictionaryKey:@"CKTapjoyUserID"];
    if ([userID isKindOfClass:[NSString class]]) {
        self.userID = userID;
    }
    id address = [bundle objectForInfoDictionaryKey:@"CKTapjoyBaseURL"];
    if ([address isKindOfClass:[NSString class]]) {
        NSURL* url = [NSURL URLWithString:address];
        if (TJCValidHTTPSURL(url) && [url query] == nil && [url fragment] == nil) {
            serviceURL_ = [url retain];
        }
    }
    id alternate = [bundle objectForInfoDictionaryKey:@"CKTapjoyAlternateBaseURL"];
    if ([alternate isKindOfClass:[NSString class]]) {
        NSURL* url = [NSURL URLWithString:alternate];
        if (TJCValidHTTPSURL(url) && [url query] == nil && [url fragment] == nil) {
            alternateServiceURL_ = [url retain];
        }
    }
}

- (NSString*)requestPath:(NSString*)path relativeToService:(NSURL*)base
{
    if (base == nil || [path length] == 0 || [appID_ length] == 0 ||
        [secretKey_ length] == 0 || [installationID_ length] == 0) {
        return nil;
    }
    NSString* prefix = [base absoluteString];
    if (![prefix hasSuffix:@"/"]) {
        prefix = [prefix stringByAppendingString:@"/"];
    }
    NSURL* url = [NSURL URLWithString:path relativeToURL:[NSURL URLWithString:prefix]];
    if (!TJCURLHasSameOrigin(url, base)) {
        return nil;
    }
    return [url absoluteString];
}

- (NSString*)servicePath:(NSString*)path
{
    return [self requestPath:path relativeToService:serviceURL_];
}

- (NSString*)alternateServicePath:(NSString*)path
{
    return [self requestPath:path relativeToService:alternateServiceURL_];
}

- (void)startConnectIfNeeded
{
    if (connectFetcher_ == nil && !connected_) {
        [self connectWithParam:[self genericParameters]];
    }
}

- (void)connectWithParam:(NSDictionary*)parameters
{
    connectFetcher_ = [[TJCCoreFetcher alloc] init];
    connectFetcher_.baseURL = [self servicePath:@"connect"];
    connectFetcher_.alternateURL = [self alternateServicePath:@"connect"];
    connectFetcher_.bindings = parameters;

    SEL completion = @selector(connectFinished:);
    NSInvocation* invocation = [NSInvocation invocationWithMethodSignature:[self methodSignatureForSelector:completion]];
    [invocation setSelector:completion];
    [invocation setTarget:self];
    [connectFetcher_ fetchAsynchronouslyWithCompletionInvocation:invocation];
}

- (void)connectFinished:(TJCCoreFetcher*)fetcher
{
    if (fetcher != connectFetcher_) {
        return;
    }
    [connectFetcher_ release];
    connectFetcher_ = nil;
    connected_ = ![fetcher hasError];
    if (connected_ && [[fetcher data] length] != 0) {
        NSString* text = [[[NSString alloc] initWithData:[fetcher data] encoding:NSUTF8StringEncoding] autorelease];
        text = [text stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
        TBXML* xml = [TBXML tbxmlWithXMLData:[fetcher data]];
        TBXMLElement* root = [xml rootXMLElement];
        if (![text isEqualToString:@"OK"]) {
            connected_ = [[TBXML elementName:root] isEqualToString:@"TapjoyConnectReturnObject"] &&
                         [TBXML childElementNamed:@"ErrorMessage" parentElement:root] == NULL;
        }
    }
    if (connected_) {
        isInitialConnect_ = NO;
        [[NSNotificationCenter defaultCenter] postNotificationName:TJC_CONNECT_SUCCESS object:nil];
    } else {
        [[NSNotificationCenter defaultCenter] postNotificationName:TJC_CONNECT_FAILED object:nil];
    }
}

+ (void)disconnect
{
    TapjoyConnect* instance = SharedTapjoyConnect;
    [TJCAdView releaseSharedTJCAdView];
    [TJCCallsWrapper releaseSharedTJCCallsWrapper];
    [TJCOffersWebView releaseSharedTJCOffersWebView];
    [TJCFeaturedAppView releaseSharedTJCFeaturedAppView];
    [TJCFeaturedAppManager releaseSharedTJCFeaturedAppManager];
    [TJCUserAccountManager releaseSharedTJCUserAccountManager];
    [TJCVideoManager releaseSharedTJCVideoManager];
    if (instance != nil) {
        [instance->connectFetcher_ cancel];
        instance->connected_ = NO;
    }
    SharedTapjoyConnect = nil;
    [instance release];
}

- (BOOL)isConnected
{
    return connected_;
}

- (NSDictionary*)genericParameters
{
    NSString* timestamp = [NSString stringWithFormat:@"%.0f", [[NSDate date] timeIntervalSince1970]];
    NSMutableDictionary* parameters = [NSMutableDictionary dictionary];
    [parameters setValue:appID_ forKey:@"app_id"];
    [parameters setValue:installationID_ forKey:@"udid"];
    [parameters setValue:[TapjoyConnect getUserID] forKey:@"publisher_user_id"];
    [parameters setObject:@"iOS" forKey:@"platform"];
    UIDevice* device = [UIDevice currentDevice];
    [parameters setValue:[device model] forKey:@"device_name"];
    [parameters setValue:[device platform] forKey:@"device_type"];
    [parameters setValue:[device systemVersion] forKey:@"os_version"];
    [parameters setObject:[TJCNetReachability getReachibilityType] forKey:@"connection_type"];
    [parameters setObject:@"8.1.5" forKey:@"library_version"];
    [parameters setObject:timestamp forKey:@"timestamp"];
    [parameters setObject:[TapjoyConnect TJCSHA256CommonParamsWithTimeStamp:timestamp] forKey:@"verifier"];
    [parameters setObject:[NSString stringWithFormat:@"%f", currencyMultiplier_] forKey:@"display_multiplier"];
    [parameters setValue:[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"] forKey:@"app_version"];
    return parameters;
}

- (NSString*)createQueryStringFromDict:(NSDictionary*)parameters
{
    TJCCoreFetcher* encoder = [[[TJCCoreFetcher alloc] init] autorelease];
    return [encoder createQueryStringFromDict:parameters];
}

+ (NSString*)createQueryStringFromDict:(NSDictionary*)parameters
{
    return [[self sharedTapjoyConnect] createQueryStringFromDict:parameters];
}

+ (NSString*)getAppID
{
    return [[self sharedTapjoyConnect] appID];
}

+ (NSString*)getUserID
{
    TapjoyConnect* connection = [self sharedTapjoyConnect];
    if ([[connection userID] length] != 0) {
        return [connection userID];
    }
    return connection->installationID_;
}

+ (void)setUserID:(NSString*)userID
{
    [[self sharedTapjoyConnect] setUserID:userID];
}

+ (NSString*)getSecretKey
{
    return [[self sharedTapjoyConnect] secretKey];
}

- (void)setCurrencyMultiplier:(float)multiplier
{
    if (multiplier > 0.0f && isfinite(multiplier)) {
        currencyMultiplier_ = multiplier;
    }
}

+ (void)setCurrencyMultiplier:(float)multiplier
{
    [[self sharedTapjoyConnect] setCurrencyMultiplier:multiplier];
}

- (float)getCurrencyMultiplier
{
    return currencyMultiplier_;
}

+ (float)getCurrencyMultiplier
{
    return [[self sharedTapjoyConnect] getCurrencyMultiplier];
}

+ (NSString*)TJCSHA256CommonParamsWithTimeStamp:(NSString*)timestamp
{
    TapjoyConnect* connection = [self sharedTapjoyConnect];
    NSString* value = [NSString stringWithFormat:@"%@:%@:%@:%@", [connection appID],
        connection->installationID_, timestamp, [connection secretKey]];
    return [self TJCSHA256WithString:value];
}

+ (NSString*)TJCSHA256CommonParamsWithTimeStamp:(NSString*)timestamp tapPointsAmount:(int)points guid:(NSString*)guid
{
    TapjoyConnect* connection = [self sharedTapjoyConnect];
    NSString* value = [NSString stringWithFormat:@"%@:%@:%@:%@:%d:%@", [connection appID],
        connection->installationID_, timestamp, [connection secretKey], points, guid];
    return [self TJCSHA256WithString:value];
}

+ (NSString*)TJCSHA256WithString:(NSString*)value
{
    NSData* input = [value dataUsingEncoding:NSUTF8StringEncoding];
    unsigned char bytes[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256([input bytes], (CC_LONG)[input length], bytes);
    NSMutableString* hash = [NSMutableString stringWithCapacity:2 * sizeof(bytes)];
    for (NSUInteger index = 0; index < sizeof(bytes); ++index) {
        [hash appendFormat:@"%02x", (unsigned int)bytes[index]];
    }
    return hash;
}

+ (NSString*)generateUUID
{
    CFUUIDRef identifier = CFUUIDCreate(kCFAllocatorDefault);
    NSString* value = (NSString*)CFUUIDCreateString(kCFAllocatorDefault, identifier);
    CFRelease(identifier);
    return [value autorelease];
}

+ (void)deviceNotificationReceived
{
    [[self sharedTapjoyConnect] startConnectIfNeeded];
}

+ (void)orientationChanged:(NSNotification*)notification
{
    UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];
    [self updateViewsWithOrientation:orientation];
}

- (void)dealloc
{
    [connectFetcher_ cancel];
    [connectFetcher_ release];
    [appID_ release];
    [secretKey_ release];
    [userID_ release];
    [data_ release];
    [connection_ cancel];
    [connection_ release];
    [serviceURL_ release];
    [alternateServiceURL_ release];
    [installationID_ release];
    [super dealloc];
}
@end
