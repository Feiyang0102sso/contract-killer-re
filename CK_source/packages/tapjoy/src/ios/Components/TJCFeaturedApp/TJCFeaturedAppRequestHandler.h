#pragma once
#import "TJCCoreFetcherHandler.h"

@interface TJCFeaturedAppRequestHandler : TJCCoreFetcherHandler
- (void)requestFeaturedApp;
- (void)requestFeaturedAppWithCurrencyID:(NSString*)currencyID;
@end
