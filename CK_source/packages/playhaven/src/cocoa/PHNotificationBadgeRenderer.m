#import "PHNotificationBadgeRenderer.h"
#import "PHConstants.h"

static UIImage* BadgeImage;

static NSString* BadgeText(NSDictionary* notification)
{
    id value = [notification objectForKey:@"value"];
    if (![value isKindOfClass:[NSString class]] && ![value isKindOfClass:[NSNumber class]]) {
        return nil;
    }
    NSString* text = [value description];
    if ([text length] == 0 || [text isEqualToString:@"0"]) {
        return nil;
    }
    return text;
}

@implementation PHNotificationBadgeRenderer
+ (void)initialize
{
    if (self == [PHNotificationBadgeRenderer class]) {
        const uint32_t* image = badge_image;
        UIScreen* screen = [UIScreen mainScreen];
        if ([screen respondsToSelector:@selector(scale)] && [screen scale] == 2.0) {
            image = badge_2x_image;
        }
        BadgeImage = [[convertByteDataToUIImage(image) stretchableImageWithLeftCapWidth:14 topCapHeight:0] retain];
    }
}

- (void)drawNotification:(NSDictionary*)notification inRect:(CGRect)rect
{
    NSString* text = BadgeText(notification);
    if (text == nil) {
        return;
    }
    CGSize size = [self sizeForNotification:notification];
    [BadgeImage drawInRect:CGRectMake(0, 0, size.width, size.height)];
    [[UIColor whiteColor] set];
    [text drawAtPoint:CGPointMake(10.0f, 1.0f) withFont:[UIFont boldSystemFontOfSize:17.0f]];
}

- (CGSize)sizeForNotification:(NSDictionary*)notification
{
    NSString* text = BadgeText(notification);
    if (text == nil) {
        return CGSizeZero;
    }
    CGSize textSize = [text sizeWithFont:[UIFont boldSystemFontOfSize:17.0f]];
    return CGSizeMake(textSize.width + 20.0f, [BadgeImage size].height);
}
@end
