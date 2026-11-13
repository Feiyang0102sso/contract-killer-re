#ifndef CBH_GPSHUD_H
#define CBH_GPSHUD_H

#include "WindowBase.h"

class CBH_ProgressBar;
class CEnergyProgressBar;
class CAnimatedProgressBar;
class CImageButton;

class CBH_GPSHud : public Window {
public:
    explicit CBH_GPSHud(bool animateXP);
    virtual ~CBH_GPSHud();
    virtual void Init();
    virtual void Update();
    virtual void OnCommand(Event& event);
    virtual void OnGetFocus();
    virtual void OnLostFocus();
    virtual void OnStartClosing();
    void UpdateXPBar(int delta);
    void SetLevelOnBar(int level);
    void SetOrigin(int x, int y);

private:
    CEnergyProgressBar* m_energy;
    CAnimatedProgressBar* m_xp;
    CBH_ProgressBar* m_credits;
    CBH_ProgressBar* m_money;
    CImageButton* m_levelBadge;
    short m_startX;
    short m_startY;
    int m_fieldbc;
    bool m_animateXP;
    bool m_updateLevel;
    int m_fieldc4;
    int m_level;
    int m_experience;
};

#endif
