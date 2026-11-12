#import "PlayHavenSDK.h"
#import "PHConstants.h"
#import "PHContent.h"
#import "PHContentView.h"
#import "PHReward.h"
#import "PHStringUtil.h"
#import <limits.h>

enum {
    ContentIdle,
    ContentFetching,
    ContentReady,
    ContentPresenting,
    ContentFinished
};

static NSDictionary* ContentCallbackError(void)
{
    return [NSDictionary dictionaryWithObject:@"1" forKey:@"error"];
}

@interface PHPublisherContentRequest () <PHContentViewDelegate, PHAPIRequestDelegate>
@end

@implementation PHPublisherContentRequest
@synthesize placement = _placement, animated = _animated, showsOverlayImmediately = _showsOverlayImmediately;
@synthesize state = _state;

+ (id)existingRequestForApp:(NSString*)token secret:(NSString*)secret placement:(NSString*)placement
{
    for (PHAPIRequest* request in [PHAPIRequest allRequests]) {
        if ([request isKindOfClass:[PHPublisherContentRequest class]] &&
            [[request token] isEqualToString:token] && [[request secret] isEqualToString:secret]) {
            PHPublisherContentRequest* contentRequest = (PHPublisherContentRequest*)request;
            if ([[contentRequest placement] isEqualToString:placement]) {
                return contentRequest;
            }
        }
    }
    return nil;
}

+ (id)requestForApp:(NSString*)token secret:(NSString*)secret placement:(NSString*)placement delegate:(id)delegate
{
    PHPublisherContentRequest* request = [self existingRequestForApp:token secret:secret placement:placement];
    if (request == nil) {
        request = [[[self alloc] initWithApp:token secret:secret placement:placement delegate:delegate] autorelease];
    }
    return request;
}

- (id)initWithApp:(NSString*)token secret:(NSString*)secret placement:(NSString*)placement delegate:(id)delegate
{
    self = [self initWithApp:token secret:secret];
    if (self) {
        self.placement = placement;
        self.delegate = delegate;
    }
    return self;
}

- (id)initWithApp:(NSString*)token secret:(NSString*)secret
{
    self = [super initWithApp:token secret:secret];
    if (self) {
        _animated = YES;
    }
    return self;
}

- (NSMutableArray*)contentViews
{
    if (_contentViews == nil) {
        _contentViews = [[NSMutableArray alloc] init];
    }
    return _contentViews;
}

- (UIWindow*)overlayWindow
{
    if (_overlayWindow == nil) {
        _overlayWindow = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
        _overlayWindow.windowLevel = UIWindowLevelStatusBar;
        _overlayWindow.backgroundColor = [UIColor clearColor];
        _overlayWindow.opaque = NO;
        UIView* overlay = [[[UIView alloc] initWithFrame:_overlayWindow.bounds] autorelease];
        overlay.backgroundColor = [UIColor colorWithWhite:0.0f alpha:0.62f];
        [_overlayWindow addSubview:overlay];
    }
    return _overlayWindow;
}

- (UIView*)overlayView
{
    NSArray* views = [_overlayWindow subviews];
    if ([views count] == 0) {
        return nil;
    }
    return [views objectAtIndex:0];
}

- (UIButton*)closeButton
{
    if (_closeButton == nil) {
        _closeButton = [[UIButton buttonWithType:UIButtonTypeCustom] retain];
        [_closeButton setImage:convertByteDataToUIImage(close_image) forState:UIControlStateNormal];
        [_closeButton setImage:convertByteDataToUIImage(close_active_image) forState:UIControlStateHighlighted];
        [_closeButton addTarget:self action:@selector(dismissFromButton) forControlEvents:UIControlEventTouchUpInside];
        _closeButton.hidden = YES;
        [[self overlayView] addSubview:_closeButton];
    }
    return _closeButton;
}

- (void)dealloc
{
    [self tearDownContent];
    [_placement release];
    [_contentViews release];
    [_content release];
    [_subcontentRequests release];
    [_rewardReceipts release];
    [super dealloc];
}

