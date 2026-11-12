#import "TJCVideoManager.h"
#import "TJCVideoObject.h"
#import "TJCVideoRequestHandler.h"
#import "TJCVideoView.h"

static TJCVideoManager* SharedVideoManager;
static const NSUInteger MaximumVideoBytes = 24 * 1024 * 1024;

@implementation TJCVideoManager
@synthesize videoView = videoView_, requestHandler = requestHandler_, currentOrientation = currentOrientation_;

+ (TJCVideoManager*)sharedTJCVideoManager
{
    if (SharedVideoManager == nil) {
        SharedVideoManager = [[self alloc] init];
    }
    return SharedVideoManager;
}

+ (void)releaseSharedTJCVideoManager
{
    [SharedVideoManager release];
    SharedVideoManager = nil;
}

- (id)init
{
    self = [super init];
    if (self) {
        videoCacheCount_ = 1;
        availableVideos_ = [[NSMutableDictionary alloc] init];
        cachedVideos_ = [[NSMutableDictionary alloc] init];
        unCachedVideoObjects_ = [[NSMutableArray alloc] init];
        pendingCompletions_ = [[NSMutableArray alloc] init];
        NSString* directory = [NSString stringWithFormat:@"CKTapjoy-%@", [TapjoyConnect generateUUID]];
        cacheDirectory_ = [[NSTemporaryDirectory() stringByAppendingPathComponent:directory] copy];
    }
    return self;
}

- (void)initVideoAdsWithDelegate:(id<TJCVideoAdDelegate>)delegate
{
    if (videoView_ == nil) {
        videoView_ = [[TJCVideoView alloc] initWithDelegate:delegate];
    } else {
        [videoView_ setAdDelegate:delegate];
    }
    if (requestHandler_ == nil) {
        requestHandler_ = [[TJCVideoRequestHandler alloc] initRequestWithDelegate:self andRequestTag:TJCVideoListRequest];
    }
    [requestHandler_ requestVideoData];
}

- (NSArray*)getCachedVideoIDs
{
    return [cachedVideos_ allKeys];
}

- (NSDictionary*)getCachedVideoDictonary
{
    return cachedVideos_;
}

- (void)setCachedVideoObjectDict:(NSDictionary*)video withKey:(NSString*)key
{
    if ([key length] != 0) {
        [cachedVideos_ setValue:video forKey:key];
    }
}

- (TJCVideoObject*)videoForOfferID:(NSString*)offerID
{
    if ([offerID length] != 0) {
        return [availableVideos_ objectForKey:offerID];
    }
    return nil;
}

- (void)fetchResponseSuccessWithData:(TBXMLElement*)root withRequestTag:(NSInteger)tag
{
    if (tag != TJCVideoListRequest) {
        return;
    }
    [videoFetcher_ cancel];
    [videoFetcher_ release];
    videoFetcher_ = nil;
    [availableVideos_ removeAllObjects];
    [unCachedVideoObjects_ removeAllObjects];
    [cachedVideos_ removeAllObjects];
    downloadIndex_ = 0;

    TBXMLElement* element = [TBXML childElementNamed:@"TapjoyVideo" parentElement:root];
    while (element != NULL) {
        TJCVideoObject* video = [[[TJCVideoObject alloc] initWithTBXML:element] autorelease];
        if ([[video offerID] length] != 0 && TJCValidHTTPSURL([NSURL URLWithString:[video videoURL]])) {
            [availableVideos_ setObject:video forKey:[video offerID]];
            [unCachedVideoObjects_ addObject:video];
        }
        element = [TBXML nextSiblingNamed:@"TapjoyVideo" searchFromElement:element];
    }
    [self beginVideoCaching];
}

- (void)fetchResponseError:(NSInteger)error errorDescription:(NSString*)description requestTag:(NSInteger)tag
{
    // Keep previously loaded offers when a refresh fails.
}

