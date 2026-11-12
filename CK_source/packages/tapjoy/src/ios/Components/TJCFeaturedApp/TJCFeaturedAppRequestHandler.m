#import "TJCFeaturedAppRequestHandler.h"
#import "TapjoyConnect.h"

@protocol TJCFeaturedAppResponseDelegate
- (void)fetchResponseSuccessWithData:(TBXMLElement*)data withRequestTag:(NSInteger)tag;
- (void)fetchResponseError:(NSInteger)error errorDescription:(NSString*)description requestTag:(NSInteger)tag;
@end

@implementation TJCFeaturedAppRequestHandler
- (id)initRequestWithDelegate:(id)delegate andRequestTag:(NSInteger)tag
{
    return [super initRequestWithDelegate:delegate andRequestTag:tag];
}

- (void)requestFeaturedApp
{
    [self requestFeaturedAppWithCurrencyID:nil];
}

- (void)requestFeaturedAppWithCurrencyID:(NSString*)currencyID
{
    TapjoyConnect* service = [TapjoyConnect sharedTapjoyConnect];
    NSMutableDictionary* params = [NSMutableDictionary dictionaryWithDictionary:[service genericParameters]];
    [params setValue:currencyID forKey:@"currency_id"];
    NSString* path = @"get_offers/featured";
    [self makeGenericRequestWithURL:[service servicePath:path] alternateURL:[service alternateServicePath:path]
                             params:params selector:@selector(featuredAppDataReceived:)];
}

- (void)featuredAppDataReceived:(TJCCoreFetcher*)fetcher
{
    TBXMLElement* root = [self parseReturnObjectAsTBXMLElement:fetcher];
    TBXMLElement* offers = [TBXML childElementNamed:@"OfferArray" parentElement:root];
    TBXMLElement* app = [TBXML childElementNamed:@"TapjoyApp" parentElement:offers];
    id<TJCFeaturedAppResponseDelegate> delegate = deleg_;
    NSInteger tag = [fetcher requestTag];
    if (app != NULL) {
        [delegate fetchResponseSuccessWithData:app withRequestTag:tag];
    } else {
        [delegate fetchResponseError:1 errorDescription:[[fetcher error] localizedDescription] requestTag:tag];
    }
}
@end
