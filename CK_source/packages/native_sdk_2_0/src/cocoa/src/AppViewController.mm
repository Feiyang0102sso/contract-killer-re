// AppViewController.mm - view controller that holds the GL view while auto-rotation is enabled.
#import "AppViewController.h"

@implementation AppViewController

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return UIInterfaceOrientationIsLandscape(interfaceOrientation);
}

@end
