#include "CFriendAvatarWindow.h"

#include "App.h"
#include "BaseDialog.h"
#include "CBH_SurfaceManager.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CNGSSocialInterface.h"
#include "CNGS_Platform.h"
#include "CWaitWindow.h"
#include "ICRenderSurface.h"
#include "SScreenAdaptingConstant.h"

namespace {
const unsigned int JpegMimeType = 0x7179dacc;
const SScreenAdaptingConstant s_borderInset(10, 20);

CBH_SurfaceManager* GetSurfaceManager()
{
    return static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
}
}

CFriendAvatarWindow::CFriendAvatarWindow(int userId)
    : Window(false), m_defaultImage(NULL)
{
    CStrWChar friendId;
    if (userId > 0) {
        CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
        CNGSRemoteUser* user = local->m_remoteUserLists[0]->findRemoteUserByID(userId);
        if (user != NULL) {
            friendId = user->GetIDForSocialNetwork(SOCIAL_NETWORK_FACEBOOK);
        }
    }
    Init(friendId);
}

CFriendAvatarWindow::CFriendAvatarWindow(CStrWChar friendId)
    : Window(false), m_defaultImage(NULL)
{
    Init(friendId);
}

CFriendAvatarWindow::~CFriendAvatarWindow()
{
    if (m_defaultImage != NULL) {
        GetSurfaceManager()->ReleaseSurface(m_defaultImage);
    }
    delete m_downloadedPicture;
}

void CFriendAvatarWindow::Init(CStrWChar friendId)
{
    m_pictureSize = 0;
    m_loading = friendId.GetLength() > 0;
    m_loaded = false;
    m_showBorder = false;
    m_downloadedPicture = NULL;
    m_facebookId = friendId;

    Window* frame = new Window(false);
    frame->SetLayoutType(LAYOUT_SIMPLE);
    frame->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    frame->SetWidthByContent(0, 0);
    frame->SetHeightByContent(0, 0);
    AddToFront(frame);
    m_border = new ImageWindow();
    m_border->SetFillType(ImageWindow::FILL_NONE);
    m_border->SetImageById("IDB_BOUNTY_MENU_FRIEND_USER_PIC");
    m_border->ClearFlags(FLAG_VISIBLE | FLAG_ENABLED);
    frame->AddToFront(m_border);
    m_picture = new ImageWindow();
    m_picture->SetLayoutType(LAYOUT_SIMPLE);
    m_picture->SetAlign(ALIGN_LEFT | ALIGN_TOP);
    frame->AddToFront(m_picture);

    m_spinner = new CWaitWindow();
    AddToFront(m_spinner);
    if (m_loading) {
        Update();
    } else {
        StopWaiting();
    }
}

void CFriendAvatarWindow::SetDefaultImage(const char* name)
{
    CBH_SurfaceManager* manager = GetSurfaceManager();
    if (m_defaultImage != NULL) {
        manager->ReleaseSurface(m_defaultImage);
    }
    m_defaultImage = name;
    ICRenderSurface* surface = manager->CreateSurface(name, true);
    SetDesiredWidth(ImageWidth(surface));
    SetDesiredHeight(ImageHeight(surface));
    if (!m_loaded) {
        m_picture->SetImage(surface);
    }
}

void CFriendAvatarWindow::StopWaiting()
{
    m_loading = false;
    if (m_spinner != NULL) {
        m_spinner->Close();
        m_spinner = NULL;
    }
}

void CFriendAvatarWindow::Update()
{
    if (!m_loading) {
        return;
    }
    CNGSSocialInterface* network = CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK);
    if (network == NULL) {
        StopWaiting();
        return;
    }
    const void* data = NULL;
    network->getFriendPicWithID(m_facebookId, data, m_pictureSize);
    if (data == NULL || m_pictureSize == 0) {
        const CStrWChar pictureType(L"square");
        if (!network->requestFriendPicWithID(m_facebookId, &pictureType)) {
            StopWaiting();
        }
        return;
    }
    if (*static_cast<const unsigned char*>(data) == 0xff) {
        m_downloadedPicture = GetSurfaceManager()->CreateSurface(data, m_pictureSize, JpegMimeType);
        if (m_downloadedPicture != NULL) {
            m_picture->SetScale(App::Scale(), App::Scale());
            m_picture->SetImage(m_downloadedPicture);
            m_loaded = true;
        }
    }
    StopWaiting();
    if (m_showBorder) {
        ShowBorder();
    }
}

void CFriendAvatarWindow::ShowBorder()
{
    m_showBorder = true;
    if (!m_loading && m_loaded) {
        int inset = s_borderInset();
        m_picture->SetOutsetSpacing(inset, 0, 0, inset);
        m_border->SetFlags(FLAG_VISIBLE | FLAG_ENABLED);
    }
}
