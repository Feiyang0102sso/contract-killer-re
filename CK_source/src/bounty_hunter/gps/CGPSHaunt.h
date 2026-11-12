#ifndef CGPSHAUNT_H
#define CGPSHAUNT_H

#include "CGPSItem.h"
#include "xString.h"

class CFont;
class CGPSHaunt;
class CGPSHauntCamera;
class TiXmlNode;

class CGPSHauntListener {
public:
    virtual void OnHauntSelected(CGPSHaunt* haunt) = 0;
};

class CGPSHaunt : public CGPSItem {
public:
    explicit CGPSHaunt(TiXmlNode* node);
    virtual ~CGPSHaunt();
    virtual void Paint(ICGraphics2d& graphics, int mapX, int mapY);
    virtual void Update(long elapsed);
    virtual void Save(TiXmlElement* parent);
    virtual void Load(TiXmlElement* parent);
    virtual void OnClick();
    virtual bool CheckHit(int x, int y, CGPSItem** item);
    virtual void UpdateState();

    void ParseNode(TiXmlNode* node);
    void ParseInfo(TiXmlNode* node);
    void SetHauntListener(CGPSHauntListener* listener);
    void LoadCameras(TiXmlElement* parent);
    CGPSHauntCamera* GetCameraById(int id);

    int m_unlockLevel;
    XString m_name;
    XString m_screenName;
    XString m_description;
    CFont* m_font;
    int m_nameWidth;
    XString m_levelText;
    int m_labelWidth;
    XString m_label;
    CGPSHauntListener* m_listener;
    Vector<CGPSHauntCamera*> m_cameras;
    int m_state;
};

#endif
