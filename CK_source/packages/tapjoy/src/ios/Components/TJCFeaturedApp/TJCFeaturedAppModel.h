#pragma once
#import "TBXML.h"

/** Featured app record delivered with TJC_FEATURED_APP_RESPONSE_NOTIFICATION. */
@interface TJCFeaturedAppModel : NSObject {
    NSString* cost_;
    NSString* storeID_;
    NSString* name_;
    NSString* description_;
    int amount_;
    NSString* iconURL_;
    NSString* largeIconURL_;
    NSString* redirectURL_;
    int maxTimesToDisplayThisApp_;
    NSString* fullScreenAdURL_;
}
@property(copy) NSString* cost;
@property(copy) NSString* storeID;
@property(copy) NSString* name;
@property(copy) NSString* description;
@property(assign) int amount;
@property(copy) NSString* iconURL;
@property(copy) NSString* largeIconURL;
@property(copy) NSString* redirectURL;
@property(assign) int maxTimesToDisplayThisApp;
@property(copy) NSString* fullScreenAdURL;

- (id)initWithTBXML:(TBXMLElement*)element;
@end
