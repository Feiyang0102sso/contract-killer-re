// GluVerticalTabBarController.m - vertical (iPad, landscape) tab bar of the Glu Games Network.
// Side bar rows: Home, Profile, Community, Games, To Game; selecting Community expands six
// community sub-rows (3..8) below it, shifting Games and To Game to rows 9 and 10.
#import "GluVerticalTabBarController.h"
#import "GluVerticalCell.h"
#import "GluGamesNetworkController.h"

@interface GluVerticalTabBarController (private)
- (void) selectionChangedTo: (unsigned int) index;
@end

@implementation GluVerticalTabBarController

- (void) loadView
{
    CGRect screenBounds = [[UIScreen mainScreen] bounds];
    UIView* contentView = [[UIView alloc] initWithFrame: screenBounds];
    [contentView setAutoresizingMask: UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleRightMargin |
                                      UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleBottomMargin];

    // 80 pt wide side bar over the full height
    CGRect sideBarFrame = CGRectMake(0, 0, 80, [contentView bounds].size.height);
    sideBar = [[UITableView alloc] initWithFrame: sideBarFrame style: UITableViewStylePlain];
    [sideBar setDataSource: self];
    [sideBar setDelegate: self];
    [sideBar setSeparatorStyle: UITableViewCellSeparatorStyleNone];
    [sideBar setScrollEnabled: NO];
    sideBar.backgroundColor = [UIColor colorWithWhite: 0.2f alpha: 1.0f];
    [contentView addSubview: sideBar];

    [self setView: contentView];
    [contentView release];

    selectedIndex = 0;
    NSIndexPath* selectedPath = [NSIndexPath indexPathForRow: selectedIndex inSection: 0];
    [sideBar selectRowAtIndexPath: selectedPath animated: NO scrollPosition: UITableViewScrollPositionNone];
    [self selectionChangedTo: 0];
}

- (void) viewDidUnload
{
    [super viewDidUnload];
    [sideBar setDataSource: nil];
    [sideBar setDelegate: nil];
    [sideBar release];
    sideBar = nil;
    [[selectedViewController view] removeFromSuperview];
    [self setSelectedViewController: nil];
}

- (BOOL) shouldAutorotateToInterfaceOrientation: (UIInterfaceOrientation) interfaceOrientation
{
    // Landscape only
    return interfaceOrientation == UIInterfaceOrientationLandscapeRight || interfaceOrientation == UIInterfaceOrientationLandscapeLeft;
}

- (NSInteger) tableView: (UITableView*) tableView numberOfRowsInSection: (NSInteger) section
{
    return submenuPresent ? [viewControllers count] + 6 : [viewControllers count];
}

- (UITableViewCell*) tableView: (UITableView*) tableView cellForRowAtIndexPath: (NSIndexPath*) indexPath
{
    GluGamesNetworkController* controller = [GluGamesNetworkController sharedInstance];
    UITableViewCell* cell;

    if (!submenuPresent || [indexPath row] < 3 || [indexPath row] > 8)
    {
        // Main row: one per view controller
        cell = [tableView dequeueReusableCellWithIdentifier: @"Custom Cell"];
        if (cell == nil)
        {
            cell = [[[GluVerticalCell alloc] initWithStyle: UITableViewCellStyleDefault reuseIdentifier: @"Custom Cell"] autorelease];
            UIView* selectedView = [[UIView alloc] init];
            [selectedView setBackgroundColor: [UIColor darkGrayColor]];
            [cell setSelectedBackgroundView: selectedView];
            [selectedView release];
        }

        int controllerIndex;
        if (submenuPresent && [indexPath row] >= 9)
        {
            controllerIndex = [indexPath row] - 6;
        }
        else
        {
            controllerIndex = [indexPath row];
        }
        UIViewController* viewController = [viewControllers objectAtIndex: controllerIndex];
        [cell setTag: 2];
        cell.textLabel.text = [viewController title];
        cell.imageView.image = [[controller tabInactiveIcons] objectAtIndex: controllerIndex];
        cell.imageView.highlightedImage = [[controller tabIcons] objectAtIndex: controllerIndex];

        if ([indexPath row] == 2)
        {
            // Community row: arrow accessory (U+27A7)
            UILabel* arrowLabel = [[UILabel alloc] init];
            unichar arrow = 0x27A7;
            NSString* arrowText = [[NSString alloc] initWithCharacters: &arrow length: 1];
            [arrowLabel setText: arrowText];
            [arrowText release];
            [arrowLabel setTextColor: [UIColor whiteColor]];
            [arrowLabel setFont: [UIFont fontWithName: @"Helvetica-Bold" size: 18]];
            [arrowLabel setBackgroundColor: [UIColor clearColor]];
            [arrowLabel sizeToFit];
            [cell setAccessoryView: arrowLabel];
            [arrowLabel release];
        }
    }
    else
    {
        // Community sub-row
        cell = [tableView dequeueReusableCellWithIdentifier: @"Custom Sub Cell"];
        if (cell == nil)
        {
            cell = [[[GluVerticalCell alloc] initWithStyle: UITableViewCellStyleDefault reuseIdentifier: @"Custom Sub Cell"] autorelease];
            UIView* selectedView = [[UIView alloc] init];
            [selectedView setBackgroundColor: [UIColor darkGrayColor]];
            [cell setSelectedBackgroundView: selectedView];
            [selectedView release];
        }

        [cell setTag: 1];
        cell.textLabel.text = [[controller subTitles] objectAtIndex: [indexPath row] - 3];
        if (lastHighlighted == 0 || [indexPath row] - 2 != lastHighlighted)
        {
            cell.imageView.image = [[controller subInactiveIcons] objectAtIndex: [indexPath row] - 3];
        }
        else
        {
            cell.imageView.image = [[controller subActiveIcons] objectAtIndex: [indexPath row] - 3];
        }
    }
    return cell;
}

