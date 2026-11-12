#import "PlayHavenSDK.h"
#import "PHConstants.h"
#import "PHStringUtil.h"
#import "SBJsonParser.h"
#import "UIDevice+HardwareString.h"

static NSUInteger NextRequestID;
static const NSUInteger MaxResponseBytes = 1024 * 1024;

NSURL* PHServiceBaseURL(void)
{
    id setting = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CKPlayHavenBaseURL"];
    if (![setting isKindOfClass:[NSString class]]) {
        return nil;
    }
    NSURL* url = [NSURL URLWithString:setting];
    if (![[[url scheme] lowercaseString] isEqualToString:@"https"] || [[url host] length] == 0 ||
        [url user] != nil || [url password] != nil || [url query] != nil || [url fragment] != nil) {
        return nil;
    }
    return url;
}

NSString* PHInstallationID(void)
{
    id value = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CKPlayHavenInstallationID"];
    if ([value isKindOfClass:[NSString class]]) {
        return value;
    }
    return @"";
}

BOOL PHURLHasSameOrigin(NSURL* url, NSURL* base)
{
    if (url == nil || base == nil || [url user] != nil || [url password] != nil) {
        return NO;
    }
    NSNumber* port = [url port];
    NSNumber* basePort = [base port];
    if (port == nil) {
        port = [NSNumber numberWithInt:443];
    }
    if (basePort == nil) {
        basePort = [NSNumber numberWithInt:443];
    }
    return [[[url scheme] lowercaseString] isEqualToString:@"https"] &&
           [[[url host] lowercaseString] isEqualToString:[[base host] lowercaseString]] &&
           [port isEqual:basePort];
}

NSError* PHRequestError(NSInteger code, NSString* message)
{
    NSDictionary* userInfo = [NSDictionary dictionaryWithObject:message forKey:NSLocalizedDescriptionKey];
    return [NSError errorWithDomain:@"PlayHavenSDK" code:code userInfo:userInfo];
}

@interface PHAPIRequest ()
- (void)failUnavailable;
@end

@implementation PHAPIRequest
@synthesize token = _token, secret = _secret, delegate = _delegate, urlPath = _urlPath;
@synthesize additionalParameters = _additionalParameters, hashCode = _hashCode;

+ (NSMutableSet*)allRequests
{
    static NSMutableSet* requests;
    if (requests == nil) {
        requests = [[NSMutableSet alloc] init];
    }
    return requests;
}

+ (void)cancelAllRequestsWithDelegate:(id)delegate
{
    NSArray* snapshot = [[PHAPIRequest allRequests] allObjects];
    for (PHAPIRequest* request in snapshot) {
        if ([request delegate] == delegate) {
            [request cancel];
        }
    }
}

+ (NSString*)base64SignatureWithString:(NSString*)value
{
    return [PHStringUtil b64DigestForString:value];
}

+ (id)requestForApp:(NSString*)token secret:(NSString*)secret
{
    return [[[self alloc] initWithApp:token secret:secret] autorelease];
}

- (id)initWithApp:(NSString*)token secret:(NSString*)secret
{
    self = [self init];
    if (self) {
        _token = [token copy];
        _secret = [secret copy];
    }
    return self;
}

- (id)init
{
    self = [super init];
    if (self) {
        _hashCode = ++NextRequestID;
    }
    return self;
}

- (NSURL*)URL
{
    if (_URL == nil) {
        NSURL* base = PHServiceBaseURL();
        NSString* path = [self urlPath];
        if (base == nil || [path length] == 0 || [_token length] == 0 || [_secret length] == 0) {
            return nil;
        }

        NSURL* service = [NSURL URLWithString:path relativeToURL:base];
        if (!PHURLHasSameOrigin(service, base)) {
            return nil;
        }
        NSString* address = [NSString stringWithFormat:@"%@?%@", [service absoluteString], [self signedParameterString]];
        _URL = [[NSURL alloc] initWithString:address];
    }
    return _URL;
}

- (NSDictionary*)signedParameters
{
    if (_signedParameters == nil) {
        NSString* nonce = [PHStringUtil uuid];
        // A configured provider may supply its own installation identifier. Never read the hardware UDID.
        NSString* device = PHInstallationID();
        NSString* signatureInput = [NSString stringWithFormat:@"%@:%@:%@:%@", _token, device, nonce, _secret];
        NSString* signature = [PHAPIRequest base64SignatureWithString:signatureInput];

        NSMutableDictionary* parameters = [NSMutableDictionary dictionary];
        if ([self additionalParameters] != nil) {
            [parameters addEntriesFromDictionary:[self additionalParameters]];
        }
        [parameters setValue:_token forKey:@"token"];
        [parameters setValue:device forKey:@"device"];
        [parameters setObject:nonce forKey:@"nonce"];
        [parameters setObject:signature forKey:@"signature"];
        [parameters setObject:@"1.3.14" forKey:@"sdk-ios"];
        NSBundle* bundle = [NSBundle mainBundle];
        [parameters setValue:[bundle bundleIdentifier] forKey:@"app"];
        [parameters setValue:[bundle objectForInfoDictionaryKey:@"CFBundleVersion"] forKey:@"app_version"];
        UIDevice* deviceInfo = [UIDevice currentDevice];
        [parameters setValue:[deviceInfo hardware] forKey:@"hardware"];
        [parameters setObject:[NSString stringWithFormat:@"%@ %@", [deviceInfo systemName], [deviceInfo systemVersion]] forKey:@"os"];
        [parameters setObject:[NSNumber numberWithInt:[deviceInfo userInterfaceIdiom]] forKey:@"idiom"];
        [parameters setObject:[NSNumber numberWithInt:PHNetworkStatus()] forKey:@"connection"];
        _signedParameters = [parameters copy];
    }
    return _signedParameters;
}

