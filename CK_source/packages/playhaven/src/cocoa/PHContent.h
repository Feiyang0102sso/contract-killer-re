#ifndef PHCONTENT_H
#define PHCONTENT_H

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

typedef enum {
    PHContentTransitionUnknown,
    PHContentTransitionModal,
    PHContentTransitionDialog
} PHContentTransitionType;

@interface PHContent : NSObject {
    NSDictionary* _frameDict;
    NSURL* _URL;
    PHContentTransitionType _transition;
    NSDictionary* _context;
    NSTimeInterval _closeButtonDelay;
    NSString* _closeButtonURLPath;
}
@property (nonatomic, retain) NSURL* URL;
@property (nonatomic, assign) PHContentTransitionType transition;
@property (nonatomic, retain) NSDictionary* context;
@property (nonatomic, assign) NSTimeInterval closeButtonDelay;
@property (nonatomic, copy) NSString* closeButtonURLPath;

+ (id)contentWithDictionary:(NSDictionary*)description;
- (CGRect)frameForOrientation:(UIInterfaceOrientation)orientation;
- (void)setFramesWithDictionary:(NSDictionary*)frames;
@end

#endif
