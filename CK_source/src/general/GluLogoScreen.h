#ifndef GLULOGOSCREEN_H
#define GLULOGOSCREEN_H

#include "WindowBase.h"

class CMedia;

/** Sprite and sound for the animated Glu logo shown by the intro. */
class CGluLogoScreen : public Window {
public:
    CGluLogoScreen();
    virtual ~CGluLogoScreen();
    virtual void Init();
    virtual void Shutdown();
    virtual void OnPointerEvent(Event& event);
    virtual void Update();
    virtual void Paint(ICGraphics2d& graphics);

private:
    ICRenderSurface* m_logo;
    CMedia* m_theme;
    int m_elapsed;
    int m_soundEvent;
};

#endif
