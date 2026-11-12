#import "PlayHavenSDK.h"

@implementation PHPublisherPromosRequest
+ (id)requestForApp:(NSString*)token secret:(NSString*)secret delegate:(id)delegate
{
    return [[[self alloc] initWithApp:token secret:secret delegate:delegate] autorelease];
}

- (id)initWithApp:(NSString*)token secret:(NSString*)secret delegate:(id)delegate
{
    self = [super initWithApp:token secret:secret];
    if (self) {
        self.delegate = delegate;
    }
    return self;
}

- (NSString*)urlPath
{
    return @"/v3/publisher/promos/";
}
@end
