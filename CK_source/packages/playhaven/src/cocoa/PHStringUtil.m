#import "PHStringUtil.h"
#import <CommonCrypto/CommonDigest.h>

@implementation PHStringUtil
+ (NSString*)stringWithQueryQuirky:(NSDictionary*)parameters
{
    return [self stringWithQuery:parameters];
}

+ (NSString*)stringWithQuery:(NSDictionary*)parameters
{
    NSMutableArray* fields = [NSMutableArray array];
    NSArray* keys = [[parameters allKeys] sortedArrayUsingSelector:@selector(compare:)];
    for (NSString* key in keys) {
        NSString* fieldName = [self stringByUrlEncodingString:key];
        NSString* value = [[parameters objectForKey:key] description];
        [fields addObject:[NSString stringWithFormat:@"%@=%@", fieldName, [self stringByUrlEncodingString:value]]];
    }
    return [fields componentsJoinedByString:@"&"];
}

+ (NSString*)stringByHtmlEscapingString:(NSString*)value
{
    NSMutableString* escaped = [NSMutableString string];
    for (NSUInteger index = 0; index < [value length]; ++index) {
        unichar character = [value characterAtIndex:index];
        if (character < 32 || character > 126 || character == '&' || character == '<' ||
            character == '>' || character == '"' || character == 39) {
            [escaped appendFormat:@"&#%u;", (unsigned int)character];
        } else {
            [escaped appendFormat:@"%C", character];
        }
    }
    return escaped;
}

+ (NSString*)stringByUrlEncodingString:(NSString*)value
{
    if (value == nil) {
        return nil;
    }
    CFStringRef encoded = CFURLCreateStringByAddingPercentEscapes(
        kCFAllocatorDefault, (CFStringRef)value, NULL, CFSTR("!*'();:@&=+$,/?%#[]"), kCFStringEncodingUTF8);
    return [(NSString*)encoded autorelease];
}

+ (NSString*)stringByUrlDecodingString:(NSString*)value
{
    return [value stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
}

+ (NSString*)uuid
{
    CFUUIDRef identifier = CFUUIDCreate(kCFAllocatorDefault);
    NSString* value = (NSString*)CFUUIDCreateString(kCFAllocatorDefault, identifier);
    CFRelease(identifier);
    return [value autorelease];
}

+ (NSData*)dataDigestForString:(NSString*)value
{
    NSData* input = [value dataUsingEncoding:NSUTF8StringEncoding];
    unsigned char digest[CC_SHA1_DIGEST_LENGTH];
    CC_SHA1([input bytes], (CC_LONG)[input length], digest);
    return [NSData dataWithBytes:digest length:sizeof(digest)];
}

+ (NSString*)base64EncodedStringForData:(NSData*)data
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    NSMutableData* output = [NSMutableData dataWithLength:(([data length] + 2) / 3) * 4];
    char* encoded = [output mutableBytes];
    const unsigned char* bytes = [data bytes];
    NSUInteger length = [data length];
    for (NSUInteger index = 0; index < length; index += 3) {
        unsigned int group = bytes[index] << 16;
        if (index + 1 < length) {
            group |= bytes[index + 1] << 8;
        }
        if (index + 2 < length) {
            group |= bytes[index + 2];
        }
        encoded[0] = alphabet[(group >> 18) & 63];
        encoded[1] = alphabet[(group >> 12) & 63];
        encoded[2] = '=';
        encoded[3] = '=';
        if (index + 1 < length) {
            encoded[2] = alphabet[(group >> 6) & 63];
        }
        if (index + 2 < length) {
            encoded[3] = alphabet[group & 63];
        }
        encoded += 4;
    }
    return [[[NSString alloc] initWithData:output encoding:NSASCIIStringEncoding] autorelease];
}

+ (NSString*)hexEncodedStringForData:(NSData*)data
{
    NSMutableString* encoded = [NSMutableString stringWithCapacity:[data length] * 2];
    const unsigned char* bytes = [data bytes];
    for (NSUInteger index = 0; index < [data length]; ++index) {
        [encoded appendFormat:@"%02x", (unsigned int)bytes[index]];
    }
    return encoded;
}

+ (NSString*)hexDigestForString:(NSString*)value
{
    return [self hexEncodedStringForData:[self dataDigestForString:value]];
}

+ (NSString*)b64DigestForString:(NSString*)value
{
    return [self base64EncodedStringForData:[self dataDigestForString:value]];
}

+ (NSDictionary*)dictionaryWithQueryString:(NSString*)query
{
    NSMutableDictionary* values = [NSMutableDictionary dictionary];
    for (NSString* field in [query componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"&;"]]) {
        NSRange separator = [field rangeOfString:@"="];
        if (separator.location == NSNotFound) {
            continue;
        }
        NSString* key = [self stringByUrlDecodingString:[field substringToIndex:separator.location]];
        NSString* value = [self stringByUrlDecodingString:[field substringFromIndex:NSMaxRange(separator)]];
        if ([key length] != 0 && value != nil) {
            [values setObject:value forKey:key];
        }
    }
    return values;
}
@end
