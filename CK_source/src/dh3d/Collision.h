// Collision.h - a 2D hit area built from rectangles.
#ifndef CK_DH3D_COLLISION_H
#define CK_DH3D_COLLISION_H

#include "CRectangle.h"
#include "Vector_ss.h"

class Collision {
public:
    Collision();
    Collision(const CRectangle& rectangle, int dx, int dy);
    virtual ~Collision();

    virtual bool checkCollision(const Collision& other) const;
    virtual bool checkCollision(int x, int y) const;
    virtual void append(const Collision& other, int dx, int dy);
    virtual void append(const CRectangle& rectangle, int dx, int dy);
    virtual void clear();
    virtual void Draw();
    void SetPosition(int x, int y);
    virtual void onSetPosition(int x, int y);

    Vector<CRectangle> m_rectangles; // 0x04
    CRectangle m_bounds;           // 0x14
    int m_x;                       // 0x1c
    int m_y;                       // 0x20
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char CollisionSizeCheck[sizeof(Collision) == 0x24 ? 1 : -1];
#endif

#endif
