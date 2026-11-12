#import "TJCUtil.h"
#import <UIKit/UIKit.h>

static int Base64Digit(unsigned char value)
{
    if (value >= 'A' && value <= 'Z') {
        return value - 'A';
    }
    if (value >= 'a' && value <= 'z') {
        return value - 'a' + 26;
    }
    if (value >= '0' && value <= '9') {
        return value - '0' + 52;
    }
    if (value == '+') {
        return 62;
    }
    if (value == '/') {
        return 63;
    }
    return -1;
}

@implementation TJCUtil
+ (BOOL)isPad
{
    return UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad;
}

+ (BOOL)caseInsenstiveCompare:(NSString*)first AndString2:(NSString*)second
{
    return first != nil && second != nil && [first caseInsensitiveCompare:second] == NSOrderedSame;
}

+ (NSData*)dataWithBase64EncodedString:(NSString*)string
{
    return [[self initWithBase64EncodedString:string] autorelease];
}

+ (NSData*)initWithBase64EncodedString:(NSString*)string
{
    const unsigned char* input = (const unsigned char*)[string UTF8String];
    if (input == NULL) {
        return nil;
    }

    NSMutableData* decoded = [[NSMutableData alloc] initWithCapacity:[string length] * 3 / 4];
    unsigned int group = 0;
    int bits = 0;
    int digits = 0;
    int padding = 0;
    while (*input != '\0') {
        unsigned char character = *input++;
        if (character == ' ' || character == '\t' || character == '\r' || character == '\n') {
            continue;
        }
        if (character == '=') {
            ++padding;
            continue;
        }
        int digit = Base64Digit(character);
        if (digit < 0 || padding != 0) {
            [decoded release];
            return nil;
        }
        ++digits;
        group = (group << 6) | (unsigned int)digit;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            unsigned char value = (unsigned char)(group >> bits);
            [decoded appendBytes:&value length:1];
        }
    }
    if (digits % 4 == 1 || padding > 2 || (padding != 0 && (digits + padding) % 4 != 0)) {
        [decoded release];
        return nil;
    }
    return decoded;
}
@end
