#pragma once
#import "TJCUIWebPageView.h"

@class TJCUINavigationBarView;

@interface TJCFeaturedAppView : TJCUIWebPageView {
    NSString* publisherUserID_;
    NSString* currencyID_;
    TJCUINavigationBarView* navigationBar_;
}
@property(nonatomic, copy) NSString* publisherUserID_;
@property(nonatomic, copy) NSString* currencyID_;
+ (TJCFeaturedAppView*)sharedTJCFeaturedAppView;
+ (void)releaseSharedTJCFeaturedAppView;
- (void)refreshWithFrame:(CGRect)frame;
- (NSURL*)setUpFeaturedAdURLWithServiceURL:(NSString*)address;
- (void)loadViewWithURL:(NSString*)address;
- (void)backtoGameAction:(id)sender;
- (void)giveBackNotification;
- (void)updateViewWithOrientation:(UIInterfaceOrientation)orientation;
@end
