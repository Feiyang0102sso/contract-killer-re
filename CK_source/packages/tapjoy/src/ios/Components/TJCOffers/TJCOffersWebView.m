#import "TJCOffersWebView.h"
#import "TapjoyConnect.h"
#import "TJCLoadingView.h"
#import "TJCCoreFetcher.h"
#import "TJCVideoManager.h"
#import "TJCUINavigationBarView.h"

static TJCOffersWebView* SharedOffersView;

@implementation TJCOffersWebView
@synthesize currencyID = currencyID_, isSelectorVisible = isSelectorVisible_, parentVController = parentVController_;

+ (TJCOffersWebView*)sharedTJCOffersWebView
{
    if (SharedOffersView == nil) {
        SharedOffersView = [[self alloc] initWithFrame:CGRectZero enableNavBar:YES];
    }
    return SharedOffersView;
}

+ (void)releaseSharedTJCOffersWebView
{
    [SharedOffersView cancelLoading];
    [SharedOffersView release];
    SharedOffersView = nil;
}

- (id)initWithFrame:(CGRect)frame enableNavBar:(BOOL)enable
{
    self = [super initWithFrame:frame];
    if (self) {
        enableNavBar = enable;
        self.alertErrorMessage = @"Service is unreachable. Do you want to try again?";
        [self loadView];
    }
    return self;
}

- (void)loadView
{
    if (navBar_ == nil) {
        navBar_ = [[TJCUINavigationBarView alloc] initWithTitle:@"" withFrame:CGRectZero AtY:0.0f];
        navBar_.autoresizingMask = UIViewAutoresizingFlexibleWidth;
        [navBar_ setNavBarLeftBtnWithTitle:@"Back"];
        [navBar_ addLeftButtonTarget:self action:@selector(closeOfferWall) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:navBar_];
    }
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    CGRect bounds = self.bounds;
    CGFloat top = 0.0f;
    if (enableNavBar) {
        top = 44.0f;
    }
    navBar_.hidden = !enableNavBar;
    navBar_.frame = CGRectMake(0, 0, bounds.size.width, top);
    cWebView_.frame = CGRectMake(0, top, bounds.size.width, MAX(0.0f, bounds.size.height - top));
    loadingView_.frame = cWebView_.frame;
    [self bringSubviewToFront:navBar_];
}

- (void)refreshWithFrame:(CGRect)frame enableNavBar:(BOOL)enable
{
    self.frame = frame;
    enableNavBar = enable;
    [self setNeedsLayout];
}

- (void)setCustomNavBarImage:(UIImage*)image
{
    [navBar_ setCustomBackgroundImage:image];
}

- (NSURL*)setUpOffersURLWithServiceURL:(NSString*)address
{
    if ([address length] == 0) {
        return nil;
    }
    TapjoyConnect* service = [TapjoyConnect sharedTapjoyConnect];
    NSMutableDictionary* params = [NSMutableDictionary dictionaryWithDictionary:[service genericParameters]];
    [params setValue:currencyID_ forKey:@"currency_id"];
    NSArray* videoIDs = [[TJCVideoManager sharedTJCVideoManager] getCachedVideoIDs];
    if ([videoIDs count] != 0) {
        [params setObject:[videoIDs componentsJoinedByString:@","] forKey:@"video_offer_ids"];
    }
    if (isSelectorVisible_) {
        [params setObject:@"true" forKey:@"currency_selector"];
    }
    TJCCoreFetcher* fetcher = [[[TJCCoreFetcher alloc] init] autorelease];
    fetcher.baseURL = address;
    fetcher.bindings = params;
    return [fetcher requestURL];
}

- (void)refreshWebView
{
    TapjoyConnect* service = [TapjoyConnect sharedTapjoyConnect];
    NSURL* url = [self setUpOffersURLWithServiceURL:[service servicePath:@"get_offers/webpage"]];
    if (url == nil) {
        [[TJCCallsWrapper sharedTJCCallsWrapper] hideView];
    } else {
        isViewVisible_ = YES;
        [self loadURLRequest:url];
    }
}

- (void)closeOfferWall
{
    isViewVisible_ = NO;
    [[TJCCallsWrapper sharedTJCCallsWrapper] hideView];
}

- (BOOL)webView:(UIWebView*)webView shouldStartLoadWithRequest:(NSURLRequest*)request navigationType:(UIWebViewNavigationType)type
{
    NSURL* url = [request URL];
    if ([[url scheme] isEqualToString:@"tjvideo"]) {
        NSURL* source = [request mainDocumentURL];
        if (TJCURLHasSameOrigin(source, lastURL_)) {
            for (NSString* field in [[url query] componentsSeparatedByString:@"&"]) {
                if ([field hasPrefix:@"video_id="]) {
                    NSString* value = [field substringFromIndex:[@"video_id=" length]];
                    NSString* offerID = [value stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
                    if ([offerID length] != 0) {
                        [TapjoyConnect showVideoAdWithViewController:[TJCCallsWrapper sharedTJCCallsWrapper] offerID:offerID];
                    }
                    break;
                }
            }
        }
        return NO;
    }
    return [super webView:webView shouldStartLoadWithRequest:request navigationType:type];
}

- (void)alertView:(UIAlertView*)alert clickedButtonAtIndex:(NSInteger)index
{
    if (index == [alert cancelButtonIndex]) {
        [self closeOfferWall];
    } else {
        [self refreshWebView];
    }
}

- (void)dealloc
{
    [navBar_ release];
    [publisherUserID_ release];
    [currencyID_ release];
    [super dealloc];
}
@end
