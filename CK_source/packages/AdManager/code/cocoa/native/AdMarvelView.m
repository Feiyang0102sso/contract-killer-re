#import "AdMarvelView.h"
#import "TJCCoreFetcher.h"
#include "CKAdsCompat.h"

@implementation AdMarvelView
+ (AdMarvelView*)createAdMarvelViewWithDelegate:(id<AdMarvelDelegate>)delegate prefetchAd:(BOOL)prefetch
{
    CGRect frame = CGRectMake(0, 0, 320.0f, 50.0f);
    if ([delegate respondsToSelector:@selector(adMarvelViewFrame)]) {
        frame = [delegate adMarvelViewFrame];
    }
    AdMarvelView* view = [[[self alloc] initWithFrame:frame] autorelease];
    [view setDelegate:delegate];
    view.hidden = YES;
    if (prefetch) {
        [view performSelector:@selector(getAdWithNotification) withObject:nil afterDelay:0];
    }
    return view;
}

+ (AdMarvelView*)createAdMarvelViewWithDelegate:(id<AdMarvelDelegate>)delegate
{
    AdMarvelView* view = [self createAdMarvelViewWithDelegate:delegate prefetchAd:NO];
    view->interstitial_ = YES;
    return view;
}

- (void)setDelegate:(id<AdMarvelDelegate>)delegate
{
    delegate_ = delegate;
    if (delegate == nil) {
        [NSObject cancelPreviousPerformRequestsWithTarget:self];
        [request_ cancel];
        [request_ release];
        request_ = nil;
        [self closePresentation];
    }
}

- (void)getAdWithNotification
{
    if (!interstitial_) {
        [self requestCreative];
    }
}

- (void)getInterstitialAd
{
    if (interstitial_ && ![self isInFullScreenMode]) {
        [self requestCreative];
    }
}

- (void)requestCreative
{
    if (delegate_ == nil || request_ != nil) {
        return;
    }
#if !CK_ADS_COMPAT
    // Replacement layer switched off: no gateway request, report "no ad" through the original callback.
    [self notifyFailure];
    return;
#endif

    // The Glu delegate consumes its current request kind when asked for the site ID.
    NSString* site = [delegate_ siteId];
    NSString* partner = [delegate_ partnerId];
    NSString* key = @"CKAdMarvelBannerURL";
    if (interstitial_) {
        key = @"CKAdMarvelInterstitialURL";
    }
    id address = [[NSBundle mainBundle] objectForInfoDictionaryKey:key];
    if (![address isKindOfClass:[NSString class]] || !TJCValidHTTPSURL([NSURL URLWithString:address])) {
        [self notifyFailure];
        return;
    }

    request_ = [[TJCCoreFetcher alloc] init];
    request_.baseURL = address;
    NSMutableDictionary* params = [NSMutableDictionary dictionary];
    [params setValue:site forKey:@"site_id"];
    [params setValue:partner forKey:@"partner_id"];
    request_.bindings = params;
    SEL selector = @selector(creativeReceived:);
    NSInvocation* completion = [NSInvocation invocationWithMethodSignature:[self methodSignatureForSelector:selector]];
    [completion setTarget:self];
    [completion setSelector:selector];
    [request_ fetchAsynchronouslyWithCompletionInvocation:completion];
}

- (void)creativeReceived:(TJCCoreFetcher*)fetcher
{
    if (fetcher != request_) {
        return;
    }
    [request_ release];
    request_ = nil;
    UIImage* creative = nil;
    if (![fetcher hasError] && [[fetcher data] length] != 0) {
        creative = [UIImage imageWithData:[fetcher data]];
    }
    if (creative == nil) {
        [self notifyFailure];
        return;
    }
    [creative_ release];
    creative_ = [creative retain];
    NSHTTPURLResponse* response = (NSHTTPURLResponse*)[fetcher response];
    NSDictionary* headers = [response allHeaderFields];
    [clickURL_ release];
    clickURL_ = nil;
    for (NSString* field in headers) {
        if ([field caseInsensitiveCompare:@"X-AdMarvel-Click-URL"] == NSOrderedSame) {
            id target = [headers objectForKey:field];
            if ([target isKindOfClass:[NSString class]]) {
                NSURL* url = [NSURL URLWithString:target];
                if (TJCValidHTTPSURL(url)) {
                    clickURL_ = [url retain];
                }
            }
            break;
        }
    }

    if (interstitial_) {
        if ([delegate_ respondsToSelector:@selector(getInterstitialAdSucceeded)]) {
            [delegate_ getInterstitialAdSucceeded];
        }
    } else {
        if (imageView_ == nil) {
            imageView_ = [[UIImageView alloc] initWithFrame:self.bounds];
            imageView_.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
            imageView_.contentMode = UIViewContentModeScaleAspectFit;
            [self addSubview:imageView_];
        }
        imageView_.image = creative_;
        if ([delegate_ respondsToSelector:@selector(getAdSucceeded)]) {
            [delegate_ getAdSucceeded];
        }
    }
}

