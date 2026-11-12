#ifndef CGPSITEM_H
#define CGPSITEM_H

#include "CRectangle.h"
#include "Vector_ss.h"

class ICGraphics2d;
class ICRenderSurface;
class TiXmlElement;

class CGPSItem {
public:
    CGPSItem();
    virtual ~CGPSItem();
    virtual void SetBlinking(bool blinking) { m_blinking = blinking; }
    virtual void SetPressed(bool pressed);
    virtual void Paint(ICGraphics2d& graphics, int mapX, int mapY);
    virtual void Update(long elapsed) = 0;
    virtual void Save(TiXmlElement* parent) = 0;
    virtual void Load(TiXmlElement* parent) = 0;
    virtual void OnClick() = 0;
    virtual CRectangle GetRect() const;
    virtual bool CheckHit(int x, int y, CGPSItem** item);
    virtual void SetPos(int x, int y);
    virtual void SetScaledPos(int x, int y);
    virtual void SetSize(int width, int height);
    virtual int GetX() const;
    virtual int GetY() const;
    virtual int GetWidth() const { return m_width; }
    virtual int GetHeight() const { return m_height; }
    virtual void SetImage(const char* name);
    virtual void SetBlinkingImage(const char* name);
    virtual void SetPressImage(const char* name);
    virtual ICRenderSurface* GetImage() { return m_image; }
    virtual float AttractSpeed() const { return m_attractSpeed; }
    virtual void SetAttractSpeed(float speed) { m_attractSpeed = speed; }
    virtual void UpdateState() {}
    virtual void SetVisible(bool visible) { m_visible = visible; }

    bool GetPressed() const;
    void setSubItem(int index, const CRectangle& rect, int x, int y);

    int m_x;
    int m_y;
    int m_width;
    int m_height;
    float m_attractSpeed;
    bool m_blinking;
    bool m_pressed;
    unsigned int m_pressTime;
    bool m_visible;
    Vector<CRectangle> m_subItems;
    ICRenderSurface* m_image;
    ICRenderSurface* m_blinkImage;
    ICRenderSurface* m_pressImage;
    float m_scaleX;
    float m_scaleY;
};

#endif
