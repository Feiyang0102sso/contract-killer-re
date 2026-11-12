#ifndef CBH_INFORMANTSHUD_H
#define CBH_INFORMANTSHUD_H

#include "CImageButton.h"

class CCountNotificationWindow;

class HudBarButton : public CImageButton {
public:
    HudBarButton(const char* image, const char* glow);
    virtual ~HudBarButton() {}
    virtual void Update();
    void SetText(XString text, int align, CFont* font, int topSpacing, int percentHeight);

private:
    ImageWindow* m_glow;
};

class CBH_InformantsHud : public Window {
public:
    enum Command {
        Friends = 0x6b3eaa5f,
        Target = 0x1228cc36,
        Coop = 0x0a8cdb04,
        Weapon = 0x100eb2b0,
        Vehicle = static_cast<int>(0xa6a8bb2eu),
        SafeHouse = static_cast<int>(0xd05cab0eu),
        Bank = 0x1487db04,
        Shop = 0x0d8cdb05
    };

    CBH_InformantsHud();
    virtual ~CBH_InformantsHud() {}
    virtual void Init();
    virtual void Update();
    void ShowFriendsButton();
    void ShowTargetButton();
    void ShowCoopButton();
    void ShowWeaponGatedButton();
    void ShowVehicleButton();
    virtual void OnStartClosing();
    virtual void OnCommand(Event& event);
    virtual void OnGetFocus();
    virtual void OnLostFocus();

private:
    HudBarButton* m_friends;       // 0xa4
    HudBarButton* m_target;
    HudBarButton* m_coop;
    HudBarButton* m_weapon;
    HudBarButton* m_bank;
    HudBarButton* m_shop;
    HudBarButton* m_home;
    HudBarButton* m_vehicle;
    CCountNotificationWindow* m_coopNotification;
    CCountNotificationWindow* m_friendsNotification;
};

#endif
