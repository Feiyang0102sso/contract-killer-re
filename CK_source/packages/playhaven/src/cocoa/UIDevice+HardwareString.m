#import "UIDevice+HardwareString.h"
#include <sys/sysctl.h>

@implementation UIDevice (HardwareString)
- (NSString*)hardware
{
    char model[64];
    size_t length = sizeof(model);
    if (sysctlbyname("hw.machine", model, &length, NULL, 0) == 0 && length > 0 && length <= sizeof(model)) {
        model[length - 1] = '\0';
        return [NSString stringWithUTF8String:model];
    }
    return nil;
}
@end
