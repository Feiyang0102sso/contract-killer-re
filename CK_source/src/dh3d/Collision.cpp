// Collision.cpp - composite 2D hit testing and debug drawing.
#include "Collision.h"

#include "CDrawUtil.h"

namespace {
bool ContainsPoint(const CRectangle& rectangle, int x, int y)
{
    return rectangle.m_x <= x && x <= rectangle.m_x + rectangle.m_width
        && rectangle.m_y <= y && y <= rectangle.m_y + rectangle.m_height;
}
}

Collision::Collision() : m_x(0), m_y(0)
{
    clear();
}

Collision::Collision(const CRectangle& rectangle, int dx, int dy) : m_x(0), m_y(0)
{
    clear();
    append(rectangle, dx, dy);
}

Collision::~Collision()
{
}

bool Collision::checkCollision(int x, int y) const
{
    if (ContainsPoint(m_bounds, x, y)) {
        for (int index = 0; index < m_rectangles.size(); ++index) {
            if (ContainsPoint(m_rectangles[index], x, y)) {
                return true;
            }
        }
    }
    return false;
}

bool Collision::checkCollision(const Collision& other) const
{
    if (m_bounds.Intersects(other.m_bounds)) {
        for (int index = 0; index < m_rectangles.size(); ++index) {
            const CRectangle& rectangle = m_rectangles[index];
            if (rectangle.Intersects(other.m_bounds)) {
                for (int otherIndex = 0; otherIndex < other.m_rectangles.size(); ++otherIndex) {
                    if (rectangle.Intersects(other.m_rectangles[otherIndex])) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void Collision::append(const Collision& other, int, int)
{
    // The original applies offsets only to the individual rectangle overload.
    for (int index = 0; index < other.m_rectangles.size(); ++index) {
        append(other.m_rectangles[index], 0, 0);
    }
}

void Collision::append(const CRectangle& rectangle, int dx, int dy)
{
    // the original builds the moved rectangle field by field
    CRectangle translated(rectangle.m_x + dx, rectangle.m_y + dy, rectangle.m_width, rectangle.m_height);
    m_rectangles.addElement(translated);
    m_bounds.Combine(translated);
}

void Collision::clear()
{
    m_rectangles.removeAllElements();
    m_bounds.m_x = 0;
    m_bounds.m_y = 0;
    m_bounds.m_width = 0;
    m_bounds.m_height = 0;
}

void Collision::Draw()
{
    for (int index = 0; index < m_rectangles.size(); ++index) {
        CDrawUtil::DrawFrame(m_rectangles[index], 0xffff00ff, 1, DRAW_FRAME_ALL);
    }
    CDrawUtil::DrawFrame(m_bounds, 0xffffffff, 1, DRAW_FRAME_ALL);
}

void Collision::SetPosition(int x, int y)
{
    short dx = static_cast<short>(x) - static_cast<short>(m_x);
    short dy = static_cast<short>(y) - static_cast<short>(m_y);
    m_x = x;
    m_y = y;
    m_bounds.m_x += dx;
    m_bounds.m_y += dy;
    for (int index = 0; index < m_rectangles.size(); ++index) {
        m_rectangles[index].m_x += dx;
        m_rectangles[index].m_y += dy;
    }
    onSetPosition(x, y);
}

void Collision::onSetPosition(int, int)
{
    // Derived hit areas can observe a move.
}
