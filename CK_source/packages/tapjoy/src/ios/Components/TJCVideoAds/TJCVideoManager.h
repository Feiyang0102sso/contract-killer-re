#pragma once
#import "TapjoyConnect.h"

@class TJCVideoRequestHandler;
@class TJCVideoView;
@class TJCVideoObject;
@class TJCCoreFetcher;

@interface TJCVideoManager : NSObject {
    TJCVideoRequestHandler* requestHandler_;
    TJCVideoView* videoView_;
    NSURLConnection* connection_;
    NSMutableData* videoData_;
    int videoCacheCount_;
    NSUInteger downloadIndex_;
    NSMutableArray* unCachedVideoObjects_;
    UIInterfaceOrientation currentOrientation_;
    NSMutableDictionary* availableVideos_;
    NSMutableDictionary* cachedVideos_;
    TJCCoreFetcher* videoFetcher_;
    TJCVideoRequestHandler* completionHandler_;
    NSMutableArray* pendingCompletions_;
    NSString* cacheDirectory_;
}
@property(nonatomic, retain) TJCVideoView* videoView;
@property(nonatomic, retain) TJCVideoRequestHandler* requestHandler;
@property(nonatomic, assign) UIInterfaceOrientation currentOrientation;
+ (TJCVideoManager*)sharedTJCVideoManager;
+ (void)releaseSharedTJCVideoManager;
- (void)initVideoAdsWithDelegate:(id<TJCVideoAdDelegate>)delegate;
- (NSArray*)getCachedVideoIDs;
- (NSDictionary*)getCachedVideoDictonary;
- (void)setCachedVideoObjectDict:(NSDictionary*)video withKey:(NSString*)key;
- (TJCVideoObject*)videoForOfferID:(NSString*)offerID;
- (void)reportCompletedOffer:(NSString*)offerID;
- (void)completeVideoReport:(BOOL)accepted;
- (void)setVideoCacheCount:(int)count;
- (void)updateViewWithOrientation:(UIInterfaceOrientation)orientation;
@end
