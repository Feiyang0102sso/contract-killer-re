// TabViewController.m - tab bar controller hosting the Glu Games Network pages (iPhone layout).
#import "TabViewController.h"

@implementation TabViewController

- (BOOL) shouldAutorotateToInterfaceOrientation: (UIInterfaceOrientation) interfaceOrientation
{
    // Portrait only
    return interfaceOrientation == UIInterfaceOrientationPortrait;
}

@end
