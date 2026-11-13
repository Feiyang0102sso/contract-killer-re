#ifndef CBASESCREEN_H
#define CBASESCREEN_H

#include "BaseDialog.h"

class CBH_MenuButton;
class CBH_ProgressBar;

class CBaseScreen : public Window {
public:
    CBaseScreen(bool redFrame, bool drawBody, bool showBack, bool showOptions, bool drawTitle, bool gpsFrame);
    virtual ~CBaseScreen();
    virtual void Init();
    virtual void Shutdown();
    virtual void Update();
    virtual void Paint(ICGraphics2d& graphics);
    virtual void OnCommand(Event& event);
    virtual void OnStartupEasy();

    int GetTitleBarHeight();
    void SetTitleText(XString title);
    static HorizImageWindow* createLineSplitter(int column, int row);

protected:
    static CBH_MenuButton* CreateMenuButton(const char* image, int command);

    ImageRes m_titleImage;              // 0xa4
    ImageRes m_bodyImage;               // 0xac
    ImageRes m_backImage;               // 0xb4
    Window* m_content;                  // 0xbc
    TextWindow* m_title;                // 0xc0
    bool m_drawBody;                    // 0xc4
    bool m_drawTitle;
    int m_titleHeight;                  // 0xc8
    CBH_ProgressBar* m_cash;
    CBH_ProgressBar* m_credits;
    CBH_MenuButton* m_back;

private:
    class FrameRed : public InsetPanel {
    public:
        FrameRed();
        virtual void Paint(ICGraphics2d& graphics);
        int m_inset;
    };

    class GPSFrame : public InsetPanel {
    public:
        GPSFrame();
        virtual void Paint(ICGraphics2d& graphics);
    };
};

#endif
