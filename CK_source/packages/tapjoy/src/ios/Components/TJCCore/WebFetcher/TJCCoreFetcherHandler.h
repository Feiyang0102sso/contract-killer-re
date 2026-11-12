#pragma once
#import "TBXML.h"
#import "TJCCoreFetcher.h"

extern NSString* const kTJCCoreFetcherReturnObjStr;
extern NSString* const kTJCCoreFetcherAdStr;
extern NSString* const kTJCCoreFetcherVideoStr;

@interface TJCCoreFetcherHandler : NSObject {
    id deleg_;
    NSInteger requestTag_;
    TJCCoreFetcher* myFetcher_;
}
- (id)initRequestWithDelegate:(id)delegate andRequestTag:(NSInteger)tag;
- (void)makeGenericRequestWithURL:(NSString*)url alternateURL:(NSString*)alternate
                           params:(NSDictionary*)params selector:(SEL)selector;
- (void)makeGenericPOSTRequestWithURL:(NSString*)url alternateURL:(NSString*)alternate
                               data:(NSData*)data params:(NSDictionary*)params selector:(SEL)selector;
- (void)makeRequestWithURL:(NSString*)url withInvoker:(NSInvocation*)invocation;
- (TBXMLElement*)parseReturnObjectAsTBXMLElement:(TJCCoreFetcher*)fetcher;
- (BOOL)validateResponseReturnedObject:(TBXMLElement*)root;
- (void)cancel;
@end
