#pragma once
#import <UIKit/UIKit.h>

@interface PHContentWebView : UIWebView {
    SEL _action;
    id _target;
    BOOL _isAnimating;
}
@property(nonatomic, assign) id target;
- (BOOL)isAnimating;
- (void)bounceInWithTarget:(id)target action:(SEL)action;
- (void)bounceOutWithTarget:(id)target action:(SEL)action;
@end
