#pragma once
#import <Foundation/Foundation.h>

@class PHURLLoader;

@protocol PHURLLoaderDelegate <NSObject>
@optional
- (void)loaderFinished:(PHURLLoader*)loader;
- (void)loaderFailed:(PHURLLoader*)loader;
@end

@interface PHURLLoader : NSObject {
    id<PHURLLoaderDelegate> _delegate;
    NSURLConnection* _connection;
    NSURL* _targetURL;
    NSUInteger _totalRedirects;
    BOOL _opensFinalURLOnDevice;
    id _context;
}
@property(nonatomic, assign) id<PHURLLoaderDelegate> delegate;
@property(nonatomic, retain) NSURL* targetURL;
@property(nonatomic, assign) BOOL opensFinalURLOnDevice;
@property(nonatomic, retain) id context;
+ (PHURLLoader*)openDeviceURL:(NSString*)address;
+ (NSMutableSet*)allLoaders;
+ (void)invalidateAllLoadersWithDelegate:(id)delegate;
- (void)open;
- (void)invalidate;
@end
