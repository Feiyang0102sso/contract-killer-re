#import "TJCAdRequestHandler.h"
#import "TapjoyConnect.h"

NSString* const kTJCAdErrorNotif = @"TJCAdErrorNotif";
NSString* const kTJCAdMsgErrorKey = @"TJCAdMsgErrorKey";
NSString* const kTJCAdClickURLStr = @"ClickURL";
NSString* const kTJCAdImageDataStr = @"Image";

@interface NSObject (TJCAdResponse)
- (void)fetchResponseSuccessWithData:(id)data withRequestTag:(NSInteger)tag;
- (void)fetchResponseError:(NSInteger)error errorDescription:(NSString*)description requestTag:(NSInteger)tag;
@end

@implementation TJCAdRequestHandler
- (id)initRequestWithDelegate:(id)delegate andRequestTag:(NSInteger)tag
{
    return [super initRequestWithDelegate:delegate andRequestTag:tag];
}

- (void)requestAdWithSize:(NSString*)size
{
    [self requestAdWithSize:size currencyID:nil];
}

- (void)requestAdWithSize:(NSString*)size currencyID:(NSString*)currencyID
{
    isDataFetchSuccessful_ = NO;
    TapjoyConnect* service = [TapjoyConnect sharedTapjoyConnect];
    NSMutableDictionary* parameters = [NSMutableDictionary dictionaryWithDictionary:[service genericParameters]];
    [parameters setValue:size forKey:@"size"];
    [parameters setValue:currencyID forKey:@"currency_id"];
    [self makeGenericRequestWithURL:[service servicePath:@"display_ad"]
                      alternateURL:[service alternateServicePath:@"display_ad"]
                             params:parameters selector:@selector(adDataReceived:)];
}

- (void)adDataReceived:(TJCCoreFetcher*)fetcher
{
    TBXMLElement* root = [self parseReturnObjectAsTBXMLElement:fetcher];
    TBXMLElement* link = [TBXML childElementNamed:kTJCAdClickURLStr parentElement:root];
    TBXMLElement* image = [TBXML childElementNamed:kTJCAdImageDataStr parentElement:root];
    NSString* encoded = [TBXML textForElement:image];
    if (link == NULL || [encoded length] == 0) {
        [self handleAdError:[[fetcher error] localizedDescription]];
        return;
    }

    [clickURL_ release];
    clickURL_ = [[TBXML textForElement:link] copy];
    [imageDataStr_ release];
    imageDataStr_ = [encoded copy];
    isDataFetchSuccessful_ = YES;
    NSArray* data = [NSArray arrayWithObjects:clickURL_, imageDataStr_, nil];
    [deleg_ fetchResponseSuccessWithData:data withRequestTag:[fetcher requestTag]];
}

- (BOOL)isDataFetchSuccessful
{
    return isDataFetchSuccessful_;
}

- (void)handleAdError:(NSString*)description
{
    isDataFetchSuccessful_ = NO;
    [deleg_ fetchResponseError:1 errorDescription:description requestTag:requestTag_];
}

- (void)parser:(NSXMLParser*)parser parseErrorOccurred:(NSError*)error
{
    [self handleAdError:[error localizedDescription]];
}

- (void)dealloc
{
    [clickURL_ release];
    [imageDataStr_ release];
    [super dealloc];
}
@end
