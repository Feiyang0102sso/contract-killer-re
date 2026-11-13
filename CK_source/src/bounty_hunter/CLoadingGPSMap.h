#ifndef CLOADGPSMAP_H
#define CLOADGPSMAP_H

#include "WindowBase.h"
#include "tinyxml.h"

class CBH_GPSMap;
class CDH_LoadingBarWidget;

/** Loads one item manager or one RGB565 map tile per update. */
class CLoadingGPSMap : public Window {
public:
    explicit CLoadingGPSMap(CBH_GPSMap* map);
    virtual ~CLoadingGPSMap();
    virtual void Init();
    virtual void Update();
    virtual void OnCommand(Event& event);
    virtual void OnPointerPressed(Event& event);
    virtual void OnPointerDragged(Event& event);
    virtual void OnPointerReleased(Event& event);

    void InitXMLFile();
    void InitGraphicElements();
    void UpdateLoading();

private:
    TiXmlDocument m_document;        // 0xa4
    XString m_backgroundId;          // 0xec
    CBH_GPSMap* m_map;                // 0xf0
    const TiXmlElement* m_nextTile;   // 0xf4
    int m_loaded;
    int m_total;
    CDH_LoadingBarWidget* m_bar;
    bool m_finished;
    bool m_firstUpdate;
    bool m_titleLoaded;
};

#endif
