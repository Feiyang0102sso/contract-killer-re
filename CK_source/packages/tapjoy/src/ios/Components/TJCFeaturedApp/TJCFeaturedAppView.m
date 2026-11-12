#import "TJCFeaturedAppView.h"
#import "TJCCoreFetcher.h"
#import "TJCLoadingView.h"
#import "TJCUINavigationBarView.h"
#import "TapjoyConnect.h"

static TJCFeaturedAppView* SharedFeaturedAppView;
static const CGFloat NavBarHeight = 44.0f;

@implementation TJCFeaturedAppView
@synthesize currencyID_, publisherUserID_;

+ (TJCFeaturedAppView*)sharedTJCFeaturedAppView
{
    if (SharedFeaturedAppView == nil) {
        SharedFeaturedAppView = [[self alloc] initWithFrame:CGRectZero];
    }
    return SharedFeaturedAppView;
}

+ (void)releaseSharedTJCFeaturedAppView
{
    [SharedFeaturedAppView release];
    SharedFeaturedAppView = nil;
}

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self != nil) {
        self.alertErrorMessage = @"Service is unreachable. Do you want to try again?";
        [self handleWebViewUI];
    }
    return self;
}

- (void)handleWebViewUI
{
    navigationBar_ = [[TJCUINavigationBarView alloc] initWithTitle:@"" withFrame:CGRectZero AtY:0.0f];
    [navigationBar_ setNavBarLeftBtnWithTitle:@"Back"];
    [navigationBar_ addLeftButtonTarget:self action:@selector(backtoGameAction:) forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:navigationBar_];
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    CGRect bounds = [self bounds];
    navigationBar_.frame = CGRectMake(0.0f, 0.0f, bounds.size.width, NavBarHeight);
    cWebView_.frame = CGRectMake(0.0f, NavBarHeight, bounds.size.width, MAX(0.0f, bounds.size.height - NavBarHeight));
    loadingView_.frame = cWebView_.frame;
}

- (void)refreshWithFrame:(CGRect)frame
{
    self.frame = frame;
    [self setNeedsLayout];
}

- (void)updateViewWithOrientation:(UIInterfaceOrientation)orientation
{
    [self setNeedsLayout];
}

- (NSURL*)setUpFeaturedAdURLWithServiceURL:(NSString*)address
{
    TapjoyConnect* service = [TapjoyConnect sharedTapjoyConnect];
    NSString* serviceAddress = [service servicePath:@"get_offers/fullscreenad"];
    if ([serviceAddress length] == 0) {
        return nil;
    }
    if ([address length] == 0) {
        address = serviceAddress;
    }
    NSURL* serviceURL = [NSURL URLWithString:serviceAddress];
    NSURL* url = [NSURL URLWithString:address];
    if (!TJCValidHTTPSURL(url)) {
        return nil;
    }
    if (!TJCURLHasSameOrigin(url, serviceURL)) {
        return url;
    }
    NSMutableDictionary* parameters = [NSMutableDictionary dictionaryWithDictionary:[service genericParameters]];
    [parameters setValue:currencyID_ forKey:@"currency_id"];
    if ([publisherUserID_ length] != 0) {
        [parameters setObject:publisherUserID_ forKey:@"publisher_user_id"];
    }
    TJCCoreFetcher* fetcher = [[[TJCCoreFetcher alloc] init] autorelease];
    fetcher.baseURL = address;
    fetcher.bindings = parameters;
    return [fetcher requestURL];
}

- (void)loadViewWithURL:(NSString*)address
{
    NSURL* url = [self setUpFeaturedAdURLWithServiceURL:address];
    if (url == nil) {
        [self giveBackNotification];
        return;
    }
    [self loadURLRequest:url];
}

- (BOOL)webView:(UIWebView*)view shouldStartLoadWithRequest:(NSURLRequest*)request navigationType:(UIWebViewNavigationType)type
{
    NSURL* url = [request URL];
    NSString* scheme = [[url scheme] lowercaseString];
    if (type == UIWebViewNavigationTypeLinkClicked && [url host] != nil && [url user] == nil
        && ([scheme isEqualToString:@"itms-apps"] || [scheme isEqualToString:@"itms"])) {
        if ([[UIApplication sharedApplication] openURL:url]) {
            [self giveBackNotification];
        }
        return NO;
    }
    return [super webView:view shouldStartLoadWithRequest:request navigationType:type];
}

- (void)alertView:(UIAlertView*)alert clickedButtonAtIndex:(NSInteger)index
{
    if (index == [alert cancelButtonIndex]) {
        [self giveBackNotification];
    } else {
        [super alertView:alert clickedButtonAtIndex:index];
    }
}

- (void)backtoGameAction:(id)sender
{
    [self giveBackNotification];
}

- (void)giveBackNotification
{
    [[TJCCallsWrapper sharedTJCCallsWrapper] hideView];
}

- (void)dealloc
{
    [publisherUserID_ release];
    [currencyID_ release];
    [navigationBar_ release];
    [super dealloc];
}
@end
