#pragma once
#import "TJCFeaturedAppModel.h"

@class TJCFeaturedAppRequestHandler;

@interface TJCFeaturedAppManager : NSObject {
    TJCFeaturedAppRequestHandler* featuredAppHandlerObj_;
    TJCFeaturedAppModel* featuredAppModelObj_;
    int featuredAppDisplayCount_;
}
@property(nonatomic, retain) TJCFeaturedAppModel* featuredAppModelObj;
@property(nonatomic, assign) int featuredAppDisplayCount;
+ (TJCFeaturedAppManager*)sharedTJCFeaturedAppManager;
+ (void)releaseSharedTJCFeaturedAppManager;
- (void)getFeaturedApp;
- (void)getFeaturedAppWithCurrencyID:(NSString*)currencyID;
- (void)releaseFeaturedAppHandler;
@end
