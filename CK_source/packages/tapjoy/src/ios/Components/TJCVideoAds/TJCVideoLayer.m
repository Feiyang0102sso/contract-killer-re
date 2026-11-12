#import "TJCVideoLayer.h"
#import "TJCVideoObject.h"
#import "TJCCoreFetcher.h"

@implementation TJCVideoLayer
- (id)initWithVideo:(TJCVideoObject*)video target:(id)target dismissAction:(SEL)action
{
    self = [super initWithFrame:CGRectZero];
    if (self == nil) {
        return nil;
    }
    self.backgroundColor = [UIColor blackColor];
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.hidden = YES;

    statusLabel_ = [[UILabel alloc] initWithFrame:CGRectZero];
    statusLabel_.text = @"Video complete";
    statusLabel_.textAlignment = UITextAlignmentCenter;
    statusLabel_.textColor = [UIColor whiteColor];
    statusLabel_.backgroundColor = [UIColor clearColor];
    [self addSubview:statusLabel_];

    linkButtons_ = [[NSMutableArray alloc] initWithCapacity:2];
    linkURLs_ = [[NSMutableArray alloc] initWithCapacity:2];
    const SEL actions[] = {@selector(buttonAction1), @selector(buttonAction2)};
    for (NSDictionary* entry in [video buttons]) {
        NSString* address = [entry objectForKey:@"TJC_VIDEO_BUTTON_URL_KEY"];
        NSString* title = [entry objectForKey:@"TJC_VIDEO_BUTTON_LABEL_KEY"];
        if ([address length] == 0 || [title length] == 0) {
            continue;
        }
        NSURL* url = [NSURL URLWithString:address];
        if (!TJCValidHTTPSURL(url)) {
            continue;
        }
        UIButton* button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
        [button setTitle:title forState:UIControlStateNormal];
        [button addTarget:self action:actions[[linkButtons_ count]] forControlEvents:UIControlEventTouchUpInside];
        [linkButtons_ addObject:button];
        [linkURLs_ addObject:url];
        [self addSubview:button];
        if ([linkButtons_ count] == 2) {
            break;
        }
    }

    doneButton_ = [[UIButton buttonWithType:UIButtonTypeRoundedRect] retain];
    [doneButton_ setTitle:@"Done" forState:UIControlStateNormal];
    [doneButton_ addTarget:target action:action forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:doneButton_];
    return self;
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    CGFloat width = MIN(CGRectGetWidth(self.bounds) * 0.8f, 280.0f);
    CGFloat x = (CGRectGetWidth(self.bounds) - width) * 0.5f;
    CGFloat height = 36.0f;
    CGFloat step = height + 8.0f;
    CGFloat panelHeight = step * (2 + [linkButtons_ count]) - 8.0f;
    CGFloat y = MAX(0.0f, (CGRectGetHeight(self.bounds) - panelHeight) * 0.5f);
    statusLabel_.frame = CGRectMake(x, y, width, height);
    for (UIButton* button in linkButtons_) {
        y += step;
        button.frame = CGRectMake(x, y, width, height);
    }
    doneButton_.frame = CGRectMake(x, y + step, width, height);
}

- (void)enableShowVideoCompleteScreen:(BOOL)show
{
    self.hidden = !show;
}

- (void)buttonAction1
{
    if ([linkURLs_ count] >= 1) {
        [[UIApplication sharedApplication] openURL:[linkURLs_ objectAtIndex:0]];
    }
}

- (void)buttonAction2
{
    if ([linkURLs_ count] >= 2) {
        [[UIApplication sharedApplication] openURL:[linkURLs_ objectAtIndex:1]];
    }
}

- (void)dealloc
{
    [statusLabel_ release];
    [linkButtons_ release];
    [linkURLs_ release];
    [doneButton_ release];
    [super dealloc];
}
@end
