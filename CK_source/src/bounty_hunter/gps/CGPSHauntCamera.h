#ifndef CGPSHAUNTCAMERA_H
#define CGPSHAUNTCAMERA_H

#include "CGPSItem.h"
#include "xString.h"

class CFont;
class CGPSHauntCamera;
class CGPSHuntingItem;
class CGPSInstantInformant;
class CGPSMissionInformant;
class CGPSSubContract;
class TiXmlNode;

class CGPSCameraListener {
public:
    virtual void OnCameraSelected(CGPSHauntCamera* camera) = 0;
    virtual void NeedSave() = 0;
    virtual void PlaceSubContract(CGPSSubContract* contract, unsigned int radius) = 0;
};

class CGPSHauntCamera : public CGPSItem {
public:
    CGPSHauntCamera(XString& hauntName, TiXmlNode* node);
    virtual ~CGPSHauntCamera();
    virtual void SetBlinking(bool blinking);
    virtual void SetPressed(bool pressed);
    virtual void Paint(ICGraphics2d& graphics, int mapX, int mapY);
    virtual void Update(long elapsed);
    virtual void Save(TiXmlElement* parent);
    virtual void Load(TiXmlElement* parent);
    virtual void OnClick();
    virtual CRectangle GetRect() const;
    virtual bool CheckHit(int x, int y, CGPSItem** item);
    virtual void UpdateState();

    void ParseNode(TiXmlNode* node);
    void ParseInfo(TiXmlNode* node);
    void ParseWeaponGatedInfo(TiXmlNode* node);
    void SetCameraListener(CGPSCameraListener* listener);
    int GetGatedWeaponID() const;
    bool IsAvailable();
    bool IsLockedAvailable();
    bool HasMission();
    bool HasBlueBounty();
    bool HasHuntingItem();
    bool HasTempMission();
    bool HasTargettingMission();
    CGPSHuntingItem* GetGPSHuntingItem();
    void SetMissionOnCamera(int missionId, CGPSMissionInformant* informant = NULL);
    void SetBountyOnCamera(CGPSInstantInformant* informant, bool replace, bool temporary);
    void SetGatedMissionOnCamera(CGPSInstantInformant* informant, int missionId, bool);
    void SetTargettingMissionOnCamera(CGPSInstantInformant* informant, int friendId, bool revenge, bool temporary);
    void OnCameraBountyComplete();
    void UpdateIcon();
    void StartNextSubContract();
    void DisappearSubContract();
    void StopSubContracts();
    void SaveSubContracts(TiXmlElement* element);
    void LoadSubContracts(TiXmlElement* element);

    int m_id;                       // 0x48
    int m_unlockLevel;
    int m_cameraIndex;
    XString m_location;
    const char* m_ambientSound;
    XString m_defaultImage;
    XString m_haunt;
    bool m_locked;
    float m_minWeaponRange;
    bool m_field6c;
    bool m_blueBounty;
    bool m_tempMission;
    bool m_tempTargettingMission;
    bool m_targettingMission;
    int m_gatedMissionId;
    bool m_field78;
    bool m_gatedMission;
    CFont* m_font;
    int m_levelTextWidth;
    XString m_levelText;
    XString m_activeImage;
    XString m_gatedWeaponCategory;
    Vector<int> m_gatedWeapons;
    Vector<int> m_randomWeapons;
    CGPSCameraListener* m_listener; // 0xb0
    CGPSHuntingItem* m_temporaryItem;
    CGPSHuntingItem* m_item;
    float m_horizontalLimit;
    float m_verticalLimit;
    int m_introTimeMs;
    int m_durationMs;
    XString m_hudModel;
    XString m_vehicle;
    int m_additionalMinionCount;
    CGPSSubContract* m_subContract;   // 0xd8
    unsigned int m_spawnedSubContracts;
    int m_state;
};

#endif
