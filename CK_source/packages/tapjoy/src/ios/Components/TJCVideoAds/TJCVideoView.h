#pragma once
#import "TapjoyConnect.h"
#import <MediaPlayer/MediaPlayer.h>

@class TJCLoadingView;
@class TJCVideoObject;
@class TJCVideoLayer;

@interface TJCVideoView : NSObject {
    id<TJCVideoAdDelegate> adDelegate_;
    BOOL isVideoReady_;
    TJCLoadingView* loadingView_;
    BOOL isAlertShowing_;
    UIView* mainView_;
    MPMoviePlayerController* mainVideoLayer_;
    TJCVideoLayer* completionLayer_;
    id iPadLayer_;
    UIView* parentView_;
    CGRect requestedFrame_;
    TJCVideoObject* activeVideo_;
    BOOL playing_;
    BOOL finishedWatching_;
}
@property(nonatomic, assign) BOOL isVideoReady;
@property(nonatomic, retain) UIView* mainView;
- (id)initWithDelegate:(id<TJCVideoAdDelegate>)delegate;
- (void)setAdDelegate:(id<TJCVideoAdDelegate>)delegate;
- (void)refreshViewWithViewController:(UIViewController*)controller;
- (void)refreshViewWithFrame:(CGRect)frame;
- (void)refreshViewWithInterfaceOrientation:(UIInterfaceOrientation)orientation;
- (void)playVideoWithOfferID:(NSString*)offerID;
- (BOOL)videoAdCurrentlyPlaying;
- (void)closeVideoView;
@end
