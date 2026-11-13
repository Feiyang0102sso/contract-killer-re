// Platform hooks of gServe and the game that lived in Objective-C files: dialogs, device and bundle
// information, directories, local time, debug output. gServe itself runs offline.
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include "../platform_sdl/Host.h"
#include "BundleInterface.h"
#include "CFacebookMessage.h"
#include "CFileUtil_gServe.h"
#include "CNGSFileMgr.h"
#include "CNGS_Platform.h"
#include "CNGSSocialInterface.h"
#include "CPlatformUtil.h"
#include "CUtil.h"
#include "ICDebug_inl.h"
#include "ICFileMgr.h"
#include "IOSInterface.h"
#include "timelib.h"

// CDebug_Cocoa_mm.o held the out-of-line copy of this inline function, which files without
// ICDebug_inl.h call. Taking its address emits that copy here.
extern "C" void (*const ckDebugLogMessage)(const char*, ...) = &ICDebug::LogMessage;

// Game Center and Facebook are not available: no social interface objects are created.
CNGS_Platform::CNGS_Platform() : CSingleton(ClassId)
{
    init();
}
CNGS_Platform::~CNGS_Platform()
{
    destroy();
}
void CNGS_Platform::init()
{
    m_gameCenterInterface = nullptr;
    m_facebookInterface = nullptr;
    m_gameCenterSupported = isGameCenterSupported();
    m_reserved10 = 0;
}
void CNGS_Platform::destroy() {}
void CNGS_Platform::Reset()
{
    destroy();
    init();
}
void CNGS_Platform::showPromptMessageDialog(CStrWChar&, CNGSMessagePrompt*, TCVector<const CStrWChar*>*, unsigned char) {}
void CNGS_Platform::showGenericTextEntryDialog(CStrWChar&, CDialogFunctor* functor, const CVector2d&, wchar_t**)
{
    // As if the player pressed the first button at once.
    if (functor != nullptr) {
        functor->buttonPressed(0);
        delete functor;
    }
}
void CNGS_Platform::showGenericAlertPopup(const CStrWChar& message)
{
    fwprintf(stderr, L"%ls\n", message.GetWCharPtr());
}
unsigned char CNGS_Platform::isGameCenterSupported() { return 0; }
CNGSSocialInterface* CNGS_Platform::getSocialInterface(int type)
{
    if (type == SOCIAL_INTERFACE_FACEBOOK) {
        static CNGSSocialInterface offlineFacebook;
        return &offlineFacebook;
    }
    return nullptr;
}

// A Facebook message only keeps its texts nowhere: it is never sent.
CFacebookMessage::CFacebookMessage() : m_actionLinks(nullptr), m_attachment(nullptr), m_privacy(nullptr) {}
CFacebookMessage::~CFacebookMessage() = default;
unsigned char CFacebookMessage::SetParam(const char*, const wchar_t*) { return 0; }
unsigned char CFacebookMessage::GetParam(const char*, CStrWChar&) { return 0; }
void* CFacebookMessage::GetMessage() { return nullptr; }
CStrWChar CFacebookMessage::GetTargetUserId() { return CStrWChar(L""); }
void CFacebookMessage::SetText(const wchar_t*, const wchar_t*, const wchar_t*) {}
void CFacebookMessage::SetActionLInk(const wchar_t*, const wchar_t*) {}
void CFacebookMessage::SetAttachment(const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*) {}
void CFacebookMessage::SetAttachmentImage(const wchar_t*, const wchar_t*) {}
NSMutableDictionary* CFacebookMessage::PushMedia() { return nullptr; }
void CFacebookMessage::PopMedia() {}

CStrWChar CPlatformUtil::getDeviceName() { return CStrWChar(L"CK PC"); }
void BundleInterface::BundleIdentifierString(CStrWChar& text) { text = L"com.glu.bounty"; }
void BundleInterface::BundleVersionString(CStrWChar& text) { text = L"1.5.2"; }
void CUtil_Cpp::getDeviceScreen(int* width, int* height)
{
    if (width == nullptr || height == nullptr) {
        return;
    }
    *width = ck::WindowWidth;
    *height = ck::WindowHeight;
}

unsigned char CNGSFileMgr::MkDir(const wchar_t* path) { return ICFileMgr::GetInstance()->MkDir(path); }
unsigned char CNGSFileMgr::RmDir(const wchar_t*) { return 0; }
unsigned char CFileUtil_gServe::ExistsDir(const wchar_t* path)
{
    struct _stat64 info;
    if (path == nullptr || _wstat64(path, &info) != 0) {
        return 0;
    }
    return (info.st_mode & _S_IFDIR) != 0;
}

void IOSInterface::showMessage(const char* title, const char* message)
{
    fprintf(stderr, "%s: %s\n", title, message);
}
// The iPad profile (CCore_Cocoa::GetPlatformIdentifier).
bool IOSInterface::isDeviceTablet() { return true; }

void timelib_time2local(timelib_time* result, timelib_sll seconds)
{
    // Seconds east of UTC of the local time zone, as [NSTimeZone localTimeZone] secondsFromGMT.
    const time_t now = static_cast<time_t>(seconds);
    struct tm local = {};
    struct tm utc = {};
    localtime_s(&local, &now);
    gmtime_s(&utc, &now);
    const long offset = static_cast<long>(_mkgmtime(&local) - _mkgmtime(&utc));
    timelib_time2struc(result, seconds + offset);
    result->is_localtime = 1;
    result->have_zone = 1;
}
