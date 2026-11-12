#pragma once
#import <Foundation/Foundation.h>

@interface PHStringUtil : NSObject
+ (NSString*)stringWithQueryQuirky:(NSDictionary*)parameters;
+ (NSString*)stringWithQuery:(NSDictionary*)parameters;
+ (NSString*)stringByHtmlEscapingString:(NSString*)value;
+ (NSString*)stringByUrlEncodingString:(NSString*)value;
+ (NSString*)stringByUrlDecodingString:(NSString*)value;
+ (NSString*)uuid;
+ (NSData*)dataDigestForString:(NSString*)value;
+ (NSString*)base64EncodedStringForData:(NSData*)data;
+ (NSString*)hexEncodedStringForData:(NSData*)data;
+ (NSString*)hexDigestForString:(NSString*)value;
+ (NSString*)b64DigestForString:(NSString*)value;
+ (NSDictionary*)dictionaryWithQueryString:(NSString*)query;
@end
