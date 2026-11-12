#import "TJCUserAccountManager.h"
#import "TJCUserAccountRequestHandler.h"
#import "TapjoyConnect.h"

static TJCUserAccountManager* SharedAccountManager;

static NSString* AccountErrorNotification(NSInteger tag)
{
    switch (tag) {
    case TJCAccountSpendPoints:
        return @"TJC_SPEND_TAP_POINTS_RESPONSE_NOTIFICATION_ERROR";
    case TJCAccountAwardPoints:
        return @"TJC_AWARD_TAP_POINTS_RESPONSE_NOTIFICATION_ERROR";
    default:
        return @"TJC_TAP_POINTS_RESPONSE_NOTIFICATION_ERROR";
    }
}

@implementation TJCUserAccountManager
@synthesize userAccountModelObj = userAccountModelObj_;

+ (TJCUserAccountManager*)sharedTJCUserAccountManager
{
    if (SharedAccountManager == nil) {
        SharedAccountManager = [[self alloc] init];
    }
    return SharedAccountManager;
}

+ (void)releaseSharedTJCUserAccountManager
{
    [SharedAccountManager release];
    SharedAccountManager = nil;
}

- (id)init
{
    self = [super init];
    if (self) {
        userAccountModelObj_ = [[TJCUserAccountModel alloc] init];
    }
    return self;
}

- (void)updateUserAccountObjWithTBXMLElement:(TBXMLElement*)element
{
    [userAccountModelObj_ updateWithTBXML:element shouldCheckEarnedPoints:YES];
}

- (TJCUserAccountRequestHandler*)beginRequest:(NSInteger)tag
{
    if (waitingForResponse_ || ![[TapjoyConnect sharedTapjoyConnect] isConnected]) {
        [[NSNotificationCenter defaultCenter] postNotificationName:AccountErrorNotification(tag) object:nil];
        return nil;
    }
    waitingForResponse_ = YES;
    if (userAccountModelObj_ == nil) {
        userAccountModelObj_ = [[TJCUserAccountModel alloc] init];
    }
    TJCUserAccountRequestHandler** handler = &userAccountSpendTapPointsObj_;
    if (tag == TJCAccountGetPoints) {
        handler = &userAccountGetTapPointsObj_;
    }
    [*handler release];
    *handler = [[TJCUserAccountRequestHandler alloc] initRequestWithDelegate:self andRequestTag:tag];
    return *handler;
}

- (void)getTapPoints
{
    [[self beginRequest:TJCAccountGetPoints] requestTapPoints];
}

- (void)spendTapPoints:(int)points
{
    if (points > 0) {
        [[self beginRequest:TJCAccountSpendPoints] subtractTapPoints:points];
    }
}

- (void)awardTapPoints:(int)points
{
    if (points > 0) {
        [[self beginRequest:TJCAccountAwardPoints] addTapPoints:points];
    }
}

- (void)fetchResponseSuccessWithData:(TBXMLElement*)element withRequestTag:(NSInteger)tag
{
    [self retain];
    if ([userAccountModelObj_ updateFromAccountElement:element checkEarned:tag == TJCAccountGetPoints]) {
        waitingForResponse_ = NO;
        NSString* name = TJC_TAP_POINTS_RESPONSE_NOTIFICATION;
        if (tag == TJCAccountSpendPoints) {
            name = TJC_SPEND_TAP_POINTS_RESPONSE_NOTIFICATION;
        } else if (tag == TJCAccountAwardPoints) {
            name = @"TJC_AWARD_TAP_POINTS_RESPONSE_NOTIFICATION";
        }
        NSNumber* balance = [NSNumber numberWithInt:[userAccountModelObj_ tapPoints]];
        [[NSNotificationCenter defaultCenter] postNotificationName:name object:balance];
    } else {
        [self fetchResponseError:1 errorDescription:nil requestTag:tag];
    }
    [self release];
}

- (void)fetchResponseError:(NSInteger)error errorDescription:(NSString*)description requestTag:(NSInteger)tag
{
    waitingForResponse_ = NO;
    [[NSNotificationCenter defaultCenter] postNotificationName:AccountErrorNotification(tag) object:nil];
}

- (void)releaseUserAccount
{
    [userAccountGetTapPointsObj_ cancel];
    [userAccountSpendTapPointsObj_ cancel];
    [userAccountGetTapPointsObj_ release];
    [userAccountSpendTapPointsObj_ release];
    [userAccountModelObj_ release];
    userAccountGetTapPointsObj_ = nil;
    userAccountSpendTapPointsObj_ = nil;
    userAccountModelObj_ = nil;
    waitingForResponse_ = NO;
}

- (void)dealloc
{
    [self releaseUserAccount];
    [super dealloc];
}
@end

@implementation TapjoyConnect (TJCUserAccountManager)
+ (void)getTapPoints
{
    [[TJCUserAccountManager sharedTJCUserAccountManager] getTapPoints];
}
+ (void)spendTapPoints:(int)points
{
    [[TJCUserAccountManager sharedTJCUserAccountManager] spendTapPoints:points];
}
+ (void)awardTapPoints:(int)points
{
    [[TJCUserAccountManager sharedTJCUserAccountManager] awardTapPoints:points];
}
@end