- (void)placeCloseButton
{
    UIView* overlay = [self overlayView];
    if (overlay == nil) {
        return;
    }
    CGRect bounds = [overlay bounds];
    CGRect frame = CGRectZero;
    PHContentView* view = [_contentViews lastObject];
    if (view != nil) {
        frame = [view frame];
    }
    CGFloat right = CGRectGetMaxX(bounds) - 4.0f;
    CGFloat top = CGRectGetMinY(bounds) + 4.0f;
    if (!CGRectIsEmpty(frame)) {
        right = MIN(right, CGRectGetMaxX(frame) + 15.0f);
        top = MAX(top, CGRectGetMinY(frame) - 15.0f);
    }
    [self closeButton].frame = CGRectMake(right - 36.0f, top, 36.0f, 36.0f);
    [overlay bringSubviewToFront:_closeButton];
}

- (void)showCloseButtonBecauseOfTimeout
{
    if (_overlayWindow != nil) {
        [self closeButton].hidden = NO;
    }
}

- (void)hideCloseButton
{
    _closeButton.hidden = YES;
}

- (void)showOverlayWindow
{
    [self overlayWindow].hidden = NO;
    [self transformForOrientation];
}

- (void)hideOverlayWindow
{
    _overlayWindow.hidden = YES;
}

- (NSString*)urlPath
{
    return @"/v3/publisher/content/";
}

- (NSDictionary*)additionalParameters
{
    if ([[self placement] length] == 0) {
        return nil;
    }
    return [NSDictionary dictionaryWithObject:[self placement] forKey:@"placement_id"];
}

- (void)preload
{
    if (_state == ContentIdle) {
        _targetState = ContentReady;
        [self continueLoadingIfNeeded];
    }
}

- (void)send
{
    if (_state == ContentFinished || _targetState == ContentPresenting) {
        return;
    }
    _targetState = ContentPresenting;
    NSNotificationCenter* notifications = [NSNotificationCenter defaultCenter];
    [notifications addObserver:self selector:@selector(dismissToBackground)
                           name:UIApplicationDidEnterBackgroundNotification object:nil];
    [notifications addObserver:self selector:@selector(transformForOrientation)
                           name:UIApplicationDidChangeStatusBarOrientationNotification object:nil];
    if (_showsOverlayImmediately && [[self placement] length] != 0 && [self URL] != nil) {
        [self showOverlayWindow];
        [self performSelector:@selector(showCloseButtonBecauseOfTimeout) withObject:nil afterDelay:4.0];
    }
    [self continueLoadingIfNeeded];
}

- (void)continueLoadingIfNeeded
{
    if (_state == ContentIdle) {
        [self getContent];
    } else if (_state == ContentReady && _targetState == ContentPresenting) {
        [self showContentIfReady];
    }
}

- (void)getContent
{
    _state = ContentFetching;
    if ([[self placement] length] == 0) {
        [self didFailWithError:PHRequestError(NSURLErrorBadURL, @"PlayHaven placement is missing.")];
        return;
    }
    [super send];
    id<PHPublisherContentRequestDelegate> delegate = (id)[self delegate];
    if ([delegate respondsToSelector:@selector(requestWillGetContent:)]) {
        [delegate requestWillGetContent:self];
    }
}

- (void)didSucceedWithResponse:(id)response
{
    [self retain];
    if ([response isKindOfClass:[NSDictionary class]]) {
        _content = [[PHContent contentWithDictionary:response] retain];
    }
    if (_content == nil) {
        [self dismissWithType:@"PHPublisherNoContentTriggeredDismiss"];
    } else if (!PHURLHasSameOrigin([_content URL], PHServiceBaseURL())) {
        [self didFailWithError:PHRequestError(NSURLErrorUnsupportedURL, @"PlayHaven content origin is unsupported.")];
    } else {
        _state = ContentReady;
        id<PHPublisherContentRequestDelegate> delegate = (id)[self delegate];
        if ([delegate respondsToSelector:@selector(requestDidGetContent:)]) {
            [delegate requestDidGetContent:self];
        }
        [self continueLoadingIfNeeded];
    }
    [self release];
}

