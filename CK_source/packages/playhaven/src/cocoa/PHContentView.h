#pragma once
#import <UIKit/UIKit.h>
#import "PHURLLoader.h"

@class PHContent;
@class PHContentView;
@class PHContentWebView;

@protocol PHContentViewDelegate <NSObject>
- (void)contentViewDidLoad:(PHContentView*)view;
- (void)contentViewDidDismiss:(PHContentView*)view;
- (void)contentView:(PHContentView*)view didFailWithError:(NSError*)error;
@end

@interface PHContentView : UIView <UIWebViewDelegate, PHURLLoaderDelegate> {
    PHContent* _content;
    UIInterfaceOrientation _orientation;
    id<PHContentViewDelegate> _delegate;
    PHContentWebView* _webView;
    BOOL _willAnimate;
    NSMutableDictionary* _redirects;
    UIActivityIndicatorView* _activityView;
    UIView* _targetView;
    BOOL _loaded;
    BOOL _dismissing;
}
@property(nonatomic, retain) PHContent* content;
@property(nonatomic, assign) id<PHContentViewDelegate> delegate;
@property(nonatomic, assign) UIView* targetView;
- (id)initWithContent:(PHContent*)content;
- (void)transformForOrientation:(UIInterfaceOrientation)orientation;
- (void)show:(BOOL)animated;
- (void)dismiss:(BOOL)animated;
- (void)dismissFromButton;
- (void)dismissWithError:(NSError*)error;
- (void)prepareForReuse;
- (void)redirectRequest:(NSString*)request toTarget:(id)target action:(SEL)action;
- (void)sendCallback:(NSString*)callback withResponse:(id)response error:(id)error;
@end
