// GluGamesNetworkController.mm - singleton driving the Glu Games Network (gWorld) overlay.
// show/hide put the tab bar (iPhone) or vertical side bar (iPad) of web pages over the game,
// badge counts come from gServe, and the Game Center alias is passed to the home page.
#import "GluGamesNetworkController.h"
#import <GameKit/GameKit.h>
#import "CGluGamesNetwork.h"
#import "GGNWebViewController.h"
#import "TabViewController.h"
#import "GluVerticalTabBarController.h"
#import "SubNavController.h"

// Root of every Glu Games Network page
#define GGN_BASE_URL @"http://m.glu.com/"

static GluGamesNetworkController* GluGamesNetworkControllerSingletonDelegate;

@implementation GluGamesNetworkController

- (id) init
{
    self = [super init];
    if (self)
    {
        isVisible = NO;
        if ([[UIDevice currentDevice] respondsToSelector: @selector(userInterfaceIdiom)] &&
            [[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad)
        {
            iPadInterface = YES;
        }
        else
        {
            iPadInterface = NO;
        }

        // One update count per community sub-page
        gServeUpdates = [[NSMutableArray alloc] initWithCapacity: 6];
        for (int i = 0; i < 6; i++)
        {
            [gServeUpdates addObject: [NSNumber numberWithInt: 0]];
        }
    }
    return self;
}

- (void) dealloc
{
    [gServeUpdates release];
    [super dealloc];
}

- (void) show
{
    if (isVisible)
    {
        return;
    }

    NSString* baseUrl = [NSString stringWithString: GGN_BASE_URL];
    if (!iPadInterface)
    {
        tabViewController = [[TabViewController alloc] init];
        [tabViewController setDelegate: self];
    }
    else
    {
        verticalTabViewController = [[GluVerticalTabBarController alloc] init];
    }

    statusBarVisibility = [[UIApplication sharedApplication] isStatusBarHidden];
    [[UIApplication sharedApplication] setStatusBarHidden: YES];

    // One web page per tab
    NSMutableArray* pages = [[NSMutableArray alloc] initWithCapacity: [tabTitles count]];
    int index = 0;
    for (NSString* title in tabTitles)
    {
        GGNWebViewController* page = [[GGNWebViewController alloc] initWithStartUrl: [baseUrl stringByAppendingString: [tabUrls objectAtIndex: index]]];
        [page setTitle: title];
        [page setTabBarItem: [[[UITabBarItem alloc] initWithTitle: [page title] image: [tabIcons objectAtIndex: index] tag: index] autorelease]];
        [pages addObject: page];
        [page release];

        // A standalone component has no "To Game" tab
        if (standaloneComponent && index + 1 == [tabTitles count] - 1)
        {
            break;
        }
        index++;
    }

    if (!iPadInterface)
    {
        [tabViewController setViewControllers: pages];
        [tabViewController setSelectedIndex: 0];
        subNavController = [[SubNavController alloc] init];
        [subNavController setLastHighlighted: lastSelectedCommunityPage];
    }
    else
    {
        [verticalTabViewController setViewControllers: pages];
        [verticalTabViewController setLastHighlighted: lastSelectedCommunityPage];
    }
    [pages release];

    // Present modally over the game's root view controller if there is one
    UIWindow* window = [[UIApplication sharedApplication] keyWindow];
    UIViewController* rootController = nil;
    for (UIView* subview in [window subviews])
    {
        UIViewController* responder = (UIViewController*)[subview nextResponder];
        if ([responder isKindOfClass: [UIViewController class]])
        {
            rootController = responder;
            if (!iPadInterface)
            {
                [tabViewController setModalTransitionStyle: UIModalTransitionStyleCrossDissolve];
            }
            else
            {
                [verticalTabViewController setModalTransitionStyle: UIModalTransitionStyleCrossDissolve];
            }
            break;
        }
    }

    if (iPadInterface)
    {
        if (rootController)
        {
            BOOL landscape = [rootController shouldAutorotateToInterfaceOrientation: UIInterfaceOrientationLandscapeRight];
            [verticalTabViewController setModalPresentationStyle: UIModalPresentationFormSheet];
            [rootController presentModalViewController: verticalTabViewController animated: YES];

            // Enlarge the form sheet
            UIView* sheet = [[verticalTabViewController view] superview];
            if (landscape)
            {
                [sheet setFrame: CGRectMake(0, 0, 870, 750)];
                [sheet setCenter: CGPointMake(512, 384)];
            }
            else
            {
                [sheet setFrame: CGRectMake(0, 0, 750, 870)];
                [sheet setCenter: CGPointMake(384, 512)];
            }
        }
        else
        {
            [window addSubview: [verticalTabViewController view]];
            [[[window subviews] objectAtIndex: 0] setHidden: YES];
        }
    }
    else
    {
        if (rootController)
        {
            [rootController presentModalViewController: tabViewController animated: YES];
        }
        else
        {
            [UIView beginAnimations: nil context: NULL];
            [UIView setAnimationDuration: 0.75];
            [UIView setAnimationTransition: UIViewAnimationTransitionFlipFromLeft forView: window cache: YES];
            [[[window subviews] objectAtIndex: 0] setHidden: YES];
            [window addSubview: [tabViewController view]];
            [UIView commitAnimations];
        }
    }

    [self authenticateLocalPlayer];
    [self checkForUpdatedItems];
    isVisible = YES;
}

- (void) hide
{
    if (!isVisible)
    {
        return;
    }

    [[NSURLCache sharedURLCache] removeAllCachedResponses];

    UIWindow* window = [[UIApplication sharedApplication] keyWindow];
    UIViewController* rootController = nil;
    for (UIView* subview in [window subviews])
    {
        UIViewController* responder = (UIViewController*)[subview nextResponder];
        if ([responder isKindOfClass: [UIViewController class]])
        {
            rootController = responder;
            if (!iPadInterface)
            {
                [tabViewController setModalTransitionStyle: UIModalTransitionStyleCrossDissolve];
            }
            else
            {
                [verticalTabViewController setModalTransitionStyle: UIModalTransitionStyleCrossDissolve];
            }
            break;
        }
    }

    // The overlay lives in its own window: give the key status back
    if ([[window subviews] count] == 0)
    {
        if (!iPadInterface)
        {
            if (![[[tabViewController view] superview] isKeyWindow])
            {
                [[[tabViewController view] superview] makeKeyAndVisible];
            }
        }
        else
        {
            if (![[[verticalTabViewController view] superview] isKeyWindow])
            {
                [[[verticalTabViewController view] superview] makeKeyAndVisible];
            }
        }
    }

    if (iPadInterface)
    {
        if (rootController)
        {
            [verticalTabViewController dismissModalViewControllerAnimated: YES];
        }
        else
        {
            [[verticalTabViewController view] removeFromSuperview];
            [[[window subviews] objectAtIndex: 0] setHidden: NO];
        }
        lastSelectedCommunityPage = [verticalTabViewController lastHighlighted];
        [verticalTabViewController release];
        verticalTabViewController = nil;
    }
    else
    {
        if (rootController)
        {
            [tabViewController dismissModalViewControllerAnimated: YES];
        }
        else
        {
            [UIView beginAnimations: nil context: NULL];
            [UIView setAnimationDuration: 0.75];
            [UIView setAnimationTransition: UIViewAnimationTransitionFlipFromRight forView: window cache: YES];
            [[tabViewController view] removeFromSuperview];
            [[[window subviews] objectAtIndex: 0] setHidden: NO];
            [UIView commitAnimations];
        }
        lastSelectedCommunityPage = [subNavController lastHighlighted];
        [subNavController release];
        subNavController = nil;
        [tabViewController setDelegate: nil];
        [tabViewController release];
        tabViewController = nil;
    }

    [UIApplication sharedApplication].statusBarHidden = statusBarVisibility;
    isVisible = NO;
}

- (void) leave
{
    if (callerData != NULL)
    {
        ((CGluGamesNetwork*)callerData)->Hide();
    }
}

- (void) createCustomBadgeFromNumber: (int) number andAddToView: (UIView*) view
{
    UIImageView* badge = (UIImageView*)[view viewWithTag: 500];
    UILabel* label;
    if (badge == nil)
    {
        badge = [[UIImageView alloc] initWithFrame: CGRectMake(40, -3, 30, 25)];
        [badge setTag: 500];

        label = [[UILabel alloc] initWithFrame: CGRectMake(2, 7, 25, 11)];
        [label setTag: 501];
        [label setTextAlignment: UITextAlignmentCenter];
        [label setTextColor: [UIColor whiteColor]];
        [label setBackgroundColor: [UIColor clearColor]];
        [label setFont: [UIFont fontWithName: @"Arial Rounded MT Bold" size: 12]];
        [badge addSubview: label];
        [label release];

        [view addSubview: badge];
        [badge release];
    }
    else
    {
        label = (UILabel*)[badge viewWithTag: 501];
    }
    [badge setUserInteractionEnabled: YES];

    if (number != 0)
    {
        // Background widens with the number of digits
        if (number < 10)
        {
            [badge setImage: [UIImage imageNamed: @"onedigit_bg.png"]];
        }
        else if (number < 100)
        {
            [badge setImage: [UIImage imageNamed: @"twodigit_bg.png"]];
            [badge setFrame: CGRectMake(40, -3, 33, 25)];
            [label setFrame: CGRectMake(4, 7, 25, 11)];
        }
        else
        {
            [badge setImage: [UIImage imageNamed: @"threedigit_bg.png"]];
            [badge setFrame: CGRectMake(40, -3, 36, 25)];
            [label setFrame: CGRectMake(5, 7, 25, 11)];
        }
        [label setText: [NSString stringWithFormat: @"%d", number]];
    }
    else
    {
        // Note: a hidden badge is never shown again
        [badge setHidden: YES];
        [label setHidden: YES];
    }
}

- (void) tabBarController: (UITabBarController*) tabBarController didSelectViewController: (UIViewController*) viewController
{
    NSUInteger selected = [[viewController tabBarController] selectedIndex];
    if (selected == 2)
    {
        // Community tab toggles the sub-page grid
        if ([[subNavController view] superview] == nil)
        {
            [self checkForUpdatedItems];
            [subNavController highlightLastUsedButton];
            [[viewController view] addSubview: [subNavController view]];
        }
        else
        {
            [[subNavController view] removeFromSuperview];
        }
    }
    else if (selected == 4)
    {
        // "To Game"
        [self leave];
    }
}

- (BOOL) isGameCenterAPIAvailable
{
    // GKLocalPlayer exists and iOS >= 4.1
    Class localPlayerClass = NSClassFromString(@"GKLocalPlayer");
    NSString* requiredVersion = @"4.1";
    NSString* currentVersion = [[UIDevice currentDevice] systemVersion];
    BOOL versionSupported = [currentVersion compare: requiredVersion options: NSNumericSearch] != NSOrderedAscending;
    return localPlayerClass && versionSupported;
}

- (void) authenticateLocalPlayer
{
    if ([self isGameCenterAPIAvailable])
    {
        GKLocalPlayer* localPlayer = [GKLocalPlayer localPlayer];
        [localPlayer authenticateWithCompletionHandler: ^(NSError* error)
        {
            dispatch_async(dispatch_get_main_queue(), ^
            {
                if ([localPlayer isAuthenticated])
                {
                    // Pass the gamer tag to the home page
                    GGNWebViewController* homePage = [[tabViewController viewControllers] objectAtIndex: 0];
                    NSUInteger selected = [tabViewController selectedIndex];
                    NSString* homeUrl = [@"http://m.glu.com/gworld/exclusive-content" stringByAppendingFormat: @"?gamerTag=%@", [localPlayer alias]];
                    if (selected == 0)
                    {
                        [homePage reloadWithUrl: homeUrl];
                    }
                    else
                    {
                        [homePage setDefaultUrl: homeUrl];
                    }
                    [NSThread detachNewThreadSelector: @selector(getAndSubmitHighScore) toTarget: self withObject: nil];
                }
                else
                {
                    NSLog(@"GameCenter not authenticated: %@", [error localizedDescription]);
                }
            });
        }];
    }
}

- (void) getAndSubmitHighScore
{
    // Runs on its own thread
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    NSString* urlString = [[NSString stringWithFormat: @"%@gworld/get_gamecenterpoints?gamerTag=%@", GGN_BASE_URL, [[GKLocalPlayer localPlayer] alias]]
                           stringByAddingPercentEscapesUsingEncoding: NSUTF8StringEncoding];
    NSURL* url = [[NSURL alloc] initWithString: urlString];
    NSError* error = nil;
    if (url != nil)
    {
        NSData* data = [NSData dataWithContentsOfURL: url options: NSDataReadingUncached error: &error];
        if (error == nil)
        {
            NSString* response = [[NSString alloc] initWithData: data encoding: NSUTF8StringEncoding];
            if (response != nil && [response length] != 0)
            {
                // Glu points become the Game Center score
                long long points = [self valueForField: @"points" inString: response];
                GKScore* score = [[[GKScore alloc] initWithCategory: @"GLUGAMES_GLU_POINTS"] autorelease];
                [score setValue: points];
                [score reportScoreWithCompletionHandler: ^(NSError* reportError)
                {
                }];
            }
            [response release];
        }
        [url release];
    }

    [pool release];
}

- (long long) valueForField: (NSString*) field inString: (NSString*) string
{
    // Number following the first occurrence of field, 0 if absent
    long long value = 0;
    if ([string rangeOfString: field].location != NSNotFound)
    {
        NSScanner* scanner = [NSScanner scannerWithString: string];
        [scanner scanUpToString: field intoString: nil];
        [scanner scanString: field intoString: nil];
        [scanner scanUpToCharactersFromSet: [NSCharacterSet characterSetWithCharactersInString: @"0123456789"] intoString: nil];
        [scanner scanLongLong: &value];
    }
    return value;
}

- (void) reloadTab: (int) tab withUrl: (NSString*) url
{
    [tabUrls replaceObjectAtIndex: tab withObject: url];
    NSString* fullUrl = [GGN_BASE_URL stringByAppendingString: url];
    if (!iPadInterface)
    {
        [[[tabViewController viewControllers] objectAtIndex: tab] reloadWithUrl: fullUrl];
    }
    else
    {
        [[[verticalTabViewController viewControllers] objectAtIndex: tab] reloadWithUrl: fullUrl];
    }
}

- (void) checkForUpdatedItems
{
    if ([self gServeConnection] != nil)
    {
        return;
    }

    // Facebook user id from the gWorld cookies
    NSArray* cookies = [[NSHTTPCookieStorage sharedHTTPCookieStorage] cookiesForURL: [NSURL URLWithString: GGN_BASE_URL]];
    NSString* facebookId = nil;
    for (NSHTTPCookie* cookie in cookies)
    {
        if ([[cookie name] isEqualToString: @"gworld_facebook_id"])
        {
            facebookId = [[cookie value] retain];
            break;
        }
        if (facebookId == nil)
        {
            NSString* value = [cookie value];
            if ([value rangeOfString: @"uid="].location != NSNotFound)
            {
                NSArray* parts = [value componentsSeparatedByString: @"="];
                if ([parts count] > 1)
                {
                    facebookId = [[parts objectAtIndex: 1] stringByReplacingOccurrencesOfString: @"\"" withString: @""];
                }
            }
        }
    }

    NSString* urlString = [NSString stringWithFormat: @"%@gworld/get-unread-count?facebookuser_id=%@", GGN_BASE_URL, facebookId];
    NSMutableURLRequest* request = [[NSMutableURLRequest alloc] initWithURL: [NSURL URLWithString: urlString]];
    [request setValue: @"iPhone OS" forHTTPHeaderField: @"User-Agent"];
    [self setGServeData: [NSMutableData data]];
    NSURLConnection* connection = [[NSURLConnection alloc] initWithRequest: request delegate: self];
    [self setGServeConnection: connection];
    [request release];
    [connection release];
}

- (void) checkForGServeUpdates: (NSTimer*) timer
{
    if (gServeId != 0)
    {
        NSString* urlString = [NSString stringWithFormat: @"http://gworld-stage.glu.com/gworld/get-unread-count?gsid=%u", gServeId];
        NSMutableURLRequest* request = [[NSMutableURLRequest alloc] initWithURL: [NSURL URLWithString: urlString]];
        [request setValue: @"iPhone OS" forHTTPHeaderField: @"User-Agent"];
        [self setUpdatesData: [NSMutableData data]];
        NSURLConnection* connection = [[NSURLConnection alloc] initWithRequest: request delegate: self];
        [self setUpdatesConnection: connection];
        [request release];
        [connection release];
    }
}

- (void) connection: (NSURLConnection*) connection didReceiveData: (NSData*) data
{
    if (gServeConnection == connection)
    {
        [[self gServeData] appendData: data];
    }
    else
    {
        [[self updatesData] appendData: data];
    }
}

- (void) connection: (NSURLConnection*) connection didFailWithError: (NSError*) error
{
    NSLog(@"[GGN]: gServe returned an error: %@", [error description]);
    if (gServeConnection == connection)
    {
        [self setGServeConnection: nil];
        [self setGServeData: nil];
    }
    else
    {
        [self setUpdatesConnection: nil];
        [self setUpdatesData: nil];
        if (callerData != NULL)
        {
            // Retry in two minutes
            [NSTimer scheduledTimerWithTimeInterval: 120 target: self selector: @selector(checkForGServeUpdates:) userInfo: nil repeats: NO];
        }
    }
}

- (void) connectionDidFinishLoading: (NSURLConnection*) connection
{
    if (gServeConnection == connection)
    {
        NSString* response = [[NSString alloc] initWithData: [self gServeData] encoding: NSUTF8StringEncoding];
        NSRange newsRange = [response rangeOfString: @"unread_news_count"];
        NSRange messagesRange = [response rangeOfString: @"unread_msg_count"];
        if (newsRange.location != NSNotFound && messagesRange.location != NSNotFound)
        {
            // Counts in community sub-page order
            NSNumber* friendsCount = [NSNumber numberWithInt: [self valueForField: @"friends_count" inString: response]];
            NSNumber* newsCount = [NSNumber numberWithInt: [self valueForField: @"unread_news_count" inString: response]];
            NSNumber* messagesCount = [NSNumber numberWithInt: [self valueForField: @"unread_msg_count" inString: response]];
            NSNumber* forumCount = [NSNumber numberWithInt: [self valueForField: @"unread_forum_count" inString: response]];
            NSNumber* leaderboardsCount = [NSNumber numberWithInt: [self valueForField: @"leaderboards_count" inString: response]];
            NSNumber* challengesCount = [NSNumber numberWithInt: [self valueForField: @"challenges_count" inString: response]];
            [self setGServeUpdates: [NSMutableArray arrayWithObjects: friendsCount, newsCount, messagesCount, forumCount, leaderboardsCount, challengesCount, nil]];
        }
        else
        {
            NSMutableArray* updates = [[NSMutableArray alloc] initWithCapacity: 6];
            for (int i = 0; i < 6; i++)
            {
                [updates addObject: [NSNumber numberWithInt: 0]];
            }
            [self setGServeUpdates: updates];
            [updates release];
        }
        [response release];
        [self setGServeConnection: nil];
        [self setGServeData: nil];

        if (!iPadInterface)
        {
            [subNavController refreshBadgeValues];
        }
        else
        {
            [verticalTabViewController refreshBadgeValues];
        }
    }
    else
    {
        NSString* response = [[NSString alloc] initWithData: [self updatesData] encoding: NSUTF8StringEncoding];
        int total = [self valueForField: @"total_count_value" inString: response];
        int refreshSeconds = [self valueForField: @"service_refresh_value" inString: response];
        if (callerData != NULL)
        {
            ((CGluGamesNetwork*)callerData)->ReportGServeUpdates(total);
            if (refreshSeconds != 0)
            {
                [NSTimer scheduledTimerWithTimeInterval: refreshSeconds target: self selector: @selector(checkForGServeUpdates:) userInfo: nil repeats: NO];
            }
        }
        [response release];
        [self setUpdatesConnection: nil];
        [self setUpdatesData: nil];
    }
}

// Singleton boilerplate

+ (GluGamesNetworkController*) sharedInstance
{
    if (GluGamesNetworkControllerSingletonDelegate == nil)
    {
        GluGamesNetworkControllerSingletonDelegate = [[super allocWithZone: NULL] init];
    }
    return GluGamesNetworkControllerSingletonDelegate;
}

+ (id) allocWithZone: (NSZone*) zone
{
    return [[self sharedInstance] retain];
}

- (id) copyWithZone: (NSZone*) zone
{
    return self;
}

- (id) retain
{
    return self;
}

- (NSUInteger) retainCount
{
    return NSUIntegerMax;
}

// declared plain void in the original (method type "v8@0:4", not oneway)
- (void) release
{
}

- (id) autorelease
{
    return self;
}

@synthesize callerData;
@synthesize gameId;
@synthesize isVisible;
@synthesize tabUrls;
@synthesize tabIcons;
@synthesize tabInactiveIcons;
@synthesize tabTitles;
@synthesize standaloneComponent;
@synthesize gServeId;
@synthesize gServeData;
@synthesize gServeConnection;
@synthesize gServeUpdates;
@synthesize updatesData;
@synthesize updatesConnection;
@synthesize tabViewController;
@synthesize subUrls;
@synthesize subTitles;
@synthesize subActiveIcons;
@synthesize subInactiveIcons;

@end
