#ifndef CGPSINSTANTINFORMANT_H
#define CGPSINSTANTINFORMANT_H

#include "CGPSItem.h"
#include "xString.h"

class CFont;
class CGPSInstantInformant;
class TiXmlNode;

class CGPSInstantInformantListener {
public:
    virtual void OnGPSInstantInformantSelected(CGPSInstantInformant* informant) = 0;
    virtual void OnTargetMissionsVisible() = 0;  // added by gps (CK vtable)
    virtual void OnVehicleMissionsVisible() = 0;  // added by gps (CK vtable)
};

class CGPSInstantInformant : public CGPSItem {
public:
    enum State { Hidden, Locked, Available, Assigned, Searching };
    enum Type { Green, Blue, Orange, Gated, Targeting, Moving };

    explicit CGPSInstantInformant(TiXmlNode* node);
    virtual ~CGPSInstantInformant();
    virtual void SetBlinking(bool enabled);
    virtual void Paint(ICGraphics2d& graphics, int mapX, int mapY);
    virtual void Update(long elapsed);
    virtual void Save(TiXmlElement* parent);
    virtual void Load(TiXmlElement* parent);
    virtual void OnClick();
    virtual void UpdateState();

    void ParseNode(TiXmlNode* node);
    void ParseInfo(TiXmlNode* node);
    void ParseBountuies(TiXmlNode* node);
    void ParseSquad(TiXmlNode* node);
    void UpdateIcon();
    void SetListener(CGPSInstantInformantListener* listener);
    int GetCooldownTimeLeft();
    bool IsWeaponGated() const;
    bool IsBountyAvailable();
    bool IsSearching();
    void OnBountyGiven(int cameraId);
    void OnBountyCompleted();
    void OnBountyCanceled();
    int GetRandomBountyType() const;
    float GetXpMultiplier() const;
    float GetMoneyMultiplier() const;
    bool HasSpecialSquad() const;
    XString GetSquadName() const;
    bool Buy();
    bool BuyFree();
    bool BuyInfoFaster();

    int m_unlockLevel;                       // 0x48
    int m_id;
    unsigned int m_lastBountyCompleted;
    bool m_bought;
    Type m_type;
    CGPSInstantInformantListener* m_listener;
    CFont* m_timerFont;
    CFont* m_nameFont;
    unsigned int m_cooldown;
    XString m_name;
    XString m_icon;
    XString m_comeLaterText;
    XString m_startText;
    XString m_cameraImage;
    int m_cashCost;
    int m_creditsCost;
    int m_resetCost;
    float m_xpMultiplier;
    float m_moneyMultiplier;
    bool m_weaponGated;
    Vector<int> m_bountyTypes;
    Vector<XString> m_squads;
    int m_cameraId;
    State m_state;
};

#endif
