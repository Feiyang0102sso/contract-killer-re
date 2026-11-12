#import "TJCVideoObject.h"

static NSString* VideoField(TBXMLElement* element, NSString* key)
{
    return [TBXML textForElement:[TBXML childElementNamed:key parentElement:element]];
}

@implementation TJCVideoObject
@synthesize videoURL = videoURL_, buttons = buttons_, currencyAmount = currencyAmount_;
@synthesize currencyName = currencyName_, videoAdName = videoAdName_, offerID = offerID_;
@synthesize clickURL = clickURL_, dataLocation = dataLocation_, videoFilename = videoFilename_;
@synthesize iconURL = iconURL_;

- (id)initWithTBXML:(TBXMLElement*)element
{
    self = [super init];
    if (self) {
        NSMutableArray* buttons = [[NSMutableArray alloc] init];
        TBXMLElement* container = [TBXML childElementNamed:@"Buttons" parentElement:element];
        TBXMLElement* button = [TBXML childElementNamed:@"Button" parentElement:container];
        while (button != NULL) {
            NSString* name = VideoField(button, @"Name");
            NSString* url = [VideoField(button, @"URL") stringByReplacingOccurrencesOfString:@"&amp;" withString:@"&"];
            [buttons addObject:[NSDictionary dictionaryWithObjectsAndKeys:
                name, @"TJC_VIDEO_BUTTON_LABEL_KEY", url, @"TJC_VIDEO_BUTTON_URL_KEY", nil]];
            button = [TBXML nextSiblingNamed:@"Button" searchFromElement:button];
        }
        buttons_ = buttons;
        videoURL_ = [VideoField(element, @"VideoURL") copy];
        currencyAmount_ = [VideoField(element, @"Amount") copy];
        offerID_ = [VideoField(element, @"OfferID") copy];
        clickURL_ = [VideoField(element, @"ClickURL") copy];
        videoAdName_ = [VideoField(element, @"Name") copy];
        videoFilename_ = [[[videoURL_ componentsSeparatedByString:@"/"] lastObject] copy];
        currencyName_ = [VideoField(element, @"CurrencyName") copy];
        NSString* iconURL = VideoField(element, @"IconURL");
        if ([iconURL length] == 0) {
            iconURL = @"NO_ICON";
        }
        iconURL_ = [iconURL copy];
    }
    return self;
}

- (NSDictionary*)getDictionary
{
    NSMutableDictionary* dictionary = [NSMutableDictionary dictionary];
    [dictionary setValue:videoURL_ forKey:@"TJC_VIDEO_OBJ_VIDEO_URL"];
    [dictionary setValue:buttons_ forKey:@"TJC_VIDEO_OBJ_BUTTONS"];
    [dictionary setValue:currencyAmount_ forKey:@"TJC_VIDEO_OBJ_CURRENCY_AMOUNT"];
    [dictionary setValue:currencyName_ forKey:@"TJC_VIDEO_OBJ_CURRENCY_NAME"];
    [dictionary setValue:offerID_ forKey:@"TJC_VIDEO_OBJ_OFFER_ID"];
    [dictionary setValue:clickURL_ forKey:@"TJC_VIDEO_OBJ_CLICK_URL"];
    [dictionary setValue:videoFilename_ forKey:@"TJC_VIDEO_OBJ_FILENAME"];
    [dictionary setValue:videoAdName_ forKey:@"TJC_VIDEO_OBJ_NAME"];
    [dictionary setValue:iconURL_ forKey:@"TJC_VIDEO_OBJ_ICON_URL"];
    [dictionary setValue:dataLocation_ forKey:@"TJC_VIDEO_OBJ_DATA_LOCATION"];
    return dictionary;
}

- (void)dealloc
{
    [videoURL_ release];
    [buttons_ release];
    [currencyAmount_ release];
    [currencyName_ release];
    [videoAdName_ release];
    [offerID_ release];
    [clickURL_ release];
    [dataLocation_ release];
    [videoFilename_ release];
    [iconURL_ release];
    [super dealloc];
}
@end
