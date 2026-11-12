#import "TJCUIWebPageView.h"
#import "TJCLoadingView.h"
#import "TJCCoreFetcher.h"
#import "TapjoyConnect.h"

@implementation TJCUIWebPageView
@synthesize alertErrorMessage = alertErrorMessage_, lastURL = lastURL_;

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        cWebView_ = [[UIWebView alloc] initWithFrame:self.bounds];
        cWebView_.delegate = self;
        cWebView_.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        cWebView_.scalesPageToFit = YES;
        [self addSubview:cWebView_];

        loadingView_ = [[TJCLoadingView alloc] initWithFrame:self.bounds];
        loadingView_.mainView = self;
        [self addSubview:loadingView_];
    }
    return self;
}

- (id)initWithFrame:(CGRect)frame WithRequestURL:(NSURL*)url
{
    self = [self initWithFrame:frame];
    if (self) {
        [self loadURLRequest:url];
    }
    return self;
}

- (void)setDelegate:(id)delegate
{
    delegate_ = delegate;
}

- (void)loadWebRequest:(NSURLRequest*)request
{
    if (!TJCValidHTTPSURL([request URL])) {
        return;
    }
    self.lastURL = [request URL];
    webPageRendered_ = NO;
    cWebView_.delegate = self;
    [cWebView_ loadRequest:request];
}

- (void)loadURLRequest:(NSURL*)url
{
    [self loadURLRequest:url withTimeOutInterval:20.0];
}

- (void)loadURLRequest:(NSURL*)url withTimeOutInterval:(NSTimeInterval)timeout
{
    if (TJCValidHTTPSURL(url)) {
        NSURLRequest* request = [NSURLRequest requestWithURL:url
            cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:timeout];
        [self loadWebRequest:request];
    }
}

- (NSURL*)appendGenericParamsWithURL:(NSURL*)url
{
    TJCCoreFetcher* fetcher = [[[TJCCoreFetcher alloc] init] autorelease];
    fetcher.baseURL = [url absoluteString];
    fetcher.bindings = [[TapjoyConnect sharedTapjoyConnect] genericParameters];
    return [fetcher requestURL];
}

- (BOOL)webView:(UIWebView*)webView shouldStartLoadWithRequest:(NSURLRequest*)request navigationType:(UIWebViewNavigationType)type
{
    NSURL* url = [request URL];
    if (TJCURLHasSameOrigin(url, lastURL_)) {
        return YES;
    }
    if (type == UIWebViewNavigationTypeLinkClicked && TJCValidHTTPSURL(url)) {
        [[UIApplication sharedApplication] openURL:url];
    }
    return NO;
}

- (void)webViewDidStartLoad:(UIWebView*)webView
{
    [loadingView_ enable];
}

- (void)webViewDidFinishLoad:(UIWebView*)webView
{
    if (![webView isLoading]) {
        webPageRendered_ = YES;
        [loadingView_ disable];
    }
}

- (void)webView:(UIWebView*)webView didFailLoadWithError:(NSError*)error
{
    [loadingView_ disable];
    if ([error code] != NSURLErrorCancelled && !webPageRendered_ &&
        retryAlert_ == nil && [alertErrorMessage_ length] != 0) {
        retryAlert_ = [[UIAlertView alloc] initWithTitle:@"" message:alertErrorMessage_
            delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:@"Retry", nil];
        [retryAlert_ show];
    }
}

- (void)alertView:(UIAlertView*)alert clickedButtonAtIndex:(NSInteger)index
{
    if (index != [alert cancelButtonIndex]) {
        [self loadURLRequest:lastURL_];
    }
}

- (void)alertView:(UIAlertView*)alert didDismissWithButtonIndex:(NSInteger)index
{
    if (alert == retryAlert_) {
        [retryAlert_ release];
        retryAlert_ = nil;
    }
}

- (void)cancelLoading
{
    cWebView_.delegate = nil;
    [cWebView_ stopLoading];
    [loadingView_ disable];
    if (retryAlert_ != nil) {
        retryAlert_.delegate = nil;
        [retryAlert_ dismissWithClickedButtonIndex:[retryAlert_ cancelButtonIndex] animated:NO];
        [retryAlert_ release];
        retryAlert_ = nil;
    }
}

- (void)dealloc
{
    [self cancelLoading];
    [cWebView_ release];
    [alertErrorMessage_ release];
    [lastClickedURL_ release];
    [lastURL_ release];
    [loadingView_ release];
    [super dealloc];
}
@end
