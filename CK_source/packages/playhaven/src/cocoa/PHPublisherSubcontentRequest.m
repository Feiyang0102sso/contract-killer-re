#import "PlayHavenSDK.h"

@implementation PHPublisherSubContentRequest
@synthesize source = _source, callback = _callback;

- (NSURL*)URL
{
    NSURL* url = [NSURL URLWithString:[self urlPath] relativeToURL:PHServiceBaseURL()];
    if (PHURLHasSameOrigin(url, PHServiceBaseURL())) {
        return url;
    }
    return nil;
}

- (void)dealloc
{
    [_callback release];
    [super dealloc];
}
@end
