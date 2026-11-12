#pragma once
#import "TBXML.h"

@interface TJCVideoObject : NSObject {
    NSString* videoURL_;
    NSArray* buttons_;
    NSString* currencyAmount_;
    NSString* currencyName_;
    NSString* videoAdName_;
    NSString* offerID_;
    NSString* clickURL_;
    NSString* dataLocation_;
    NSString* videoFilename_;
    NSString* iconURL_;
}
@property(copy) NSString* videoURL;
@property(nonatomic, retain) NSArray* buttons;
@property(copy) NSString* currencyAmount;
@property(copy) NSString* currencyName;
@property(copy) NSString* videoAdName;
@property(copy) NSString* offerID;
@property(copy) NSString* clickURL;
@property(copy) NSString* dataLocation;
@property(copy) NSString* videoFilename;
@property(copy) NSString* iconURL;

- (id)initWithTBXML:(TBXMLElement*)element;
- (NSDictionary*)getDictionary;
@end
