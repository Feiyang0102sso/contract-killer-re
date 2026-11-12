#ifndef CK_TBXML_H
#define CK_TBXML_H

#import <Foundation/Foundation.h>

typedef struct TBXMLAttribute {
    char* name;
    char* value;
    struct TBXMLAttribute* next;
} TBXMLAttribute;

typedef struct TBXMLElement {
    char* name;
    char* text;
    TBXMLAttribute* firstAttribute;
    struct TBXMLElement* parentElement;
    struct TBXMLElement* firstChild;
    struct TBXMLElement* currentChild;
    struct TBXMLElement* nextSibling;
    struct TBXMLElement* previousSibling;
} TBXMLElement;

struct TBXMLElementBuffer;
struct TBXMLAttributeBuffer;

@interface TBXML : NSObject {
    TBXMLElement* rootXMLElement;
    struct TBXMLElementBuffer* currentElementBuffer;
    struct TBXMLAttributeBuffer* currentAttributeBuffer;
    unsigned int currentElement;
    unsigned int currentAttribute;
    char* bytes;
    NSUInteger bytesLength;
    TBXMLElement* parsingElement;
    NSMutableArray* textStack;
}
+ (id)tbxmlWithURL:(NSURL*)url;
+ (id)tbxmlWithXMLString:(NSString*)xml;
+ (id)tbxmlWithXMLData:(NSData*)data;
- (id)initWithURL:(NSURL*)url;
- (id)initWithXMLString:(NSString*)xml;
- (id)initWithXMLData:(NSData*)data;
- (TBXMLElement*)rootXMLElement;
@end

@interface TBXML (StaticFunctions)
+ (NSString*)elementName:(TBXMLElement*)element;
+ (NSString*)attributeName:(TBXMLAttribute*)attribute;
+ (NSString*)attributeValue:(TBXMLAttribute*)attribute;
+ (NSString*)textForElement:(TBXMLElement*)element;
+ (int)numberForElement:(TBXMLElement*)element;
+ (BOOL)boolForElement:(TBXMLElement*)element;
+ (int)negativeNumberForUnknownElement:(TBXMLElement*)element;
+ (NSString*)valueOfAttributeNamed:(NSString*)name forElement:(TBXMLElement*)element;
+ (TBXMLElement*)childElementNamed:(NSString*)name parentElement:(TBXMLElement*)element;
+ (TBXMLElement*)nextSiblingNamed:(NSString*)name searchFromElement:(TBXMLElement*)element;
@end

#endif
