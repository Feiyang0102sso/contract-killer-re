// GluGamesNetworkController.h - singleton driving the Glu Games Network (gWorld) overlay:
// tab/side bar of web pages, badge counts from gServe and Game Center hooks.
#import <UIKit/UIKit.h>

@class TabViewController;
@class GluVerticalTabBarController;
@class SubNavController;

@interface GluGamesNetworkController : NSObject <UITabBarControllerDelegate>
{
    TabViewController*              tabViewController;
    GluVerticalTabBarController*    verticalTabViewController;
    SubNavController*               subNavController;
    NSString*                       gameId;
    BOOL                            statusBarVisibility;
    BOOL                            isVisible;
    void*                           callerData;         // CGluGamesNetwork that owns the overlay
    NSMutableArray*                 tabUrls;
    NSArray*                        tabIcons;
    NSArray*                        tabTitles;
    NSArray*                        subUrls;
    NSArray*                        subTitles;
    NSArray*                        subActiveIcons;
    NSArray*                        subInactiveIcons;
    BOOL                            iPadInterface;
    BOOL                            standaloneComponent;
    unsigned int                    gServeId;
    unsigned int                    lastSelectedCommunityPage;
    NSMutableData*                  gServeData;
    NSMutableData*                  updatesData;
    NSURLConnection*                gServeConnection;
    NSURLConnection*                updatesConnection;
    NSMutableArray*                 gServeUpdates;
    NSArray*                        tabInactiveIcons;
}

@property (nonatomic, assign) void* callerData;
@property (nonatomic, copy) NSString* gameId;
@property (nonatomic, readonly) BOOL isVisible;
@property (nonatomic, retain) NSMutableArray* tabUrls;
@property (nonatomic, retain) NSArray* tabIcons;
@property (nonatomic, retain) NSArray* tabInactiveIcons;
@property (nonatomic, retain) NSArray* tabTitles;
@property (nonatomic, retain) NSArray* subUrls;
@property (nonatomic, retain) NSArray* subTitles;
@property (nonatomic, retain) NSArray* subActiveIcons;
@property (nonatomic, retain) NSArray* subInactiveIcons;
@property (nonatomic) BOOL standaloneComponent;
@property (nonatomic) unsigned int gServeId;
@property (nonatomic, readonly) TabViewController* tabViewController;
@property (nonatomic, retain) NSMutableData* gServeData;
@property (nonatomic, retain) NSURLConnection* gServeConnection;
@property (nonatomic, retain) NSMutableData* updatesData;
@property (nonatomic, retain) NSURLConnection* updatesConnection;
@property (nonatomic, retain) NSMutableArray* gServeUpdates;

+ (GluGamesNetworkController*) sharedInstance;

- (void) show;
- (void) hide;
- (void) leave;
- (void) createCustomBadgeFromNumber: (int) number andAddToView: (UIView*) view;
- (BOOL) isGameCenterAPIAvailable;
- (void) authenticateLocalPlayer;
- (void) getAndSubmitHighScore;
- (long long) valueForField: (NSString*) field inString: (NSString*) string;
- (void) reloadTab: (int) tab withUrl: (NSString*) url;
- (void) checkForUpdatedItems;
- (void) checkForGServeUpdates: (NSString*) url;

@end
