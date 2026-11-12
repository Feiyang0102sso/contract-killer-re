// CDH_Weapon.h - CK's weapon configuration, magazine and scene model.
#ifndef CDH_WEAPON_H
#define CDH_WEAPON_H

#include "CShopItem.h"
#include "Vector_ss.h"
#include "tws.h"

class CAmmo;
class CDH_Animation;
class ICRenderSurface;

class CDH_Weapon : public CItem {
public:
    typedef ::Node Node;
    typedef ::World World;
    typedef ::Camera Camera;
    typedef ::Group Group;
    typedef ::Transformable Transformable;
    typedef ::Graphics3D Graphics3D;

    enum EWeaponCategory {
        Category_Handgun = 1,
        Category_Automatic = 4,
        Category_Sniper = 8,
        Category_Shotgun = 0x10,
        Category_Exotic = 0x20
    };

    struct Upgrade {
        int m_value;
        int m_pricePoints;
        int m_priceCredits;
    };

    struct ItemDesc {
        ItemDesc(const XString& name, const Vector<XString>& names, const char* resource,
                 int priceCredits, int pricePoints, bool available);
        ~ItemDesc();

        XString m_name;
        Vector<XString> m_nodeNames;
        Vector<Node> m_nodes;
        const char* m_resourceId;
        ICRenderSurface* m_image;
        int m_priceCredits;
        int m_pricePoints;
        bool m_available;
    };

    struct ItemsGroup {
        ItemsGroup(const XString& type, const XString& name);
        ~ItemsGroup();
        void setActiveID(int id);

        XString m_type;
        XString m_name;
        int m_activeId;
        Vector<ItemDesc*> m_items;
    };

    explicit CDH_Weapon(int id);
    virtual ~CDH_Weapon();
    virtual void init(const TiXmlElement& element);
    virtual ICRenderSurface* getShopImage() const;
    virtual bool isAvailable() const { return m_available; }
    virtual void setAvailable(bool available);
    virtual int getType() { return 0; }
    virtual void loadResources();
    virtual void releaseResources();
    virtual XString GetStatsString() const;
    virtual void Parse(const TiXmlElement& element);
    virtual void ParseTexts(const TiXmlElement& element);
    virtual void ParseStats(const TiXmlElement& element);
    virtual void ParseGraphics(const TiXmlElement& element);
    virtual XString getUIDString() const;
    virtual void ParseCategory(const TiXmlElement& element);
    virtual void ParseSounds(const TiXmlElement& element);
    virtual void ParseParts(const TiXmlElement& element);
    virtual void ParseUpgrades(const TiXmlElement& element, Vector<Upgrade>& upgrades);
    virtual void ParseCompatibleAmmo(const TiXmlElement& element);

    static EWeaponCategory GetWeaponCategoryType(const XString& category);
    static XString GetWeaponCategoryName(EWeaponCategory category);
    static void InitUniversalAmmo();

    int GetId() const { return m_id; }
    void Load();
    void Save(bool queueSave);
    void init();
    void restoreAmmo();
    const CAmmo* GetActiveAmmo() const;
    void SetActiveAmmo(const CAmmo* ammo);
    const CAmmo* GetDefaultAmmo() const;
    bool CanUseAmmo(const CAmmo* ammo);
    void UpdateUniversalAmmo();
    bool isSilent() const;
    bool IsPriceless() const;
    float effectiveRange() const;
    float wanderAmount() const;
    float wanderAmountMax() const;
    int damage() const;
    int getRPM() const;
    void FlushAllUpgrades();
    void SetCyberScopeEquiped(bool equipped);
    ItemsGroup* getGroup(const XString& type);
    void checkForScopeAttachment();

    ICRenderSurface* getGameImage() const;
    ICRenderSurface* getMenuImage() const;
    ICRenderSurface* getBulletOnImage() const;
    void loadWeaponModel(bool forMenu);
    void releaseWeaponModel();
    void paintWeaponModel(Graphics3D& graphics);
    void update(unsigned int elapsed);
    void animateWeaponModel();
    void UpdateAsyncShot(int elapsed);
    void UpdateState(int elapsed);
    void hideWeaponModel();
    void fetchWeaponModel();
    void unfetchWeaponModel();
    void scopeReset();
    int isModelAnimationComplete() const;
    void playFireSound() const;
    void PlayShotAnimation();
    void AfterStateChanged();
    void AfterReloading();
    void ScopeIn();
    void EndSpin();
    bool CheckForReload(bool force);
    void CheckForOutOfAmmo();
    void StopAllAnimation();
    void Reload();
    void ResetModelAnimation();
    bool IsReadyForAutoShot();
    bool IsReadyForShot();
    bool IsReadyForUse();
    bool IsReloading();
    bool DoesShowReloadingMessage();
    bool MakeShot();
    void ResetAutoShot();
    void HandleAutoShot();
    void MakeShotImpl();
    bool StartSpin();
    void PlayScopeAnimation(bool out, bool reset);
    void ScopeOut();
    void ResetInScropeRoutine();
    void PlayReloadAnimation();
    void PlaySpinStartAnimation();
    void PlaySpinEndAnimation();

