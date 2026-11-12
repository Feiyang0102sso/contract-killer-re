#pragma once
#import "TJCCoreFetcherHandler.h"

enum { TJCVideoListRequest, TJCVideoClickRequest, TJCVideoCompleteRequest };

@interface TJCVideoRequestHandler : TJCCoreFetcherHandler {
    BOOL clickSuccessful_;
}
@property(nonatomic, assign) BOOL clickSuccessful;
- (void)requestVideoData;
- (void)recordVideoClickWithURL:(NSString*)url;
- (void)requestVideoCompleteWithOfferID:(NSString*)offerID;
@end
