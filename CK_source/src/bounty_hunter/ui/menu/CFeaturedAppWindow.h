// Initial declarations by b079. Layout and setters verified against CTjManager::Update.
#ifndef CFEATUREDAPPWINDOW_H
#define CFEATUREDAPPWINDOW_H

#include "WindowBase.h"
#include "xString.h"
#include "BaseDialog.h"
#include "CArrayInputStream.h"
class WebUtil;
class ScaledURLImageWindow : public ImageWindow {
public:
    virtual ~ScaledURLImageWindow() {}
    virtual void Paint(ICGraphics2d& graphics);
    void DrawScaledImage(ICGraphics2d& graphics,
        ICRenderSurface* surface, int x, int y,
        int align, int transform, float scaleX, float scaleY);
    float m_scale;
};

class CFeaturedAppWindow : public Window {
public:
    CFeaturedAppWindow();
    virtual ~CFeaturedAppWindow();
    virtual void Update();
    virtual void OnCommand(Event& event);
    void SetDescriptionText(const XString& text);
    void SetImageUrl(const XString& url);
    void SetRedirectUrl(XString url);

    // Historical thin header: unsigned char m_unresolved[0xc0 - 0xac];
    ICRenderSurface* CreateSurfaceFromPNGStream(
        const CArrayInputStream& stream);
    TextWindow* m_description;                  // 0xa4
    ScaledURLImageWindow* m_imageWindow;         // 0xa8
    WebUtil* m_web;                             // 0xac, owned for the duration of the download.
    ICRenderSurface* m_downloadedSurface;        // 0xb0
    XString m_redirectUrl;                       // 0xb4
};

typedef char FeaturedAppWindowSizeCheck[sizeof(CFeaturedAppWindow) == 0xb8 ? 1 : -1];
typedef char ScaledURLImageWindowSizeCheck[sizeof(ScaledURLImageWindow) == 0xd0 ? 1 : -1];

#endif
