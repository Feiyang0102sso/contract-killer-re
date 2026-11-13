// ButtonWindow.cpp - image, label and command handling for window buttons.
#include "ButtonWindow.h"

#include "App.h"
#include "AppMedia.h"
#include "CAppProperties.h"
#include "CDrawUtil.h"

extern const char JADKEY__SOFTKEY_REVERSE[];

ButtonWindow::ButtonWindow() : Window(false)
{
    m_softkey = false;
    ClearFlags(FLAG_FOCUSABLE);
    m_background = Solid;
    m_font = NULL;
    m_selectedFont = NULL;
    m_secondCommand = 0;
    m_command = 0;
    m_labelX = 0;
    m_labelY = 0;
    m_backgroundOpacity = 1.0f;
    m_pressed = false;
    m_sleeped = false;
    m_selected = false;
    m_lastActivationTime = 0;
    m_reserved114 = 0;
}

void ButtonWindow::SetSleeped(bool sleeped)
{
    m_sleeped = sleeped;
}

void ButtonWindow::SetImage(const ImageRes& image)
{
    m_background = CenteredImage;
    m_images[0] = image;
    m_images[1].Release();
    m_images[2].Release();
    m_selectedImages[0] = image;
    m_selectedImages[1].Release();
    m_selectedImages[2].Release();
}

void ButtonWindow::SetImages(const ImageRes& image, const ImageRes& selected)
{
    m_background = CenteredImage;
    m_images[0] = image;
    m_images[1].Release();
    m_images[2].Release();
    m_selectedImages[0] = selected;
    m_selectedImages[1].Release();
    m_selectedImages[2].Release();
}

void ButtonWindow::SetImages(const ImageRes& left, const ImageRes& tile, const ImageRes& right,
                             const ImageRes& selectedLeft, const ImageRes& selectedTile,
                             const ImageRes& selectedRight)
{
    m_background = TiledImage;
    m_images[0] = left;
    m_images[1] = tile;
    m_images[2] = right;
    m_selectedImages[0] = selectedLeft;
    m_selectedImages[1] = selectedTile;
    m_selectedImages[2] = selectedRight;
}

void ButtonWindow::SetBackgroundOpacity(float opacity)
{
    m_backgroundOpacity = opacity;
}

void ButtonWindow::SetName(const XString& name, CFont* font)
{
    m_name = name;
    m_selectedName = name;
    m_font = font;
    m_selectedFont = font;
}

void ButtonWindow::SetSounds(const MediaRes& pressed, const MediaRes& released)
{
    m_pressSound = pressed;
    m_releaseSound = released;
    m_secondarySound = released;
}

void ButtonWindow::SetCommands(int command, int secondCommand)
{
    m_command = command;
    m_secondCommand = secondCommand;
}

void ButtonWindow::SetCommand(int command)
{
    m_command = command;
    m_secondCommand = 0;
}

void ButtonWindow::Shutdown()
{
    // The original override has no shutdown work.
}

void ButtonWindow::Paint(ICGraphics2d& graphics)
{
    bool highlighted = (m_sleeped && m_selected)
        || (m_pressed && (m_selected || WindowApp::PointerCapture(0) == this));
    if (!highlighted) {
        switch (m_background) {
        case Solid:
            CDrawUtil::Fill(0, 0, m_width, m_height, 0xff3232ff, ICGraphics2d::Primitive_Rectangles);
            break;
        case CenteredImage: {
            BeginBlend(graphics, m_backgroundOpacity, 0.0f, 0.0f, 0.0f, false, false);
            ICRenderSurface* surface = m_images[0].GetSurface();
            DrawImage(graphics, surface, m_width / 2, m_height / 2, ALIGN_HCENTER | ALIGN_VCENTER, 0);
            EndBlend(graphics);
            break;
        }
        case TiledImage: {
            BeginBlend(graphics, m_backgroundOpacity, 0.0f, 0.0f, 0.0f, false, false);
            int leftWidth = ImageWidth(m_images[0].GetSurface());
            int rightWidth = ImageWidth(m_images[2].GetSurface());
            ICRenderSurface* tile = m_images[1].GetSurface();
            DrawTileImageHoriz(graphics, tile, leftWidth, 0, m_width - (leftWidth + rightWidth));
            DrawImage(graphics, m_images[0].GetSurface(), 0, 0, ALIGN_LEFT | ALIGN_TOP, 0);
            ICRenderSurface* right = m_images[2].GetSurface();
            DrawImage(graphics, right, m_width, 0, ALIGN_RIGHT | ALIGN_TOP, 0);
            EndBlend(graphics);
            break;
        }
        }
        App::Inc2DLayer();
        DrawFontText(graphics, m_font, m_name.c_str(),
                     m_labelX + m_width / 2, m_labelY + m_height / 2, ALIGN_HCENTER | ALIGN_VCENTER);
    } else {
        switch (m_background) {
        case Solid:
            FillRect(graphics, 0, 0, m_width, m_height, 0xff323232);
            break;
        case CenteredImage: {
            BeginBlend(graphics, m_backgroundOpacity, 0.0f, 0.0f, 0.0f, false, false);
            const ImageRes* image = m_images;
            if (m_selectedImages[0].m_resource != NULL) {
                image = m_selectedImages;
            }
            ICRenderSurface* surface = image[0].GetSurface();
            DrawImage(graphics, surface, m_width / 2, m_height / 2, ALIGN_HCENTER | ALIGN_VCENTER, 0);
            EndBlend(graphics);
            break;
        }
        case TiledImage: {
            BeginBlend(graphics, m_backgroundOpacity, 0.0f, 0.0f, 0.0f, false, false);
            int leftWidth = ImageWidth(m_selectedImages[0].GetSurface());
            int rightWidth = ImageWidth(m_selectedImages[2].GetSurface());
            ICRenderSurface* tile = m_selectedImages[1].GetSurface();
            DrawTileImageHoriz(graphics, tile, leftWidth, 0, m_width - (leftWidth + rightWidth));
            DrawImage(graphics, m_selectedImages[0].GetSurface(), 0, 0, ALIGN_LEFT | ALIGN_TOP, 0);
            ICRenderSurface* right = m_selectedImages[2].GetSurface();
            DrawImage(graphics, right, m_width, 0, ALIGN_RIGHT | ALIGN_TOP, 0);
            EndBlend(graphics);
            break;
        }
        }
        App::Inc2DLayer();
        DrawFontText(graphics, m_selectedFont, m_selectedName.c_str(),
                     m_labelX + m_width / 2 + 1, m_labelY + m_height / 2 + 1, ALIGN_HCENTER | ALIGN_VCENTER);
    }
}

