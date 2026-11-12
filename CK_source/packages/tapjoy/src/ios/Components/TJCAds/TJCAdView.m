#import "TapjoyConnect.h"
#import "TJCAdRequestHandler.h"
#import "TJCUtil.h"

NSString* const kTJCAdFailStr = @"Tapjoy Ad request failed to receive any data.";

static TJCAdView* SharedAdView;

@implementation TJCAdView
@synthesize clickURL = clickURL_, imageDataStr = imageDataStr_, imageView = imageView_;
@synthesize previousImageView = previousImageView_, adViewOverlay = adViewOverlay_;
@synthesize adHandlerObj = adHandlerObj_, adDelegate = adDelegate_;

+ (TJCAdView*)sharedTJCAdView
{
    if (SharedAdView == nil) {
        SharedAdView = [[self alloc] init];
    }
    return SharedAdView;
}

+ (void)releaseSharedTJCAdView
{
    [SharedAdView detach];
    [SharedAdView release];
    SharedAdView = nil;
}

- (id)init
{
    self = [super initWithFrame:CGRectMake(0, 0, 320, 50)];
    if (self) {
        self.hidden = YES;
        contentSizeStr_ = [TJC_AD_BANNERSIZE_320X50 copy];
        adHandlerObj_ = [[TJCAdRequestHandler alloc] initRequestWithDelegate:self andRequestTag:0];
    }
    return self;
}

- (void)getAdWithDelegate:(id<TJCAdDelegate>)delegate
{
    [self getAdWithDelegate:delegate currencyID:nil];
}

- (void)getAdWithDelegate:(id<TJCAdDelegate>)delegate currencyID:(NSString*)currencyID
{
    self.adDelegate = delegate;
    NSString* size = [delegate adContentSize];
    CGRect frame = [self frame];
    if ([size isEqualToString:TJC_AD_BANNERSIZE_768X90]) {
        frame.size = CGSizeMake(768.0f, 90.0f);
    } else if ([size isEqualToString:@"640x100"]) {
        frame.size = CGSizeMake(640.0f, 100.0f);
    } else {
        size = TJC_AD_BANNERSIZE_320X50;
        frame.size = CGSizeMake(320.0f, 50.0f);
    }
    [contentSizeStr_ release];
    contentSizeStr_ = [size copy];
    self.frame = frame;
    [self refreshAdWithCurrencyID:currencyID];
}

- (void)refreshAd:(NSTimer*)timer
{
    [self refreshAdWithCurrencyID:nil];
}

- (void)refreshAdWithCurrencyID:(NSString*)currencyID
{
    if (adDelegate_ != nil) {
        [adHandlerObj_ requestAdWithSize:contentSizeStr_ currencyID:currencyID];
    }
}

- (BOOL)isAdLoaded
{
    return imageView_ != nil && [adHandlerObj_ isDataFetchSuccessful];
}

- (void)fetchResponseSuccessWithData:(NSArray*)data withRequestTag:(NSInteger)tag
{
    if ([data count] < 2) {
        [self fetchResponseError:1 errorDescription:nil requestTag:tag];
        return;
    }
    NSString* encoded = [data objectAtIndex:1];
    UIImage* image = [UIImage imageWithData:[TJCUtil dataWithBase64EncodedString:encoded]];
    if (image == nil) {
        [self fetchResponseError:1 errorDescription:nil requestTag:tag];
        return;
    }

    self.clickURL = [data objectAtIndex:0];
    self.imageDataStr = encoded;
    [imageView_ removeFromSuperview];
    UIImageView* view = [[[UIImageView alloc] initWithImage:image] autorelease];
    view.frame = self.bounds;
    view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.imageView = view;
    [self addSubview:view];
    if ([adDelegate_ respondsToSelector:@selector(didReceiveAd:)]) {
        [adDelegate_ didReceiveAd:self];
    } else if ([adDelegate_ respondsToSelector:@selector(didRecieveAd:)]) {
        [adDelegate_ didRecieveAd:self];
    }

    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(refreshAd:) object:nil];
    if ([adDelegate_ shouldRefreshAd]) {
        [self performSelector:@selector(refreshAd:) withObject:nil afterDelay:60.0];
    }
}

- (void)fetchResponseError:(NSInteger)error errorDescription:(NSString*)description requestTag:(NSInteger)tag
{
    [imageView_ removeFromSuperview];
    self.imageView = nil;
    self.clickURL = nil;
    self.hidden = YES;
    if ([description length] == 0) {
        description = kTJCAdFailStr;
    }
    [adDelegate_ didFailWithMessage:description];
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
    if (![self isAdLoaded]) {
        return;
    }
    UITouch* touch = [touches anyObject];
    if (!CGRectContainsPoint(self.bounds, [touch locationInView:self])) {
        return;
    }
    NSURL* url = [NSURL URLWithString:clickURL_];
    if (TJCValidHTTPSURL(url)) {
        [[UIApplication sharedApplication] openURL:url];
    }
}

- (void)detach
{
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    [adHandlerObj_ cancel];
    adDelegate_ = nil;
    [self removeFromSuperview];
}

- (void)dealloc
{
    [adHandlerObj_ cancel];
    [clickURL_ release];
    [imageDataStr_ release];
    [imageView_ release];
    [previousImageView_ release];
    [contentSizeStr_ release];
    [adViewOverlay_ release];
    [adHandlerObj_ release];
    [super dealloc];
}
@end

@implementation TapjoyConnect (TJCAdView)
+ (void)getDisplayAdWithDelegate:(id<TJCAdDelegate>)delegate
{
    [[TJCAdView sharedTJCAdView] getAdWithDelegate:delegate];
}

+ (void)getDisplayAdWithDelegate:(id<TJCAdDelegate>)delegate currencyID:(NSString*)currencyID
{
    [[TJCAdView sharedTJCAdView] getAdWithDelegate:delegate currencyID:currencyID];
}

+ (void)refreshDisplayAd
{
    [[TJCAdView sharedTJCAdView] refreshAd:nil];
}

+ (void)refreshDisplayAdWithCurrencyID:(NSString*)currencyID
{
    [[TJCAdView sharedTJCAdView] refreshAdWithCurrencyID:currencyID];
}

+ (BOOL)isDisplayAdLoaded
{
    return [[TJCAdView sharedTJCAdView] isAdLoaded];
}

+ (TJCAdView*)getDisplayAdView
{
    return [TJCAdView sharedTJCAdView];
}
@end
