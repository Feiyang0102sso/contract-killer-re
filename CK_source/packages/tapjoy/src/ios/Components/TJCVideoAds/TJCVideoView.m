#import "TJCVideoView.h"
#import "TJCLoadingView.h"
#import "TJCVideoManager.h"
#import "TJCVideoLayer.h"
#import "TJCVideoObject.h"
#import "TJCCoreFetcher.h"

@implementation TJCVideoView
@synthesize mainView = mainView_, isVideoReady = isVideoReady_;

- (id)initWithDelegate:(id<TJCVideoAdDelegate>)delegate
{
    self = [super init];
    if (self) {
        adDelegate_ = delegate;
        mainView_ = [[UIView alloc] initWithFrame:CGRectZero];
        mainView_.backgroundColor = [UIColor blackColor];
        mainView_.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    }
    return self;
}

- (void)setAdDelegate:(id<TJCVideoAdDelegate>)delegate
{
    adDelegate_ = delegate;
}

- (void)refreshViewWithViewController:(UIViewController*)controller
{
    if (controller != nil) {
        parentView_ = [controller view];
    } else {
        parentView_ = [[UIApplication sharedApplication] keyWindow];
    }
    requestedFrame_ = CGRectZero;
}

- (void)refreshViewWithFrame:(CGRect)frame
{
    parentView_ = [[UIApplication sharedApplication] keyWindow];
    requestedFrame_ = frame;
}

- (void)refreshViewWithInterfaceOrientation:(UIInterfaceOrientation)orientation
{
    if (parentView_ != nil && CGRectIsEmpty(requestedFrame_)) {
        mainView_.frame = parentView_.bounds;
    }
}

- (void)playVideoWithOfferID:(NSString*)offerID
{
    if (playing_ || parentView_ == nil) {
        return;
    }
    TJCVideoManager* manager = [TJCVideoManager sharedTJCVideoManager];
    TJCVideoObject* video = [manager videoForOfferID:offerID];
    NSURL* url = nil;
    if ([[video videoURL] length] != 0) {
        url = [NSURL URLWithString:[video videoURL]];
    }
    NSString* cachedFile = [video dataLocation];
    if ([cachedFile length] != 0 && [[NSFileManager defaultManager] fileExistsAtPath:cachedFile]) {
        url = [NSURL fileURLWithPath:cachedFile];
    }
    if (video == nil || (!TJCValidHTTPSURL(url) && ![url isFileURL])) {
        if ([adDelegate_ respondsToSelector:@selector(videoAdError:)]) {
            [adDelegate_ videoAdError:@"Tapjoy video is unavailable."];
        }
        return;
    }
    activeVideo_ = [video retain];
    [self bufferVideoWithPath:[url absoluteString]];
}

- (void)bufferVideoWithPath:(NSString*)path
{
    mainVideoLayer_ = [[MPMoviePlayerController alloc] initWithContentURL:[NSURL URLWithString:path]];
    if (mainVideoLayer_ == nil) {
        [activeVideo_ release];
        activeVideo_ = nil;
        return;
    }
    mainVideoLayer_.controlStyle = MPMovieControlStyleNone;
    mainVideoLayer_.scalingMode = MPMovieScalingModeAspectFit;
    mainVideoLayer_.shouldAutoplay = NO;
    if (CGRectIsEmpty(requestedFrame_)) {
        mainView_.frame = parentView_.bounds;
    } else {
        mainView_.frame = requestedFrame_;
    }

    UIView* playerView = [mainVideoLayer_ view];
    playerView.frame = mainView_.bounds;
    playerView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [mainView_ addSubview:playerView];

    loadingView_ = [[TJCLoadingView alloc] initWithFrame:mainView_.bounds];
    loadingView_.mainView = mainView_;
    [mainView_ addSubview:loadingView_];
    [loadingView_ enable];

    UIButton* close = [UIButton buttonWithType:UIButtonTypeRoundedRect];
    close.frame = CGRectMake(MAX(0.0f, CGRectGetWidth(mainView_.bounds) - 76.0f), 6.0f, 68.0f, 34.0f);
    close.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin;
    [close setTitle:@"Close" forState:UIControlStateNormal];
    [close addTarget:self action:@selector(closeVideoView) forControlEvents:UIControlEventTouchUpInside];
    [mainView_ addSubview:close];
    [parentView_ addSubview:mainView_];

    NSNotificationCenter* notifications = [NSNotificationCenter defaultCenter];
    [notifications addObserver:self selector:@selector(loadStateChanged:)
                           name:MPMoviePlayerLoadStateDidChangeNotification object:mainVideoLayer_];
    [notifications addObserver:self selector:@selector(playbackFinished:)
                           name:MPMoviePlayerPlaybackDidFinishNotification object:mainVideoLayer_];
    [notifications addObserver:self selector:@selector(cancelVideo:)
                           name:UIApplicationDidEnterBackgroundNotification object:nil];
    playing_ = YES;
    [self retain];
    if ([adDelegate_ respondsToSelector:@selector(videoAdBegan)]) {
        [adDelegate_ videoAdBegan];
    }
    [mainVideoLayer_ prepareToPlay];
    [self playActiveVideo];
    [self release];
}

