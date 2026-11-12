// GGNWebViewController.m - one Glu Games Network web page (UIWebView with a loading spinner).
// Pages are loaded with the device/game parameters appended; App Store links leave the app.
#import "GGNWebViewController.h"
#import "GluGamesNetworkController.h"

@implementation GGNWebViewController

- (id) initWithStartUrl: (NSString*) url
{
    self = [super init];
    if (self)
    {
        defaultUrl = [[NSString alloc] initWithString: url];
        if ([[UIDevice currentDevice] respondsToSelector: @selector(userInterfaceIdiom)] &&
            [[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad)
        {
            platformString = [[NSString alloc] initWithString: @"ipad"];
        }
        else
        {
            platformString = [[NSString alloc] initWithString: @"iphone"];
        }
    }
    return self;
}

- (void) reloadWithUrl: (NSString*) url
{
    [self stopLoading];
    [self setDefaultUrl: url];
    isLoaded = NO;
    loadingErrorCode = 0;
    [self startLoading];
}

- (void) loadView
{
    CGRect frame = [[UIScreen mainScreen] bounds];

    webView = [[UIWebView alloc] initWithFrame: frame];
    webView.backgroundColor = [UIColor darkGrayColor];
    [webView setAutoresizingMask: UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];
    if ([webView respondsToSelector: @selector(allowsInlineMediaPlayback)])
    {
        [webView setAllowsInlineMediaPlayback: YES];
    }

    // 40x40 spinner in the middle of the screen
    CGRect spinnerFrame = CGRectMake((frame.size.width - 40) / 2, (frame.size.height - 40) / 2, 40, 40);
    loadingSpinner = [[UIActivityIndicatorView alloc] initWithFrame: spinnerFrame];
    [loadingSpinner setActivityIndicatorViewStyle: UIActivityIndicatorViewStyleWhiteLarge];
    [loadingSpinner setAutoresizingMask: UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin |
                                         UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin];
    [webView addSubview: loadingSpinner];
    [loadingSpinner release];

    [self setView: webView];
}

- (void) viewDidUnload
{
    [super viewDidUnload];
    [webView setDelegate: nil];
    [webView release];
    webView = nil;
}

- (void) viewWillAppear: (BOOL) animated
{
    [super viewWillAppear: animated];
    [webView setDelegate: self];
    if (![self isComplete])
    {
        [self startLoading];
    }
}

- (void) viewWillDisappear: (BOOL) animated
{
    [self stopLoading];
    // Stop any inline video still playing
    [webView stringByEvaluatingJavaScriptFromString: @"document.getElementsByTagName(\"video\")[0].pause();"];
    [webView setDelegate: nil];
    [super viewWillDisappear: animated];
}

- (void) dealloc
{
    [platformString release];
    [defaultUrl release];
    [iTunesURL release];
    [super dealloc];
}

- (void) webViewDidStartLoad: (UIWebView*) aWebView
{
    [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible: YES];
    [loadingSpinner startAnimating];
}

- (void) webViewDidFinishLoad: (UIWebView*) aWebView
{
    // Keep WebKit from caching pages
    [[NSUserDefaults standardUserDefaults] setInteger: 0 forKey: @"WebKitCacheModelPreferenceKey"];
    [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible: NO];
    [loadingSpinner stopAnimating];

    // -999 is NSURLErrorCancelled
    if (loadingErrorCode != -999 && loadingErrorCode != 0)
    {
        [aWebView reload];
    }
    else
    {
        isLoaded = YES;
    }
    loadingErrorCode = 0;
}

- (void) webView: (UIWebView*) aWebView didFailLoadWithError: (NSError*) error
{
    loadingErrorCode = [error code];
    isLoaded = NO;
    [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible: NO];
    [loadingSpinner stopAnimating];

    if (loadingErrorCode != -999)
    {
        NSLog(@"[GGNWebViewController] loading error %@", [error localizedDescription]);
        [self stopLoading];
        UIAlertView* alert = [[UIAlertView alloc] initWithTitle: @"Network error"
                                                        message: @"You are not connected to the network. Please check your connection and try again."
                                                       delegate: nil
                                              cancelButtonTitle: @"OK"
                                              otherButtonTitles: nil];
        [alert show];
        [alert release];
    }
}

- (BOOL) webView: (UIWebView*) aWebView shouldStartLoadWithRequest: (NSURLRequest*) request navigationType: (UIWebViewNavigationType) navigationType
{
    NSString* urlString = [[request URL] absoluteString];
    NSLog(@"[GGN] Loading URL string %@", urlString);

    if (navigationType == UIWebViewNavigationTypeLinkClicked)
    {
        if ([urlString rangeOfString: @"itunes.apple.com"].location != NSNotFound)
        {
            [[UIApplication sharedApplication] openURL: [NSURL URLWithString: urlString]];
            return NO;
        }
        if ([urlString rangeOfString: @"gworld_logout"].location != NSNotFound)
        {
            // Logged out: clear the badge of the third tab
            [[[[[self tabBarController] tabBar] items] objectAtIndex: 2] setBadgeValue: nil];
        }
    }
    else if (navigationType == UIWebViewNavigationTypeOther)
    {
        if ([urlString rangeOfString: @"glu.com"].location == NSNotFound)
        {
            // Foreign page: resolve it through a connection (App Store redirects end in connectionDidFinishLoading:)
            NSURLConnection* connection = [[NSURLConnection alloc] initWithRequest: request delegate: self startImmediately: YES];
            [connection release];
            return NO;
        }
        if ([urlString rangeOfString: @"exclusive-content"].location != NSNotFound)
        {
            [[self tabBarController] setSelectedIndex: 0];
        }
        else if ([urlString rangeOfString: @"facebook.com/extern/login_status"].location != NSNotFound)
        {
            [[GluGamesNetworkController sharedInstance] checkForUpdatedItems];
        }
    }
    return YES;
}

- (NSURLRequest*) connection: (NSURLConnection*) connection willSendRequest: (NSURLRequest*) request redirectResponse: (NSURLResponse*) redirectResponse
{
    [self setITunesURL: [redirectResponse URL]];
    return request;
}

- (void) connectionDidFinishLoading: (NSURLConnection*) connection
{
    [[UIApplication sharedApplication] openURL: [self iTunesURL]];
    if (isLoaded)
    {
        isLoaded = NO;
    }
}

- (BOOL) isComplete
{
    return isLoaded && loadingErrorCode == 0;
}

- (BOOL) isLoading
{
    return [webView isLoading];
}

- (void) startLoading
{
    isLoaded = NO;

    NSString* url;
    if ([defaultUrl rangeOfString: @"?"].location == NSNotFound)
    {
        url = [defaultUrl stringByAppendingString: @"?"];
    }
    else
    {
        url = defaultUrl;
    }

    NSString* parameters;
    if ([[GluGamesNetworkController sharedInstance] gServeId] != 0)
    {
        NSString* gServeIdString = [NSString stringWithFormat: @"%u", [[GluGamesNetworkController sharedInstance] gServeId]];
        parameters = [[NSString alloc] initWithFormat: @"&udid=%@&src=%@&p=%@&l=%@&gsid=%@",
                      [[UIDevice currentDevice] uniqueIdentifier],
                      [[GluGamesNetworkController sharedInstance] gameId],
                      platformString,
                      [[NSLocale currentLocale] localeIdentifier],
                      gServeIdString];
    }
    else
    {
        parameters = [[NSString alloc] initWithFormat: @"&udid=%@&src=%@&p=%@&l=%@",
                      [[UIDevice currentDevice] uniqueIdentifier],
                      [[GluGamesNetworkController sharedInstance] gameId],
                      platformString,
                      [[NSLocale currentLocale] localeIdentifier]];
    }
    url = [url stringByAppendingString: parameters];
    [parameters release];
    url = [url stringByAddingPercentEscapesUsingEncoding: NSASCIIStringEncoding];

    [webView loadRequest: [NSURLRequest requestWithURL: [NSURL URLWithString: url]]];
}

- (void) stopLoading
{
    if ([webView isLoading])
    {
        [webView stopLoading];
        [loadingSpinner stopAnimating];
    }
    [[UIApplication sharedApplication] setNetworkActivityIndicatorVisible: NO];
}

@synthesize iTunesURL;
@synthesize defaultUrl;

@end
