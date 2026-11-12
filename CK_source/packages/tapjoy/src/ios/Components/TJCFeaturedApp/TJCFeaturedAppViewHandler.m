#import "TapjoyConnect.h"
#import "TJCFeaturedAppManager.h"
#import "TJCFeaturedAppView.h"

@interface TJCFeaturedAppViewHandler : UIViewController
+ (TJCFeaturedAppViewHandler*)sharedTJCFeaturedAppViewHandler;
+ (void)showFullScreenAdWithURL:(NSString*)address;
+ (void)showFullScreenAdWithURL:(NSString*)address withFrame:(CGRect)frame;
+ (void)showFullScreenAdWithURL:(NSString*)address withViewController:(UIViewController*)controller;
@end

@implementation TJCFeaturedAppViewHandler
+ (TJCFeaturedAppViewHandler*)sharedTJCFeaturedAppViewHandler
{
    static TJCFeaturedAppViewHandler* handler;
    if (handler == nil) {
        handler = [[self alloc] init];
    }
    return handler;
}

+ (void)showFullScreenAdWithURL:(NSString*)address
{
    [self showFullScreenAdWithURL:address withFrame:CGRectZero];
}

+ (void)showFullScreenAdWithURL:(NSString*)address withFrame:(CGRect)frame
{
    [self presentFeaturedAd:address fromController:nil frame:frame];
}

+ (void)showFullScreenAdWithURL:(NSString*)address withViewController:(UIViewController*)controller
{
    [self presentFeaturedAd:address fromController:controller frame:CGRectZero];
}

+ (void)presentFeaturedAd:(NSString*)address fromController:(UIViewController*)controller frame:(CGRect)frame
{
    TJCCallsWrapper* wrapper = [TJCCallsWrapper sharedTJCCallsWrapper];
    if (![[TapjoyConnect sharedTapjoyConnect] isConnected]) {
        [wrapper scheduleCloseNotification];
        return;
    }
    TJCFeaturedAppView* featured = [TJCFeaturedAppView sharedTJCFeaturedAppView];
    [wrapper showContent:featured fromController:controller frame:frame];
    if ([featured superview] != nil) {
        [featured loadViewWithURL:address];
    }
}
@end

@implementation TapjoyConnect (TJCFeaturedAppViewHandler)
+ (void)getFeaturedApp
{
    [[TJCFeaturedAppManager sharedTJCFeaturedAppManager] getFeaturedApp];
}

+ (void)getFeaturedAppWithCurrencyID:(NSString*)currencyID
{
    TJCFeaturedAppView* view = [TJCFeaturedAppView sharedTJCFeaturedAppView];
    view.currencyID_ = currencyID;
    [[TJCFeaturedAppManager sharedTJCFeaturedAppManager] getFeaturedAppWithCurrencyID:currencyID];
}

+ (void)showFeaturedAppFullScreenAd
{
    TJCFeaturedAppModel* app = [[TJCFeaturedAppManager sharedTJCFeaturedAppManager] featuredAppModelObj];
    [TJCFeaturedAppViewHandler showFullScreenAdWithURL:[app fullScreenAdURL]];
}

+ (void)showFeaturedAppFullScreenAdWithFrame:(CGRect)frame
{
    TJCFeaturedAppModel* app = [[TJCFeaturedAppManager sharedTJCFeaturedAppManager] featuredAppModelObj];
    [TJCFeaturedAppViewHandler showFullScreenAdWithURL:[app fullScreenAdURL] withFrame:frame];
}

+ (void)showFeaturedAppFullScreenAdWithViewController:(UIViewController*)controller
{
    TJCFeaturedAppModel* app = [[TJCFeaturedAppManager sharedTJCFeaturedAppManager] featuredAppModelObj];
    [TJCFeaturedAppViewHandler showFullScreenAdWithURL:[app fullScreenAdURL] withViewController:controller];
}

+ (void)setFeaturedAppDisplayCount:(int)count
{
    [[TJCFeaturedAppManager sharedTJCFeaturedAppManager] setFeaturedAppDisplayCount:count];
}
@end

@implementation TJCCallsWrapper (TJCFeaturedAppViewHandler)
- (void)showFeaturedAppFullScreenAdWithURL:(NSString*)address
{
    [TJCFeaturedAppViewHandler showFullScreenAdWithURL:address];
}

- (void)showFeaturedAppFullScreenAd:(NSString*)address withFrame:(CGRect)frame
{
    [TJCFeaturedAppViewHandler showFullScreenAdWithURL:address withFrame:frame];
}

- (void)showFeaturedAppFullScreenAd:(NSString*)address withViewController:(UIViewController*)controller
{
    [TJCFeaturedAppViewHandler showFullScreenAdWithURL:address withViewController:controller];
}
@end
