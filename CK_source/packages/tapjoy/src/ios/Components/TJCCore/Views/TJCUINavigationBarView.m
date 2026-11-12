#import "TJCUINavigationBarView.h"

@implementation TJCUINavigationBarView
@synthesize navBar, navBarTitle, navBarLeftBtn, navBarRightBtn;

- (id)initWithFrame:(CGRect)frame
{
    return [self initWithTitle:nil withFrame:frame AtY:frame.origin.y];
}

- (id)initWithTitle:(NSString*)title withFrame:(CGRect)frame AtY:(CGFloat)y
{
    frame.origin.y = y;
    self = [super initWithFrame:frame];
    if (self != nil) {
        if (title == nil) {
            title = @"";
        }
        navBar = [[UINavigationBar alloc] initWithFrame:[self bounds]];
        navBar.tintColor = [UIColor darkGrayColor];
        navBar.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        navBarTitle = [[UINavigationItem alloc] initWithTitle:title];
        [navBar setItems:[NSArray arrayWithObject:navBarTitle]];
        [self addSubview:navBar];
    }
    return self;
}

- (void)setCustomBackgroundImage:(UIImage*)image
{
    if ([navBar respondsToSelector:@selector(setBackgroundImage:forBarMetrics:)]) {
        [navBar setBackgroundImage:image forBarMetrics:UIBarMetricsDefault];
        return;
    }
    if (bgImage == nil && image != nil) {
        bgImage = [[UIImageView alloc] initWithFrame:[navBar bounds]];
        bgImage.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        [navBar insertSubview:bgImage atIndex:0];
    }
    bgImage.image = image;
    bgImage.hidden = image == nil;
}

- (void)updateLeftNavBarBtnWithTitle:(NSString*)title
{
    navBarLeftBtn.title = title;
}

- (void)updateRightNavBarBtnWithTitle:(NSString*)title
{
    navBarRightBtn.title = title;
}

- (void)setNavBarLeftBtn:(UIBarButtonItem*)item
{
    if (navBarLeftBtn != item) {
        [navBarLeftBtn release];
        navBarLeftBtn = [item retain];
        navBarTitle.leftBarButtonItem = item;
    }
}

- (void)setNavBarRightBtn:(UIBarButtonItem*)item
{
    if (navBarRightBtn != item) {
        [navBarRightBtn release];
        navBarRightBtn = [item retain];
        navBarTitle.rightBarButtonItem = item;
    }
}

- (void)setNavBarLeftBtnWithTitle:(NSString*)title
{
    if (navBarLeftBtn == nil) {
        self.navBarLeftBtn = [[[UIBarButtonItem alloc] initWithTitle:title style:UIBarButtonItemStylePlain
                                                            target:nil action:NULL] autorelease];
    } else {
        [self updateLeftNavBarBtnWithTitle:title];
    }
}

- (void)setNavBarRightBtnWithTitle:(NSString*)title
{
    if (navBarRightBtn == nil) {
        self.navBarRightBtn = [[[UIBarButtonItem alloc] initWithTitle:title style:UIBarButtonItemStylePlain
                                                             target:nil action:NULL] autorelease];
    } else {
        [self updateRightNavBarBtnWithTitle:title];
    }
}

- (void)addLeftButtonTarget:(id)target action:(SEL)action forControlEvents:(UIControlEvents)events
{
    navBarLeftBtn.target = target;
    navBarLeftBtn.action = action;
}

- (void)addRightButtonTarget:(id)target action:(SEL)action forControlEvents:(UIControlEvents)events
{
    navBarRightBtn.target = target;
    navBarRightBtn.action = action;
}

- (UILabel*)titleLabel
{
    return titleLabel;
}

- (void)dealloc
{
    [bgImage release];
    [titleLabel release];
    [navBarLeftBtn release];
    [navBarRightBtn release];
    [navBarTitle release];
    [navBar release];
    [super dealloc];
}
@end