    EWeaponCategory m_category;             // 0x5c
    int m_modelRefCount;                    // 0x60
    int m_shellReloadTime;                  // 0x64
    int m_reloadTarget;                     // 0x68
    int m_shellsRemaining;                  // 0x6c
    int m_shellsTotal;                      // 0x70
    int m_reloadDuration;                   // 0x74
    int m_projectileType;                   // 0x78
    float m_projectileOffsetX;              // 0x7c
    float m_projectileOffsetY;              // 0x80
    int m_state;                            // 0x84
    int m_action;                           // 0x88
    int m_actionTimeLeft;                   // 0x8c
    bool m_reloadPending;                   // 0x90
    bool m_fetched;
    bool m_cyberScopeEquipped;
    XString m_field94;
    int m_field98;
    XString m_statsText;                    // 0x9c
    bool m_available;                      // 0xa0
    int m_damageUpgrade;                    // 0xa4
    int m_rangeUpgrade;
    int m_wanderUpgrade;
    int m_activeAmmoIndex;                  // 0xb0
    Vector<const CAmmo*> m_compatibleAmmo;   // 0xb4
    const CAmmo* m_universalAmmoChoice;      // 0xc4
    const CAmmo* m_firstNamedAmmo;           // 0xc8
    int m_shootType;                        // 0xcc
    int m_clipSize;
    int m_rateOfFire;
    int m_reloadTime;
    int m_scopeTime;
    int m_spinUpTime;
    int m_spinEndTime;
    int m_autoShotsLeft;                    // 0xe8
    bool m_autoShot;
    bool m_spread;
    int m_wanderTimeX;                      // 0xf0
    int m_wanderTimeY;
    int m_bonusXp;
    float m_zoomFovMin;
    float m_zoomFovMax;
    bool m_tranquillizer;                   // 0x104
    bool m_scopeAttached;
    bool m_zoomWithoutScope;
    bool m_originallySilent;
    bool m_silencerAvailable;
    bool m_silencerActive;
    bool m_autoAim;
    bool m_priceless;
    int m_tranquillizingHPSpeed;
    int m_countInQueue;
    Vector<Upgrade> m_damageLevels;          // 0x114
    Vector<Upgrade> m_rangeLevels;           // 0x124
    Vector<Upgrade> m_wanderLevels;          // 0x134
    int m_ammo;                            // 0x144
    const char* m_modelId;
    const char* m_gameImageId;
    const char* m_menuImageId;
    const char* m_bulletOnImageId;
    const char* m_bulletOffImageId;
    int m_spriteScopeArch;                  // 0x15c
    void* m_field160[4];                    // 0x160 cleared (in reverse) by the constructor body only
    int m_lightSettings[5];                 // 0x170
    World m_world;                          // 0x184
    Transformable m_controller;             // 0x188
    CDH_Animation* m_animation;            // 0x18c
    CDH_Animation* m_secondaryAnimation;   // 0x190
    Camera m_eyeCamera;                    // 0x194
    Camera m_menuCamera;                   // 0x198
    XString m_scopeHudModel;                // 0x19c
    const char* m_shootSound;
    const char* m_noAmmoSound;
    const char* m_reloadSound;
    bool m_reloadInSteps;
    const char* m_reloadStartSound;
    const char* m_reloadProcessSound;
    const char* m_reloadEndSound;
    Vector<ItemsGroup*> m_itemsGroups;       // 0x1bc
    // Animation controller ids, -1 when the model lacks the controller (0x1cc..0x1f8).
    int m_shootAnimId;                  // 0x1cc
    int m_shootLeftAnimId;              // 0x1d0
    int m_shootRightAnimId;             // 0x1d4
    int m_fetchAnimId;                  // 0x1d8
    int m_reloadAnimId;                 // 0x1dc
    int m_reloadStartAnimId;            // 0x1e0
    int m_reloadProcessAnimId;          // 0x1e4
    int m_reloadEndAnimId;              // 0x1e8
    int m_startShootAnimId;             // 0x1ec
    int m_endShootAnimId;               // 0x1f0
    int m_scopeAnimId;                  // 0x1f4
    int m_scopeOutAnimId;               // 0x1f8
    int m_asyncShotTime;                    // 0x1fc
    int m_field200;
    int m_field204;
    bool m_skipShotAnimation;
    bool m_pendingShot;
    bool m_field20a;

    static int g_availabilityChangesCount;
    static Vector<const CAmmo*> m_universalAmmo;
};

#endif