- (void)showContentIfReady
{
    _state = ContentPresenting;
    [self retain];
    id<PHPublisherContentRequestDelegate> delegate = (id)[self delegate];
    if ([delegate respondsToSelector:@selector(request:contentWillDisplay:)]) {
        [delegate request:self contentWillDisplay:_content];
    }
    if (_state == ContentPresenting) {
        [self showOverlayWindow];
        [self pushContent:_content];
    }
    [self release];
}

- (void)pushContent:(PHContent*)content
{
    if (!PHURLHasSameOrigin([content URL], PHServiceBaseURL())) {
        [self didFailWithError:PHRequestError(NSURLErrorUnsupportedURL, @"PlayHaven content origin is unsupported.")];
        return;
    }
    UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];
    CGRect frame = [content frameForOrientation:orientation];
    if (CGRectIsNull(frame) || CGRectIsEmpty(frame)) {
        [self didFailWithError:PHCreateError(PHMissingOrientation)];
        return;
    }

    PHContentView* view = [[[PHContentView alloc] initWithContent:content] autorelease];
    view.targetView = [self overlayView];
    view.delegate = self;
    [view redirectRequest:@"ph://subcontent" toTarget:self action:@selector(requestSubcontent:callback:source:)];
    [view redirectRequest:@"ph://reward" toTarget:self action:@selector(requestRewards:callback:source:)];
    [view redirectRequest:@"ph://closeButton" toTarget:self action:@selector(requestCloseButton:callback:source:)];
    [[self contentViews] addObject:view];
    [view transformForOrientation:orientation];
    [view show:_animated];
    [self placeCloseButton];
    if (_closeButton.hidden) {
        [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(showCloseButtonBecauseOfTimeout) object:nil];
        NSTimeInterval delay = [content closeButtonDelay];
        [self performSelector:@selector(showCloseButtonBecauseOfTimeout) withObject:nil afterDelay:MIN(10.0, delay)];
    }
}

- (void)transformForOrientation
{
    UIView* overlay = [self overlayView];
    if (overlay == nil) {
        return;
    }
    UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];
    CGRect bounds = [_overlayWindow bounds];
    CGFloat angle = 0.0f;
    if (orientation == UIInterfaceOrientationLandscapeLeft) {
        angle = -(CGFloat)M_PI_2;
    } else if (orientation == UIInterfaceOrientationLandscapeRight) {
        angle = (CGFloat)M_PI_2;
    } else if (orientation == UIInterfaceOrientationPortraitUpsideDown) {
        angle = (CGFloat)M_PI;
    }
    if (UIInterfaceOrientationIsLandscape(orientation)) {
        bounds.size = CGSizeMake(bounds.size.height, bounds.size.width);
    }
    overlay.bounds = bounds;
    overlay.center = CGPointMake(CGRectGetMidX(_overlayWindow.bounds), CGRectGetMidY(_overlayWindow.bounds));
    overlay.transform = CGAffineTransformMakeRotation(angle);
    for (PHContentView* view in _contentViews) {
        [view transformForOrientation:orientation];
    }
    [self placeCloseButton];
}

- (void)contentViewDidLoad:(PHContentView*)view
{
    if (_state != ContentPresenting || [_contentViews count] != 1 || view != [_contentViews lastObject]) {
        return;
    }
    id<PHPublisherContentRequestDelegate> delegate = (id)[self delegate];
    if ([delegate respondsToSelector:@selector(request:contentDidDisplay:)]) {
        [delegate request:self contentDidDisplay:[view content]];
    }
}

- (void)contentViewDidDismiss:(PHContentView*)view
{
    if (view == [_contentViews lastObject]) {
        if ([_contentViews count] > 1) {
            [self removeContentView:view];
            [self placeCloseButton];
        } else {
            [self dismissWithType:@"PHPublisherContentUnitTriggeredDismiss"];
        }
    }
}

- (void)contentView:(PHContentView*)view didFailWithError:(NSError*)error
{
    if (view == [_contentViews lastObject]) {
        [self didFailWithError:error];
    }
}

