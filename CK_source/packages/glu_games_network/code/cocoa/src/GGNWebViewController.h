// GGNWebViewController.h - one Glu Games Network web page (UIWebView with a loading spinner).
#import <UIKit/UIKit.h>

@interface GGNWebViewController : UIViewController <UIWebViewDelegate>
{
    UIWebView*                  webView;
    UIActivityIndicatorView*    loadingSpinner;
    NSString*                   defaultUrl;
    NSString*                   platformString;     // "ipad" or "iphone"
    BOOL                        isLoaded;
    int                         loadingErrorCode;
    NSURL*                      iTunesURL;          // last redirect target of an App Store link
}

@property (nonatomic, retain) NSURL* iTunesURL;
@property (nonatomic, retain) NSString* defaultUrl;

- (id) initWithStartUrl: (NSString*) url;
- (void) reloadWithUrl: (NSString*) url;
- (BOOL) isComplete;
- (BOOL) isLoading;
- (void) startLoading;
- (void) stopLoading;

@end
