#pragma once
#import "TBXML.h"

#define TJC_TAPPOINTS_EARNED_NOTIFICATION @"TJC_TAPPOINTS_EARNED_NOTIFICATION"

@interface TJCUserAccountModel : NSObject {
    int tapPoints_;
    NSString* pointsID_;
    NSString* currencyName_;
}
@property(nonatomic, assign) int tapPoints;
@property(nonatomic, copy) NSString* pointsID;
@property(nonatomic, copy) NSString* currencyName;

- (id)initWithTBXML:(TBXMLElement*)element;
- (void)updateWithTBXML:(TBXMLElement*)element shouldCheckEarnedPoints:(BOOL)checkEarned;
- (BOOL)updateFromAccountElement:(TBXMLElement*)element checkEarned:(BOOL)checkEarned;
@end
