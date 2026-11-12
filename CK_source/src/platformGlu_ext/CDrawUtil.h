// CDrawUtil.h - small 2d drawing helpers on top of the platform ICGraphics2d.
#ifndef CDRAWUTIL_H
#define CDRAWUTIL_H

#include "ICGraphics2d.h"

enum eDrawFrameFlags {
    DRAW_FRAME_ALL = 0,
    DRAW_FRAME_SKIP_TOP = 1,
    DRAW_FRAME_SKIP_BOTTOM = 2,
    DRAW_FRAME_SKIP_LEFT = 4,
    DRAW_FRAME_SKIP_RIGHT = 8
};

class CDrawUtil {
public:
    /**
     * Draw the box (x1, y1)-(x2, y2) in a packed 0xAARRGGBB color.
     * Primitive 1 draws the segment from (x1, y1) to (x2, y2). Primitive 3 fills the rectangle.
     */
    static void Fill(int x1, int y1, int x2, int y2, unsigned int color,
                     ICGraphics2d::Primitive primitive);
    static void DrawFrame(const CRectangle& rectangle, unsigned int color, int thickness, eDrawFrameFlags flags);
};

#endif
