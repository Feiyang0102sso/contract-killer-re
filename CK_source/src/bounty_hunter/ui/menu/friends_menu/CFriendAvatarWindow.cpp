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

CBH_SurfaceManager* GetSurfaceManager()
{
    return static_cast<App*>(WindowApp::m_instance)->m_surfaceManager;
}
}

CFriendAvatarWindow::CFriendAvatarWindow(int userId)
    : Window(false)
{
    CStrWChar friendId;
    if (userId > 0) {
        CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
        m_remoteUser = local->m_remoteUserLists[0]->findRemoteUserByID(userId);
        if (m_remoteUser != NULL) {
            friendId = m_remoteUser->GetIDForSocialNetwork(SOCIAL_NETWORK_FACEBOOK);
        }
    }
    Init(friendId);
}

CFriendAvatarWindow::CFriendAvatarWindow(CStrWChar friendId)
    : Window(false)
{
    Init(friendId);
}

CFriendAvatarWindow::~CFriendAvatarWindow()
{
    if (m_defaultImage != NULL) {
        GetSurfaceManager()->ReleaseSurface(m_defaultImage);
    }
}

void CFriendAvatarWindow::Init(CStrWChar friendId)
{
    m_remoteUser = NULL;
    m_loading = false;
    m_defaultImage = NULL;
    m_loaded = false;
    m_showBorder = false;
    m_facebookId = friendId;

    Window* frame = new Window(false);
    frame->SetLayoutType(LAYOUT_SIMPLE);
    frame->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    frame->SetHeightByContent(0, 0);
    frame->SetWidthByContent(0, 0);
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
    if (m_facebookId.GetLength() > 0) {
        CNGSSocialInterface* network = CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK);
        const void* data = NULL;
        network->getFriendPicWithID(m_facebookId, data, m_pictureSize);
        if (m_pictureSize == 0 || data == NULL) {
            CStrWChar pictureType(L"square");
            network->requestFriendPicWithID(m_facebookId, &pictureType);
            m_loading = true;
            return;
        }
        if (*static_cast<const unsigned char*>(data) == 0xff) {
            ICRenderSurface* surface = GetSurfaceManager()->CreateSurface(data, m_pictureSize, JpegMimeType);
            ImageWindow* picture = m_picture;
            int scaleX = App::Scale();
            int scaleY = App::Scale();
            picture->SetScale(scaleX, scaleY);
            m_picture->SetImage(surface);
            m_loading = false;
            m_loaded = true;
            m_spinner->Close();
            if (m_showBorder) {
                Window* picture = m_picture;
                int top = SScreenAdaptingConstant(10, 20)();
                int left = SScreenAdaptingConstant(10, 20)();
                picture->SetOutsetSpacing(top, 0, 0, left);
                m_border->SetFlags(FLAG_VISIBLE | FLAG_ENABLED);
            }
            return;
        }
    }
    m_loading = false;
    m_loaded = false;
    if (m_spinner != NULL) {
        m_spinner->Close();
    }
}

void CFriendAvatarWindow::SetDefaultImage(const char* name)
{
    if (m_defaultImage != NULL) {
        GetSurfaceManager()->ReleaseSurface(m_defaultImage);
    }
    m_defaultImage = name;
    ICRenderSurface* surface = GetSurfaceManager()->CreateSurface(name, true);
    SetDesiredWidth(ImageWidth(surface));
    SetDesiredHeight(ImageHeight(surface));
    if (!m_loaded) {
        m_picture->SetImage(surface);
    }
}

void CFriendAvatarWindow::Update()
{
    if (!m_loading) {
        return;
    }
    CNGSSocialInterface* network = CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_INTERFACE_FACEBOOK);
    const void* data = NULL;
    network->getFriendPicWithID(m_facebookId, data, m_pictureSize);
    if (m_pictureSize != 0 || data != NULL) {
        if (*static_cast<const unsigned char*>(data) != 0xff) {
            m_loading = false;
            m_loaded = false;
            if (m_spinner != NULL) {
                m_spinner->Close();
            }
            return;
        }
        ICRenderSurface* surface = GetSurfaceManager()->CreateSurface(data, m_pictureSize, JpegMimeType);
        ImageWindow* picture = m_picture;
        int scaleX = App::Scale();
        int scaleY = App::Scale();
        picture->SetScale(scaleX, scaleY);
        m_picture->SetImage(surface);
        m_loading = false;
        m_loaded = true;
        m_spinner->Close();
        if (m_showBorder) {
            Window* picture = m_picture;
            int top = SScreenAdaptingConstant(10, 20)();
            int left = SScreenAdaptingConstant(10, 20)();
            picture->SetOutsetSpacing(top, 0, 0, left);
            m_border->SetFlags(FLAG_VISIBLE | FLAG_ENABLED);
        }
    } else {
        const CStrWChar pictureType(L"square");
        network->requestFriendPicWithID(m_facebookId, &pictureType);
    }
}

void CFriendAvatarWindow::ShowBorder()
{
    m_showBorder = true;
    if (!m_loading && m_loaded) {
        Window* picture = m_picture;
        int top = SScreenAdaptingConstant(10, 20)();
        int left = SScreenAdaptingConstant(10, 20)();
        picture->SetOutsetSpacing(top, 0, 0, left);
        m_border->SetFlags(FLAG_VISIBLE | FLAG_ENABLED);
    }
}
