#ifndef CADCOLONYWINDOW_H
#define CADCOLONYWINDOW_H

#include "AdColony_Facade.h"
#include "WindowBase.h"

class CCheckBox;

class CAdColonyWindow : public Window, public AdColony_Facade::IVideoEndedCallbackDelegate {
public:
    explicit CAdColonyWindow(bool featuredInstall);
    virtual ~CAdColonyWindow() {}
    // no Shutdown override: the original vtable keeps Window::Shutdown (the video delegate is never cancelled)
    virtual void OnCommand(Event& event);
    virtual void Update();
    virtual void OnVideoEnded(bool completed, const char* zoneId);

private:
    void ShowCreditsEarnedDialog(int credits);
    void ShowErrorDialog();

    int m_credits;
    CCheckBox* m_skipFeatured;
    bool m_soundEnabled;
    bool m_musicEnabled;
    bool m_finished;
    bool m_playing;
};

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AdColonyWindowSizeCheck[sizeof(CAdColonyWindow) == 0xb4 ? 1 : -1];
#endif

#endif
