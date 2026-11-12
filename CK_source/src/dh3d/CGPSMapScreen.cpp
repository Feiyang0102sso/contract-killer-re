// GPS map state and its ARMv7 save format.
#include "CGPSMapScreen.h"

#include "App.h"
#include "CAmmoManager.h"
#include "CBH_GPSMap.h"
#include "CBH_Player.h"
#include "CDH_SoundManager.h"
#include "CDialogWindow.h"
#include "CDH_TextScreen.h"
#include "CHelpScreen.h"
#include "CLoadingGPSMap.h"
#include "CTutorialManager.h"
#include "DGHelper.h"

const unsigned int CGPSMapScreen::CDH_GPS_SCREEN_HUNT_DAY_DURATION_MS = 720000;

CGPSMapScreen::CachedData::CachedData()
    : m_huntDayElapsed(0), m_field18(0), m_field19(0), m_field1a(-1),
      m_field1b(false), m_field1c(false), m_field20(0), m_field24(0)
{
}

CGPSMapScreen::CachedData::~CachedData()
{
}

void CGPSMapScreen::CachedData::Serialize(DataOutputStream& stream)
{
    stream.writeInt(m_huntDayElapsed);
    stream.writeByte(m_field18);
    stream.writeByte(m_field19);
    stream.writeByte(m_points.size());
    for (int index = 0; index < m_points.size(); ++index) {
        stream.writeInt(m_points[index].x);
        stream.writeInt(m_points[index].y);
    }
    stream.writeByte(m_field1b);
    stream.writeByte(m_field1c);
}

void CGPSMapScreen::CachedData::DeSerialize(JDataInputStream& stream)
{
    m_huntDayElapsed = stream.readInt();
    m_field18 = stream.readByte();
    m_field19 = stream.readByte();
    m_points.removeAllElements();
    int remaining = stream.readByte();
    for (; remaining > 0; --remaining) {
        Point2D point;
        point.x = stream.readInt();
        point.y = stream.readInt();
        m_points.addElement(point);
    }
    m_field1b = stream.readByte() != 0;
    m_field1c = stream.readByte() != 0;
}

typedef char AssertCachedDataSize[sizeof(CGPSMapScreen::CachedData) == 0x28 ? 1 : -1];

CGPSMapScreen::CGPSMapScreen(CachedData* data)
    : Window(false), m_cachedData(data), m_mapState(-1), m_map(NULL), m_mapAttached(false), m_skipTutorialAutoStart(false),
      m_auxiliaryWindow(NULL), m_resourcesCreated(false)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_actualGpsTime = app->m_helper->m_config.GetIntegerValueByToken("ACTUAL_GPS_TIME_MS", true);
    CreateResources();
    m_offerPollSeconds = 5.0f;
}

CGPSMapScreen::~CGPSMapScreen()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_soundManager->StopAllMusic();
    CBH_Player::GetInstance()->Save();
    app->m_soundManager->StopAllSoundsAndRelease();
    ReleaseResources();
    if (!m_mapAttached) {
        delete m_map;
    }
    if (app->m_helper->m_gpsScreen == this) {
        app->m_helper->initGPSScreen(NULL);
    }
}

void CGPSMapScreen::Init()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_helper->initGPSScreen(this);
    m_map = new CBH_GPSMap();
    AddNewState(6);
}

void CGPSMapScreen::OnPointerEvent(Event& event)
{
    switch (event.m_type) {
    case Event::POINTER_PRESSED:
        m_pointer.TouchDown(event.m_param1, event.m_param2);
        event.Clear();
        break;
    case Event::POINTER_DRAGGED:
        m_pointer.TouchMove(event.m_param1, event.m_param2);
        event.Clear();
        break;
    case Event::POINTER_RELEASED:
        m_pointer.TouchUp();
        break;
    default:
        break;
    }
}

void CGPSMapScreen::CreateResources()
{
    if (!m_resourcesCreated) {
        m_resourcesCreated = true;
    }
}

void CGPSMapScreen::ReleaseResources()
{
    if (m_resourcesCreated) {
        m_resourcesCreated = false;
    }
}