- (void)loadStateChanged:(NSNotification*)notification
{
    if (([mainVideoLayer_ loadState] & MPMovieLoadStatePlayable) != 0) {
        isVideoReady_ = YES;
        [loadingView_ disable];
    }
}

- (void)playActiveVideo
{
    [mainVideoLayer_ play];
}

- (void)stopActiveVideo
{
    [mainVideoLayer_ stop];
}

- (BOOL)videoAdCurrentlyPlaying
{
    return playing_;
}

- (void)playbackFinished:(NSNotification*)notification
{
    if (!playing_ || finishedWatching_) {
        return;
    }
    [self retain];
    NSNumber* result = [[notification userInfo] objectForKey:MPMoviePlayerPlaybackDidFinishReasonUserInfoKey];
    if ([result isKindOfClass:[NSNumber class]] && [result integerValue] == MPMovieFinishReasonPlaybackEnded) {
        finishedWatching_ = YES;
        [loadingView_ disable];
        completionLayer_ = [[TJCVideoLayer alloc] initWithVideo:activeVideo_ target:self dismissAction:@selector(closeVideoView)];
        completionLayer_.frame = mainView_.bounds;
        [mainView_ addSubview:completionLayer_];
        [completionLayer_ enableShowVideoCompleteScreen:YES];
        [[TJCVideoManager sharedTJCVideoManager] reportCompletedOffer:[activeVideo_ offerID]];
        if ([adDelegate_ respondsToSelector:@selector(videoAdFinished)]) {
            [adDelegate_ videoAdFinished];
        }
        [self release];
        return;
    } else if ([result isKindOfClass:[NSNumber class]] && [result integerValue] == MPMovieFinishReasonPlaybackError &&
               [adDelegate_ respondsToSelector:@selector(videoAdError:)]) {
        [adDelegate_ videoAdError:@"Tapjoy video could not be played."];
    }
    [self closeVideoView];
    [self release];
}

- (void)cancelVideo:(NSNotification*)notification
{
    [self closeVideoView];
}

- (void)closeVideoView
{
    if (!playing_) {
        return;
    }
    playing_ = NO;
    finishedWatching_ = NO;
    isVideoReady_ = NO;
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [self stopActiveVideo];
    for (UIView* view in [mainView_ subviews]) {
        [view removeFromSuperview];
    }
    [mainView_ removeFromSuperview];
    [mainVideoLayer_ release];
    mainVideoLayer_ = nil;
    [loadingView_ release];
    loadingView_ = nil;
    [completionLayer_ release];
    completionLayer_ = nil;
    [activeVideo_ release];
    activeVideo_ = nil;
    if ([adDelegate_ respondsToSelector:@selector(videoAdClosed)]) {
        [adDelegate_ videoAdClosed];
    }
}

- (void)dealloc
{
    adDelegate_ = nil;
    [self closeVideoView];
    [mainView_ release];
    [super dealloc];
}
@end
