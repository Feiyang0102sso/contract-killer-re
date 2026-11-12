#pragma once
#import "TJCUIWebPageView.h"

@class TJCUINavigationBarView;

@interface TJCOffersWebView : TJCUIWebPageView {
    TJCUINavigationBarView* navBar_;
    NSString* publisherUserID_;
    UIViewController* parentVController_;
    BOOL enableNavBar;
    BOOL flagOrientationManaged;
    BOOL isViewVisible_;
    NSString* currencyID_;
    BOOL isSelectorVisible_;
}
@property(nonatomic, copy) NSString* currencyID;
@property(nonatomic, assign) BOOL isSelectorVisible;
@property(nonatomic, assign) UIViewController* parentVController;
+ (TJCOffersWebView*)sharedTJCOffersWebView;
+ (void)releaseSharedTJCOffersWebView;
- (id)initWithFrame:(CGRect)frame enableNavBar:(BOOL)enable;
- (void)refreshWithFrame:(CGRect)frame enableNavBar:(BOOL)enable;
- (void)setCustomNavBarImage:(UIImage*)image;
- (void)refreshWebView;
@end
