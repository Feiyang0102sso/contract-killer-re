#import "TJCLoadingView.h"

@implementation TJCLoadingView
@synthesize mainView = mainView_;

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        self.backgroundColor = [UIColor colorWithWhite:0.0f alpha:0.45f];
        spinner_ = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
        spinner_.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin |
                                    UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin;
        spinner_.center = CGPointMake(CGRectGetMidX(self.bounds), CGRectGetMidY(self.bounds));
        [self addSubview:spinner_];
        self.hidden = YES;
    }
    return self;
}

- (void)disable
{
    [spinner_ stopAnimating];
    self.hidden = YES;
}

- (void)enable
{
    self.hidden = NO;
    [spinner_ startAnimating];
    [mainView_ bringSubviewToFront:self];
}

- (void)fadeIn
{
    [self enable];
}

- (void)fadeOut
{
    [self disable];
}

- (void)dealloc
{
    [spinner_ release];
    [super dealloc];
}
@end
