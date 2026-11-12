// CHTTPNative.mm - NSURLConnection delegate (HTTPHandler) and its C++ wrapper CHTTPNative.
#include "CHTTPNative.h"
#import <Foundation/Foundation.h>

@interface HTTPHandler : NSObject {
    NSMutableData* data;               // 0x04
    NSURLConnection* connection;       // 0x08
    NSString* response;                // 0x0c
    NSError* connectionError;          // 0x10
}
@property (nonatomic, retain) NSMutableData* data;
@property (nonatomic, retain) NSURLConnection* connection;
@property (nonatomic, retain) NSString* response;
@property (nonatomic, retain) NSError* connectionError;
- (void)startConnectionWithRequest:(NSURLRequest*)request;
@end

@implementation HTTPHandler

- (void)dealloc
{
    [data release];
    [connection cancel];
    [connection release];
    [response release];
    [connectionError release];
    [super dealloc];
}

- (void)startConnectionWithRequest:(NSURLRequest*)request
{
    NSMutableData* newData = [[NSMutableData alloc] init];
    NSURLConnection* newConnection = [[NSURLConnection alloc] initWithRequest:request delegate:self];
    [self setData:newData];
    [self setConnection:newConnection];
    [newData release];
    [newConnection release];
    [self setConnectionError:nil];
    [self setResponse:nil];
}

- (void)connection:(NSURLConnection*)theConnection didReceiveData:(NSData*)received
{
    [[self data] appendData:received];
}

- (void)connection:(NSURLConnection*)theConnection didFailWithError:(NSError*)error
{
    [self setConnection:nil];
    [self setData:nil];
    [self setConnectionError:error];
}

- (void)connectionDidFinishLoading:(NSURLConnection*)theConnection
{
    NSString* text = [[NSString alloc] initWithData:[self data] encoding:NSUTF8StringEncoding];
    [self setResponse:text];
    [text release];
    [self setConnection:nil];
    [self setData:nil];
}

@synthesize data;
@synthesize connection;
@synthesize response;
@synthesize connectionError;

@end

CHTTPNative::CHTTPNative()
{
    m_handler = [[HTTPHandler alloc] init];
}

CHTTPNative::~CHTTPNative()
{
    [(HTTPHandler*)m_handler release];
    m_handler = NULL;
}

void CHTTPNative::Cancel()
{
    [[(HTTPHandler*)m_handler connection] cancel];
}

void CHTTPNative::ClearResponse()
{
    [(HTTPHandler*)m_handler setResponse:nil];
}

void CHTTPNative::Send(unsigned long timeoutMs, const char* url, const char* body, unsigned long bodyLength,
                       const char* userAgent, const char* contentType, const char* method)
{
    HTTPHandler* handler = (HTTPHandler*)m_handler;
    if ([handler connection] != nil) {
        return;
    }
    NSString* urlText = [[NSString alloc] initWithCString:url encoding:NSASCIIStringEncoding];
    NSMutableURLRequest* request = [[NSMutableURLRequest alloc] initWithURL:[NSURL URLWithString:urlText]];
    [urlText release];
    [request setTimeoutInterval:(float)timeoutMs / 1000.0f];
    if (body != NULL && bodyLength != 0) {
        [request setHTTPBody:[NSData dataWithBytes:body length:bodyLength]];
    }
    if (userAgent != NULL) {
        NSString* value = [[NSString alloc] initWithUTF8String:userAgent];
        [request setValue:value forHTTPHeaderField:@"User-Agent"];
        [value release];
    }
    if (contentType != NULL) {
        NSString* value = [[NSString alloc] initWithUTF8String:contentType];
        [request setValue:value forHTTPHeaderField:@"Content-Type"];
        [value release];
    }
    if (method != NULL) {
        // Only the second letter is checked: pUt, gEt, otherwise POST.
        switch (method[1]) {
        case 'E':
            [request setHTTPMethod:@"GET"];
            break;
        case 'U':
            [request setHTTPMethod:@"PUT"];
            break;
        default:
            [request setHTTPMethod:@"POST"];
            break;
        }
    }
    [(HTTPHandler*)m_handler startConnectionWithRequest:request];
    [request release];
}

bool CHTTPNative::IsBusy() const
{
    return [(HTTPHandler*)m_handler connection] != nil;
}

int CHTTPNative::GetError()
{
    return [[(HTTPHandler*)m_handler connectionError] code];
}

const char* CHTTPNative::GetResponsePtr()
{
    if ([(HTTPHandler*)m_handler response] == nil) {
        return NULL;
    }
    return [[(HTTPHandler*)m_handler response] UTF8String];
}

unsigned int CHTTPNative::GetResponseLen()
{
    if ([(HTTPHandler*)m_handler response] == nil) {
        return 0;
    }
    return [[(HTTPHandler*)m_handler response] lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
}
