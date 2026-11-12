// The base PlayHaven notification renderer represents a notification without artwork.
#import "PHNotificationRenderer.h"

@implementation PHNotificationRenderer
- (void)drawNotification:(NSDictionary*)notification inRect:(CGRect)rect
{
}

- (CGSize)sizeForNotification:(NSDictionary*)notification
{
    return CGSizeZero;
}
@end
