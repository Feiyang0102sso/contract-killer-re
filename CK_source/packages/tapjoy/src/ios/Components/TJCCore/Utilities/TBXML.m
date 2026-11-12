// Tapjoy's XML element interface. Foundation parses the input into the original linked tree shape.
#import "TBXML.h"

#include <stdlib.h>
#include <string.h>

enum { TBXMLBufferCapacity = 100 };

typedef struct TBXMLElementBuffer {
    TBXMLElement elements[TBXMLBufferCapacity];
    struct TBXMLElementBuffer* previous;
} TBXMLElementBuffer;

typedef struct TBXMLAttributeBuffer {
    TBXMLAttribute attributes[TBXMLBufferCapacity];
    struct TBXMLAttributeBuffer* previous;
} TBXMLAttributeBuffer;

@interface TBXML (Private)
- (void)decodeData:(NSData*)data;
- (void)decodeBytes;
- (TBXMLElement*)nextAvailableElement;
- (TBXMLAttribute*)nextAvailableAttribute;
@end

@implementation TBXML

+ (id)tbxmlWithURL:(NSURL*)url
{
    return [[[self alloc] initWithURL:url] autorelease];
}

+ (id)tbxmlWithXMLString:(NSString*)xml
{
    return [[[self alloc] initWithXMLString:xml] autorelease];
}

+ (id)tbxmlWithXMLData:(NSData*)data
{
    return [[[self alloc] initWithXMLData:data] autorelease];
}

- (id)init
{
    self = [super init];
    if (self != nil) {
        currentElement = TBXMLBufferCapacity;
        currentAttribute = TBXMLBufferCapacity;
    }
    return self;
}

- (id)initWithURL:(NSURL*)url
{
    return [self initWithXMLData:[NSData dataWithContentsOfURL:url]];
}

- (id)initWithXMLString:(NSString*)xml
{
    return [self initWithXMLData:[xml dataUsingEncoding:NSUTF8StringEncoding]];
}

- (id)initWithXMLData:(NSData*)data
{
    self = [self init];
    if (self != nil) {
        [self decodeData:data];
    }
    return self;
}

- (TBXMLElement*)rootXMLElement
{
    return rootXMLElement;
}

@end

@implementation TBXML (StaticFunctions)

+ (NSString*)elementName:(TBXMLElement*)element
{
    if (element == NULL || element->name == NULL) {
        return @"";
    }
    return [NSString stringWithCString:element->name encoding:NSUTF8StringEncoding];
}

+ (NSString*)attributeName:(TBXMLAttribute*)attribute
{
    if (attribute == NULL || attribute->name == NULL) {
        return @"";
    }
    return [NSString stringWithCString:attribute->name encoding:NSUTF8StringEncoding];
}

+ (NSString*)attributeValue:(TBXMLAttribute*)attribute
{
    if (attribute == NULL || attribute->value == NULL) {
        return @"";
    }
    return [NSString stringWithCString:attribute->value encoding:NSUTF8StringEncoding];
}

+ (NSString*)textForElement:(TBXMLElement*)element
{
    if (element == NULL || element->text == NULL) {
        return @"";
    }
    return [NSString stringWithCString:element->text encoding:NSUTF8StringEncoding];
}

+ (int)numberForElement:(TBXMLElement*)element
{
    return [[self textForElement:element] intValue];
}

+ (BOOL)boolForElement:(TBXMLElement*)element
{
    NSString* value = [[self textForElement:element] lowercaseString];
    return [value isEqualToString:@"yes"] || [value isEqualToString:@"true"];
}

+ (int)negativeNumberForUnknownElement:(TBXMLElement*)element
{
    if (element == NULL || element->text == NULL) {
        return -1;
    }
    return [[self textForElement:element] intValue];
}

+ (NSString*)valueOfAttributeNamed:(NSString*)name forElement:(TBXMLElement*)element
{
    if (element == NULL || name == nil) {
        return nil;
    }
    const char* key = [name UTF8String];
    for (TBXMLAttribute* attr = element->firstAttribute; attr != NULL; attr = attr->next) {
        if (strcmp(attr->name, key) == 0) {
            return [NSString stringWithCString:attr->value encoding:NSUTF8StringEncoding];
        }
    }
    return nil;
}

+ (TBXMLElement*)childElementNamed:(NSString*)name parentElement:(TBXMLElement*)element
{
    if (element == NULL || name == nil) {
        return NULL;
    }
    const char* key = [name UTF8String];
    for (TBXMLElement* child = element->firstChild; child != NULL; child = child->nextSibling) {
        if (strcmp(child->name, key) == 0) {
            return child;
        }
    }
    return NULL;
}

