#ifndef CBOUNTYRESULTSCREEN_H
#define CBOUNTYRESULTSCREEN_H

#include "BaseDialog.h"

class CStampAnimWindow;
class SG_Instance;
class SG_Presenter;

class CBountyResultScreen : public InsetPanel {
public:
    explicit CBountyResultScreen(bool fadeIn);
    virtual ~CBountyResultScreen();
    virtual void OnEasyFinished();
    virtual void OnStartupEasy();
    virtual void OnCommand(Event& event);
    virtual void Update();

    void Create();
    void AddTextLine(Window* parent, CFont* font, XString text);
    void AddTextLine(Window* parent, CFont* font, const char* text);
    void AddStatisticLine(Window* parent, int column, CFont* font, const char* title, int number, XString prefix);
    void AddButton(Window* parent, int column);

private:
    Window* m_image;
    CStampAnimWindow* m_stamp;
    int m_row;
public:
    bool m_accepted;

private:
    int m_fieldfc;
    int m_field100;
    int m_field104;
    bool m_field108;
    bool m_fadeIn;
    int m_elapsed;
};

class CStampAnimWindow : public Window {
public:
    explicit CStampAnimWindow(bool friendTarget);
    virtual ~CStampAnimWindow();
    virtual void Paint(ICGraphics2d& graphics);
    virtual void Update();

private:
    SG_Instance* m_sprites;
    SG_Presenter* m_stamp;
    bool m_soundPlayed;
};

#endif
