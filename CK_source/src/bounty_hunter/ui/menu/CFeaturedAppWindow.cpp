#include "CFeaturedAppWindow.h"

#include "App.h"
#include "CApplet.h"
#include "CFontMgr.h"
#include "CImageButton.h"
#include "ICCore.h"
#include "SScreenAdaptingConstant.h"
#include "WebUtil.h"

namespace {
const int SkipOffer = 0x292395;
const int OpenOffer = 0x2358586;
const int DismissOffer = -0x68dfb87c;
const unsigned int PngMimeType = 0xb7178678;
const SScreenAdaptingConstant LeftInset(38, 76);
const SScreenAdaptingConstant LogoTop(8, 16);
const SScreenAdaptingConstant ButtonInset(65, 130);
const SScreenAdaptingConstant ButtonBottom(40, 80, 80, -1);
}

CFeaturedAppWindow::CFeaturedAppWindow()
    : Window(false), m_description(NULL), m_imageWindow(NULL), m_web(NULL), m_downloadedSurface(NULL)
{
    SetOutsetSpacing(LeftInset(), 0, 0, 0);
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
    ImageWindow* background = new ImageWindow();
    background->SetFillType(ImageWindow::FILL_NONE);
    background->SetImageById("IDB_DIALOG_WINDOW_BG");
    AddToFront(background);

    WindowTransparent* content = new WindowTransparent(false);
    content->SetLayoutType(LAYOUT_TABLE);
    ImageWindow* logo = new ImageWindow();
    logo->SetFillType(ImageWindow::FILL_NONE);
    logo->SetImageById("IDB_CK_LOGO");
    logo->SetCellPos(0, 0, 2, 1);
    logo->SetOutsetSpacing(LogoTop(), 0, 0, 0);
    logo->SetAlign(ALIGN_HCENTER | ALIGN_TOP);
    logo->SetFlags(FLAG_CLIP_SCREEN);
    logo->SetPercentHeight(30, 0, 0);
    content->AddToFront(logo);

    WindowTransparent* picture = new WindowTransparent(false);
    picture->SetPercentWidth(25, 0, 0);
    picture->SetHeightByContent(0, 0);
    m_imageWindow = new ScaledURLImageWindow();
    m_imageWindow->m_scale = 1.0f;
    m_imageWindow->SetFillType(ImageWindow::FILL_NONE);
    m_imageWindow->SetImageById("IDB_USER_PIC");
    m_imageWindow->SetAlign(ALIGN_RIGHT | ALIGN_VCENTER);
    picture->AddToFront(m_imageWindow);
    content->AddToFront(picture, 0, 1);

    WindowTransparent* text = new WindowTransparent(false);
    CFont* font = CFontMgr::GetInstance()->GetFont(15);
    m_description = new TextWindow(XString(), font);
    m_description->SetPercentWidth(80, 0, 0);
    m_description->SetPercentHeight(100, 0, 0);
    m_description->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
    text->SetPercentHeight(35, 0, 0);
    text->SetPercentWidth(60, 0, 0);
    text->AddToFront(m_description);
    content->AddToFront(text, 1, 1);

    Window* buttons = new Window(false);
    const char* captions[] = { "SKIP", "GET IT!" };
    const int commands[] = { SkipOffer, OpenOffer };
    for (int index = 0; index < 2; ++index) {
        WindowTransparent* panel = new WindowTransparent(false);
        CImageButton* button = new CImageButton();
        button->AddImage("IDB_SHOP_CARD_BUTTON", 0, 0);
        button->SetText(XString(captions[index]), ALIGN_HCENTER | ALIGN_TOP, NULL, 0, 0);
        button->m_command = commands[index];
        button->SetFlags(FLAG_CLIP_SCREEN);
        panel->SetPercentWidth(50, 0, 0);
        if (index == 0) {
            button->SetAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
            panel->SetOutsetSpacing(0, 0, ButtonInset(), ButtonBottom());
        } else {
            button->SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
            panel->SetOutsetSpacing(0, ButtonBottom(), ButtonInset(), 0);
        }
        panel->AddToFront(button);
        buttons->AddToFront(panel, index, 0);
    }
    buttons->SetCellPos(0, 2, 2, 1);
    content->AddToFront(buttons);
    AddToFront(content);
}

CFeaturedAppWindow::~CFeaturedAppWindow()
{
    delete m_web;
    delete m_downloadedSurface;
}

void CFeaturedAppWindow::SetDescriptionText(const XString& text)
{
    m_description->SetText(text, CFontMgr::GetInstance()->GetFont(15));
}

