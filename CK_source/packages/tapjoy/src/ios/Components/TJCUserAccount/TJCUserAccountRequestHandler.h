#pragma once
#import "TJCCoreFetcherHandler.h"

enum {
    TJCAccountGetPoints,
    TJCAccountSpendPoints,
    TJCAccountAwardPoints
};

@interface TJCUserAccountRequestHandler : TJCCoreFetcherHandler
- (void)requestTapPoints;
- (void)subtractTapPoints:(int)points;
- (void)addTapPoints:(int)points;
@end
