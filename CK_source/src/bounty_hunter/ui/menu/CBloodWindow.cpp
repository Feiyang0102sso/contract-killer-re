// CBloodWindow.cpp - framed menu window: shop frame images, title and content area.
#include "CBloodWindow.h"
#include "App.h"
#include "BaseDialog.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "SScreenAdaptingConstant.h"

static SScreenAdaptingConstant s_minTitleWidth(160, 320);

void CBloodWindow::OnStartupEasy()
{
    EasyParams params(this, (EasyLib::EMethod)0x12);
    params.SetFullTime(0.7f);
    params.SetOpacity(0.0f, 1.0f);
    StartEasy(params, 0x100);
}

CBloodWindow::CBloodWindow()
    : Window(false), m_titleWidth(0)
{
    m_frameTop.Create("SUR_SHOP_FRAME_TOP");
    m_frameTopLeft.Create("SUR_SHOP_FRAME_TOP_LEFT");
    m_frameTopRight.Create("SUR_SHOP_FRAME_TOP_RIGHT");
    m_frameLeft.Create("SUR_SHOP_FRAME_LEFT");
    m_frameDownLeft.Create("SUR_SHOP_FRAME_DOWN_LEFT");
    m_frameDown.Create("SUR_SHOP_FRAME_DOWN");
    m_frameRight.Create("SUR_SHOP_FRAME_RIGHT");
    m_frameDownRight.Create("SUR_SHOP_FRAME_DOWN_RIGHT");
    m_bgDownLeft.Create("SUR_SHOP_BG_DOWN_LEFT");
    m_bgDownRight.Create("SUR_SHOP_BG_DOWN_RIGHT");
    m_bgTopRight.Create("SUR_SHOP_BG_TOP_RIGHT");
    m_frameTopHeight = ImageHeight(m_frameTopLeft.GetSurface());
    m_titleFont = CFontMgr::GetInstance()->GetFont(10);
    m_bgTile.Create("SUR_SHOP_BG_TILE");

    m_background = CreateBg();
    AddToFront(m_background);

    Window* titleArea = new Window(false);
    m_titleText = new TextWindow();
    m_titleText->SetFont(m_titleFont);
    m_titleText->SetAlign(0xc);
    titleArea->AddToFront(m_titleText);
    AddToFront(titleArea, 0, 0);
    titleArea->SetPercentHeight(10, 0, 0);

    m_content = new Window(false);
    m_content->SetLayoutType((ELayoutType)0);
    // the content starts below the top frame
    m_content->SetRect(m_x, m_frameTopHeight, m_width, m_height - m_frameTopHeight, false);
    AddToFront(m_content);
}

Window* CBloodWindow::CreateBg()
{
    Window* background = new Window(false);
    ImageWindow* image = new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_SHOP_FRAME_TOP_LEFT"));
    background->AddToFront(image, 0, 0);
    image = new ImageWindow(ImageWindow::FILL_HORIZONTAL, ImageRes("SUR_SHOP_FRAME_TOP"));
    background->AddToFront(image, 1, 0);
    image = new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_SHOP_FRAME_TOP_RIGHT"));
    background->AddToFront(image, 2, 0);
    image = new ImageWindow(ImageWindow::FILL_VERTICAL, ImageRes("SUR_SHOP_FRAME_LEFT"));
    background->AddToFront(image, 0, 1);
    image = new ImageWindow(ImageWindow::FILL_BOTH, ImageRes("SUR_SHOP_BG_TILE"));
    background->AddToFront(image, 1, 1);
    image = new ImageWindow(ImageWindow::FILL_VERTICAL, ImageRes("SUR_SHOP_FRAME_RIGHT"));
    background->AddToFront(image, 2, 1);
    image = new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_SHOP_FRAME_DOWN_LEFT"));
    background->AddToFront(image, 0, 2);
    image = new ImageWindow(ImageWindow::FILL_HORIZONTAL, ImageRes("SUR_SHOP_FRAME_DOWN"));
    background->AddToFront(image, 1, 2);
    image = new ImageWindow(ImageWindow::FILL_NONE, ImageRes("SUR_SHOP_FRAME_DOWN_RIGHT"));
    background->AddToFront(image, 2, 2);
    return background;
}

void CBloodWindow::SetTitle(const XString& title)
{
    m_title = title.Upper();
    m_titleWidth = m_titleFont->MeasureTextWidth(m_title.c_str(), -1, -1, NULL) + App::Scale() * 20;
    // the title plate is never narrower than the screen adapted minimum
    float minWidth = (float)s_minTitleWidth();
    float width = (float)m_titleWidth;
    if (minWidth > width) {
        width = minWidth;
    }
    m_titleWidth = (int)width;
    m_titleText->SetText(m_title);
}

void CBloodWindow::CreateBgEffects()
{
    Window* effects = new Window(false);
    ImageWindow* image = new ImageWindow(ImageWindow::FILL_NONE, m_bgDownLeft);
    image->SetLayoutType((ELayoutType)1);
    image->SetAlign(0x11);
    effects->AddToFront(image);
    image = new ImageWindow(ImageWindow::FILL_NONE, m_bgDownRight);
    image->SetLayoutType((ELayoutType)1);
    image->SetAlign(0x12);
    effects->AddToFront(image);
    image = new ImageWindow(ImageWindow::FILL_NONE, m_bgTopRight);
    image->SetLayoutType((ELayoutType)1);
    image->SetAlign(10);
    effects->AddToFront(image);
    m_background->AddToFront(effects);
}
