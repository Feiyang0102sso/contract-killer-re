// Tapjoy iOS SDK declarations first recovered by b033x from the selectors and
// notifications used by TapjoyManager.mm. Implementations are under Components/TJC*.
#ifndef TAPJOYCONNECT_H
#define TAPJOYCONNECT_H

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "TJCFeaturedAppModel.h"

@class TJCCoreFetcher;
@class TJCAdRequestHandler;
@class TJCUIWebPageView;
@protocol TJCAdDelegate;

// Notification names (posted by the SDK)
#define TJC_CONNECT_SUCCESS                         @"TJC_Connect_Success"
#define TJC_CONNECT_FAILED                          @"TJC_Connect_Failed"
#define TJC_VIEW_CLOSED_NOTIFICATION                @"TJC_SHOW_BOX_NOTIFICATION"
#define TJC_FEATURED_APP_RESPONSE_NOTIFICATION      @"TJC_FEATURED_APP_RESPONSE_NOTIFICATION"
#define TJC_FEATURED_APP_RESPONSE_NOTIFICATION_ERROR @"TJC_FEATURED_APP_RESPONSE_NOTIFICATION_ERROR"
#define TJC_TAP_POINTS_RESPONSE_NOTIFICATION        @"TJC_TAP_POINTS_RESPONSE_NOTIFICATION"
#define TJC_SPEND_TAP_POINTS_RESPONSE_NOTIFICATION  @"TJC_SPEND_TAP_POINTS_RESPONSE_NOTIFICATION"

// Display ad sizes (TJCAdDelegate adContentSize)
#define TJC_AD_BANNERSIZE_320X50                    @"320x50"
#define TJC_AD_BANNERSIZE_768X90                    @"768x90"

/** Display ad banner view (SDK class; b142: the original type encoding of TapjoyManager's m_adView ivar is @"TJCAdView"). */
@interface TJCAdView : UIView {
    NSString* clickURL_;
    NSString* imageDataStr_;
    UIImageView* imageView_;
    UIImageView* previousImageView_;
    NSString* contentSizeStr_;
    UIView* adViewOverlay_;
    TJCAdRequestHandler* adHandlerObj_;
    id<TJCAdDelegate> adDelegate_;
}
@property(nonatomic, copy) NSString* clickURL;
@property(nonatomic, copy) NSString* imageDataStr;
@property(nonatomic, retain) UIImageView* imageView;
@property(nonatomic, retain) UIImageView* previousImageView;
@property(nonatomic, retain) UIView* adViewOverlay;
@property(nonatomic, retain) TJCAdRequestHandler* adHandlerObj;
@property(nonatomic, assign) id<TJCAdDelegate> adDelegate;
+ (TJCAdView*)sharedTJCAdView;
+ (void)releaseSharedTJCAdView;
- (void)getAdWithDelegate:(id<TJCAdDelegate>)delegate;
- (void)getAdWithDelegate:(id<TJCAdDelegate>)delegate currencyID:(NSString*)currencyID;
- (void)refreshAd:(NSTimer*)timer;
- (void)refreshAdWithCurrencyID:(NSString*)currencyID;
- (BOOL)isAdLoaded;
@end

/** View controller the SDK presents its offer wall with. */
@interface TJCCallsWrapper : UIViewController {
    UIInterfaceOrientation currentInterfaceOrientation;
    TJCUIWebPageView* contentView_;
    UIViewController* parentController_;
}
@property(nonatomic, assign) UIInterfaceOrientation currentInterfaceOrientation;
+ (TJCCallsWrapper*)sharedTJCCallsWrapper;
+ (void)releaseSharedTJCCallsWrapper;
- (void)showContent:(TJCUIWebPageView*)content fromController:(UIViewController*)controller;
- (void)showContent:(TJCUIWebPageView*)content fromController:(UIViewController*)controller frame:(CGRect)frame;
- (void)scheduleCloseNotification;
- (void)updateViewsWithOrientation:(UIInterfaceOrientation)orientation;
- (void)hideView;
@end

// Protocol method lists as in the original metadata (TapjoyManager.o): TJCAdDelegate has four required
// methods and no optional ones; TapjoyManager itself implements the misspelled didRecieveAd: (sent by
// TJCAdView via respondsToSelector:). TJCVideoAdDelegate declares only videoAdBegan / videoAdClosed;
// TJCVideoView still sends videoAdFinished / videoAdError: after respondsToSelector:.
@protocol TJCAdDelegate <NSObject>
- (void)didReceiveAd:(UIView*)adView;
- (void)didFailWithMessage:(NSString*)message;
- (NSString*)adContentSize;
- (BOOL)shouldRefreshAd;
@end

