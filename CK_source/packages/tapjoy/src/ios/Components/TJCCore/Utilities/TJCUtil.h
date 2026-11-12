#pragma once
#import <Foundation/Foundation.h>

@interface TJCUtil : NSObject
+ (BOOL)isPad;
+ (BOOL)caseInsenstiveCompare:(NSString*)first AndString2:(NSString*)second;
+ (NSData*)dataWithBase64EncodedString:(NSString*)string;
+ (NSData*)initWithBase64EncodedString:(NSString*)string;
@end
