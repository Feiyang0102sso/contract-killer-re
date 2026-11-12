#import "TJCUserAccountModel.h"

static NSString* const PointsKey = @"TJC_POINTS_KEY_NAME";
static NSString* const PointsIDKey = @"TJC_POINT_ID_KEY_NAME";
static NSString* const PointsDeltaKey = @"TJC_POINTS_DELTA_KEY_NAME";
static NSString* const CurrencyKey = @"TJC_CURRENCY_KEY_NAME";

@implementation TJCUserAccountModel
@synthesize tapPoints = tapPoints_, pointsID = pointsID_, currencyName = currencyName_;

- (id)init
{
    self = [super init];
    if (self) {
        NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
        tapPoints_ = [defaults integerForKey:PointsKey];
        NSString* pointsID = [defaults stringForKey:PointsIDKey];
        if ([pointsID length] == 0) {
            pointsID = @"0";
        }
        pointsID_ = [pointsID copy];

        NSString* currencyName = [defaults stringForKey:CurrencyKey];
        if ([currencyName length] == 0) {
            currencyName = @"Points";
            [defaults setObject:currencyName forKey:CurrencyKey];
        }
        currencyName_ = [currencyName copy];
        [defaults setInteger:0 forKey:PointsDeltaKey];
    }
    return self;
}

- (id)initWithTBXML:(TBXMLElement*)element
{
    self = [self init];
    if (self) {
        [self updateWithTBXML:element shouldCheckEarnedPoints:NO];
    }
    return self;
}

- (void)updateWithTBXML:(TBXMLElement*)element shouldCheckEarnedPoints:(BOOL)checkEarned
{
    [self updateFromAccountElement:element checkEarned:checkEarned];
}

- (BOOL)updateFromAccountElement:(TBXMLElement*)element checkEarned:(BOOL)checkEarned
{
    NSString* points = [TBXML textForElement:[TBXML childElementNamed:@"TapPoints" parentElement:element]];
    NSScanner* scanner = [NSScanner scannerWithString:points];
    int newPoints;
    if (![scanner scanInt:&newPoints] || ![scanner isAtEnd] || newPoints < 0) {
        return NO;
    }

    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    int oldPoints = tapPoints_;
    tapPoints_ = newPoints;
    [defaults setObject:[NSString stringWithFormat:@"%d", tapPoints_] forKey:PointsKey];

    NSString* pointsID = [TBXML textForElement:[TBXML childElementNamed:@"PointsID" parentElement:element]];
    if ([pointsID length] != 0) {
        self.pointsID = pointsID;
        [defaults setObject:pointsID forKey:PointsIDKey];
    }
    NSString* currency = [TBXML textForElement:[TBXML childElementNamed:@"CurrencyName" parentElement:element]];
    if ([currency length] != 0) {
        self.currencyName = currency;
        [defaults setObject:currency forKey:CurrencyKey];
    }

    // Commit the server balance before notifying observers.
    if (checkEarned && newPoints > oldPoints) {
        NSNumber* delta = [NSNumber numberWithInt:newPoints - oldPoints];
        [defaults setObject:delta forKey:PointsDeltaKey];
        [[NSNotificationCenter defaultCenter] postNotificationName:TJC_TAPPOINTS_EARNED_NOTIFICATION object:delta];
    }
    return YES;
}

- (void)dealloc
{
    [pointsID_ release];
    [currencyName_ release];
    [super dealloc];
}
@end