- (void)dismissFromButton
{
    PHContentView* currentView = [_contentViews lastObject];
    NSString* address = [[currentView content] closeButtonURLPath];
    if ([address length] != 0) {
        NSURL* ping = [NSURL URLWithString:address relativeToURL:PHServiceBaseURL()];
        if (PHURLHasSameOrigin(ping, PHServiceBaseURL())) {
            PHURLLoader* loader = [[[PHURLLoader alloc] init] autorelease];
            loader.targetURL = ping;
            loader.opensFinalURLOnDevice = NO;
            [loader open];
        }
    }
    if ([_contentViews count] > 1) {
        [currentView dismissFromButton];
        return;
    }
    [self dismissWithType:@"PHPublisherNativeCloseButtonTriggeredDismiss"];
}

- (void)dismissToBackground
{
    [self dismissWithType:@"PHPublisherApplicationBackgroundTriggeredDismiss"];
}

- (void)dismissWithType:(PHPublisherContentDismissType*)type
{
    if (_state == ContentFinished) {
        return;
    }
    [self retain];
    id<PHPublisherContentRequestDelegate> delegate = (id)[self delegate];
    [self finish];
    if ([delegate respondsToSelector:@selector(request:contentDidDismissWithType:)]) {
        [delegate request:self contentDidDismissWithType:type];
    } else if ([delegate respondsToSelector:@selector(requestContentDidDismiss:)]) {
        [delegate requestContentDidDismiss:self];
    }
    [self release];
}

- (void)finish
{
    _state = ContentFinished;
    [self tearDownContent];
    [super finish];
}

- (void)tearDownContent
{
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    for (PHPublisherSubContentRequest* request in _subcontentRequests) {
        [request cancel];
    }
    [_subcontentRequests removeAllObjects];
    for (PHContentView* view in _contentViews) {
        [view prepareForReuse];
        [view removeFromSuperview];
    }
    [_contentViews removeAllObjects];
    [self hideOverlayWindow];
    [_closeButton release];
    _closeButton = nil;
    [_overlayWindow release];
    _overlayWindow = nil;
}

- (void)removeContentView:(PHContentView*)view
{
    NSArray* pending = [_subcontentRequests allObjects];
    for (PHPublisherSubContentRequest* request in pending) {
        if ([request source] == view) {
            [request cancel];
            [_subcontentRequests removeObject:request];
        }
    }
    [view prepareForReuse];
    [view removeFromSuperview];
    [_contentViews removeObjectIdenticalTo:view];
}

- (void)requestSubcontent:(NSDictionary*)context callback:(NSString*)callback source:(PHContentView*)source
{
    id address = [context objectForKey:@"url"];
    if (![address isKindOfClass:[NSString class]] || [address length] == 0) {
        [source sendCallback:callback withResponse:nil error:ContentCallbackError()];
        return;
    }
    PHPublisherSubContentRequest* request = [PHPublisherSubContentRequest requestForApp:[self token] secret:[self secret]];
    request.delegate = self;
    request.urlPath = address;
    request.source = source;
    request.callback = callback;
    if (_subcontentRequests == nil) {
        _subcontentRequests = [[NSMutableSet alloc] init];
    }
    [_subcontentRequests addObject:request];
    [request send];
}

- (void)request:(PHAPIRequest*)request didSucceedWithResponse:(id)response
{
    PHPublisherSubContentRequest* subcontent = (PHPublisherSubContentRequest*)request;
    PHContentView* source = [subcontent source];
    [_subcontentRequests removeObject:subcontent];
    if (_state != ContentPresenting || source == nil || ![_contentViews containsObject:source]) {
        return;
    }
    PHContent* content = nil;
    if ([response isKindOfClass:[NSDictionary class]]) {
        content = [PHContent contentWithDictionary:response];
    }
    if (content == nil || !PHURLHasSameOrigin([content URL], PHServiceBaseURL())) {
        [source sendCallback:[subcontent callback] withResponse:nil error:ContentCallbackError()];
        return;
    }
    [self retain];
    [source retain];
    [self pushContent:content];
    if (_state == ContentPresenting) {
        [source sendCallback:[subcontent callback] withResponse:response error:nil];
    }
    [source release];
    [self release];
}

