#import "TapjoyConnect.h"
#import "TJCVideoManager.h"

@interface TJCViewCommons : NSObject {
    int currentTransitionEffect_;
    int reverseTransitionEffect_;
    int viewsIntegrationType_;
    int primaryColorCode_;
    UIColor* userDefinedColor_;
    float transitionDelay_;
}
+ (TJCViewCommons*)sharedObject;
- (void)setTransitionEffect:(int)effect;
- (float)getTransitionDelay;
- (int)getCurrentTransitionEffect;
- (int)getReverseTransitionEffect;
@end

@implementation TJCViewCommons
+ (TJCViewCommons*)sharedObject
{
    static TJCViewCommons* shared;
    if (shared == nil) {
        shared = [[self alloc] init];
    }
    return shared;
}

- (id)init
{
    self = [super init];
    if (self) {
        primaryColorCode_ = -1;
        [self setTransitionEffect:0];
    }
    return self;
}

- (void)setTransitionEffect:(int)effect
{
    currentTransitionEffect_ = effect;
    transitionDelay_ = 0.4f;
    if (effect == -1) {
        transitionDelay_ = 0.0f;
        reverseTransitionEffect_ = -1;
    } else if (effect >= 0 && effect <= 4) {
        reverseTransitionEffect_ = effect + 5;
    } else if (effect == 10) {
        reverseTransitionEffect_ = 11;
    } else if (effect == 11) {
        reverseTransitionEffect_ = 10;
    } else if (effect == 12 || effect == 14) {
        reverseTransitionEffect_ = effect + 1;
    } else {
        reverseTransitionEffect_ = 5;
    }
}

- (float)getTransitionDelay
{
    return transitionDelay_;
}

- (int)getCurrentTransitionEffect
{
    return currentTransitionEffect_;
}

- (int)getReverseTransitionEffect
{
    return reverseTransitionEffect_;
}

- (void)dealloc
{
    [userDefinedColor_ release];
    [super dealloc];
}
@end

@implementation TapjoyConnect (TJCViewCommons)
+ (void)setTransitionEffect:(int)effect
{
    [[TJCViewCommons sharedObject] setTransitionEffect:effect];
}

+ (float)getTransitionDelay
{
    return [[TJCViewCommons sharedObject] getTransitionDelay];
}

+ (int)getCurrentTransitionEffect
{
    return [[TJCViewCommons sharedObject] getCurrentTransitionEffect];
}

+ (int)getReverseTransitionEffect
{
    return [[TJCViewCommons sharedObject] getReverseTransitionEffect];
}

+ (void)updateViewsWithOrientation:(UIInterfaceOrientation)orientation
{
    [[TJCCallsWrapper sharedTJCCallsWrapper] updateViewsWithOrientation:orientation];
    [[TJCVideoManager sharedTJCVideoManager] updateViewWithOrientation:orientation];
}
@end
