#pragma once
#import <Foundation/Foundation.h>

NSError* TJCFetchError(NSInteger code, NSString* message);
BOOL TJCValidHTTPSURL(NSURL* url);
BOOL TJCURLHasSameOrigin(NSURL* url, NSURL* base);

@interface TJCCoreFetcher : NSObject {
    NSInteger requestTag_;
    unsigned int retryCount_;
    NSURLResponse* response_;
    NSTimeInterval requestTimeout_;
    NSURLConnection* connection_;
    NSInvocation* invocation_;
    NSString* requestMethod_;
    NSDictionary* postParameters_;
    BOOL hasFetched_;
    NSString* baseURL_;
    NSString* alternateURL_;
    NSDictionary* bindings_;
    NSError* error_;
    NSMutableData* data_;
    NSData* POSTdata_;
    NSUInteger maximumResponseBytes_;
}
@property(nonatomic, assign) NSInteger requestTag;
@property(nonatomic, assign) BOOL hasFetched;
@property(nonatomic, copy) NSString* baseURL;
@property(nonatomic, copy) NSString* alternateURL;
@property(nonatomic, retain) NSDictionary* bindings;
@property(nonatomic, retain) NSError* error;
@property(nonatomic, retain) NSMutableData* data;
@property(nonatomic, retain) NSData* POSTdata;
@property(nonatomic, retain) NSURLResponse* response;
@property(nonatomic, assign) NSTimeInterval requestTimeout;
@property(nonatomic, retain) NSURLConnection* connection;
@property(nonatomic, retain) NSInvocation* invocation;
@property(nonatomic, copy) NSString* requestMethod;
@property(nonatomic, retain) NSDictionary* postParameters;
@property(nonatomic, assign) NSUInteger maximumResponseBytes;

- (void)fetchAsynchronouslyWithCompletionInvocation:(NSInvocation*)invocation;
- (NSString*)createQueryStringFromDict:(NSDictionary*)parameters;
- (NSURL*)requestURL;
- (NSString*)urlEncodedBindings;
- (BOOL)hasError;
- (void)retry;
- (void)cancel;
@end
