#import "TJCLog.h"

static int logThreshold = 4;

@implementation TJCLog
+ (void)setLogThreshold:(int)threshold
{
    logThreshold = threshold;
}

+ (void)logWithLevel:(int)level format:(NSString*)format, ...
{
    if (level > logThreshold) {
        return;
    }
    va_list arguments;
    va_start(arguments, format);
    NSString* message = [[[NSString alloc] initWithFormat:format arguments:arguments] autorelease];
    va_end(arguments);
    NSLog(@"[Log Level: %d]; %@", level, message);
}
@end
