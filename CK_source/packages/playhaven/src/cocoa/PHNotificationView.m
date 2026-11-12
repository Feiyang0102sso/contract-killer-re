#import "PlayHavenSDK.h"
#import "PHNotificationBadgeRenderer.h"

static NSMutableDictionary* NotificationRenderers;

@implementation PHNotificationView
@synthesize notificationData = _notificationData;

+ (void)initialize
{
    if (self == [PHNotificationView class]) {
        NotificationRenderers = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
            [PHNotificationBadgeRenderer class], @"badge", nil];
    }
}

+ (void)setRendererClass:(Class)renderer forType:(NSString*)type
{
    if ([type length] != 0) {
        [NotificationRenderers setValue:renderer forKey:type];
    }
}

+ (id)newRendererForData:(NSDictionary*)data
{
    id type = [data objectForKey:@"type"];
    if (![type isKindOfClass:[NSString class]]) {
        return nil;
    }
    Class renderer = [NotificationRenderers objectForKey:type];
    return [[renderer alloc] init];
}

- (id)initWithApp:(NSString*)token secret:(NSString*)secret placement:(NSString*)placement
{
    self = [self initWithFrame:CGRectZero];
    if (self) {
        _app = [token copy];
        _secret = [secret copy];
        _placement = [placement copy];
    }
    return self;
}

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        self.backgroundColor = [UIColor clearColor];
        self.opaque = NO;
        self.userInteractionEnabled = NO;
        [self addObserver:self forKeyPath:@"notificationData" options:NSKeyValueObservingOptionNew context:NULL];
    }
    return self;
}

- (void)observeValueForKeyPath:(NSString*)path ofObject:(id)object change:(NSDictionary*)change context:(void*)context
{
    if (object != self || ![path isEqualToString:@"notificationData"]) {
        [super observeValueForKeyPath:path ofObject:object change:change context:context];
        return;
    }
    NSDictionary* data = [self notificationData];
    [_notificationRenderer release];
    _notificationRenderer = [PHNotificationView newRendererForData:data];
    CGPoint center = [self center];
    CGSize size = CGSizeZero;
    if (_notificationRenderer != nil) {
        size = [_notificationRenderer sizeForNotification:data];
    }
    [self setFrame:CGRectMake(0, 0, size.width, size.height)];
    [self setCenter:center];
    [self setNeedsDisplay];
}

- (void)dealloc
{
    [_request cancel];
    [self removeObserver:self forKeyPath:@"notificationData"];
    [_app release];
    [_secret release];
    [_placement release];
    [_notificationData release];
    [_notificationRenderer release];
    [super dealloc];
}

- (void)refresh
{
    if (_request == nil) {
        _request = [PHPublisherMetadataRequest requestForApp:_app secret:_secret placement:_placement delegate:self];
        [_request send];
    }
}

- (void)request:(PHAPIRequest*)request didSucceedWithResponse:(id)response
{
    _request = nil;
    id data = nil;
    if ([response isKindOfClass:[NSDictionary class]]) {
        data = [response objectForKey:@"notification"];
    }
    if (![data isKindOfClass:[NSDictionary class]]) {
        data = nil;
    }
    self.notificationData = data;
}

- (void)request:(PHAPIRequest*)request didFailWithError:(NSError*)error
{
    _request = nil;
    self.notificationData = nil;
}

- (void)test
{
    [self clear];
    self.notificationData = [NSDictionary dictionaryWithObjectsAndKeys:@"badge", @"type", @"1", @"value", nil];
}

- (void)clear
{
    [_request cancel];
    _request = nil;
    self.notificationData = nil;
}

- (void)drawRect:(CGRect)rect
{
    [_notificationRenderer drawNotification:[self notificationData] inRect:rect];
}
@end
