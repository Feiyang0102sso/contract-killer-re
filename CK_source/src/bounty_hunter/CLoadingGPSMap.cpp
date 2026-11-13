#include "CLoadingGPSMap.h"

#include "App.h"
#include "BaseDialog.h"
#include "CBH_GPSMap.h"
#include "CBH_SurfaceManager.h"
#include "CDH_LoadingBarWidget.h"
#include "CDH_SoundManager.h"
#include "CFontMgr.h"
#include "CResourceManager.h"
#include "CTip.h"
#include "CTipManager.h"
#include "CXmlHelper.h"
#include "DGHelper.h"
#include "ICRenderSurface.h"
#include "ResMap.h"

CLoadingGPSMap::CLoadingGPSMap(CBH_GPSMap* map)
    : Window(false), m_map(map), m_nextTile(NULL), m_loaded(0), m_bar(NULL),
      m_finished(false), m_firstUpdate(true)
{
    if (m_map != NULL) {
        m_total = m_map->m_columns * m_map->m_rows + m_map->GetPrefetchedManagerCount() + 1;
    }
}

CLoadingGPSMap::~CLoadingGPSMap()
{
    CResourceManager::GetInstance()->ReleaseResource("SUR_TITLE");
    CBH_SurfaceManager* surfaces = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
    surfaces->ReleaseSurface(XString::AnsiString(m_backgroundId.c_str()));
}

void CLoadingGPSMap::Init()
{
    InitXMLFile();
    InitGraphicElements();

    CFont* font = CFontMgr::GetInstance()->GetFont(GAME_FONT_DEFAULT);
    if (font != NULL) {
        CTip* tip = static_cast<App*>(WindowApp::m_instance)->m_tipManager->GetRandomTip();
        if (tip != NULL) {
            WindowTransparent* panel = new WindowTransparent(false);
            panel->SetAlign(ALIGN_RIGHT | ALIGN_VCENTER);
            panel->SetPercentWidth(50, 0, 0);
            TextWindow* text = new TextWindow();
            text->SetText(tip->m_description, font);
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
    XString filename = static_cast<App*>(WindowApp::m_instance)->InitResMap()->getFilename("GPS_MAP_DESCRIPTION");
    char* xml = static_cast<App*>(WindowApp::m_instance)->InitResMap()->loadText(filename);
    m_document.Parse(xml);
    delete[] xml;

    const TiXmlNode* maps = m_document.FirstChild("maps");
    const TiXmlNode* map = maps->FirstChild("map");
    m_nextTile = static_cast<const TiXmlElement*>(map->FirstChild("tile"));
}

void CLoadingGPSMap::InitGraphicElements()
{
    // The title is preloaded even though the background is a different resource.
    ICRenderSurface* title;
    static_cast<App*>(WindowApp::m_instance)->m_helper->LoadImageSurface(&title, "SUR_TITLE", false);
    ImageWindow* background = new ImageWindow();
    m_backgroundId = static_cast<App*>(WindowApp::m_instance)->m_helper->GetRandomBackgroundResId();
    background->SetImage(static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface(
        XString::AnsiString(m_backgroundId.c_str()), true));
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

    if ((int)((unsigned int)m_total - m_loaded) < 2
        && !static_cast<App*>(WindowApp::m_instance)->m_soundManager->IsPlaying("IDM_MENU_MUSIC")) {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlayMusic("IDM_MENU_MUSIC", true, false);
    }
    if (m_map == NULL) {
        return;
    }
    if (m_map->LoadPrefetchedManagerResource() == true) {
        ++m_loaded;
        return;
    }
    if (m_nextTile == NULL) {
        m_finished = true;
        m_loaded++;
        WindowApp::HandleTunnelCommand(0x451967d0, 0, 0, 0);
        return;
    }

    XString path = CXmlHelper::GetAttributeValue(m_nextTile, "path");
    const char* id = static_cast<App*>(WindowApp::m_instance)->InitResMap()->getResourceID(path);
    // The map deletes its patches directly when it closes.
    ICRenderSurface* tile = static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->CreateSurface565(id, false);
    ICRenderSurface::Description description;
    tile->GetDescription(description);
    m_map->AddMapPatch(tile);
    m_nextTile = static_cast<const TiXmlElement*>(m_nextTile->NextSibling("tile"));
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

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertLoadingGPSMapSize[sizeof(CLoadingGPSMap) == 0x108 ? 1 : -1];
#endif
