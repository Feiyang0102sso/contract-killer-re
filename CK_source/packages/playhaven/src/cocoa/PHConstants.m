#import "PHConstants.h"
#import "PlayHavenSDK.h"
#import "Reachability.h"
#include "PHImageData.inc"

NSError* PHCreateError(PHErrorCode code)
{
    NSString* message = @"PlayHaven request failed.";
    switch (code) {
    case PHAPIError:
        message = @"PlayHaven received an error response from the API. Please check your token and secret values and try again.";
        break;
    case PHMissingResponse:
        message = @"Response was successful, but did not contain a response object.";
        break;
    case PHMissingOrientation:
        message = @"The content you requested was not able to be shown because it is missing required orientation data.";
        break;
    case PHTemplateLoadError:
        message = @"The content you requested has been dismissed because PlayHaven was not able to load content data.";
        break;
    case PHOverlayError:
        message = @"PlayHaven was not able to create the content unit overlay";
        break;
    }
    return PHRequestError(code, message);
}

int PHNetworkStatus(void)
{
    Reachability* reachability = [Reachability reachabilityForInternetConnection];
    if ([reachability connectionRequired]) {
        return 0;
    }
    switch ([reachability currentReachabilityStatus]) {
    case ReachableViaWWAN:
        return 1;
    case ReachableViaWiFi:
        return 2;
    default:
        return 0;
    }
}

UIImage* convertByteDataToUIImage(const uint32_t* data)
{
    size_t width = data[0];
    size_t height = data[1];
    NSData* pixels = [NSData dataWithBytes:data + 3 length:data[2]];
    CGDataProviderRef provider = CGDataProviderCreateWithCFData((CFDataRef)pixels);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGImageRef image = CGImageCreate(width, height, 8, 32, width * 4, colorSpace,
                                    kCGImageAlphaPremultipliedLast, provider, NULL, false,
                                    kCGRenderingIntentDefault);
    CGDataProviderRelease(provider);
    CGColorSpaceRelease(colorSpace);
    UIImage* result = [UIImage imageWithCGImage:image];
    CGImageRelease(image);
    return result;
}