@protocol TJCVideoAdDelegate <NSObject>
@optional
- (void)videoAdBegan;
- (void)videoAdClosed;
@end

@interface TapjoyConnect : NSObject {
    NSString* appID_;
    NSString* secretKey_;
    NSString* userID_;
    float currencyMultiplier_;
    NSMutableData* data_;
    NSURLConnection* connection_;
    TBXMLElement* currentXMLElement_;
    int connectAttempts_;
    BOOL isInitialConnect_;
    BOOL connected_;
    NSURL* serviceURL_;
    NSURL* alternateServiceURL_;
    NSString* installationID_;
    TJCCoreFetcher* connectFetcher_;
}
@property(nonatomic, copy) NSString* appID;
@property(nonatomic, copy) NSString* secretKey;
@property(nonatomic, copy) NSString* userID;
@property(nonatomic, assign) BOOL isInitialConnect;
+ (TapjoyConnect*)sharedTapjoyConnect;
+ (TapjoyConnect*)requestTapjoyConnect:(NSString*)appID secretKey:(NSString*)secretKey;
+ (void)disconnect;
+ (NSString*)getAppID;
+ (NSString*)getUserID;
+ (void)setUserID:(NSString*)userID;
+ (NSString*)getSecretKey;
+ (void)setCurrencyMultiplier:(float)multiplier;
+ (float)getCurrencyMultiplier;
+ (NSString*)createQueryStringFromDict:(NSDictionary*)parameters;
+ (NSString*)TJCSHA256CommonParamsWithTimeStamp:(NSString*)timestamp;
+ (NSString*)TJCSHA256CommonParamsWithTimeStamp:(NSString*)timestamp tapPointsAmount:(int)points guid:(NSString*)guid;
+ (NSString*)TJCSHA256WithString:(NSString*)value;
+ (NSString*)generateUUID;
- (NSString*)servicePath:(NSString*)path;
- (NSString*)alternateServicePath:(NSString*)path;
- (NSDictionary*)genericParameters;
- (BOOL)isConnected;
@end

@interface TapjoyConnect (TJCAdView)
+ (void)getDisplayAdWithDelegate:(id<TJCAdDelegate>)delegate;
+ (void)getDisplayAdWithDelegate:(id<TJCAdDelegate>)delegate currencyID:(NSString*)currencyID;
+ (TJCAdView*)getDisplayAdView;
+ (BOOL)isDisplayAdLoaded;
+ (void)refreshDisplayAd;
+ (void)refreshDisplayAdWithCurrencyID:(NSString*)currencyID;
@end

@interface TapjoyConnect (TJCViewCommons)
+ (void)setTransitionEffect:(int)transitionEffect;
+ (float)getTransitionDelay;
+ (int)getCurrentTransitionEffect;
+ (int)getReverseTransitionEffect;
+ (void)updateViewsWithOrientation:(UIInterfaceOrientation)orientation;
@end

@interface TapjoyConnect (TJCVideoViewHandler)
+ (void)initVideoAdWithDelegate:(id<TJCVideoAdDelegate>)delegate;
+ (void)setVideoCacheCount:(int)count;
+ (void)showVideoAdWithFrame:(CGRect)frame offerID:(NSString*)offerID;
+ (void)showVideoAdWithOfferID:(NSString*)offerID;
+ (void)showVideoAdWithViewController:(UIViewController*)controller offerID:(NSString*)offerID;
@end

@interface TapjoyConnect (TJCOffersViewHandler)
+ (void)showOffersWithViewController:(UIViewController*)viewController;
@end

@interface TapjoyConnect (TJCUserAccountManager)
+ (void)getTapPoints;
+ (void)spendTapPoints:(int)points;
+ (void)awardTapPoints:(int)points;
@end

@interface TapjoyConnect (TJCFeaturedAppViewHandler)
+ (void)getFeaturedApp;
+ (void)getFeaturedAppWithCurrencyID:(NSString*)currencyID;
+ (void)showFeaturedAppFullScreenAd;
+ (void)showFeaturedAppFullScreenAdWithFrame:(CGRect)frame;
+ (void)showFeaturedAppFullScreenAdWithViewController:(UIViewController*)controller;
+ (void)setFeaturedAppDisplayCount:(int)count;
@end

#endif