- (void)request:(PHAPIRequest*)request didFailWithError:(NSError*)error
{
    PHPublisherSubContentRequest* subcontent = (PHPublisherSubContentRequest*)request;
    PHContentView* source = [subcontent source];
    [_subcontentRequests removeObject:subcontent];
    if (_state == ContentPresenting && source != nil && [_contentViews containsObject:source]) {
        [source sendCallback:[subcontent callback] withResponse:nil error:ContentCallbackError()];
    }
}

- (BOOL)isValidReward:(NSDictionary*)reward
{
    if (![reward isKindOfClass:[NSDictionary class]] || [[self secret] length] == 0) {
        return NO;
    }
    id name = [reward objectForKey:@"reward"];
    id quantity = [reward objectForKey:@"quantity"];
    id receipt = [reward objectForKey:@"receipt"];
    id signature = [reward objectForKey:@"signature"];
    if (![name isKindOfClass:[NSString class]] || [name length] == 0 ||
        ![signature isKindOfClass:[NSString class]] ||
        (![receipt isKindOfClass:[NSString class]] && ![receipt isKindOfClass:[NSNumber class]]) ||
        (![quantity isKindOfClass:[NSString class]] && ![quantity isKindOfClass:[NSNumber class]])) {
        return NO;
    }
    NSScanner* scanner = [NSScanner scannerWithString:[quantity description]];
    long long count;
    if (![scanner scanLongLong:&count] || ![scanner isAtEnd] || count <= 0 || count > INT_MAX ||
        [[receipt description] length] == 0) {
        return NO;
    }
    NSString* input = [NSString stringWithFormat:@"%@:%@:%@:%@:%@",
        name, quantity, PHInstallationID(), receipt, [self secret]];
    return [[PHStringUtil hexDigestForString:input] isEqualToString:signature];
}

- (void)requestRewards:(NSDictionary*)context callback:(NSString*)callback source:(PHContentView*)source
{
    id rewards = [context objectForKey:@"rewards"];
    if (![rewards isKindOfClass:[NSArray class]]) {
        [source sendCallback:callback withResponse:nil error:ContentCallbackError()];
        return;
    }
    if (_rewardReceipts == nil) {
        _rewardReceipts = [[NSMutableSet alloc] init];
    }
    [self retain];
    [source retain];
    for (NSDictionary* record in rewards) {
        if (_state != ContentPresenting) {
            break;
        }
        if (![self isValidReward:record]) {
            continue;
        }
        NSString* receipt = [[record objectForKey:@"receipt"] description];
        if ([_rewardReceipts containsObject:receipt]) {
            continue;
        }
        [_rewardReceipts addObject:receipt];
        PHReward* reward = [[[PHReward alloc] init] autorelease];
        reward.name = [record objectForKey:@"reward"];
        reward.quantity = [[record objectForKey:@"quantity"] intValue];
        reward.receipt = receipt;
        id<PHPublisherContentRequestDelegate> delegate = (id)[self delegate];
        if ([delegate respondsToSelector:@selector(request:unlockedReward:)]) {
            [delegate request:self unlockedReward:reward];
        }
    }
    if (_state == ContentPresenting) {
        [source sendCallback:callback withResponse:nil error:nil];
    }
    [source release];
    [self release];
}

- (void)requestCloseButton:(NSDictionary*)context callback:(NSString*)callback source:(PHContentView*)source
{
    id hidden = [context objectForKey:@"hidden"];
    if ([hidden respondsToSelector:@selector(boolValue)]) {
        [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(showCloseButtonBecauseOfTimeout) object:nil];
        [self closeButton].hidden = [hidden boolValue];
        if (_closeButton.hidden) {
            [self performSelector:@selector(showCloseButtonBecauseOfTimeout) withObject:nil afterDelay:10.0];
        }
    }
    NSDictionary* response = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:[self closeButton].hidden]
                                                        forKey:@"hidden"];
    [source sendCallback:callback withResponse:response error:nil];
}
@end
