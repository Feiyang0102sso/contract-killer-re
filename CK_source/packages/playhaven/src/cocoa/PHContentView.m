#import "PHContentView.h"
#import "PHConstants.h"
#import "PHContent.h"
#import "PHContentWebView.h"
#import "PHStringUtil.h"
#import "PlayHavenSDK.h"
#import "SBJsonParser.h"
#import "SBJsonWriter.h"

static const NSTimeInterval TemplateTimeout = 20.0;

@implementation PHContentView
@synthesize content = _content, delegate = _delegate, targetView = _targetView;

- (id)initWithContent:(PHContent*)content
{
    self = [super initWithFrame:CGRectZero];
    if (self) {
        _content = [content retain];
        _webView = [[PHContentWebView alloc] initWithFrame:CGRectZero];
        _webView.delegate = self;
        _webView.hidden = YES;
        _webView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        _webView.backgroundColor = [UIColor clearColor];
        _webView.opaque = NO;
        [self addSubview:_webView];
    }
    return self;
}

- (void)dealloc
{
    [self prepareForReuse];
    [_webView release];
    [_activityView release];
    [_content release];
    [_redirects release];
    [super dealloc];
}

- (void)prepareForReuse
{
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    [PHURLLoader invalidateAllLoadersWithDelegate:self];
    _webView.delegate = nil;
    _webView.target = nil;
    [_webView stopLoading];
    [_activityView stopAnimating];
    _delegate = nil;
    _targetView = nil;
    [_redirects removeAllObjects];
}

- (UIActivityIndicatorView*)activityView
{
    if (_activityView == nil) {
        _activityView = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
        _activityView.autoresizingMask = UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin |
                                         UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
        CGRect bounds = [self bounds];
        _activityView.center = CGPointMake(CGRectGetMidX(bounds), CGRectGetMidY(bounds));
        [self addSubview:_activityView];
    }
    return _activityView;
}

- (void)transformForOrientation:(UIInterfaceOrientation)orientation
{
    _orientation = orientation;
    CGRect frame = [_content frameForOrientation:orientation];
    if (!CGRectIsNull(frame) && !CGRectIsEmpty(frame)) {
        self.frame = frame;
        _webView.frame = self.bounds;
    }
}

- (void)show:(BOOL)animated
{
    _willAnimate = animated;
    [self transformForOrientation:_orientation];
    [_targetView addSubview:self];
    [[self activityView] startAnimating];
    [self loadTemplate];
}

- (void)loadTemplate
{
    NSURL* templateURL = [_content URL];
    if (templateURL == nil) {
        [self dismissWithError:PHRequestError(NSURLErrorBadURL, @"PlayHaven template URL is missing.")];
        return;
    }
    NSURLRequest* request = [NSURLRequest requestWithURL:templateURL];
    [_webView loadRequest:request];
    [self performSelector:@selector(templateTimedOut) withObject:nil afterDelay:TemplateTimeout];
}

- (void)redirectRequest:(NSString*)request toTarget:(id)target action:(SEL)action
{
    if ([request length] == 0 || target == nil || action == NULL) {
        return;
    }
    if (_redirects == nil) {
        _redirects = [[NSMutableDictionary alloc] init];
    }
    NSArray* dispatch = [NSArray arrayWithObjects:[NSValue valueWithNonretainedObject:target],
        NSStringFromSelector(action), nil];
    [_redirects setObject:dispatch forKey:[request lowercaseString]];
}

- (void)dispatchCommand:(NSString*)command context:(NSDictionary*)context callback:(NSString*)callback
{
    NSArray* dispatch = [_redirects objectForKey:command];
    if (dispatch != nil) {
        id target = [[dispatch objectAtIndex:0] nonretainedObjectValue];
        SEL action = NSSelectorFromString([dispatch objectAtIndex:1]);
        NSMethodSignature* signature = [target methodSignatureForSelector:action];
        if ([signature numberOfArguments] == 5) {
            NSInvocation* invocation = [NSInvocation invocationWithMethodSignature:signature];
            [invocation setTarget:target];
            [invocation setSelector:action];
            [invocation setArgument:&context atIndex:2];
            [invocation setArgument:&callback atIndex:3];
            PHContentView* source = self;
            [invocation setArgument:&source atIndex:4];
            [invocation invoke];
        }
        return;
    }

    if ([command isEqualToString:@"ph://dismiss"]) {
        [self performSelector:@selector(handleDismiss:) withObject:context afterDelay:0];
    } else if ([command isEqualToString:@"ph://loadcontext"]) {
        [self handleLoadContext:context callback:callback];
    } else if ([command isEqualToString:@"ph://launch"]) {
        [self handleLaunch:context callback:callback];
    }
}

- (void)templateTimedOut
{
    [self dismissWithError:PHCreateError(PHTemplateLoadError)];
}

- (void)viewDidShow
{
    if (!_dismissing) {
        [_delegate contentViewDidLoad:self];
    }
}

- (void)dismiss:(BOOL)animated
{
    if (_dismissing) {
        return;
    }
    _dismissing = YES;
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    [_activityView stopAnimating];
    if (animated && _loaded) {
        [_webView bounceOutWithTarget:self action:@selector(viewDidDismiss)];
    } else {
        [self viewDidDismiss];
    }
}

