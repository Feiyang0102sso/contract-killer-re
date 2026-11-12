#include "CLoadingGPSMap.h"

#include "App.h"
#include "BaseDialog.h"
#include "CBH_GPSMap.h"
#include "CBH_SurfaceManager.h"
#include "CDH_LoadingBarWidget.h"
#include "CDH_SoundManager.h"
#include "CFontMgr.h"
#include "CTip.h"
#include "CTipManager.h"
#include "DGHelper.h"
#include "ResMap.h"

CLoadingGPSMap::CLoadingGPSMap(CBH_GPSMap* map)
    : Window(false), m_map(map), m_nextTile(NULL), m_loaded(0), m_total(1), m_bar(NULL),
      m_finished(false), m_firstUpdate(true)
{
}

CLoadingGPSMap::~CLoadingGPSMap()
{
}

void CLoadingGPSMap::Init()
{
    InitXMLFile();
    InitGraphicElements();

    App* app = static_cast<App*>(WindowApp::m_instance);
    CFont* font = CFontMgr::GetInstance()->GetFont(GAME_FONT_DEFAULT);
    if (font != NULL && app->m_tipManager != NULL) {
        CTip* tip = app->m_tipManager->GetRandomTip();
        if (tip != NULL) {
            WindowTransparent* panel = new WindowTransparent(false);
            panel->SetAlign(ALIGN_RIGHT | ALIGN_VCENTER);
            panel->SetPercentWidth(50, 0, 0);
            TextWindow* text = new TextWindow(tip->m_description, font);
            text->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
            text->SetPercentWidth(100, 0, 0);
            text->SetPercentHeight(100, 0, 0);
            panel->AddToFront(text);
            AddToFront(panel);
        }
    }
}

void CLoadingGPSMap::InitXMLFile()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    ResMap* resources = app->InitResMap();
    char* xml = resources->loadText(resources->getFilename("GPS_MAP_DESCRIPTION"));
    m_document.Parse(xml);
    delete[] xml;

    const TiXmlElement* maps = m_document.FirstChildElement("maps");
    if (maps == NULL) {
        return;
    }
    const TiXmlElement* map = maps->FirstChildElement("map");
    if (map == NULL) {
        return;
    }
    m_map->SetTileLayout(*map);
    m_nextTile = map->FirstChildElement("tile");
    m_total = m_map->m_columns * m_map->m_rows + 1;
}

void CLoadingGPSMap::InitGraphicElements()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_backgroundId = app->m_helper->GetRandomBackgroundResId();
    ImageWindow* background = new ImageWindow();
    XString::AnsiString id(m_backgroundId.c_str());
    background->SetImageById(id);
    background->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    AddToFront(background);

    m_bar = new CDH_LoadingBarWidget();
    m_bar->m_progress = 0;
    AddToFront(m_bar);
}

void CLoadingGPSMap::Update()
{
    UpdateLoading();
    if (m_bar != NULL) {
        m_bar->m_progress = m_loaded * 100 / m_total;
    }
}

void CLoadingGPSMap::UpdateLoading()
{
    if (m_firstUpdate) {
        m_firstUpdate = false;
        return;
    }
    if (m_finished) {
        SetInterrupt(1);
        return;
    }

    App* app = static_cast<App*>(WindowApp::m_instance);
    if (m_total - m_loaded < 2 && !app->m_soundManager->IsPlaying("IDM_MENU_MUSIC")) {
        app->m_soundManager->PlayMusic("IDM_MENU_MUSIC", true, false);
    }
    if (m_nextTile == NULL) {
        m_finished = true;
        m_loaded++;
        WindowApp::HandleTunnelCommand(0x451967d0, 0, 0, 0);
        return;
    }

    const char* path = m_nextTile->Attribute("path");
    ICRenderSurface* tile = NULL;
    if (path != NULL) {
        const char* id = app->InitResMap()->getResourceID(XString(path));
        if (id != NULL) {
            tile = app->m_surfaceManager->CreateSurface565(id, true);
        }
    }
    m_map->AddMapPatch(tile);
    m_nextTile = m_nextTile->NextSiblingElement("tile");
    m_loaded++;
}

void CLoadingGPSMap::OnCommand(Event&)
{
}

void CLoadingGPSMap::OnPointerPressed(Event& event)
{
    event.Clear();
}

void CLoadingGPSMap::OnPointerDragged(Event& event)
{
    event.Clear();
}

void CLoadingGPSMap::OnPointerReleased(Event& event)
{
    event.Clear();
}

typedef char AssertLoadingGPSMapSize[sizeof(CLoadingGPSMap) == 0x108 ? 1 : -1];
