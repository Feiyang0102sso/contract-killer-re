#ifndef CGPSINFORMANTMISSON_H
#define CGPSINFORMANTMISSON_H

#include "CGPSItem.h"
#include "xString.h"

class CFont;
class CGPSMissionInformant;
class TiXmlNode;

class IGPSMissionInformantListener {
public:
    virtual void OnGpsMissionInformantSelected(CGPSMissionInformant* informant) = 0;
};

class CGPSMissionInformant : public CGPSItem {
public:
    explicit CGPSMissionInformant(TiXmlNode* node);
    virtual ~CGPSMissionInformant();
    virtual void SetBlinking(bool blinking);
    virtual void Paint(ICGraphics2d& graphics, int mapX, int mapY);
    virtual void Update(long elapsed);
    virtual void Save(TiXmlElement* parent);
    virtual void Load(TiXmlElement* parent);
    virtual void OnClick();
    virtual void SetVisible(bool visible);

    void ParseNode(TiXmlNode* node);
    void ParseInfo(TiXmlNode* node);
    void SetListener(IGPSMissionInformantListener* listener);
    bool IsAvailable();

    int m_id;                            // 0x48
    int m_cameraId;
    int m_unlockLevel;
    int m_missionId;
    XString m_name;
    XString m_startText;
    XString m_cameraImage;
    int m_field64;
    CFont* m_font;
    IGPSMissionInformantListener* m_listener;
    int m_hidden;
};

#endif