- (void)dismissFromButton
{
    [self dismiss:_willAnimate];
}

- (void)dismissWithError:(NSError*)error
{
    if (_dismissing) {
        return;
    }
    _dismissing = YES;
    [self retain];
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    [_delegate contentView:self didFailWithError:error];
    [self release];
}

- (void)viewDidDismiss
{
    [self retain];
    [_delegate contentViewDidDismiss:self];
    [self release];
}

- (BOOL)webView:(UIWebView*)webView shouldStartLoadWithRequest:(NSURLRequest*)request navigationType:(UIWebViewNavigationType)type
{
    if (_dismissing) {
        return NO;
    }
    NSURL* url = [request URL];
    if ([[url scheme] isEqualToString:@"ph"] && [url host] != nil) {
        NSURL* source = [request mainDocumentURL];
        if (!PHURLHasSameOrigin(source, [_content URL]) && _loaded) {
            source = [[webView request] URL];
        }
        if (!PHURLHasSameOrigin(source, [_content URL])) {
            return NO;
        }
        NSDictionary* query = [PHStringUtil dictionaryWithQueryString:[url query]];
        NSString* callback = [query objectForKey:@"callback"];
        NSString* contextJSON = [query objectForKey:@"context"];
        NSDictionary* context = query;
        if ([contextJSON length] != 0) {
            FB_SBJsonParser* parser = [[[FB_SBJsonParser alloc] init] autorelease];
            id decoded = [parser objectWithString:contextJSON];
            if (![decoded isKindOfClass:[NSDictionary class]]) {
                [self sendCallback:callback withResponse:nil error:[NSNumber numberWithInt:1]];
                return NO;
            }
            context = decoded;
        }
        NSString* command = [NSString stringWithFormat:@"ph://%@", [[url host] lowercaseString]];
        [self dispatchCommand:command context:context callback:callback];
        return NO;
    }
    return PHURLHasSameOrigin(url, [_content URL]);
}

- (void)webView:(UIWebView*)webView didFailLoadWithError:(NSError*)error
{
    if (!_loaded && [error code] != NSURLErrorCancelled) {
        [self dismissWithError:error];
    }
}

- (void)webViewDidFinishLoad:(UIWebView*)webView
{
    if (_loaded || _dismissing || [webView isLoading]) {
        return;
    }
    _loaded = YES;
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(templateTimedOut) object:nil];
    [_activityView stopAnimating];
    _webView.hidden = NO;
    [_webView stringByEvaluatingJavaScriptFromString:@"window.PlayHavenDispatchProtocolVersion = 4"];
    if (_willAnimate) {
        [_webView bounceInWithTarget:self action:@selector(viewDidShow)];
    } else {
        [self viewDidShow];
    }
}

- (void)handleLaunch:(NSDictionary*)context callback:(NSString*)callback
{
    id address = [context objectForKey:@"url"];
    if (![address isKindOfClass:[NSString class]] || [address length] == 0) {
        [self sendCallback:callback withResponse:nil error:[NSNumber numberWithInt:1]];
        return;
    }
    PHURLLoader* loader = [[[PHURLLoader alloc] init] autorelease];
    loader.targetURL = [NSURL URLWithString:address];
    loader.delegate = self;
    loader.context = callback;
    [loader open];
}

- (void)handleDismiss:(NSDictionary*)context
{
    id address = [context objectForKey:@"ping"];
    if ([address isKindOfClass:[NSString class]]) {
        NSURL* ping = [NSURL URLWithString:address relativeToURL:PHServiceBaseURL()];
        if (PHURLHasSameOrigin(ping, PHServiceBaseURL())) {
            PHURLLoader* loader = [[[PHURLLoader alloc] init] autorelease];
            loader.opensFinalURLOnDevice = NO;
            loader.targetURL = ping;
            [loader open];
        }
    }
    [self dismiss:_willAnimate];
}

- (void)handleLoadContext:(NSDictionary*)context callback:(NSString*)callback
{
    [_webView stringByEvaluatingJavaScriptFromString:@"window.PlayHavenDispatchProtocolVersion = 4"];
    [self sendCallback:callback withResponse:[_content context] error:nil];
}

- (void)loaderFinished:(PHURLLoader*)loader
{
    [self sendCallback:[loader context] withResponse:[[loader targetURL] absoluteString] error:nil];
}

- (void)loaderFailed:(PHURLLoader*)loader
{
    [self sendCallback:[loader context] withResponse:nil error:[NSNumber numberWithInt:1]];
}

- (void)sendCallback:(NSString*)callback withResponse:(id)response error:(id)error
{
    if ([callback length] == 0) {
        return;
    }
    if (response == nil) {
        response = [NSNull null];
    }
    if (error == nil) {
        error = [NSNull null];
    }
    NSArray* arguments = [NSArray arrayWithObjects:callback, response, error, nil];
    FB_SBJsonWriter* writer = [[[FB_SBJsonWriter alloc] init] autorelease];
    NSString* json = [writer stringWithObject:arguments];
    if (json != nil) {
        NSString* script = [NSString stringWithFormat:
            @"if (typeof window.PlayHavenAPICallback === 'function') { window.PlayHavenAPICallback.apply(window,%@); }", json];
        [_webView stringByEvaluatingJavaScriptFromString:script];
    }
}
@end
