// CDrawUtil.cpp - rectangle and line helpers for the CK fixed-point renderer.
#include "CDrawUtil.h"
#include "CRectangle.h"

void CDrawUtil::Fill(int x1, int y1, int x2, int y2, unsigned int color, ICGraphics2d::Primitive primitive)
{
    ICGraphics2d* graphics = ICGraphics2d::GetInstance();
    graphics->PushState(ICGraphics2d::State_ColorKey);
    unsigned int alpha = color >> 24;
    // Keep the extracted alpha across the virtual state calls.
    __asm__ __volatile__("" : "+r"(alpha));
    graphics->Disable(ICGraphics2d::State_ColorKey);
    if (alpha > 0xf0) {
        graphics->PushState(ICGraphics2d::State_AlphaTest);
        graphics->Disable(ICGraphics2d::State_AlphaTest);
    }
    graphics->SetColor(color);
    short vertices[] = { (short)x1, (short)y1, (short)x2, (short)y2 };
    graphics->SetVertexPositionCoords(2, 0x3f9726, 4, vertices);
    graphics->Draw(primitive, 0, 2);
    if (alpha > 0xf0) {
        graphics->PopState(ICGraphics2d::State_AlphaTest);
    }
    graphics->PopState(ICGraphics2d::State_ColorKey);
}

void CDrawUtil::DrawFrame(const CRectangle& rectangle, unsigned int color, int thickness, eDrawFrameFlags flags)
{
    short edgeThickness = thickness;
    if ((flags & DRAW_FRAME_SKIP_TOP) == 0) {
        Fill(rectangle.m_x, rectangle.m_y, rectangle.m_x + rectangle.m_width, rectangle.m_y + edgeThickness,
             color, ICGraphics2d::Primitive_Rectangles);
    }
    if ((flags & DRAW_FRAME_SKIP_BOTTOM) == 0) {
        short y = rectangle.m_y + rectangle.m_height - thickness;
        Fill(rectangle.m_x, y, rectangle.m_x + rectangle.m_width, y + edgeThickness, color, ICGraphics2d::Primitive_Rectangles);
    }
    if ((flags & DRAW_FRAME_SKIP_LEFT) == 0) {
        Fill(rectangle.m_x, rectangle.m_y, rectangle.m_x + edgeThickness, rectangle.m_y + rectangle.m_height,
             color, ICGraphics2d::Primitive_Rectangles);
    }
    if ((flags & DRAW_FRAME_SKIP_RIGHT) == 0) {
        short x = rectangle.m_x + rectangle.m_width - thickness;
        Fill(x, rectangle.m_y, x + edgeThickness, rectangle.m_y + rectangle.m_height, color, ICGraphics2d::Primitive_Rectangles);
    }
}
