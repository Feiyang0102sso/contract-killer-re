#import "TapjoyConnect.h"
#import "TJCOffersWebView.h"

@interface TJCOffersViewHandler : NSObject
+ (void)showOffersWithViewController:(UIViewController*)controller;
+ (void)showOffersWithViewController:(UIViewController*)controller withInternalNavBar:(BOOL)showBar;
+ (void)showOffersWithCurrencyID:(NSString*)currencyID withViewController:(UIViewController*)controller
              withInternalNavBar:(BOOL)showBar withCurrencySelector:(BOOL)showSelector;
@end

@implementation TJCOffersViewHandler
+ (void)showOffersWithViewController:(UIViewController*)controller
{
    [self showOffersWithViewController:controller withInternalNavBar:YES];
}

+ (void)showOffersWithViewController:(UIViewController*)controller withInternalNavBar:(BOOL)showBar
{
    [self showOffersWithCurrencyID:nil withViewController:controller withInternalNavBar:showBar withCurrencySelector:NO];
}

+ (void)showOffersWithCurrencyID:(NSString*)currencyID withViewController:(UIViewController*)controller
              withInternalNavBar:(BOOL)showBar withCurrencySelector:(BOOL)showSelector
{
    [self presentOffersForCurrency:currencyID fromController:controller frame:CGRectZero
                           showBar:showBar showSelector:showSelector];
}

+ (void)presentOffersForCurrency:(NSString*)currencyID fromController:(UIViewController*)controller
                           frame:(CGRect)frame showBar:(BOOL)showBar showSelector:(BOOL)showSelector
{
    TJCCallsWrapper* wrapper = [TJCCallsWrapper sharedTJCCallsWrapper];
    if (![[TapjoyConnect sharedTapjoyConnect] isConnected]) {
        [wrapper scheduleCloseNotification];
        return;
    }
    TJCOffersWebView* offers = [TJCOffersWebView sharedTJCOffersWebView];
    offers.currencyID = currencyID;
    offers.isSelectorVisible = showSelector;
    offers.parentVController = controller;
    [offers refreshWithFrame:CGRectZero enableNavBar:showBar];
    [wrapper showContent:offers fromController:controller frame:frame];
    [offers refreshWebView];
}

+ (void)showOffersWithFrame:(CGRect)frame
{
    [self showOffersWithCurrencyID:nil withFrame:frame withCurrencySelector:NO];
}

+ (void)showOffers
{
    [self showOffersWithViewController:nil];
}

+ (void)showOffersWithCurrencyID:(NSString*)currencyID withViewController:(UIViewController*)controller
            withCurrencySelector:(BOOL)showSelector
{
    [self showOffersWithCurrencyID:currencyID withViewController:controller withInternalNavBar:YES withCurrencySelector:showSelector];
}

+ (void)showOffersWithCurrencyID:(NSString*)currencyID withCurrencySelector:(BOOL)showSelector
{
    [self showOffersWithCurrencyID:currencyID withViewController:nil withCurrencySelector:showSelector];
}

+ (void)showOffersWithCurrencyID:(NSString*)currencyID withFrame:(CGRect)frame withCurrencySelector:(BOOL)showSelector
{
    [self presentOffersForCurrency:currencyID fromController:nil frame:frame showBar:YES showSelector:showSelector];
}
@end

@implementation TapjoyConnect (TJCOffersViewHandler)
+ (void)showOffersWithViewController:(UIViewController*)controller
{
    [TJCOffersViewHandler showOffersWithViewController:controller];
}

+ (void)showOffersWithViewController:(UIViewController*)controller withInternalNavBar:(BOOL)showBar
{
    [TJCOffersViewHandler showOffersWithViewController:controller withInternalNavBar:showBar];
}

+ (void)showOffersWithFrame:(CGRect)frame
{
    [TJCOffersViewHandler showOffersWithFrame:frame];
}

+ (void)showOffers
{
    [TJCOffersViewHandler showOffers];
}

+ (void)showOffersWithCurrencyID:(NSString*)currencyID withCurrencySelector:(BOOL)showSelector
{
    [TJCOffersViewHandler showOffersWithCurrencyID:currencyID withCurrencySelector:showSelector];
}

+ (void)showOffersWithCurrencyID:(NSString*)currencyID withFrame:(CGRect)frame withCurrencySelector:(BOOL)showSelector
{
    [TJCOffersViewHandler showOffersWithCurrencyID:currencyID withFrame:frame withCurrencySelector:showSelector];
}

+ (void)showOffersWithCurrencyID:(NSString*)currencyID withViewController:(UIViewController*)controller
            withCurrencySelector:(BOOL)showSelector
{
    [TJCOffersViewHandler showOffersWithCurrencyID:currencyID withViewController:controller withCurrencySelector:showSelector];
}

+ (void)showOffersWithCurrencyID:(NSString*)currencyID withViewController:(UIViewController*)controller
              withInternalNavBar:(BOOL)showBar withCurrencySelector:(BOOL)showSelector
{
    [TJCOffersViewHandler showOffersWithCurrencyID:currencyID withViewController:controller
                               withInternalNavBar:showBar withCurrencySelector:showSelector];
}

+ (void)setOffersNavBarImage:(UIImage*)image
{
    [[TJCOffersWebView sharedTJCOffersWebView] setCustomNavBarImage:image];
}
@end
