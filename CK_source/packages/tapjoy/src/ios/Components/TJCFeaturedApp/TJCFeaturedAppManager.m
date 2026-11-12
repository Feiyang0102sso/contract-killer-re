#import "TJCFeaturedAppManager.h"
#import "TJCFeaturedAppDBManager.h"
#import "TJCFeaturedAppRequestHandler.h"
#import "TapjoyConnect.h"

static TJCFeaturedAppManager* SharedFeaturedAppManager;

@implementation TJCFeaturedAppManager
@synthesize featuredAppModelObj = featuredAppModelObj_, featuredAppDisplayCount = featuredAppDisplayCount_;

+ (TJCFeaturedAppManager*)sharedTJCFeaturedAppManager
{
    if (SharedFeaturedAppManager == nil) {
        SharedFeaturedAppManager = [[self alloc] init];
    }
    return SharedFeaturedAppManager;
}

+ (void)releaseSharedTJCFeaturedAppManager
{
    [SharedFeaturedAppManager release];
    SharedFeaturedAppManager = nil;
}

- (id)init
{
    self = [super init];
    if (self) {
        featuredAppDisplayCount_ = -1;
    }
    return self;
}

- (void)getFeaturedApp
{
    [self getFeaturedAppWithCurrencyID:nil];
}

- (void)getFeaturedAppWithCurrencyID:(NSString*)currencyID
{
    if (featuredAppHandlerObj_ == nil) {
        featuredAppHandlerObj_ = [[TJCFeaturedAppRequestHandler alloc] initRequestWithDelegate:self andRequestTag:0];
    }
    [featuredAppHandlerObj_ requestFeaturedAppWithCurrencyID:currencyID];
}

- (void)fetchResponseSuccessWithData:(TBXMLElement*)element withRequestTag:(NSInteger)tag
{
    TJCFeaturedAppModel* app = [[[TJCFeaturedAppModel alloc] initWithTBXML:element] autorelease];
    NSString* storeID = [app storeID];
    if ([storeID length] == 0 || [[app name] length] == 0) {
        [self fetchResponseError:1 errorDescription:nil requestTag:tag];
        return;
    }

    TJCFeaturedAppDBManager* database = [TJCFeaturedAppDBManager sharedTJCFeaturedAppDBManager];
    int displayed = [database getDisplayedCountForStoreID:storeID];
    if (featuredAppDisplayCount_ >= 0 && displayed > featuredAppDisplayCount_) {
        [self fetchResponseError:1 errorDescription:nil requestTag:tag];
        return;
    }
    if (displayed == 0) {
        [database addApp:app];
    } else {
        [database incrementDisplayedCountForStoreID:storeID];
    }
    self.featuredAppModelObj = app;
    [[NSNotificationCenter defaultCenter] postNotificationName:TJC_FEATURED_APP_RESPONSE_NOTIFICATION object:app];
}

- (void)fetchResponseError:(NSInteger)error errorDescription:(NSString*)description requestTag:(NSInteger)tag
{
    self.featuredAppModelObj = nil;
    [[NSNotificationCenter defaultCenter] postNotificationName:TJC_FEATURED_APP_RESPONSE_NOTIFICATION_ERROR object:nil];
}

- (void)releaseFeaturedAppHandler
{
    [featuredAppHandlerObj_ cancel];
    [featuredAppHandlerObj_ release];
    featuredAppHandlerObj_ = nil;
}

- (void)dealloc
{
    [self releaseFeaturedAppHandler];
    [featuredAppModelObj_ release];
    [super dealloc];
}
@end
