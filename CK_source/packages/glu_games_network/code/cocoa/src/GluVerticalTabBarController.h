// GluVerticalTabBarController.h - vertical (iPad, landscape) tab bar of the Glu Games Network:
// a side bar table on the left, the selected page on the right.
#import <UIKit/UIKit.h>

@interface GluVerticalTabBarController : UIViewController <UITableViewDataSource, UITableViewDelegate>
{
    UITableView*        sideBar;
    NSArray*            viewControllers;
    UIViewController*   selectedViewController;
    unsigned int        selectedIndex;          // selected side bar row
    BOOL                submenuPresent;         // community sub-rows (3..8) are shown
    unsigned int        lastHighlighted;        // last selected sub-row - 2 (1-based), 0 = none
}

@property (nonatomic, retain) UIViewController* selectedViewController;
@property (nonatomic, retain) NSArray* viewControllers;
@property (nonatomic) unsigned int selectedIndex;
@property (nonatomic) unsigned int lastHighlighted;

- (void) refreshBadgeValues;

@end
