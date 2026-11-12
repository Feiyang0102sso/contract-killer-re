// CRectangle.h - axis aligned rectangle (x, y, width, height) in 16 bit coordinates.
// (created by b037x, header-only; first emitted instances in graphics/src/CGraphics.cpp)
// (gfx 2026-11-10: CK's CRectangle is a plain class, not the CKZ template; Combine / Clip /
// Intersects live in components/src/CRectangle.cpp, the 1d clip in CGenUtil.)
#ifndef CRECTANGLE_H
#define CRECTANGLE_H



class CRectangle {
public:
    CRectangle() : m_x(0), m_y(0), m_width(0), m_height(0) {}

    CRectangle(short x, short y, short width, short height)
        : m_x(x), m_y(y), m_width(width), m_height(height)
    {
    }

    CRectangle(const CRectangle& other)
        : m_x(other.m_x), m_y(other.m_y), m_width(other.m_width), m_height(other.m_height)
    {
    }

    CRectangle& operator=(const CRectangle& other)
    {
        m_x = other.m_x;
        m_y = other.m_y;
        m_width = other.m_width;
        m_height = other.m_height;
        return *this;
    }

    /** Grow this rectangle to also cover `other` (empty rectangles are ignored / replaced). */
    void Combine(const CRectangle& other);

    /** Clip this rectangle to `bounds` (an empty result gets a zero width / height). */
    void Clip(const CRectangle& bounds);

    /** True when the two rectangles overlap or touch. */
    bool Intersects(const CRectangle& other) const;

    short m_x;          // 0x00
    short m_y;          // 0x02
    short m_width;      // 0x04
    short m_height;     // 0x06
};



#endif
