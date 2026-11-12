#pragma once
#import "TJCUserAccountModel.h"

@class TJCUserAccountRequestHandler;

@interface TJCUserAccountManager : NSObject {
    TJCUserAccountModel* userAccountModelObj_;
    TJCUserAccountRequestHandler* userAccountGetTapPointsObj_;
    TJCUserAccountRequestHandler* userAccountSpendTapPointsObj_;
    BOOL waitingForResponse_;
}
@property(nonatomic, readonly) TJCUserAccountModel* userAccountModelObj;
+ (TJCUserAccountManager*)sharedTJCUserAccountManager;
+ (void)releaseSharedTJCUserAccountManager;
- (void)getTapPoints;
- (void)spendTapPoints:(int)points;
- (void)awardTapPoints:(int)points;
- (void)releaseUserAccount;
@end