- (CGFloat) tableView: (UITableView*) tableView heightForRowAtIndexPath: (NSIndexPath*) indexPath
{
    return 60;
}

- (void) tableView: (UITableView*) tableView willDisplayCell: (UITableViewCell*) cell forRowAtIndexPath: (NSIndexPath*) indexPath
{
    // Sub-rows are darker
    if ([cell tag] == 1)
    {
        [cell setBackgroundColor: [UIColor colorWithWhite: 0.1f alpha: 1.0f]];
    }
}

- (void) tableView: (UITableView*) tableView didSelectRowAtIndexPath: (NSIndexPath*) indexPath
{
    if (submenuPresent && [indexPath row] > 2 && [indexPath row] < 9)
    {
        GluGamesNetworkController* controller = [GluGamesNetworkController sharedInstance];

        // Swap the icon of the previously selected sub-row back to inactive
        if (lastHighlighted != 0)
        {
            UITableViewCell* oldCell = [sideBar cellForRowAtIndexPath: [NSIndexPath indexPathForRow: lastHighlighted + 2 inSection: 0]];
            oldCell.imageView.image = [[controller subInactiveIcons] objectAtIndex: lastHighlighted - 1];
        }
        lastHighlighted = [indexPath row] - 2;
        UITableViewCell* cell = [sideBar cellForRowAtIndexPath: indexPath];
        cell.imageView.image = [[controller subActiveIcons] objectAtIndex: [indexPath row] - 3];
    }
    [self selectionChangedTo: [indexPath row]];
}

- (void) refreshBadgeValues
{
    GluGamesNetworkController* controller = [GluGamesNetworkController sharedInstance];

    // One badge per community sub-row
    if (submenuPresent)
    {
        for (int i = 0; i < 6; i++)
        {
            int count = [[[controller gServeUpdates] objectAtIndex: i] intValue];
            NSIndexPath* rowPath = [NSIndexPath indexPathForRow: i + 3 inSection: 0];
            [controller createCustomBadgeFromNumber: count andAddToView: [[sideBar cellForRowAtIndexPath: rowPath] contentView]];
        }
    }

    // The community row shows the total
    int total = 0;
    for (NSNumber* count in [controller gServeUpdates])
    {
        total += [count intValue];
    }
    [controller createCustomBadgeFromNumber: total andAddToView: [[sideBar cellForRowAtIndexPath: [NSIndexPath indexPathForRow: 2 inSection: 0]] contentView]];
}

@synthesize selectedViewController;
@synthesize viewControllers;
@synthesize selectedIndex;
@synthesize lastHighlighted;

@end

@implementation GluVerticalTabBarController (private)

- (void) selectionChangedTo: (unsigned int) index
{
    GluGamesNetworkController* controller = [GluGamesNetworkController sharedInstance];

    if (index == 2)
    {
        // Community row toggles the sub-rows
        submenuPresent = !submenuPresent;
        NSArray* indexPaths = [[NSArray alloc] initWithObjects:
                               [NSIndexPath indexPathForRow: 3 inSection: 0],
                               [NSIndexPath indexPathForRow: 4 inSection: 0],
                               [NSIndexPath indexPathForRow: 5 inSection: 0],
                               [NSIndexPath indexPathForRow: 6 inSection: 0],
                               [NSIndexPath indexPathForRow: 7 inSection: 0],
                               [NSIndexPath indexPathForRow: 8 inSection: 0],
                               nil];
        if (submenuPresent)
        {
            [sideBar insertRowsAtIndexPaths: indexPaths withRowAnimation: UITableViewRowAnimationTop];
        }
        else
        {
            // Drop the badges before the rows go away
            for (NSIndexPath* indexPath in indexPaths)
            {
                UITableViewCell* cell = [sideBar cellForRowAtIndexPath: indexPath];
                for (UIView* subview in [[cell contentView] subviews])
                {
                    [subview removeFromSuperview];
                }
            }
            [sideBar deleteRowsAtIndexPaths: indexPaths withRowAnimation: UITableViewRowAnimationTop];
        }
        [indexPaths release];
        [controller checkForUpdatedItems];
    }

    // "To Game" row
    if ((!submenuPresent && index == 4) || index == 10)
    {
        [controller leave];
        return;
    }

    [selectedViewController viewWillDisappear: NO];
    [[selectedViewController view] removeFromSuperview];
    [selectedViewController viewDidDisappear: NO];

    if (!submenuPresent || index <= 2)
    {
        [self setSelectedViewController: [viewControllers objectAtIndex: index]];
    }
    else if (index >= 3 && index <= 8)
    {
        // Community sub-page: reload the community tab with its url
        [self setSelectedViewController: [viewControllers objectAtIndex: 2]];
        [controller reloadTab: 2 withUrl: [[controller subUrls] objectAtIndex: index - 3]];
    }
    else
    {
        [self setSelectedViewController: [viewControllers objectAtIndex: index - 6]];
    }

    // Page fills the space right of the side bar
    CGRect pageFrame = CGRectMake(sideBar.bounds.size.width, 0, self.view.bounds.size.width - sideBar.bounds.size.width, self.view.bounds.size.height);
    selectedViewController.view.frame = pageFrame;
    [self.view addSubview: selectedViewController.view];
    [selectedViewController viewWillAppear: NO];
    [selectedViewController viewDidAppear: NO];
    [self setTitle: [selectedViewController title]];
    selectedIndex = index;
}

@end
