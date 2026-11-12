// Reward payload and Objective-C ownership from CK PHReward.o.
#import "PHReward.h"

@implementation PHReward
@synthesize name = _reward;
@synthesize quantity = _quanity;
@synthesize receipt = _receipt;

- (void)dealloc
{
    [_reward release];
    _reward = nil;
    [_receipt release];
    _receipt = nil;
    [super dealloc];
}
@end
