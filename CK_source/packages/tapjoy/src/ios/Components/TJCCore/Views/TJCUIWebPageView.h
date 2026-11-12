#pragma once
#import <UIKit/UIKit.h>

@class TJCLoadingView;

@interface TJCUIWebPageView : UIView <UIWebViewDelegate, UIAlertViewDelegate> {
    UIWebView* cWebView_;
    NSString* alertErrorMessage_;
    NSURL* lastClickedURL_;
    id delegate_;
    BOOL webPageRendered_;
    NSURL* lastURL_;
    TJCLoadingView* loadingView_;
    UIAlertView* retryAlert_;
}
@property(nonatomic, copy) NSString* alertErrorMessage;
@property(nonatomic, retain) NSURL* lastURL;
- (id)initWithFrame:(CGRect)frame WithRequestURL:(NSURL*)url;
- (void)setDelegate:(id)delegate;
- (void)loadWebRequest:(NSURLRequest*)request;
- (void)loadURLRequest:(NSURL*)url;
- (void)loadURLRequest:(NSURL*)url withTimeOutInterval:(NSTimeInterval)timeout;
- (void)cancelLoading;
@end