- (NSString*)signedParameterString
{
    return [PHStringUtil stringWithQuery:[self signedParameters]];
}

- (void)dealloc
{
    [_connection cancel];
    [_connection release];
    [_connectionData release];
    [_response release];
    [_URL release];
    [_signedParameters release];
    [_token release];
    [_secret release];
    [_urlPath release];
    [_additionalParameters release];
    [super dealloc];
}

- (void)send
{
    NSMutableSet* activeRequests = [PHAPIRequest allRequests];
    if ([activeRequests containsObject:self]) {
        return;
    }
    [activeRequests addObject:self];

    NSURL* url = [self URL];
    if (url == nil) {
        [self performSelector:@selector(failUnavailable) withObject:nil afterDelay:0];
        return;
    }
    NSURLRequest* request = [NSURLRequest requestWithURL:url
                                           cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
                                       timeoutInterval:20.0];
    _connection = [[NSURLConnection alloc] initWithRequest:request delegate:self startImmediately:NO];
    if (_connection == nil) {
        [self performSelector:@selector(failUnavailable) withObject:nil afterDelay:0];
        return;
    }
    [_connection start];
}

- (void)failUnavailable
{
    [self didFailWithError:PHRequestError(NSURLErrorNotConnectedToInternet, @"PlayHaven service is unavailable.")];
}

- (void)cancel
{
    _delegate = nil;
    [self finish];
}

- (void)finish
{
    [self retain];
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(failUnavailable) object:nil];
    [_connection cancel];
    [_connection release];
    _connection = nil;
    [_connectionData release];
    _connectionData = nil;
    [_response release];
    _response = nil;
    [_URL release];
    _URL = nil;
    [_signedParameters release];
    _signedParameters = nil;
    [[PHAPIRequest allRequests] removeObject:self];
    [self release];
}

- (NSURLRequest*)connection:(NSURLConnection*)connection willSendRequest:(NSURLRequest*)request redirectResponse:(NSURLResponse*)response
{
    if (response != nil) {
        // Signed requests may only follow redirects on the configured origin.
        if (!PHURLHasSameOrigin([request URL], PHServiceBaseURL())) {
            [self didFailWithError:PHRequestError(NSURLErrorRedirectToNonExistentLocation, @"PlayHaven rejected an unexpected redirect.")];
            return nil;
        }
    }
    return request;
}

- (void)connection:(NSURLConnection*)connection didReceiveResponse:(NSURLResponse*)response
{
    if (connection != _connection) {
        return;
    }
    NSInteger status = [(NSHTTPURLResponse*)response statusCode];
    if (status < 200 || status >= 300) {
        [self didFailWithError:PHRequestError(status, @"PlayHaven returned an HTTP error.")];
        return;
    }
    [_response release];
    _response = [response retain];
    [_connectionData release];
    _connectionData = [[NSMutableData alloc] init];
}

- (void)connection:(NSURLConnection*)connection didReceiveData:(NSData*)data
{
    if (connection != _connection) {
        return;
    }
    if ([data length] > MaxResponseBytes - [_connectionData length]) {
        [self didFailWithError:PHRequestError(NSURLErrorDataLengthExceedsMaximum, @"PlayHaven response is too large.")];
        return;
    }
    [_connectionData appendData:data];
}

- (void)connectionDidFinishLoading:(NSURLConnection*)connection
{
    if (connection != _connection) {
        return;
    }
    [self retain];
    NSString* body = [[[NSString alloc] initWithData:_connectionData encoding:NSUTF8StringEncoding] autorelease];
    FB_SBJsonParser* parser = [[[FB_SBJsonParser alloc] init] autorelease];
    id response = [parser objectWithString:body];
    [self processRequestResponse:response];
    [self afterConnectionDidFinishLoading];
    [self release];
}

- (void)afterConnectionDidFinishLoading
{
}

- (void)connection:(NSURLConnection*)connection didFailWithError:(NSError*)error
{
    if (connection == _connection) {
        [self didFailWithError:error];
    }
}

- (void)processRequestResponse:(NSDictionary*)envelope
{
    if (![envelope isKindOfClass:[NSDictionary class]]) {
        [self didFailWithError:PHRequestError(NSURLErrorCannotParseResponse, @"PlayHaven response is invalid.")];
        return;
    }
    id error = [envelope objectForKey:@"error"];
    if (error != nil && error != [NSNull null]) {
        [self didFailWithError:PHCreateError(PHAPIError)];
        return;
    }
    id response = [envelope objectForKey:@"response"];
    if (response == nil) {
        [self didFailWithError:PHCreateError(PHMissingResponse)];
        return;
    }
    if (response == [NSNull null]) {
        response = nil;
    }
    [self didSucceedWithResponse:response];
}

- (void)didSucceedWithResponse:(id)response
{
    [self retain];
    id<PHAPIRequestDelegate> delegate = _delegate;
    [self finish];
    if ([delegate respondsToSelector:@selector(request:didSucceedWithResponse:)]) {
        [delegate request:self didSucceedWithResponse:response];
    }
    [self release];
}

- (void)didFailWithError:(NSError*)error
{
    [self retain];
    id<PHAPIRequestDelegate> delegate = _delegate;
    [self finish];
    if ([delegate respondsToSelector:@selector(request:didFailWithError:)]) {
        [delegate request:self didFailWithError:error];
    }
    [self release];
}
@end
