// CGluGamesNetwork.mm - C++ entry point of the Glu Games Network (gWorld) overlay.
// Configures the GluGamesNetworkController singleton (tabs, community sub-pages, icons) and shows it.
#import "CGluGamesNetwork.h"
#import "GluGamesNetworkController.h"

CGluGamesNetwork::CGluGamesNetwork(const char* gameId, void (*onExit)(void*), void* exitContext, unsigned int gServeId, bool standaloneComponent)
    : m_onGServeUpdates(NULL)
    , m_gServeUpdatesContext(NULL)
    , m_lastGServeUpdates(0)
{
    NSString* gameIdString = [NSString stringWithCString: gameId encoding: NSASCIIStringEncoding];
    [[GluGamesNetworkController sharedInstance] setGameId: gameIdString];
    m_onExit = onExit;
    m_exitContext = exitContext;
    [[GluGamesNetworkController sharedInstance] setGServeId: gServeId];
    [[GluGamesNetworkController sharedInstance] setStandaloneComponent: standaloneComponent];
    [[GluGamesNetworkController sharedInstance] setCallerData: this];
}

void CGluGamesNetwork::Show()
{
    GluGamesNetworkController* controller = [GluGamesNetworkController sharedInstance];

    // Defaults for everything the game did not configure itself
    if ([controller tabTitles] == nil)
    {
        [controller setTabTitles: [NSArray arrayWithObjects: @"Home", @"Profile", @"Community", @"Games", @"To Game", nil]];
    }
    if ([controller tabIcons] == nil)
    {
        [controller setTabIcons: [NSArray arrayWithObjects:
                                  [UIImage imageNamed: @"menu_home.png"],
                                  [UIImage imageNamed: @"menu_profile.png"],
                                  [UIImage imageNamed: @"menu_community.png"],
                                  [UIImage imageNamed: @"menu_more.png"],
                                  [UIImage imageNamed: @"menu_back.png"],
                                  nil]];
    }
    if ([controller tabInactiveIcons] == nil)
    {
        [controller setTabInactiveIcons: [NSArray arrayWithObjects:
                                          [UIImage imageNamed: @"menu_home_off.png"],
                                          [UIImage imageNamed: @"menu_profile_off.png"],
                                          [UIImage imageNamed: @"menu_community_off.png"],
                                          [UIImage imageNamed: @"menu_more_off.png"],
                                          [UIImage imageNamed: @"menu_back.png"],
                                          nil]];
    }
    if ([controller tabUrls] == nil)
    {
        // The last tab ("To Game") has no page
        [controller setTabUrls: [NSMutableArray arrayWithObjects: @"gworld/exclusive-content", @"gworld/profile", @"gworld/community", @"gworld/more-games", @"", nil]];
    }
    if ([controller subUrls] == nil)
    {
        [controller setSubUrls: [NSArray arrayWithObjects:
                                 [NSString stringWithFormat: @"gworld/more-friendlist"],
                                 [NSString stringWithFormat: @"gworld/news"],
                                 [NSString stringWithFormat: @"gworld/messages/inbox"],
                                 [NSString stringWithFormat: @"gworld/gworldforum"],
                                 [NSString stringWithFormat: @"gworld/leaderboard"],
                                 [NSString stringWithFormat: @"gworld/challenge"],
                                 nil]];
    }
    if ([controller subTitles] == nil)
    {
        [controller setSubTitles: [NSArray arrayWithObjects: @"Friends", @"News", @"Messages", @"Forums", @"Leaderboards", @"Challenges", nil]];
    }
    if ([controller subActiveIcons] == nil)
    {
        [controller setSubActiveIcons: [NSArray arrayWithObjects:
                                        [UIImage imageNamed: @"friends_on.png"],
                                        [UIImage imageNamed: @"news_on.png"],
                                        [UIImage imageNamed: @"messages_on.png"],
                                        [UIImage imageNamed: @"forums_on.png"],
                                        [UIImage imageNamed: @"leaderboards_on.png"],
                                        [UIImage imageNamed: @"challenges_on.png"],
                                        nil]];
    }
    if ([controller subInactiveIcons] == nil)
    {
        [controller setSubInactiveIcons: [NSArray arrayWithObjects:
                                          [UIImage imageNamed: @"friends_off.png"],
                                          [UIImage imageNamed: @"news_off.png"],
                                          [UIImage imageNamed: @"messages_off.png"],
                                          [UIImage imageNamed: @"forums_off.png"],
                                          [UIImage imageNamed: @"leaderboards_off.png"],
                                          [UIImage imageNamed: @"challenges_off.png"],
                                          nil]];
    }

    [controller show];
}

void CGluGamesNetwork::Hide()
{
    GluGamesNetworkController* controller = [GluGamesNetworkController sharedInstance];
    [controller hide];

    // Drop the icon arrays (reloaded by the next Show)
    [controller setTabIcons: nil];
    [controller setTabInactiveIcons: nil];
    [controller setSubActiveIcons: nil];
    [controller setSubInactiveIcons: nil];

    m_onExit(m_exitContext);
}

void CGluGamesNetwork::ReportGServeUpdates(int count)
{
    if (m_lastGServeUpdates != count && m_onGServeUpdates != NULL)
    {
        m_onGServeUpdates(m_gServeUpdatesContext, count);
        m_lastGServeUpdates = count;
    }
}
