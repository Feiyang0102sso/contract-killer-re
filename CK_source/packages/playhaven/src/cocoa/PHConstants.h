#pragma once
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#include <stdint.h>

extern const uint32_t badge_image[];
extern const uint32_t badge_2x_image[];
extern const uint32_t close_image[];
extern const uint32_t close_active_image[];
UIImage* convertByteDataToUIImage(const uint32_t* data);

typedef enum {
    PHAPIError = 0,
    PHMissingResponse = 1,
    PHMissingOrientation = 2,
    PHTemplateLoadError = 3,
    PHOverlayError = 4
} PHErrorCode;

NSError* PHCreateError(PHErrorCode code);
int PHNetworkStatus(void);
