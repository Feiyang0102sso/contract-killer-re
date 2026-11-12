// SubNavController.m - pop-up grid of community sub-page buttons (Friends, News, Messages, ...).
// Two columns of buttons over a background image; a press reloads the community tab.
#import "SubNavController.h"
#import "GluGamesNetworkController.h"

@implementation SubNavController

- (void) loadView
{
    UIImageView* backgroundView = [[UIImageView alloc] initWithImage: [UIImage imageNamed: @"community_bg.png"]];
    [backgroundView setBounds: CGRectMake(0, 0, 210, 200)];

    // Sit at the bottom centre of the current page
    CGRect pageBounds = [[[[[GluGamesNetworkController sharedInstance] tabViewController] selectedViewController] view] bounds];
    [backgroundView setCenter: CGPointMake(pageBounds.size.width / 2 - 1, pageBounds.size.height - [backgroundView bounds].size.height / 2)];
    [backgroundView setAutoresizingMask: UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleTopMargin];
    [backgroundView setUserInteractionEnabled: YES];

    NSMutableArray* buttonList = [[NSMutableArray alloc] initWithCapacity: 6];
    GluGamesNetworkController* controller = [GluGamesNetworkController sharedInstance];

    int leftY = 20;
    int rightY = 20;
    int index = 0;
    for (UIImage* activeIcon in [controller subActiveIcons])
    {
        UIButton* button = [UIButton buttonWithType: UIButtonTypeCustom];

        // Even buttons in the left column, odd ones in the right column
        CGRect frame;
        if (index % 2 == 0)
        {
            frame = CGRectMake(17, leftY, 80, 42);
            leftY += 55;
        }
        else
        {
            frame = CGRectMake(117, rightY, 80, 42);
            rightY += 55;
        }
        [button setFrame: frame];
        [button setTag: index + 1];
        [button setBackgroundImage: [[controller subInactiveIcons] objectAtIndex: index] forState: UIControlStateNormal];
        [button setBackgroundImage: activeIcon forState: UIControlStateHighlighted];
        [button setTitle: [[controller subTitles] objectAtIndex: index] forState: UIControlStateNormal];
        [button setTitleColor: [UIColor darkGrayColor] forState: UIControlStateNormal];
        [button setTitleColor: [UIColor whiteColor] forState: UIControlStateHighlighted];
        button.titleLabel.font = [UIFont fontWithName: @"Arial Rounded MT Bold" size: 10];
        [button setTitleEdgeInsets: UIEdgeInsetsMake(14, 0, -14, 0)];
        [button addTarget: self action: @selector(subNavButtonAction:) forControlEvents: UIControlEventTouchUpInside];
        [backgroundView addSubview: button];
        [buttonList addObject: button];
        index++;
    }

    [self setButtons: [NSArray arrayWithArray: buttonList]];
    [buttonList release];
    [self highlightLastUsedButton];

    [self setView: backgroundView];
    [backgroundView release];
}

- (void) viewDidUnload
{
    [super viewDidUnload];
    [self setButtons: nil];
}

- (void) subNavButtonAction: (id) sender
{
    if (lastHighlighted != 0)
    {
        [[[self buttons] objectAtIndex: lastHighlighted - 1] setHighlighted: NO];
    }
    lastHighlighted = [sender tag];
    [[self view] removeFromSuperview];

    NSString* url = [[[GluGamesNetworkController sharedInstance] subUrls] objectAtIndex: [sender tag] - 1];
    if ([url length] != 0)
    {
        // The community pages live in the third tab
        [[GluGamesNetworkController sharedInstance] reloadTab: 2 withUrl: url];
    }
}

- (void) highlightLastUsedButton
{
    if (lastHighlighted != 0)
    {
        [[[self buttons] objectAtIndex: lastHighlighted - 1] setHighlighted: YES];
    }
}

- (void) refreshBadgeValues
{
    GluGamesNetworkController* controller = [GluGamesNetworkController sharedInstance];

    // One badge per sub-page button
    int index = 0;
    for (UIButton* button in [self buttons])
    {
        [controller createCustomBadgeFromNumber: [[[controller gServeUpdates] objectAtIndex: index] intValue] andAddToView: button];
        index++;
    }

    // The community tab shows the total
    int total = 0;
    for (NSNumber* count in [controller gServeUpdates])
    {
        total += [count intValue];
    }
    if (total != 0)
    {
        NSString* badgeValue = [NSString stringWithFormat: @"%i", total];
        [[[[[controller tabViewController] tabBar] items] objectAtIndex: 2] setBadgeValue: badgeValue];
    }
    else
    {
        [[[[[controller tabViewController] tabBar] items] objectAtIndex: 2] setBadgeValue: nil];
    }
}

@synthesize buttons;
@synthesize lastHighlighted;

@end
