#import "TapjoyConnect.h"
#import "TJCUIWebPageView.h"

static TJCCallsWrapper* SharedCallsWrapper;

@implementation TJCCallsWrapper
@synthesize currentInterfaceOrientation;

+ (TJCCallsWrapper*)sharedTJCCallsWrapper
{
    if (SharedCallsWrapper == nil) {
        SharedCallsWrapper = [[self alloc] init];
    }
    return SharedCallsWrapper;
}

+ (void)releaseSharedTJCCallsWrapper
{
    if (SharedCallsWrapper != nil) {
        [NSObject cancelPreviousPerformRequestsWithTarget:SharedCallsWrapper];
    }
    [SharedCallsWrapper closeContentAndNotify:NO];
    [SharedCallsWrapper release];
    SharedCallsWrapper = nil;
}

- (id)init
{
    self = [super init];
    if (self) {
        currentInterfaceOrientation = [[UIApplication sharedApplication] statusBarOrientation];
    }
    return self;
}

- (void)loadView
{
    UIView* view = [[[UIView alloc] initWithFrame:[[UIScreen mainScreen] applicationFrame]] autorelease];
    view.backgroundColor = [UIColor blackColor];
    view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.view = view;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
    return orientation == currentInterfaceOrientation;
}

- (void)updateViewsWithOrientation:(UIInterfaceOrientation)orientation
{
    if (UIInterfaceOrientationIsPortrait(orientation) || UIInterfaceOrientationIsLandscape(orientation)) {
        currentInterfaceOrientation = orientation;
    }
}

- (void)moveViewToFront
{
    UIView* container = [self.view superview];
    [container bringSubviewToFront:self.view];
}

- (void)showContent:(TJCUIWebPageView*)content fromController:(UIViewController*)controller
{
    [self showContent:content fromController:controller frame:CGRectZero];
}

- (void)showContent:(TJCUIWebPageView*)content fromController:(UIViewController*)controller frame:(CGRect)frame
{
    [self closeContentAndNotify:NO];
    UIWindow* window = [[UIApplication sharedApplication] keyWindow];
    if (controller == nil) {
        controller = [window rootViewController];
    }
    if (controller == self) {
        controller = nil;
    }
    if (controller == nil && window == nil) {
        [self scheduleCloseNotification];
        return;
    }

    contentView_ = [content retain];
    parentController_ = controller;
    if (controller != nil) {
        BOOL animated = [TapjoyConnect getTransitionDelay] > 0.0f;
        [controller presentModalViewController:self animated:animated];
    } else {
        self.view.frame = window.bounds;
        [window addSubview:self.view];
    }
    if (CGRectIsEmpty(frame)) {
        frame = self.view.bounds;
        contentView_.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    } else {
        contentView_.autoresizingMask = UIViewAutoresizingNone;
    }
    contentView_.frame = frame;
    [self.view addSubview:contentView_];
}

- (void)hideView
{
    [self closeContentAndNotify:YES];
}

- (void)closeContentAndNotify:(BOOL)notify
{
    if (contentView_ == nil) {
        return;
    }
    [contentView_ cancelLoading];
    [contentView_ removeFromSuperview];
    [contentView_ release];
    contentView_ = nil;

    UIViewController* controller = parentController_;
    parentController_ = nil;
    if ([controller modalViewController] == self) {
        [controller dismissModalViewControllerAnimated:NO];
    } else {
        [self.view removeFromSuperview];
    }
    if (notify) {
        [self scheduleCloseNotification];
    }
}

- (void)scheduleCloseNotification
{
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(showBoxCloseNotification:) object:nil];
    [self performSelector:@selector(showBoxCloseNotification:) withObject:nil afterDelay:0];
}

- (void)showBoxCloseNotification:(NSNotification*)notification
{
    [[NSNotificationCenter defaultCenter] postNotificationName:TJC_VIEW_CLOSED_NOTIFICATION object:nil];
}

- (void)dealloc
{
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    [self closeContentAndNotify:NO];
    [super dealloc];
}
@end