void ButtonWindow::OnCommand(Event& event)
{
    if (m_softkey && event.m_type != Event::NONE && (m_align & (ALIGN_HCENTER | ALIGN_VCENTER)) == 0) {
        int actions[2];
        if (CAppProperties::GetInstance()->m_properties.GetBool(JADKEY__SOFTKEY_REVERSE)) {
            actions[0] = static_cast<int>(0xd45f52de);
            actions[1] = 0x5f61a84a;
        } else {
            actions[1] = static_cast<int>(0xd45f52de);
            actions[0] = 0x5f61a84a;
        }
        if (event.m_id == actions[(m_align & ALIGN_RIGHT) >> 1]) {
            WindowApp::PutEvent(Event::TUNNEL_COMMAND, m_command, 0, 0, 0);
            event.Clear();
            return;
        }
    }
    if (event.m_type != Event::NONE && m_sleeped) {
        if (event.m_id == m_command) {
            m_selected = true;
        } else if (event.m_id == m_secondCommand) {
            m_selected = false;
        }
    }
}

void ButtonWindow::OnPointerPressed(Event& event)
{
    m_pressed = true;
    event.Clear();
}

void ButtonWindow::OnPointerDragged(Event& event)
{
    m_pressed = event.m_param1 >= 0 && event.m_param1 < m_width
        && event.m_param2 >= 0 && event.m_param2 < m_height;
    event.Clear();
}

void ButtonWindow::OnPointerReleased(Event& event)
{
    if (event.m_param1 >= 0 && event.m_param1 < m_width && event.m_param2 >= 0 && event.m_param2 < m_height) {
        int updateTime = static_cast<App*>(WindowApp::m_instance)->m_updateTime;
        if (!m_sleeped || !m_selected) {
            if (m_lastActivationTime != updateTime) {
                m_lastActivationTime = updateTime;
                App* app = static_cast<App*>(WindowApp::m_instance);
                app->m_media->Play(AppMedia::CHANNEL_MULTIPLE, m_pressSound, 0.0f,
                                   AppMedia::PLAY_ONCE, AppMedia::START_IF_IDLE);
                if (m_command != 0) {
                    WindowApp::PutEvent(Event::TUNNEL_COMMAND, m_command, m_commandParam, 0, 0);
                }
                m_selected = m_sleeped;
                OnPressed();
            }
        } else {
            if (m_lastActivationTime != updateTime) {
                m_lastActivationTime = updateTime;
                App* app = static_cast<App*>(WindowApp::m_instance);
                app->m_media->Play(AppMedia::CHANNEL_MULTIPLE, m_releaseSound, 0.0f,
                                   AppMedia::PLAY_ONCE, AppMedia::START_IF_IDLE);
                if (m_secondCommand != 0) {
                    WindowApp::PutEvent(Event::TUNNEL_COMMAND, m_secondCommand, 0, 0, 0);
                }
                m_selected = false;
                OnUnpressed();
            }
        }
    }
    m_pressed = false;
    event.Clear();
}
