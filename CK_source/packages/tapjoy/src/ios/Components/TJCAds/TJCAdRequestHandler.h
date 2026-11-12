#pragma once
#import "TJCCoreFetcherHandler.h"

@interface TJCAdRequestHandler : TJCCoreFetcherHandler {
    BOOL accumulatingClickURL;
    NSString* clickURL_;
    BOOL accumulatingImageDataStr;
    NSString* imageDataStr_;
    BOOL isDataFetchSuccessful_;
}
- (void)requestAdWithSize:(NSString*)size;
- (void)requestAdWithSize:(NSString*)size currencyID:(NSString*)currencyID;
- (BOOL)isDataFetchSuccessful;
@end