+ (TBXMLElement*)nextSiblingNamed:(NSString*)name searchFromElement:(TBXMLElement*)element
{
    if (element == NULL || name == nil) {
        return NULL;
    }
    const char* key = [name UTF8String];
    for (TBXMLElement* sibling = element->nextSibling; sibling != NULL; sibling = sibling->nextSibling) {
        if (strcmp(sibling->name, key) == 0) {
            return sibling;
        }
    }
    return NULL;
}

@end

@implementation TBXML (Private)

- (void)decodeData:(NSData*)data
{
    bytesLength = [data length];
    if (bytesLength == 0) {
        return;
    }
    bytes = malloc(bytesLength);
    memcpy(bytes, [data bytes], bytesLength);
    [self decodeBytes];
}

- (void)decodeBytes
{
    NSData* source = [NSData dataWithBytesNoCopy:bytes length:bytesLength freeWhenDone:NO];
    NSXMLParser* parser = [[NSXMLParser alloc] initWithData:source];
    [parser setDelegate:self];
    [parser setShouldResolveExternalEntities:NO];
    textStack = [[NSMutableArray alloc] init];
    if (![parser parse]) {
        rootXMLElement = NULL;
    }
    [textStack release];
    textStack = nil;
    parsingElement = NULL;
    [parser release];
}

- (TBXMLElement*)nextAvailableElement
{
    if (currentElement == TBXMLBufferCapacity) {
        TBXMLElementBuffer* block = calloc(1, sizeof(TBXMLElementBuffer));
        block->previous = currentElementBuffer;
        currentElementBuffer = block;
        currentElement = 0;
    }
    return &currentElementBuffer->elements[currentElement++];
}

- (TBXMLAttribute*)nextAvailableAttribute
{
    if (currentAttribute == TBXMLBufferCapacity) {
        TBXMLAttributeBuffer* block = calloc(1, sizeof(TBXMLAttributeBuffer));
        block->previous = currentAttributeBuffer;
        currentAttributeBuffer = block;
        currentAttribute = 0;
    }
    return &currentAttributeBuffer->attributes[currentAttribute++];
}

- (void)parser:(NSXMLParser*)parser didStartElement:(NSString*)elementName
    namespaceURI:(NSString*)namespaceURI qualifiedName:(NSString*)qualifiedName
    attributes:(NSDictionary*)attributes
{
    TBXMLElement* element = [self nextAvailableElement];
    element->name = strdup([elementName UTF8String]);
    element->parentElement = parsingElement;
    if (parsingElement == NULL) {
        rootXMLElement = element;
    } else {
        element->previousSibling = parsingElement->currentChild;
        if (element->previousSibling == NULL) {
            parsingElement->firstChild = element;
        } else {
            element->previousSibling->nextSibling = element;
        }
        parsingElement->currentChild = element;
    }
    TBXMLAttribute** tail = &element->firstAttribute;
    for (NSString* key in attributes) {
        TBXMLAttribute* attribute = [self nextAvailableAttribute];
        attribute->name = strdup([key UTF8String]);
        attribute->value = strdup([[attributes objectForKey:key] UTF8String]);
        *tail = attribute;
        tail = &attribute->next;
    }
    parsingElement = element;
    [textStack addObject:[NSMutableString string]];
}

- (void)parser:(NSXMLParser*)parser foundCharacters:(NSString*)characters
{
    [[textStack lastObject] appendString:characters];
}

- (void)parser:(NSXMLParser*)parser foundCDATA:(NSData*)data
{
    NSString* characters = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    if (characters != nil) {
        [[textStack lastObject] appendString:characters];
    }
    [characters release];
}

- (void)parser:(NSXMLParser*)parser didEndElement:(NSString*)elementName
    namespaceURI:(NSString*)namespaceURI qualifiedName:(NSString*)qualifiedName
{
    NSString* content = [[textStack lastObject]
        stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    if ([content length] != 0) {
        parsingElement->text = strdup([content UTF8String]);
    }
    [textStack removeLastObject];
    parsingElement = parsingElement->parentElement;
}

- (void)dealloc
{
    while (currentElementBuffer != NULL) {
        TBXMLElementBuffer* block = currentElementBuffer;
        for (unsigned int index = 0; index < TBXMLBufferCapacity; ++index) {
            free(block->elements[index].name);
            free(block->elements[index].text);
        }
        currentElementBuffer = block->previous;
        free(block);
    }
    while (currentAttributeBuffer != NULL) {
        TBXMLAttributeBuffer* block = currentAttributeBuffer;
        for (unsigned int index = 0; index < TBXMLBufferCapacity; ++index) {
            free(block->attributes[index].name);
            free(block->attributes[index].value);
        }
        currentAttributeBuffer = block->previous;
        free(block);
    }
    free(bytes);
    [super dealloc];
}

@end
