// Platform services required by the Glu game framework (replaces CCore_Cocoa.cpp / CCore_Cocoa_mm.mm,
// CStdUtil_Cocoa.cpp / CStdUtil_Cocoa_mm.mm and CDebug_Cocoa_mm.mm).
#include "Host.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>
#include "CCore_Cocoa.h"
#include "CStrChar.h"
#include "CStrWChar.h"
#include "CDebug_Cocoa.h"
#include "CStdUtil_Cocoa.h"
#include "CHash.h"
#include "bvsprintf.h"

ICCore* ICCore::CreateInstance() { return new CCore_Cocoa; }

CCore_Cocoa::CCore_Cocoa()
{
    m_maxTouches = 2;
    m_heapBudget = 0x1e951a0;
    m_watchdogMillis = 30000;
    m_hasAccelerometer = 0;
    m_hasTouchScreen = 1;
    m_hasTelephony = 0;
    m_canVibrate = 0;
    m_keyboardType = 3;
    m_autoRotationIsActive = 0;
    m_autoRotationAllowed = 0;
}
CCore_Cocoa::~CCore_Cocoa() = default;

void CCore_Cocoa::ExitApp(eExitStatus status)
{
    fprintf(stderr, "Application exit status: %d\n", static_cast<int>(status));
    CApplet::m_pThis->ExitApp();
}
unsigned char CCore_Cocoa::DeviceCanMultitask() const { return 1; }
unsigned char CCore_Cocoa::LaunchURL(const wchar_t*, unsigned char) { return 0; }
unsigned char CCore_Cocoa::CanDetectLanguage() const { return 1; }
unsigned char CCore_Cocoa::DetectLanguage(CStrChar& language) const { language = "en"; return 1; }
unsigned char CCore_Cocoa::CanDetectLocale() const { return 1; }
unsigned char CCore_Cocoa::DetectLocale(CStrChar& locale) const { locale = "en_US"; return 1; }
unsigned char CCore_Cocoa::ApplicationCanSetOrientation() const { return 0; }
ICCore::eDeviceOrientation CCore_Cocoa::GetDeviceOrientation() const { return DeviceOrientation_Portrait; }
unsigned char CCore_Cocoa::CanSetDeviceOrientation(eDeviceOrientation) const { return 0; }
unsigned char CCore_Cocoa::SetDeviceOrientation(eDeviceOrientation) { return 0; }
ICCore::eSoftkeyPosition CCore_Cocoa::GetSoftkeyPosition(eKeyCode key) const
{
    if (key == KeyCode_SoftkeyLeft) {
        return SoftkeyPosition_BottomLeft;
    }
    return SoftkeyPosition_BottomRight;
}
CStrWChar CCore_Cocoa::GetDeviceIdentifier() const { return CStrWChar(L"CK-PC"); }
// The iPad 2 profile: the game picks its 1024 x 768 (XGA) resources from the screen size.
CStrWChar CCore_Cocoa::GetPlatformIdentifier() const { return CStrWChar(L"iPad2,1"); }
CStrWChar CCore_Cocoa::GetPlatformVersion() const { return CStrWChar(L"5.0"); }
CStrWChar CCore_Cocoa::GetPlatformName() const { return CStrWChar(L"Windows"); }
CStrWChar CCore_Cocoa::GetDeviceModel() const { return CStrWChar(L"PC"); }
unsigned char CCore_Cocoa::CanMultipleTouch() const { return 1; }
unsigned char CCore_Cocoa::IsMultipleTouchEnabled() const { return CApplet::m_pThis->m_multipleTouchEnabled; }
void CCore_Cocoa::SetMultipleTouchEnabled(unsigned char enabled) { CApplet::m_pThis->m_multipleTouchEnabled = enabled; }
unsigned char CCore_Cocoa::SetBacklightEnabled(unsigned char) { return 0; }
unsigned int CCore_Cocoa::GetDeviceAvailableMemory()
{
    return static_cast<unsigned int>(SDL_GetSystemRAM()) * 1024 * 1024;
}
void CCore_Cocoa::SystemMessageBox(const CStrWChar& title, const CStrWChar& message) const
{
    fwprintf(stderr, L"%ls: %ls\n", title.GetWCharPtr(), message.GetWCharPtr());
}
unsigned char CCore_Cocoa::IsNetworkAvailable() const { return 0; }
unsigned char CCore_Cocoa::CanDetectTimezone() const { return 0; }
CStrWChar CCore_Cocoa::GetTimezone() const { return CStrWChar(L""); }
// The desktop window never rotates: there is no view controller to move the view into.
void CCore_Cocoa::AttachView() const {}
void CCore_Cocoa::DetachView() const {}

long CStdUtil_Cocoa::GetTimeSeconds() { return static_cast<long>(time(nullptr)); }
int CStdUtil_Cocoa::GetUpTimeMS() { return static_cast<int>(SDL_GetTicks()); }
int CStdUtil_Cocoa::WcsICmp(const wchar_t* left, const wchar_t* right) { return _wcsicmp(left, right); }
wchar_t CStdUtil_Cocoa::ToWUpper(wchar_t character) { return static_cast<wchar_t>(towupper(character)); }
wchar_t CStdUtil_Cocoa::ToWLower(wchar_t character) { return static_cast<wchar_t>(towlower(character)); }

// Glu's own formatter, as on iOS: its wide version reads %s as a wchar_t string, while the
// C library (ISO wide specifiers) reads it as a char string and stops after the first letter.
int CStdUtil_Cocoa::VSPrintF(char* buffer, const char* format, void* arguments)
{
    return bvsprintf(buffer, format, static_cast<va_list>(arguments));
}
int CStdUtil_Cocoa::VSPrintF_S(char* buffer, int size, const char* format, void* arguments)
{
    return bvsprintf_s(buffer, size, format, static_cast<va_list>(arguments));
}
int CStdUtil_Cocoa::VSWPrintF(wchar_t* buffer, const wchar_t* format, void* arguments)
{
    return bwsprintf(buffer, format, static_cast<va_list>(arguments));
}

CStdTime* CStdUtil_Cocoa::LocalTime(unsigned int seconds)
{
    time_t clock = seconds;
    struct tm* local = localtime(&clock);
    void* found = nullptr;
    CApplet::m_pApp->m_singletons->Find(0xad3a63d9, &found);
    CStdTimeManager* manager = static_cast<CStdTimeManager*>(found);
    if (manager == nullptr) {
        manager = new CStdTimeManager();
    }
    if (local == nullptr) {
        return nullptr;
    }
    CStdTime* result = &manager->m_time;
    result->m_second = local->tm_sec;
    result->m_minute = local->tm_min;
    result->m_hour = local->tm_hour;
    result->m_dayOfMonth = local->tm_mday;
    result->m_month = local->tm_mon;
    result->m_year = local->tm_year;
    result->m_dayOfWeek = local->tm_wday;
    result->m_isDst = local->tm_isdst;
    return result;
}

void CDebug_Cocoa::LogMessage(const char* format, void* arguments)
{
    vfprintf(stderr, format, static_cast<va_list>(arguments));
    fputc('\n', stderr);
}
