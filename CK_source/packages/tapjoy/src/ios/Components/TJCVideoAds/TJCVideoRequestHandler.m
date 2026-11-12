#import "TJCVideoRequestHandler.h"
#import "TapjoyConnect.h"
#import "TJCVideoManager.h"

@protocol TJCVideoResponseDelegate
- (void)fetchResponseSuccessWithData:(TBXMLElement*)data withRequestTag:(NSInteger)tag;
- (void)fetchResponseError:(NSInteger)error errorDescription:(NSString*)description requestTag:(NSInteger)tag;
@end

@implementation TJCVideoRequestHandler
@synthesize clickSuccessful = clickSuccessful_;

- (id)initRequestWithDelegate:(id)delegate andRequestTag:(NSInteger)tag
{
    return [super initRequestWithDelegate:delegate andRequestTag:tag];
}

- (void)requestVideoData
{
    TapjoyConnect* service = [TapjoyConnect sharedTapjoyConnect];
    [self makeGenericRequestWithURL:[service servicePath:@"videos"] alternateURL:[service alternateServicePath:@"videos"]
                             params:[service genericParameters] selector:@selector(videoDataReceived:)];
}

- (void)recordVideoClickWithURL:(NSString*)url
{
    clickSuccessful_ = NO;
    [self makeGenericRequestWithURL:url alternateURL:nil params:nil selector:@selector(videoClickReceived:)];
}

- (void)requestVideoCompleteWithOfferID:(NSString*)offerID
{
    if ([offerID length] == 0) {
        return;
    }
    TapjoyConnect* service = [TapjoyConnect sharedTapjoyConnect];
    NSMutableDictionary* params = [NSMutableDictionary dictionaryWithDictionary:[service genericParameters]];
    [params setObject:offerID forKey:@"app_id"];
    [self makeGenericRequestWithURL:[service servicePath:@"connect"] alternateURL:nil
                             params:params selector:@selector(videoCompleteReceived:)];
}

- (void)videoDataReceived:(TJCCoreFetcher*)fetcher
{
    TBXMLElement* root = [self parseReturnObjectAsTBXMLElement:fetcher];
    id<TJCVideoResponseDelegate> delegate = deleg_;
    if (root != NULL) {
        [delegate fetchResponseSuccessWithData:root withRequestTag:requestTag_];
    } else {
        [delegate fetchResponseError:1 errorDescription:[[fetcher error] localizedDescription] requestTag:requestTag_];
    }
}

- (void)videoClickReceived:(TJCCoreFetcher*)fetcher
{
    NSString* text = [[[NSString alloc] initWithData:[fetcher data] encoding:NSUTF8StringEncoding] autorelease];
    clickSuccessful_ = ![fetcher hasError] && [[text stringByTrimmingCharactersInSet:
        [NSCharacterSet whitespaceAndNewlineCharacterSet]] isEqualToString:@"OK"];
}

- (void)videoCompleteReceived:(TJCCoreFetcher*)fetcher
{
    [deleg_ completeVideoReport:![fetcher hasError]];
}
@end
