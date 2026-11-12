// CRectangle.cpp - rectangle helpers (CK only; the CKZ port had a header-only template).
// (created by gfx 2026-11-10 from the CK disassembly of CRectangle.o)
#include "CRectangle.h"
#include "CGenUtil.h"



void CRectangle::Combine(const CRectangle& other)
{
    short otherWidth = other.m_width;
    short otherHeight = other.m_height;
    if (otherWidth == 0 || otherHeight == 0) {
        return;
    }
    short width = m_width;
    short height = m_height;
    if (width == 0 || height == 0) {
        m_x = other.m_x;
        m_y = other.m_y;
        m_width = other.m_width;
        m_height = other.m_height;
        return;
    }
    short x = m_x;
    short otherX = other.m_x;
    short y = m_y;
    short otherY = other.m_y;
    short left = x;
    if (otherX < x) {
        left = otherX;
    }
    m_x = left;
    short top = y;
    if (otherY < y) {
        top = otherY;
    }
    m_y = top;
    short otherRight = otherX + otherWidth;
    short right = x + width;
    if (otherRight > right) {
        right = otherRight;
    }
    m_width = right - left;
    short otherBottom = otherY + otherHeight;
    short bottom = y + height;
    if (otherBottom > bottom) {
        bottom = otherBottom;
    }
    m_height = bottom - top;
}

void CRectangle::Clip(const CRectangle& bounds)
{
    CGenUtil::ClipRegionToRegion(bounds.m_x, bounds.m_width, m_x, m_width);
    CGenUtil::ClipRegionToRegion(bounds.m_y, bounds.m_height, m_y, m_height);
}

bool CRectangle::Intersects(const CRectangle& other) const
{
    if (m_x >= other.m_x) {
        if (other.m_x + other.m_width < m_x) {
            return false;
        }
    } else if (m_x + m_width < other.m_x) {
        return false;
    }
    if (m_y >= other.m_y) {
        return other.m_y + other.m_height >= m_y;
    }
    return m_y + m_height >= other.m_y;
}
