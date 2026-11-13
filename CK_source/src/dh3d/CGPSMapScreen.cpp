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

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertCachedDataSize[sizeof(CGPSMapScreen::CachedData) == 0x28 ? 1 : -1];
#endif

CGPSMapScreen::CGPSMapScreen(CachedData*)
    : Window(false),
      m_actualGpsTime(static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken(
          "ACTUAL_GPS_TIME_MS", true))
{
    m_field104 = 0;
    m_field108 = 0;
    m_field10c = false;
    m_field124 = 0;
    m_resourcesCreated = false;
    CreateResources();
    m_offerPollSeconds = 5.0f;
}

CGPSMapScreen::~CGPSMapScreen()
{
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->StopAllMusic();
    CBH_Player::GetInstance()->Save();
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->StopAllSoundsAndRelease();
    ReleaseResources();
}

void CGPSMapScreen::Init()
{
    CBH_GPSMap* map = new CBH_GPSMap();
    m_mapState = -1;
    m_map = map;
    if (!static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsAutoStartMission()) {
        AddNewState(6);
        return;
    }
    m_map->ClearFlags(FLAG_VISIBLE);
    AddToFront(m_map);
}

void CGPSMapScreen::OnPointerEvent(Event& event)
{
    switch (event.m_type) {
    case Event::POINTER_DRAGGED:
        m_pointer.TouchMove(event.m_param1, event.m_param2);
        event.Clear();
        break;
    case Event::POINTER_PRESSED:
        m_pointer.TouchDown(event.m_param1, event.m_param2);
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

void CGPSMapScreen::Build()
{
}

void CGPSMapScreen::Update()
{
    if (m_currentState == 6) {
        return;
    }
    if (!static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsAutoStartMission() &&
        !static_cast<App*>(WindowApp::m_instance)->m_soundManager->IsPlaying("IDM_MENU_MUSIC")) {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlayMusic("IDM_MENU_MUSIC", true, false);
    }

    m_pointer.TouchUpdate(WindowApp::m_instance->m_updateFrameTime);
    PointerEventController::Position position = {0, 0};
    PointerEventController::Event event(position, 0, 0, PointerEventController::EVENT_NONE);
    while (m_pointer.GetEvent(event)) {
        if (m_firstChild == m_map) {
            m_map->OnPointerEvent(event);
        }
    }
    CBH_Player::GetInstance()->Update();
    m_offerPollSeconds -= WindowApp::m_instance->m_frameTimeSeconds;
    if (m_offerPollSeconds <= 0.0f) {
        m_offerPollSeconds = 5.0f;
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
    Window* created = NULL;
    switch (state) {
    case 1: {
        CDH_TextScreen* screen = new CDH_TextScreen();
        screen->setText("Informant found");
        screen->setButtons("IDS_NO", "IDS_YES");
        created = screen;
        break;
    }
    case 3: {
        CDH_TextScreen* screen = new CDH_TextScreen();
        screen->setText("You got cash!");
        screen->setButtons(NULL, "IDS_YES");
        created = screen;
        break;
    }
    case 4: {
        CDH_TextScreen* screen = new CDH_TextScreen();
        screen->setText("You got ammo!");
        screen->setButtons(NULL, "IDS_YES");
        created = screen;
        break;
    }
    case 5: {
        CDH_TextScreen* screen = new CDH_TextScreen();
        screen->setText("Informant has provide you Bounty info");
        screen->setButtons(NULL, "IDS_YES");
        // The original falls through and returns the loading screen.
    }
    case 6:
        created = new CLoadingGPSMap(m_map);
        break;
    }
    *window = created;
}

void CGPSMapScreen::OnStateInterrupt(int state, Window*)
{
    if (state != 3) {
        if (state != 6) {
            if (state != 4) {
                return;
            }
            CAmmoManager* manager = static_cast<App*>(WindowApp::m_instance)->InitAmmoManager();
            CAmmo* ammo = manager->GetAmmoByType(XString("pistol"));
            CBH_Player::GetInstance()->IncAmmo(ammo, 2);
        } else {
            PopState();
            // CK: the map builds itself in CBH_GPSMap::Init (run by AddToFront).
            if (!static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsAutoStartMission() &&
                !static_cast<App*>(WindowApp::m_instance)->m_soundManager->IsPlaying("IDM_MENU_MUSIC")) {
                static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlayMusic("IDM_MENU_MUSIC", true, false);
            }
            AddToFront(m_map);
            return;
        }
    }
    PopState();
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
        return;
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

#ifndef CK_PC // PORT: layout check of the ARMv7 build only
typedef char AssertGPSMapScreenSize[sizeof(CGPSMapScreen) == 0x134 ? 1 : -1];
#endif
