#import "PHContentWebView.h"

@implementation PHContentWebView
@synthesize target = _target;

- (BOOL)isAnimating
{
    return _isAnimating;
}

- (void)bounceInWithTarget:(id)target action:(SEL)action
{
    _target = target;
    _action = action;
    _isAnimating = YES;
    self.transform = CGAffineTransformMakeScale(0.85f, 0.85f);
    self.alpha = 0.0f;
    [self continueBounceIn];
}

- (void)continueBounceIn
{
    [UIView beginAnimations:nil context:NULL];
    [UIView setAnimationDuration:0.22];
    [UIView setAnimationDelegate:self];
    [UIView setAnimationDidStopSelector:@selector(finishBounceIn)];
    self.transform = CGAffineTransformIdentity;
    self.alpha = 1.0f;
    [UIView commitAnimations];
}

- (void)finishBounceIn
{
    _isAnimating = NO;
    if ([_target respondsToSelector:_action]) {
        [_target performSelector:_action];
    }
}

- (void)bounceOutWithTarget:(id)target action:(SEL)action
{
    _target = target;
    _action = action;
    _isAnimating = YES;
    [self continueBounceOut];
}

- (void)continueBounceOut
{
    [UIView beginAnimations:nil context:NULL];
    [UIView setAnimationDuration:0.18];
    [UIView setAnimationDelegate:self];
    [UIView setAnimationDidStopSelector:@selector(finishBounceOut)];
    self.transform = CGAffineTransformMakeScale(0.85f, 0.85f);
    self.alpha = 0.0f;
    [UIView commitAnimations];
}

- (void)finishBounceOut
{
    _isAnimating = NO;
    if ([_target respondsToSelector:_action]) {
        [_target performSelector:_action];
    }
}
@end
