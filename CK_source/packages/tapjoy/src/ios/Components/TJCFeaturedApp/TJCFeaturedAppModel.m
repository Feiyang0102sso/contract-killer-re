#import "TJCFeaturedAppModel.h"

@implementation TJCFeaturedAppModel
@synthesize cost = cost_, storeID = storeID_, name = name_, description = description_;
@synthesize amount = amount_, iconURL = iconURL_, largeIconURL = largeIconURL_;
@synthesize redirectURL = redirectURL_, maxTimesToDisplayThisApp = maxTimesToDisplayThisApp_;
@synthesize fullScreenAdURL = fullScreenAdURL_;

- (id)initWithTBXML:(TBXMLElement*)element
{
    self = [super init];
    if (self) {
        cost_ = [[TBXML textForElement:[TBXML childElementNamed:@"Cost" parentElement:element]] copy];
        storeID_ = [[TBXML textForElement:[TBXML childElementNamed:@"StoreID" parentElement:element]] copy];
        name_ = [[TBXML textForElement:[TBXML childElementNamed:@"Name" parentElement:element]] copy];
        description_ = [[TBXML textForElement:[TBXML childElementNamed:@"Description" parentElement:element]] copy];
        iconURL_ = [[TBXML textForElement:[TBXML childElementNamed:@"IconURL" parentElement:element]] copy];
        largeIconURL_ = [[TBXML textForElement:[TBXML childElementNamed:@"MediumIconURL" parentElement:element]] copy];
        redirectURL_ = [[TBXML textForElement:[TBXML childElementNamed:@"RedirectURL" parentElement:element]] copy];
        amount_ = [TBXML numberForElement:[TBXML childElementNamed:@"Amount" parentElement:element]];
        if (maxTimesToDisplayThisApp_ < 0) {
            maxTimesToDisplayThisApp_ = 4;
        }
        fullScreenAdURL_ = [[TBXML textForElement:[TBXML childElementNamed:@"FullScreenAdURL" parentElement:element]] copy];
    }
    return self;
}

- (void)dealloc
{
    [cost_ release];
    [storeID_ release];
    [name_ release];
    [redirectURL_ release];
    [iconURL_ release];
    [largeIconURL_ release];
    [description_ release];
    [fullScreenAdURL_ release];
    [super dealloc];
}
@end
