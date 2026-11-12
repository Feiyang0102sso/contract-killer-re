#import "TJCUserAccountRequestHandler.h"
#import "TapjoyConnect.h"

@protocol TJCAccountResponseDelegate
- (void)fetchResponseSuccessWithData:(TBXMLElement*)data withRequestTag:(NSInteger)tag;
- (void)fetchResponseError:(NSInteger)error errorDescription:(NSString*)description requestTag:(NSInteger)tag;
@end

@implementation TJCUserAccountRequestHandler
- (id)initRequestWithDelegate:(id)delegate andRequestTag:(NSInteger)tag
{
    return [super initRequestWithDelegate:delegate andRequestTag:tag];
}

- (void)requestTapPoints
{
    TapjoyConnect* service = [TapjoyConnect sharedTapjoyConnect];
    NSString* path = @"get_vg_store_items/user_account";
    [self makeGenericRequestWithURL:[service servicePath:path] alternateURL:[service alternateServicePath:path]
                             params:[service genericParameters] selector:@selector(userAccountDataReceived:)];
}

- (void)subtractTapPoints:(int)points
{
    TapjoyConnect* service = [TapjoyConnect sharedTapjoyConnect];
    NSMutableDictionary* params = [NSMutableDictionary dictionaryWithDictionary:[service genericParameters]];
    [params setObject:[NSNumber numberWithInt:points] forKey:@"tap_points"];
    // A failed spend may have reached the server. Never retry on the alternate host.
    [self makeGenericRequestWithURL:[service servicePath:@"points/spend"] alternateURL:nil
                             params:params selector:@selector(userAccountDataReceived:)];
}

- (void)addTapPoints:(int)points
{
    TapjoyConnect* service = [TapjoyConnect sharedTapjoyConnect];
    NSMutableDictionary* params = [NSMutableDictionary dictionaryWithDictionary:[service genericParameters]];
    NSString* timestamp = [params objectForKey:@"timestamp"];
    NSString* identifier = [TapjoyConnect generateUUID];
    [params setObject:[TapjoyConnect TJCSHA256CommonParamsWithTimeStamp:timestamp tapPointsAmount:points guid:identifier]
               forKey:@"verifier"];
    [params setObject:identifier forKey:@"guid"];
    [params setObject:[NSNumber numberWithInt:points] forKey:@"tap_points"];
    [self makeGenericRequestWithURL:[service servicePath:@"points/award"] alternateURL:nil
                             params:params selector:@selector(userAccountDataReceived:)];
}

- (void)userAccountDataReceived:(TJCCoreFetcher*)fetcher
{
    TBXMLElement* root = [self parseReturnObjectAsTBXMLElement:fetcher];
    TBXMLElement* account = [TBXML childElementNamed:@"UserAccountObject" parentElement:root];
    id<TJCAccountResponseDelegate> delegate = deleg_;
    NSInteger tag = [fetcher requestTag];
    if (account != NULL) {
        [delegate fetchResponseSuccessWithData:account withRequestTag:tag];
    } else {
        [delegate fetchResponseError:1 errorDescription:[[fetcher error] localizedDescription] requestTag:tag];
    }
}
@end
