#import "TJCCoreFetcher.h"
#import "TJCLog.h"

static const NSUInteger MaxResponseBytes = 4 * 1024 * 1024;

NSError* TJCFetchError(NSInteger code, NSString* message)
{
    return [NSError errorWithDomain:@"TapjoyConnect" code:code
                           userInfo:[NSDictionary dictionaryWithObject:message forKey:NSLocalizedDescriptionKey]];
}

BOOL TJCValidHTTPSURL(NSURL* url)
{
    return [[[url scheme] lowercaseString] isEqualToString:@"https"] &&
           [[url host] length] != 0 && [url user] == nil && [url password] == nil;
}

BOOL TJCURLHasSameOrigin(NSURL* url, NSURL* base)
{
    if (!TJCValidHTTPSURL(url) || !TJCValidHTTPSURL(base)) {
        return NO;
    }
    int port = [[url port] intValue];
    int basePort = [[base port] intValue];
    if (port == 0) {
        port = 443;
    }
    if (basePort == 0) {
        basePort = 443;
    }
    return port == basePort && [[[url host] lowercaseString] isEqualToString:[[base host] lowercaseString]];
}

static NSString* EncodeQueryValue(NSString* string)
{
    return [(NSString*)CFURLCreateStringByAddingPercentEscapes(
        kCFAllocatorDefault, (CFStringRef)string, NULL, CFSTR("!*'();:@&=+$,/?%#[]"), kCFStringEncodingUTF8) autorelease];
}

@implementation TJCCoreFetcher
@synthesize requestTag = requestTag_, hasFetched = hasFetched_, baseURL = baseURL_, alternateURL = alternateURL_;
@synthesize bindings = bindings_, error = error_, data = data_, POSTdata = POSTdata_, response = response_;
@synthesize requestTimeout = requestTimeout_, connection = connection_, invocation = invocation_;
@synthesize requestMethod = requestMethod_, postParameters = postParameters_;
@synthesize maximumResponseBytes = maximumResponseBytes_;

- (id)init
{
    self = [super init];
    if (self) {
        requestTimeout_ = 20.0;
        maximumResponseBytes_ = MaxResponseBytes;
        requestMethod_ = [@"GET" copy];
    }
    return self;
}

- (void)fetchAsynchronouslyWithCompletionInvocation:(NSInvocation*)invocation
{
    [self cancel];
    self.invocation = invocation;
    self.error = nil;
    self.response = nil;
    self.data = [NSMutableData data];
    self.hasFetched = NO;
    retryCount_ = 0;
    [self startRequest];
}

- (void)startRequest
{
    NSURL* url = [self requestURL];
    if (!TJCValidHTTPSURL(url)) {
        [self performSelector:@selector(failUnavailable) withObject:nil afterDelay:0];
        return;
    }
    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url
        cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:requestTimeout_];
    [request setHTTPMethod:requestMethod_];
    if ([requestMethod_ isEqualToString:@"POST"]) {
        NSData* body = POSTdata_;
        if (body == nil) {
            body = [[self createQueryStringFromDict:postParameters_] dataUsingEncoding:NSUTF8StringEncoding];
            [request setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
        }
        [request setHTTPBody:body];
    }
    connection_ = [[NSURLConnection alloc] initWithRequest:request delegate:self startImmediately:NO];
    if (connection_ == nil) {
        [self performSelector:@selector(failUnavailable) withObject:nil afterDelay:0];
        return;
    }
    [connection_ start];
}

- (void)failUnavailable
{
    self.error = TJCFetchError(NSURLErrorNotConnectedToInternet, @"Tapjoy service is unavailable.");
    [self complete];
}

- (NSCachedURLResponse*)connection:(NSURLConnection*)connection willCacheResponse:(NSCachedURLResponse*)response
{
    return nil;
}

- (NSURLRequest*)connection:(NSURLConnection*)connection willSendRequest:(NSURLRequest*)request redirectResponse:(NSURLResponse*)response
{
    if (response != nil && !TJCURLHasSameOrigin([request URL], [NSURL URLWithString:baseURL_])) {
        self.error = TJCFetchError(NSURLErrorRedirectToNonExistentLocation, @"Tapjoy rejected an unexpected redirect.");
        [self complete];
        return nil;
    }
    return request;
}

