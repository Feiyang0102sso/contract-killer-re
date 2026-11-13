// CChallengeWindow.h - challenge card and shared deadline text.
#ifndef CCHALLENGEWINDOW_H
#define CCHALLENGEWINDOW_H

#include "BaseDialog.h"
#include "Vector_ss.h"

class CChallenge;
struct SChallengeReward;

class CChallengeWindow : public Window {
public:
    explicit CChallengeWindow(const CChallenge* challenge);
    virtual void Update();
    virtual void OnPointerPressed(Event& event);
    virtual void OnCommand(Event& event);

    void StartHeightAnim(int start, int end, int duration);

private:
    void CreateUI(const CChallenge* challenge);
    static Window* CreateBgWindow();
    Window* CreateDescription(const CChallenge* challenge);
    Window* CreateRewardInfo(const CChallenge* challenge);
    Window* CreateProgressWindow(int progress, int goal);
    Window* CreateRewardInfoWindow(const SChallengeReward* reward, int tier);
    static Window* CreateArrowWindow();
    void OnHeightAnimEnded();
    void HandleShowInviteToChallenge(int challengeId);
    void HandleShareChallengePressed(Vector<int>& friendIds, int challengeId);

    Window* m_rewards;
    int m_fieldA8;                  // unused by the original card
    int m_startHeight;
    int m_endHeight;
    float m_heightPerMs;
    int m_animationTime;
    int m_elapsedMs;
    bool m_animating;
    bool m_reservedC1;
    bool m_animationEnded;
    int m_challengeId;
    int m_inviteCooldownMs;
};

typedef char ChallengeWindowSizeCheck[sizeof(CChallengeWindow) == 0xcc ? 1 : -1];

class CChallengeTimerWindow : public TextWindow {
public:
    CChallengeTimerWindow(unsigned int endTime, CFont* font);
    virtual ~CChallengeTimerWindow() {}
    virtual void Update();

    void SetTimeFormat(XString format);

    bool m_useChallengeTime;        // 0xd0

private:
    unsigned int m_endTime;
    int m_displayedSeconds;
    XString m_format;
};

typedef char ChallengeTimerWindowSizeCheck[sizeof(CChallengeTimerWindow) == 0xe0 ? 1 : -1];

#endif
