// Owner: tools/mqaa/cocoa/CPlatformUtil.mm (b028x; started as a PROVISIONAL)
// CPlatformUtil.h - device information for the gServe message layer.
#ifndef CPLATFORMUTIL_H
#define CPLATFORMUTIL_H

#include "CStrWChar.h"

class CPlatformUtil {
public:
    /** Device model reported by UIDevice. */
    static CStrWChar getDeviceName();
};

#endif
