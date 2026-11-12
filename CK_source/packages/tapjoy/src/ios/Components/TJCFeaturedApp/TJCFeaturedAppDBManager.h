#pragma once
#import "TJCFeaturedAppModel.h"

@interface TJCFeaturedAppDBManager : NSObject {
    NSString* currentDBPath_;
}
+ (TJCFeaturedAppDBManager*)sharedTJCFeaturedAppDBManager;
- (void)addApp:(TJCFeaturedAppModel*)app;
- (void)incrementDisplayedCountForStoreID:(NSString*)storeID;
- (int)getDisplayedCountForStoreID:(NSString*)storeID;
@end
