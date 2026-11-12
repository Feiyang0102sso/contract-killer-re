#pragma once
#import <UIKit/UIKit.h>

@interface TJCUINavigationBarView : UIView {
    UINavigationBar* navBar;
    UINavigationItem* navBarTitle;
    UIBarButtonItem* navBarLeftBtn;
    UIBarButtonItem* navBarRightBtn;
    UIImageView* bgImage;
    UILabel* titleLabel;
}
@property(nonatomic, readonly) UINavigationBar* navBar;
@property(nonatomic, readonly) UINavigationItem* navBarTitle;
@property(nonatomic, retain) UIBarButtonItem* navBarLeftBtn;
@property(nonatomic, retain) UIBarButtonItem* navBarRightBtn;
- (id)initWithTitle:(NSString*)title withFrame:(CGRect)frame AtY:(CGFloat)y;
- (void)setCustomBackgroundImage:(UIImage*)image;
- (void)updateLeftNavBarBtnWithTitle:(NSString*)title;
- (void)updateRightNavBarBtnWithTitle:(NSString*)title;
- (void)setNavBarLeftBtnWithTitle:(NSString*)title;
- (void)setNavBarRightBtnWithTitle:(NSString*)title;
- (void)addLeftButtonTarget:(id)target action:(SEL)action forControlEvents:(UIControlEvents)events;
- (void)addRightButtonTarget:(id)target action:(SEL)action forControlEvents:(UIControlEvents)events;
- (UILabel*)titleLabel;
@end
