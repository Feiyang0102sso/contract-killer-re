#import "TJCCoreFetcherHandler.h"

NSString* const kTJCCoreFetcherReturnObjStr = @"TapjoyConnectReturnObject";
NSString* const kTJCCoreFetcherAdStr = @"TapjoyAd";
NSString* const kTJCCoreFetcherVideoStr = @"TapjoyVideos";

@implementation TJCCoreFetcherHandler
- (id)initRequestWithDelegate:(id)delegate andRequestTag:(NSInteger)tag
{
    self = [super init];
    if (self) {
        deleg_ = delegate;
        requestTag_ = tag;
        myFetcher_ = [[TJCCoreFetcher alloc] init];
        myFetcher_.requestTag = tag;
    }
    return self;
}

- (NSInvocation*)completionInvocationForSelector:(SEL)selector
{
    NSMethodSignature* signature = [self methodSignatureForSelector:selector];
    if (signature == nil || [signature numberOfArguments] != 3) {
        return nil;
    }
    NSInvocation* invocation = [NSInvocation invocationWithMethodSignature:signature];
    [invocation setSelector:selector];
    [invocation setTarget:self];
    return invocation;
}

- (void)makeGenericRequestWithURL:(NSString*)url alternateURL:(NSString*)alternate
                           params:(NSDictionary*)params selector:(SEL)selector
{
    myFetcher_.requestMethod = @"GET";
    myFetcher_.alternateURL = alternate;
    myFetcher_.bindings = params;
    [self makeRequestWithURL:url withInvoker:[self completionInvocationForSelector:selector]];
}

- (void)makeGenericPOSTRequestWithURL:(NSString*)url alternateURL:(NSString*)alternate
                               data:(NSData*)data params:(NSDictionary*)params selector:(SEL)selector
{
    myFetcher_.requestMethod = @"POST";
    myFetcher_.alternateURL = alternate;
    myFetcher_.bindings = nil;
    myFetcher_.postParameters = params;
    myFetcher_.POSTdata = data;
    [self makeRequestWithURL:url withInvoker:[self completionInvocationForSelector:selector]];
}

- (void)makeRequestWithURL:(NSString*)url withInvoker:(NSInvocation*)invocation
{
    myFetcher_.baseURL = url;
    [myFetcher_ fetchAsynchronouslyWithCompletionInvocation:invocation];
}

- (TBXMLElement*)parseReturnObjectAsTBXMLElement:(TJCCoreFetcher*)fetcher
{
    if ([fetcher hasError] || [[fetcher data] length] == 0) {
        return NULL;
    }
    TBXML* xml = [TBXML tbxmlWithXMLData:[fetcher data]];
    TBXMLElement* root = [xml rootXMLElement];
    if ([self validateResponseReturnedObject:root]) {
        return root;
    }
    return NULL;
}

- (BOOL)validateResponseReturnedObject:(TBXMLElement*)root
{
    if (root == NULL || [TBXML childElementNamed:@"ErrorMessage" parentElement:root] != NULL) {
        return NO;
    }
    NSString* rootName = [TBXML elementName:root];
    return [rootName isEqualToString:kTJCCoreFetcherReturnObjStr] ||
           [rootName isEqualToString:kTJCCoreFetcherAdStr] ||
           [rootName isEqualToString:kTJCCoreFetcherVideoStr];
}

- (void)cancel
{
    [myFetcher_ cancel];
}

- (void)dealloc
{
    [myFetcher_ cancel];
    [myFetcher_ release];
    [super dealloc];
}
@end
