// SubNavController.h - pop-up grid of community sub-page buttons (Friends, News, Messages, ...).
#import <UIKit/UIKit.h>

@interface SubNavController : UIViewController
{
    NSArray*        buttons;
    unsigned int    lastHighlighted;    // tag of the last pressed button (1-based), 0 = none
}

@property (nonatomic, retain) NSArray* buttons;
@property (nonatomic) unsigned int lastHighlighted;

- (void) subNavButtonAction: (id) sender;
- (void) highlightLastUsedButton;
- (void) refreshBadgeValues;

@end
