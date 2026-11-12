#ifndef PHREWARD_H
#define PHREWARD_H

#import <Foundation/Foundation.h>

@interface PHReward : NSObject {
    NSString* _reward;
    int _quanity;  // Spelling follows the CK ObjC ivar record.
    NSString* _receipt;
}
@property (nonatomic, copy) NSString* name;
@property (nonatomic, assign) int quantity;
@property (nonatomic, copy) NSString* receipt;
@end

#endif
