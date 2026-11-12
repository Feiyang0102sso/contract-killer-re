#ifndef CLOADMISSIONSCREEN_H
#define CLOADMISSIONSCREEN_H

#include "WindowBase.h"

class CDH_LoadingBarWidget;
class CSwerveGame;
class CTip;

/** Runs the mission's loading steps and lets the player continue when ready. */
class CLoadingScreen : public Window {
public:
    typedef void (CSwerveGame::*LoadingStep)();

    explicit CLoadingScreen(CSwerveGame* game);
    virtual ~CLoadingScreen();
    virtual void Init();
    virtual void Update();
    virtual void OnCommand(Event& event) { event.Clear(); }
    virtual void OnPointerPressed(Event& event);
    void AddLoadingStep(LoadingStep step);

private:
    Vector<LoadingStep> m_steps;
    CSwerveGame* m_game;
    CDH_LoadingBarWidget* m_bar;
    CTip* m_tip;
    int m_currentStep;
    bool m_ready;
    XString m_backgroundId;
};

#endif
