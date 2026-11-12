#ifndef CUNITINFOSCREEN_H
#define CUNITINFOSCREEN_H

#include "WindowBase.h"

class CAnimatedProgressBar;
class CSwerveGame;
class CUnit;

class CUnitInfoScreen : public Window {
public:
    CUnitInfoScreen(CSwerveGame* game, CUnit* unit, float distance, float identificationRange,
                    bool tagged, int startingDamage, bool revealTarget);
    virtual ~CUnitInfoScreen() {}
    virtual void Init();
    virtual void Update();
    virtual void OnClosed();
    virtual void OnEasyFinished();
    virtual void Paint(ICGraphics2d& graphics);

    void CreateContent();
    void FlushBeforeCloseTimer();

    CAnimatedProgressBar* m_hpBar;       // 0xa4
    XString m_title;
    XString m_name;
    XString m_distanceText;
    XString m_type;
    bool m_revealTarget;
    float m_distance;
    float m_identificationRange;
    int m_maxHp;
    int m_hp;
    unsigned char m_reservedcc[0x7c];
    CSwerveGame* m_game;                // 0x148
    unsigned int m_openedAt;
    int m_state;
    bool m_tagged;
    bool m_civilian;
    bool m_outOfRange;
    unsigned int m_duration;
    int m_guid;                        // 0x15c
};

#endif
