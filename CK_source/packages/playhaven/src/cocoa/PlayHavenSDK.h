// PlayHaven declarations first recovered by b033x from the selectors used by CPHManager.mm.
// Request and display implementations are in the neighboring Objective-C source files.
#ifndef PLAYHAVENSDK_H
#define PLAYHAVENSDK_H

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class PHContent;
@class PHReward;
@class PHNotificationRenderer;
@class PHPublisherContentRequest;
@class PHAPIRequest;

NSURL* PHServiceBaseURL(void);
NSString* PHInstallationID(void);
BOOL PHURLHasSameOrigin(NSURL* url, NSURL* base);
NSError* PHRequestError(NSInteger code, NSString* message);

/** Reason passed to request:contentDidDismissWithType:. */
typedef NSString PHPublisherContentDismissType;

@protocol PHAPIRequestDelegate <NSObject>
@optional
- (void)request:(PHAPIRequest*)request didSucceedWithResponse:(id)response;
- (void)request:(PHAPIRequest*)request didFailWithError:(NSError*)error;
@end

@interface PHAPIRequest : NSObject {
    NSURL* _URL;
    NSString* _token;
    NSString* _secret;
    NSURLConnection* _connection;
    NSDictionary* _signedParameters;
    id<PHAPIRequestDelegate> _delegate;
    NSMutableData* _connectionData;
    NSString* _urlPath;
    NSDictionary* _additionalParameters;
    NSURLResponse* _response;
    NSUInteger _hashCode;
}
@property(nonatomic, readonly) NSString* token;
@property(nonatomic, readonly) NSString* secret;
@property(nonatomic, assign) id<PHAPIRequestDelegate> delegate;
@property(nonatomic, copy) NSString* urlPath;
@property(nonatomic, retain) NSDictionary* additionalParameters;
@property(nonatomic, assign) NSUInteger hashCode;
+ (NSMutableSet*)allRequests;
+ (void)cancelAllRequestsWithDelegate:(id)delegate;
+ (NSString*)base64SignatureWithString:(NSString*)value;
+ (id)requestForApp:(NSString*)token secret:(NSString*)secret;
- (id)initWithApp:(NSString*)token secret:(NSString*)secret;
- (NSURL*)URL;
- (NSDictionary*)signedParameters;
- (NSString*)signedParameterString;
- (void)send;
- (void)cancel;
- (void)finish;
- (void)afterConnectionDidFinishLoading;
- (void)processRequestResponse:(NSDictionary*)envelope;
- (void)didSucceedWithResponse:(id)response;
- (void)didFailWithError:(NSError*)error;
@end

@interface PHPublisherOpenRequest : PHAPIRequest
@end

@interface PHPublisherMetadataRequest : PHAPIRequest {
    NSString* _placement;
}
@property(nonatomic, copy) NSString* placement;
+ (id)requestForApp:(NSString*)token secret:(NSString*)secret placement:(NSString*)placement delegate:(id)delegate;
- (id)initWithApp:(NSString*)token secret:(NSString*)secret placement:(NSString*)placement delegate:(id)delegate;
@end

@interface PHPublisherPromosRequest : PHAPIRequest
+ (id)requestForApp:(NSString*)token secret:(NSString*)secret delegate:(id)delegate;
- (id)initWithApp:(NSString*)token secret:(NSString*)secret delegate:(id)delegate;
@end

@interface PHPublisherSubContentRequest : PHAPIRequest {
    id _source;
    NSString* _callback;
}
@property(nonatomic, assign) id source;
@property(nonatomic, copy) NSString* callback;
@end

// b142: methods and order completed from the original protocol_t metadata (the original SDK header was a newer version)
@class PHPurchase;

@protocol PHPublisherContentRequestDelegate <NSObject>
@optional
- (void)requestWillGetContent:(PHPublisherContentRequest*)request;
- (void)requestDidGetContent:(PHPublisherContentRequest*)request;
- (void)request:(PHPublisherContentRequest*)request contentWillDisplay:(PHContent*)content;
- (void)request:(PHPublisherContentRequest*)request contentDidDisplay:(PHContent*)content;
- (void)requestContentDidDismiss:(PHPublisherContentRequest*)request;
- (void)request:(PHPublisherContentRequest*)request contentDidDismissWithType:(PHPublisherContentDismissType*)type;
- (void)request:(PHPublisherContentRequest*)request didFailWithError:(NSError*)error;
- (void)request:(PHPublisherContentRequest*)request contentDidFailWithError:(NSError*)error;
- (UIImage*)request:(PHPublisherContentRequest*)request closeButtonImageForControlState:(UIControlState)state content:(PHContent*)content;
- (UIColor*)request:(PHPublisherContentRequest*)request borderColorForContent:(PHContent*)content;
- (void)request:(PHPublisherContentRequest*)request unlockedReward:(PHReward*)reward;
// no request:makePurchase: in this SDK version (original protocol metadata ends at unlockedReward:)
@end

@interface PHPublisherContentRequest : PHAPIRequest {
    NSString* _placement;
    BOOL _animated;
    NSMutableArray* _contentViews;
    BOOL _showsOverlayImmediately;
    UIButton* _closeButton;
    UIWindow* _overlayWindow;
    PHContent* _content;
    NSUInteger _state;
    NSUInteger _targetState;
    NSMutableSet* _subcontentRequests;
    NSMutableSet* _rewardReceipts;
}
@property(nonatomic, copy) NSString* placement;
@property(nonatomic, assign) BOOL animated;
@property(nonatomic, assign) BOOL showsOverlayImmediately;
@property(nonatomic, assign) NSUInteger state;
+ (id)existingRequestForApp:(NSString*)token secret:(NSString*)secret placement:(NSString*)placement;
+ (id)requestForApp:(NSString*)token secret:(NSString*)secret placement:(NSString*)placement delegate:(id)delegate;
- (id)initWithApp:(NSString*)token secret:(NSString*)secret placement:(NSString*)placement delegate:(id)delegate;
- (void)preload;
@end

/** Badge view showing the number of pending notifications of a placement. */
@interface PHNotificationView : UIView <PHAPIRequestDelegate> {
    NSString* _app;
    NSString* _secret;
    NSString* _placement;
    NSDictionary* _notificationData;
    PHNotificationRenderer* _notificationRenderer;
    PHPublisherMetadataRequest* _request;
}
@property(nonatomic, retain) NSDictionary* notificationData;
+ (void)setRendererClass:(Class)renderer forType:(NSString*)type;
+ (id)newRendererForData:(NSDictionary*)data;
- (id)initWithApp:(NSString*)token secret:(NSString*)secret placement:(NSString*)placement;
- (void)clear;
- (void)refresh;
- (void)test;
@end

#endif
