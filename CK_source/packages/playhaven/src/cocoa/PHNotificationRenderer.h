#ifndef PHNOTIFICATIONRENDERER_H
#define PHNOTIFICATIONRENDERER_H

#import <UIKit/UIKit.h>

@interface PHNotificationRenderer : NSObject
- (void)drawNotification:(NSDictionary*)notification inRect:(CGRect)rect;
- (CGSize)sizeForNotification:(NSDictionary*)notification;
@end

#endif
