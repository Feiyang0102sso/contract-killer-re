#ifndef CGPSSUBCONTRACT_H
#define CGPSSUBCONTRACT_H

#include "CGPSItem.h"
#include "xString.h"

class CGPSHauntCamera;
class AnimatedWindow;

class CGPSSubContract : public CGPSItem {
public:
    explicit CGPSSubContract(CGPSHauntCamera* camera);
    CGPSSubContract(CGPSHauntCamera* camera, unsigned int index);
    virtual ~CGPSSubContract();
    virtual void Update(long elapsed);
    virtual void Paint(ICGraphics2d& graphics, int mapX, int mapY);
    virtual void OnClick();
    virtual void Save(TiXmlElement* element);
    virtual void Load(TiXmlElement* element);
    virtual bool CheckHit(int x, int y, CGPSItem** item);

    void Init();
    void Init(unsigned int activate, unsigned int disappear, float rewardFactor);
    CGPSHauntCamera* GetHauntCamera() const;
    unsigned int GetActivateTime() const;
    unsigned int GetDisappearTime() const;
    void ShowInactiveInfo();
    void Collect();
    void ShowReward(int money, int xp) const;
    void StartFly();
    static AnimatedWindow* MakeRewardWindow(const XString& text);

    int m_screenX;                      // 0x48
    int m_screenY;
    CGPSHauntCamera* m_camera;           // 0x50
    unsigned int m_activateTime;
    unsigned int m_disappearTime;
    float m_flyTime;
    float m_flyX;
    float m_flyY;
    float m_flyVelocityX;
    float m_flyVelocityY;
    float m_rewardFactor;
    unsigned int m_index;
    bool m_flying;
};

#endif
