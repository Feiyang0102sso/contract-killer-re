#ifndef BOUNTY_HUNTER_POINT_H
#define BOUNTY_HUNTER_POINT_H

/** Two integer coordinates, also used for navigation edge endpoints. */
class CPoint {
public:
    CPoint(int x = 0, int y = 0) : m_x(x), m_y(y) {}

    int m_x;
    int m_y;
};

#endif