- (void)beginVideoCaching
{
    if (videoFetcher_ != nil || [cachedVideos_ count] >= (NSUInteger)videoCacheCount_ ||
        downloadIndex_ >= [unCachedVideoObjects_ count]) {
        return;
    }
    TJCVideoObject* video = [unCachedVideoObjects_ objectAtIndex:downloadIndex_++];
    videoFetcher_ = [[TJCCoreFetcher alloc] init];
    videoFetcher_.baseURL = [video videoURL];
    videoFetcher_.maximumResponseBytes = MaximumVideoBytes;
    videoFetcher_.requestTimeout = 45.0;
    SEL callback = @selector(videoDownloadFinished:);
    NSInvocation* invocation = [NSInvocation invocationWithMethodSignature:[self methodSignatureForSelector:callback]];
    [invocation setTarget:self];
    [invocation setSelector:callback];
    [videoFetcher_ fetchAsynchronouslyWithCompletionInvocation:invocation];
}

- (void)videoDownloadFinished:(TJCCoreFetcher*)fetcher
{
    if (fetcher != videoFetcher_) {
        return;
    }
    [videoFetcher_ release];
    videoFetcher_ = nil;
    if (![fetcher hasError] && [[fetcher data] length] != 0 &&
        [cachedVideos_ count] < (NSUInteger)videoCacheCount_) {
        TJCVideoObject* video = [unCachedVideoObjects_ objectAtIndex:downloadIndex_ - 1];
        NSFileManager* files = [NSFileManager defaultManager];
        [files createDirectoryAtPath:cacheDirectory_ withIntermediateDirectories:YES attributes:nil error:NULL];
        NSString* name = [[TapjoyConnect TJCSHA256WithString:[video offerID]] stringByAppendingPathExtension:@"mp4"];
        NSString* path = [cacheDirectory_ stringByAppendingPathComponent:name];
        if ([[fetcher data] writeToFile:path atomically:YES]) {
            video.dataLocation = path;
            [self setCachedVideoObjectDict:[video getDictionary] withKey:[video offerID]];
        }
    }
    [self beginVideoCaching];
}

- (void)setVideoCacheCount:(int)count
{
    videoCacheCount_ = MAX(0, MIN(count, 10));
    [self beginVideoCaching];
}

- (void)updateViewWithOrientation:(UIInterfaceOrientation)orientation
{
    currentOrientation_ = orientation;
    [videoView_ refreshViewWithInterfaceOrientation:orientation];
}

- (void)reportCompletedOffer:(NSString*)offerID
{
    if ([offerID length] == 0 || [pendingCompletions_ containsObject:offerID]) {
        return;
    }
    [pendingCompletions_ addObject:offerID];
    [self sendNextVideoReport];
}

- (void)sendNextVideoReport
{
    if (completionHandler_ != nil || [pendingCompletions_ count] == 0) {
        return;
    }
    NSString* offerID = [pendingCompletions_ objectAtIndex:0];
    completionHandler_ = [[TJCVideoRequestHandler alloc] initRequestWithDelegate:self andRequestTag:TJCVideoCompleteRequest];
    [completionHandler_ requestVideoCompleteWithOfferID:offerID];
}

- (void)completeVideoReport:(BOOL)accepted
{
    if (completionHandler_ == nil || [pendingCompletions_ count] == 0) {
        return;
    }
    [self retain];
    [completionHandler_ release];
    completionHandler_ = nil;
    [pendingCompletions_ removeObjectAtIndex:0];
    if (accepted) {
        [TapjoyConnect getTapPoints];
    }
    [self sendNextVideoReport];
    [self release];
}

- (void)dealloc
{
    [videoView_ setAdDelegate:nil];
    [requestHandler_ cancel];
    [completionHandler_ cancel];
    [videoFetcher_ cancel];
    [requestHandler_ release];
    [completionHandler_ release];
    [videoFetcher_ release];
    [videoView_ release];
    [availableVideos_ release];
    [unCachedVideoObjects_ release];
    [cachedVideos_ release];
    [pendingCompletions_ release];
    if (cacheDirectory_ != nil) {
        [[NSFileManager defaultManager] removeItemAtPath:cacheDirectory_ error:NULL];
        [cacheDirectory_ release];
    }
    [super dealloc];
}
@end
