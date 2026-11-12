// The content response model and frame rules used by CK's PlayHaven SDK.
#import "PHContent.h"

@implementation PHContent
@synthesize URL = _URL;
@synthesize transition = _transition;
@synthesize context = _context;
@synthesize closeButtonDelay = _closeButtonDelay;
@synthesize closeButtonURLPath = _closeButtonURLPath;

+ (id)contentWithDictionary:(NSDictionary*)description
{
    id frame = [description valueForKey:@"frame"];
    NSString* url = [description valueForKey:@"url"];
    NSString* transition = [description valueForKey:@"transition"];
    if (frame == nil || ![url isKindOfClass:[NSString class]] ||
        ![transition isKindOfClass:[NSString class]]) {
        return nil;
    }
    PHContent* content = [[[PHContent alloc] init] autorelease];
    if ([frame isKindOfClass:[NSString class]]) {
        frame = [NSDictionary dictionaryWithObject:frame forKey:frame];
    } else if (![frame isKindOfClass:[NSDictionary class]]) {
        return nil;
    }
    [content setFramesWithDictionary:frame];
    [content setURL:[NSURL URLWithString:url]];
    if ([@"PH_MODAL" isEqualToString:transition]) {
        [content setTransition:PHContentTransitionModal];
    } else if ([@"PH_DIALOG" isEqualToString:transition]) {
        [content setTransition:PHContentTransitionDialog];
    }
    id context = [description objectForKey:@"context"];
    if ([context isKindOfClass:[NSDictionary class]]) {
        [content setContext:context];
    }
    id delayValue = [description objectForKey:@"close_delay"];
    if ([delayValue respondsToSelector:@selector(floatValue)]) {
        NSTimeInterval delay = [delayValue floatValue];
        if (delay > 0.0) {
            [content setCloseButtonDelay:delay];
        }
    }
    id closePing = [description objectForKey:@"close_ping"];
    if ([closePing isKindOfClass:[NSString class]]) {
        [content setCloseButtonURLPath:closePing];
    }
    return content;
}

- (id)init
{
    self = [super init];
    if (self != nil) {
        _closeButtonDelay = 10.0;
    }
    return self;
}

- (void)dealloc
{
    [_URL release];
    _URL = nil;
    [_context release];
    _context = nil;
    [_closeButtonURLPath release];
    _closeButtonURLPath = nil;
    [_frameDict release];
    _frameDict = nil;
    [super dealloc];
}

- (CGRect)frameForOrientation:(UIInterfaceOrientation)orientation
{
    BOOL landscape = UIInterfaceOrientationIsLandscape(orientation);
    NSString* key = @"PH_PORTRAIT";
    if (landscape) {
        key = @"PH_LANDSCAPE";
    }
    NSDictionary* frame = [_frameDict valueForKey:key];
    if ([_frameDict valueForKey:@"PH_FULLSCREEN"] != nil) {
        CGRect screen = [[UIScreen mainScreen] applicationFrame];
        if (landscape) {
            return CGRectMake(0, 0, screen.size.height, screen.size.width);
        }
        return CGRectMake(0, 0, screen.size.width, screen.size.height);
    }
    if (![frame isKindOfClass:[NSDictionary class]]) {
        return CGRectNull;
    }
    CGFloat x = [[frame valueForKey:@"x"] floatValue];
    CGFloat y = [[frame valueForKey:@"y"] floatValue];
    CGFloat width = [[frame valueForKey:@"w"] floatValue];
    CGFloat height = [[frame valueForKey:@"h"] floatValue];
    return CGRectMake(x, y, width, height);
}

- (void)setFramesWithDictionary:(NSDictionary*)frames
{
    if (_frameDict != frames) {
        [_frameDict release];
        _frameDict = [frames retain];
    }
}
@end