- (void)notifyFailure
{
    if (interstitial_) {
        if ([delegate_ respondsToSelector:@selector(getInterstitialAdFailed)]) {
            [delegate_ getInterstitialAdFailed];
        }
    } else if ([delegate_ respondsToSelector:@selector(getAdFailed)]) {
        [delegate_ getAdFailed];
    }
}

- (BOOL)isInFullScreenMode
{
    return presentation_ != nil;
}

- (BOOL)isInterstitialReady
{
    return interstitial_ && creative_ != nil && presentation_ == nil;
}

- (void)displayInterstitial
{
    if (![self isInterstitialReady] || ![self openPresentation]) {
        return;
    }
    UIImageView* image = [[[UIImageView alloc] initWithImage:creative_] autorelease];
    image.frame = presentation_.view.bounds;
    image.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    image.contentMode = UIViewContentModeScaleAspectFit;
    if (clickURL_ != nil) {
        image.userInteractionEnabled = YES;
        UITapGestureRecognizer* gesture = [[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(openClickURL)] autorelease];
        [image addGestureRecognizer:gesture];
    }
    [presentation_.view insertSubview:image atIndex:0];
    [creative_ release];
    creative_ = nil;
    if ([delegate_ respondsToSelector:@selector(interstitialActivated)]) {
        [delegate_ interstitialActivated];
    }
}

- (BOOL)openPresentation
{
    if (presentation_ != nil) {
        return YES;
    }
    UIViewController* controller = [delegate_ applicationUIViewController];
    if (controller == nil || [controller modalViewController] != nil) {
        return NO;
    }
    presentation_ = [[UIViewController alloc] init];
    presentation_.view.backgroundColor = [UIColor blackColor];
    presentingController_ = controller;
    UIButton* close = [UIButton buttonWithType:UIButtonTypeRoundedRect];
    close.frame = CGRectMake(8.0f, 8.0f, 72.0f, 34.0f);
    [close setTitle:@"Close" forState:UIControlStateNormal];
    [close addTarget:self action:@selector(dismissAd) forControlEvents:UIControlEventTouchUpInside];
    [presentation_.view addSubview:close];
    [controller presentModalViewController:presentation_ animated:YES];
    return YES;
}

- (void)openClickURL
{
    if (clickURL_ == nil) {
        return;
    }
    if ([delegate_ respondsToSelector:@selector(handleAdMarvelSDKClick:)]) {
        [delegate_ handleAdMarvelSDKClick:[clickURL_ absoluteString]];
    }
    if (interstitial_) {
        [[UIApplication sharedApplication] openURL:clickURL_];
        return;
    }
    if (![self openPresentation]) {
        return;
    }
    UIWebView* browser = [[[UIWebView alloc] initWithFrame:presentation_.view.bounds] autorelease];
    browser.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    browser.scalesPageToFit = YES;
    [presentation_.view insertSubview:browser atIndex:0];
    [browser loadRequest:[NSURLRequest requestWithURL:clickURL_]];
    if ([delegate_ respondsToSelector:@selector(fullScreenWebViewActivated)]) {
        [delegate_ fullScreenWebViewActivated];
    }
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
    UITouch* touch = [touches anyObject];
    if (clickURL_ != nil && CGRectContainsPoint(self.bounds, [touch locationInView:self])) {
        [self openClickURL];
    }
}

- (void)dismissAd
{
    [self closePresentation];
    if (interstitial_) {
        if ([delegate_ respondsToSelector:@selector(interstitialClosed)]) {
            [delegate_ interstitialClosed];
        }
    } else if ([delegate_ respondsToSelector:@selector(fullScreenWebViewClosed)]) {
        [delegate_ fullScreenWebViewClosed];
    }
}

- (void)closePresentation
{
    if (presentation_ != nil) {
        [presentingController_ dismissModalViewControllerAnimated:NO];
        [presentation_ release];
        presentation_ = nil;
        presentingController_ = nil;
    }
}

- (void)dealloc
{
    [request_ cancel];
    [request_ release];
    [self closePresentation];
    [creative_ release];
    [imageView_ release];
    [clickURL_ release];
    [super dealloc];
}
@end
