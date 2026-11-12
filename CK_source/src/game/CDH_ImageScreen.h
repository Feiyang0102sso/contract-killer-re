#ifndef CDH_IMAGESCREEN_H
#define CDH_IMAGESCREEN_H

#include "WindowBase.h"

/** Centered image with a solid background and a timeout. */
class CDH_ImageScreen : public Window {
public:
    CDH_ImageScreen(unsigned int color, const char* image, int duration);
    virtual ~CDH_ImageScreen();
    virtual void OnPointerEvent(Event& event);
    virtual void Update();
    virtual void Paint(ICGraphics2d& graphics);

private:
    unsigned int m_color;
    int m_remainingMs;
};

#endif
