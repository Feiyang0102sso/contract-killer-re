#pragma once
#import <UIKit/UIKit.h>

@interface TJCLoadingView : UIView {
    UIView* mainView_;
    UIActivityIndicatorView* spinner_;
}
@property(nonatomic, assign) UIView* mainView;
- (void)enable;
- (void)disable;
- (void)fadeIn;
- (void)fadeOut;
@end
