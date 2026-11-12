#import "TapjoyConnect.h"
#import "TJCVideoManager.h"
#import "TJCVideoView.h"

@interface TJCVideoViewHandler : NSObject
+ (void)showVideoAdWithFrame:(CGRect)frame offerID:(NSString*)offerID;
+ (void)showVideoAdWithOfferID:(NSString*)offerID;
+ (void)showVideoAdWithViewController:(UIViewController*)controller offerID:(NSString*)offerID;
@end

@implementation TJCVideoViewHandler
+ (void)showVideoAdWithFrame:(CGRect)frame offerID:(NSString*)offerID
{
    TJCVideoView* view = [[TJCVideoManager sharedTJCVideoManager] videoView];
    [view refreshViewWithFrame:frame];
    [view playVideoWithOfferID:offerID];
}

+ (void)showVideoAdWithOfferID:(NSString*)offerID
{
    [self showVideoAdWithFrame:CGRectZero offerID:offerID];
}

+ (void)showVideoAdWithViewController:(UIViewController*)controller offerID:(NSString*)offerID
{
    TJCVideoView* view = [[TJCVideoManager sharedTJCVideoManager] videoView];
    [view refreshViewWithViewController:controller];
    [view playVideoWithOfferID:offerID];
}
@end

@implementation TapjoyConnect (TJCVideoViewHandler)
+ (void)initVideoAdWithDelegate:(id<TJCVideoAdDelegate>)delegate
{
    [[TJCVideoManager sharedTJCVideoManager] initVideoAdsWithDelegate:delegate];
}

+ (void)setVideoCacheCount:(int)count
{
    [[TJCVideoManager sharedTJCVideoManager] setVideoCacheCount:count];
}

+ (void)showVideoAdWithFrame:(CGRect)frame offerID:(NSString*)offerID
{
    [TJCVideoViewHandler showVideoAdWithFrame:frame offerID:offerID];
}

+ (void)showVideoAdWithOfferID:(NSString*)offerID
{
    [TJCVideoViewHandler showVideoAdWithOfferID:offerID];
}

+ (void)showVideoAdWithViewController:(UIViewController*)controller offerID:(NSString*)offerID
{
    [TJCVideoViewHandler showVideoAdWithViewController:controller offerID:offerID];
}
@end
