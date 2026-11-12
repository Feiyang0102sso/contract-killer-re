// CPlatformUtil.mm - device information for the gServe message layer.
#include "CPlatformUtil.h"
#import <UIKit/UIKit.h>


CStrWChar CPlatformUtil::getDeviceName()
{
    // Original quirk: despite the name, this is the device model ("iPhone", "iPod touch", ...).
    const char* model = [[[UIDevice currentDevice] model] cStringUsingEncoding:NSASCIIStringEncoding];
    return CStrWChar(model);
}