void CFeaturedAppWindow::SetImageUrl(const XString& url)
{
    delete m_web;
    m_web = NULL;
    if (url.Length() != 0) {
        m_web = new WebUtil();
        m_web->httpGet(url);
    }
}

void CFeaturedAppWindow::Update()
{
    if (m_web == NULL) {
        return;
    }
    m_web->update();
    if (m_web->m_state == WebUtil::STATE_BUSY) {
        return;
    }
    if (m_web->m_state == WebUtil::STATE_IDLE && m_web->m_response != NULL && m_web->m_responseLength > 0) {
        CArrayInputStream stream;
        stream.Open(m_web->m_response, m_web->m_responseLength);
        ICRenderSurface* surface = CreateSurfaceFromPNGStream(stream);
        stream.Close();
        if (surface != NULL) {
            delete m_downloadedSurface;
            m_downloadedSurface = surface;
            m_imageWindow->m_scale = 1.0f;
            if (App::IsHD()) {
                m_imageWindow->m_scale = 2.0f;
            }
            m_imageWindow->SetImage(surface);
        }
    }
    delete m_web;
    m_web = NULL;
}

ICRenderSurface* CFeaturedAppWindow::CreateSurfaceFromPNGStream(const CArrayInputStream& stream)
{
    ICGraphics::Abstraction abstraction = ICGraphics2d::GetInstance()->GetActiveAbstractionLayer();
    ICRenderSurface* surface = ICRenderSurface::CreateInstance(abstraction, ICRenderSurface::Type_Default,
        ICRenderSurface::Targetability_0);
    if (surface == NULL) {
        return NULL;
    }
    ICRenderSurface::InitParam params[] = { {6, 0}, {15, PngMimeType}, {14, (unsigned int)&stream}, {0, 0} };
    if (!surface->Initialize(params)) {
        delete surface;
        return NULL;
    }
    return surface;
}

void CFeaturedAppWindow::OnCommand(Event& event)
{
    if (event.m_id == SkipOffer || event.m_id == DismissOffer) {
        Close();
    } else if (event.m_id == OpenOffer) {
        Close();
        if (m_redirectUrl.Length() != 0) {
            CApplet::m_pCore->LaunchURL(m_redirectUrl.c_str(), 0);
        }
    } else {
        return;
    }
    event.Clear();
}

void CFeaturedAppWindow::SetRedirectUrl(XString url)
{
    m_redirectUrl = url;
}

void ScaledURLImageWindow::Paint(ICGraphics2d& graphics)
{
    int align = m_align;
    int x = 0;
    int y = 0;
    if ((align & ALIGN_LEFT) == 0) {
        if ((align & ALIGN_RIGHT) != 0) {
            x = m_width;
        } else {
            align |= ALIGN_HCENTER;
            x = m_width / 2;
        }
    }
    if ((align & ALIGN_TOP) == 0) {
        if ((align & ALIGN_BOTTOM) != 0) {
            y = m_height;
        } else {
            align |= ALIGN_VCENTER;
            y = m_height / 2;
        }
    }
    DrawScaledImage(graphics, m_surface, x, y, align, 0, m_scale, m_scale);
}

void ScaledURLImageWindow::DrawScaledImage(ICGraphics2d& graphics, ICRenderSurface* surface,
    int x, int y, int align, int transform, float scaleX, float scaleY)
{
    if (surface == NULL) {
        return;
    }
    if ((align & ALIGN_NONE) == 0) {
        unsigned int sourceWidth;
        unsigned int sourceHeight;
        surface->GetWidthAndHeight(sourceWidth, sourceHeight);
        int width = (int)(sourceWidth * scaleX);
        int height = (int)(sourceHeight * scaleY);
        if ((align & ALIGN_HCENTER) != 0) {
            x -= width / 2;
        } else if ((align & (ALIGN_LEFT | ALIGN_RIGHT)) == ALIGN_RIGHT) {
            x -= width;
        }
        if ((align & ALIGN_VCENTER) != 0) {
            y -= height / 2;
        } else if ((align & (ALIGN_TOP | ALIGN_BOTTOM)) == ALIGN_BOTTOM) {
            y -= height;
        }
    }
    graphics.PushTransform();
    graphics.Translate(x * 65536, y * 65536);
    graphics.Scale((int)(scaleX * 65536), (int)(scaleY * 65536));
    DrawImage(graphics, surface, 0, 0, ALIGN_NONE, transform);
    graphics.PopTransform();
}