int CGPSMapScreen::GetState()
{
    return m_mapState;
}

void CGPSMapScreen::SkipTutorialAutoStart()
{
    m_skipTutorialAutoStart = true;
}

void CGPSMapScreen::Build()
{
}

void CGPSMapScreen::Update()
{
    Window::Update();
    if (m_currentState == 6) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (!app->m_soundManager->IsPlaying("IDM_MENU_MUSIC")) {
        app->m_soundManager->PlayMusic("IDM_MENU_MUSIC", true, false);
    }

    int elapsed = app->m_updateFrameTime;
    m_pointer.TouchUpdate(elapsed);
    PointerEventController::Position position = {0, 0};
    PointerEventController::Event event(position, 0, 0, PointerEventController::EVENT_NONE);
    while (m_pointer.GetEvent(event)) {
        if (m_firstChild == m_map) {
            m_map->OnPointerEvent(event);
        }
    }
    CBH_Player::GetInstance()->Update();
    if (m_firstChild == m_map && elapsed > 0) {
        m_cachedData->m_huntDayElapsed += elapsed;
    }
}

void CGPSMapScreen::onReceiveTapjoyPoints(int points)
{
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetTitleText("IDS_TJ_DIALOG_TITLE");
    dialog->SetMessageText(XString::Format(Window::ResString("IDS_TJ_DIALOG_MESSAGE").c_str(), points), NULL);
    dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_NONE, CDialogWindow::BUTTON_NONE);
    AddToFront(dialog);
}

void CGPSMapScreen::OnCreateState(Window** window, int state)
{
    *window = NULL;
    switch (state) {
    case 1: {
        CDH_TextScreen* screen = new CDH_TextScreen();
        screen->setText("Informant found");
        screen->setButtons("IDS_NO", "IDS_YES");
        *window = screen;
        break;
    }
    case 3:
    case 4: {
        CDH_TextScreen* screen = new CDH_TextScreen();
        if (state == 3) {
            screen->setText("You got cash!");
        } else {
            screen->setText("You got ammo!");
        }
        screen->setButtons(NULL, "IDS_YES");
        *window = screen;
        break;
    }
    case 6:
        *window = new CLoadingGPSMap(m_map);
        break;
    }
}

void CGPSMapScreen::OnStateInterrupt(int state, Window*)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (state == 6) {
        PopState();
        // CK: the map builds itself in CBH_GPSMap::Init (run by AddToFront).
        if (!CTutorialManager::IsAutoStartMission() && !app->m_soundManager->IsPlaying("IDM_MENU_MUSIC")) {
            app->m_soundManager->PlayMusic("IDM_MENU_MUSIC", true, false);
        }
        AddToFront(m_map);
        m_mapAttached = true;
        return;
    }
    if (state == 4) {
        CAmmo* ammo = app->InitAmmoManager()->GetAmmoByType(XString("pistol"));
        if (ammo != NULL) {
            CBH_Player::GetInstance()->IncAmmo(ammo, 2);
        }
        PopState();
    } else if (state == 3) {
        PopState();
    }
}

void CGPSMapScreen::OnCommand(Event& event)
{
    switch (event.m_id) {
    case 0x121683a7:
        SetInterrupt(2);
        break;
    case 0x2120ecaf:
        AddModal(new CHelpScreen());
        WindowApp::HandleTunnelCommand(0x718a0676, 0, 0, 0);
        break;
    case 0x451967d0:
        break;
    case 0x703ba476:
        if (IsChildExisting(m_auxiliaryWindow)) {
            m_auxiliaryWindow->SetFlags(FLAG_VISIBLE);
        }
        break;
    case 0x718a0676:
        if (IsChildExisting(m_auxiliaryWindow)) {
            m_auxiliaryWindow->ClearFlags(FLAG_VISIBLE);
        }
        break;
    default:
        return;
    }
    event.Clear();
}

typedef char AssertGPSMapScreenSize[sizeof(CGPSMapScreen) == 0x134 ? 1 : -1];
