#import "PHURLLoader.h"
#import "PlayHavenSDK.h"
#import <UIKit/UIKit.h>

static const NSUInteger RedirectLimit = 8;

static BOOL IsHTTPSURL(NSURL* url)
{
    return [[[url scheme] lowercaseString] isEqualToString:@"https"] &&
           [[url host] length] != 0 && [url user] == nil && [url password] == nil;
}

static BOOL IsStoreURL(NSURL* url)
{
    NSString* scheme = [[url scheme] lowercaseString];
    return [scheme isEqualToString:@"itms-apps"] || [scheme isEqualToString:@"itms"];
}

@implementation PHURLLoader
@synthesize delegate = _delegate, targetURL = _targetURL, opensFinalURLOnDevice = _opensFinalURLOnDevice;
@synthesize context = _context;

+ (PHURLLoader*)openDeviceURL:(NSString*)address
{
    PHURLLoader* loader = [[[PHURLLoader alloc] init] autorelease];
    if ([address length] != 0) {
        [loader setTargetURL:[NSURL URLWithString:address]];
    }
    [loader open];
    return loader;
}

+ (NSMutableSet*)allLoaders
{
    static NSMutableSet* loaders;
    if (loaders == nil) {
        loaders = [[NSMutableSet alloc] init];
    }
    return loaders;
}

+ (void)invalidateAllLoadersWithDelegate:(id)delegate
{
    NSArray* snapshot = [[PHURLLoader allLoaders] allObjects];
    for (PHURLLoader* loader in snapshot) {
        if ([loader delegate] == delegate) {
            [loader invalidate];
        }
    }
}

- (id)init
{
    self = [super init];
    if (self) {
        _opensFinalURLOnDevice = YES;
    }
    return self;
}

- (void)dealloc
{
    [_connection cancel];
    [_connection release];
    [_targetURL release];
    [_context release];
    [super dealloc];
}

- (void)open
{
    NSMutableSet* loaders = [PHURLLoader allLoaders];
    if ([loaders containsObject:self]) {
        return;
    }
    [loaders addObject:self];
    _totalRedirects = 0;
    if (_opensFinalURLOnDevice && IsStoreURL(_targetURL)) {
        [self performSelector:@selector(finish) withObject:nil afterDelay:0];
        return;
    }
    if (!IsHTTPSURL(_targetURL)) {
        [self performSelector:@selector(fail) withObject:nil afterDelay:0];
        return;
    }
    NSURLRequest* request = [NSURLRequest requestWithURL:_targetURL
        cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:15.0];
    _connection = [[NSURLConnection alloc] initWithRequest:request delegate:self startImmediately:NO];
    if (_connection == nil) {
        [self performSelector:@selector(fail) withObject:nil afterDelay:0];
    } else {
        [_connection start];
    }
}

- (void)invalidate
{
    [self retain];
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    [_connection cancel];
    [_connection release];
    _connection = nil;
    _delegate = nil;
    [[PHURLLoader allLoaders] removeObject:self];
    [self release];
}

- (void)finish
{
    [self retain];
    id<PHURLLoaderDelegate> delegate = _delegate;
    NSURL* url = [[_targetURL retain] autorelease];
    BOOL openOnDevice = _opensFinalURLOnDevice;
    [self invalidate];
    if ([delegate respondsToSelector:@selector(loaderFinished:)]) {
        [delegate loaderFinished:self];
    }
    if (openOnDevice) {
        [[UIApplication sharedApplication] openURL:url];
    }
    [self release];
}

- (void)fail
{
    [self retain];
    id<PHURLLoaderDelegate> delegate = _delegate;
    [self invalidate];
    if ([delegate respondsToSelector:@selector(loaderFailed:)]) {
        [delegate loaderFailed:self];
    }
    [self release];
}

- (NSURLRequest*)connection:(NSURLConnection*)connection willSendRequest:(NSURLRequest*)request redirectResponse:(NSURLResponse*)response
{
    if (response == nil) {
        return request;
    }
    NSURL* destination = [request URL];
    ++_totalRedirects;
    if (_totalRedirects > RedirectLimit) {
        [self fail];
        return nil;
    }
    if (_opensFinalURLOnDevice && IsStoreURL(destination)) {
        self.targetURL = destination;
        [self finish];
        return nil;
    }
    if (!IsHTTPSURL(destination) || (!_opensFinalURLOnDevice && !PHURLHasSameOrigin(destination, _targetURL))) {
        [self fail];
        return nil;
    }
    self.targetURL = destination;
    return request;
}

- (void)connection:(NSURLConnection*)connection didFailWithError:(NSError*)error
{
    if (connection == _connection) {
        [self fail];
    }
}

- (void)connection:(NSURLConnection*)connection didReceiveResponse:(NSURLResponse*)response
{
    if (connection != _connection) {
        return;
    }
    NSInteger status = [(NSHTTPURLResponse*)response statusCode];
    if (status >= 200 && status < 300) {
        self.targetURL = [response URL];
        [self finish];
    } else {
        [self fail];
    }
}
@end
