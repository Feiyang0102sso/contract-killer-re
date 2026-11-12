#include "CBHUtil_Cocoa.h"
#import <UIKit/UIKit.h>
#import <GameKit/GameKit.h>

void CBHUtil_Cocoa::LaunchGameCenter()
{
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"gamecenter:"]];
}

bool CBHUtil_Cocoa::IsGameCenterAuthenticated()
{
    return [[GKLocalPlayer localPlayer] isAuthenticated];
}

id CBHUtil_Cocoa::GetRootViewController()
{
    id delegate = [[UIApplication sharedApplication] delegate];
    if ([delegate respondsToSelector:@selector(viewController)]) {
        return [delegate performSelector:@selector(viewController)];
    }
    return nil;
}
