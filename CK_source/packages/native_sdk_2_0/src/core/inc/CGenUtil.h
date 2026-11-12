// CGenUtil.h - miscellaneous helpers of the Glu platform core.
#ifndef CGENUTIL_H
#define CGENUTIL_H

                                                                     

/** Grab bag of small static helpers. */
class CGenUtil {
public:
    /**
     * Parse a hexadecimal string ("1F", "ff", no "0x" prefix).
     * Stops at the first character that is not a hex digit; NULL gives 0.
     */
    static unsigned int HexAToI(const char* text);

    /**
     * Clip the 1d region [start, start + length) to [clipStart, clipStart + clipLength); the
     * length becomes 0 when nothing is left or when clipping flipped its sign.
     */
    static void ClipRegionToRegion(short clipStart, short clipLength, short& start, short& length);
};

       

#endif
