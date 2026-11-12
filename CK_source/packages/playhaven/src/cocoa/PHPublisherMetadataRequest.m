#import "PlayHavenSDK.h"

@implementation PHPublisherMetadataRequest
@synthesize placement = _placement;

+ (id)requestForApp:(NSString*)token secret:(NSString*)secret placement:(NSString*)placement delegate:(id)delegate
{
    return [[[self alloc] initWithApp:token secret:secret placement:placement delegate:delegate] autorelease];
}

- (id)initWithApp:(NSString*)token secret:(NSString*)secret placement:(NSString*)placement delegate:(id)delegate
{
    self = [super initWithApp:token secret:secret];
    if (self) {
        self.placement = placement;
        self.delegate = delegate;
    }
    return self;
}

- (void)dealloc
{
    [_placement release];
    [super dealloc];
}

- (NSString*)urlPath
{
    return @"/v3/publisher/content/";
}

- (NSDictionary*)additionalParameters
{
    return [NSDictionary dictionaryWithObjectsAndKeys:@"1", @"metadata", [self placement], @"placement_id", nil];
}
@end