- (NSString*)createQueryStringFromDict:(NSDictionary*)parameters
{
    NSMutableArray* pairs = [NSMutableArray array];
    NSArray* keys = [[parameters allKeys] sortedArrayUsingSelector:@selector(compare:)];
    for (NSString* key in keys) {
        NSString* value = [[parameters objectForKey:key] description];
        [pairs addObject:[NSString stringWithFormat:@"%@=%@", EncodeQueryValue(key), EncodeQueryValue(value)]];
    }
    return [pairs componentsJoinedByString:@"&"];
}

- (NSURL*)requestURL
{
    if ([baseURL_ length] == 0) {
        return nil;
    }
    NSString* query = [self urlEncodedBindings];
    if ([query length] == 0) {
        return [NSURL URLWithString:baseURL_];
    }
    NSString* separator = @"?";
    if ([baseURL_ rangeOfString:@"?"].location != NSNotFound) {
        separator = @"&";
    }
    return [NSURL URLWithString:[NSString stringWithFormat:@"%@%@%@", baseURL_, separator, query]];
}

- (NSString*)urlEncodedBindings
{
    return [self createQueryStringFromDict:bindings_];
}

- (void)retry
{
    if (connection_ != nil || invocation_ == nil || retryCount_ != 0 ||
        [alternateURL_ length] == 0 || ![requestMethod_ isEqualToString:@"GET"]) {
        return;
    }
    ++retryCount_;
    self.baseURL = alternateURL_;
    self.response = nil;
    [data_ setLength:0];
    [self startRequest];
}

- (BOOL)hasError
{
    return error_ != nil;
}

- (void)cancel
{
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(failUnavailable) object:nil];
    [connection_ cancel];
    self.connection = nil;
    self.invocation = nil;
}

- (void)dealloc
{
    [connection_ cancel];
    [connection_ release];
    [invocation_ release];
    [requestMethod_ release];
    [postParameters_ release];
    [response_ release];
    [baseURL_ release];
    [alternateURL_ release];
    [bindings_ release];
    [error_ release];
    [data_ release];
    [POSTdata_ release];
    [super dealloc];
}

- (void)connection:(NSURLConnection*)connection didReceiveResponse:(NSURLResponse*)response
{
    if (connection == connection_) {
        self.response = response;
        [data_ setLength:0];
    }
}

- (void)connection:(NSURLConnection*)connection didReceiveData:(NSData*)data
{
    if (connection != connection_) {
        return;
    }
    if ([data_ length] > maximumResponseBytes_ || [data length] > maximumResponseBytes_ - [data_ length]) {
        self.error = TJCFetchError(NSURLErrorDataLengthExceedsMaximum, @"Tapjoy response is too large.");
        [self complete];
        return;
    }
    [data_ appendData:data];
}

- (void)connection:(NSURLConnection*)connection didFailWithError:(NSError*)error
{
    if (connection != connection_) {
        return;
    }
    self.connection = nil;
    if (retryCount_ == 0 && [alternateURL_ length] != 0 && [requestMethod_ isEqualToString:@"GET"]) {
        [self retry];
        return;
    }
    self.error = error;
    [TJCLog logWithLevel:3 format:@"Tapjoy request failed (%ld)", (long)[error code]];
    [self complete];
}

- (void)connectionDidFinishLoading:(NSURLConnection*)connection
{
    if (connection != connection_) {
        return;
    }
    NSInteger status = [(NSHTTPURLResponse*)response_ statusCode];
    if (status < 200 || status >= 300) {
        self.error = TJCFetchError(status, @"Tapjoy returned an HTTP error.");
    }
    [self complete];
}

- (void)complete
{
    [self retain];
    NSInvocation* invocation = [invocation_ retain];
    [self cancel];
    hasFetched_ = YES;
    if (invocation != nil) {
        id target = [[invocation target] retain];
        TJCCoreFetcher* result = self;
        [invocation setArgument:&result atIndex:2];
        [invocation invoke];
        [target release];
        [invocation release];
    }
    [self release];
}
@end
