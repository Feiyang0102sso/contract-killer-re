// CCore_Cocoa.h - iOS implementation of ICCore (UIKit / CoreFoundation services).
// C++ part in CCore_Cocoa.cpp, UIKit part in CCore_Cocoa_mm.mm.
#ifndef CCORE_COCOA_H
#define CCORE_COCOA_H

#include "CCore.h"
#include "CApplet.h"

class CCore_Cocoa : public CCore {
public:
    CCore_Cocoa();
    virtual ~CCore_Cocoa();

    virtual void SetTickRate(unsigned int milliseconds)
    {
        CApplet::m_pThis->m_tickRate = milliseconds;
    }

    virtual void ExitApp(eExitStatus status);
    virtual unsigned char DeviceCanMultitask() const;
    virtual unsigned char LaunchURL(const wchar_t* url, unsigned char exitApplication);
    virtual unsigned char CanDetectLanguage() const;
    virtual unsigned char DetectLanguage(CStrChar& language) const;
    virtual unsigned char CanDetectLocale() const;
    virtual unsigned char DetectLocale(CStrChar& locale) const;
    virtual unsigned char ApplicationCanSetOrientation() const;
    virtual eDeviceOrientation GetDeviceOrientation() const;
    virtual unsigned char CanSetDeviceOrientation(eDeviceOrientation orientation) const;
    virtual unsigned char SetDeviceOrientation(eDeviceOrientation orientation);
    virtual unsigned char AutoRotationIsActive() const;
    virtual eAutoRotationOrientation GetDisplayOrientationInTermsOfAutoRotation() const;
    virtual eSoftkeyPosition GetSoftkeyPosition(eKeyCode key) const;
    virtual CStrWChar GetDeviceIdentifier() const;
    virtual CStrWChar GetPlatformIdentifier() const;
    virtual CStrWChar GetPlatformVersion() const;
    virtual CStrWChar GetPlatformName() const;

    virtual void SetAccelerometerFrequency(unsigned int frequency)
    {
        CApplet::m_pThis->SetAccelerometerFrequency(frequency);
    }

    virtual unsigned char CanMultipleTouch() const;
    virtual unsigned char IsMultipleTouchEnabled() const;
    virtual void SetMultipleTouchEnabled(unsigned char enabled);
    virtual unsigned char SetBacklightEnabled(unsigned char enabled);

    virtual unsigned char IsGyroscopeAvailable() const
    {
        return CApplet::m_pThis->IsGyroscopeAvailable();
    }

    virtual void SetGyroscopeFrequency(unsigned int frequency)
    {
        CApplet::m_pThis->SetGyroscopeFrequency(frequency);
    }

    virtual unsigned int GetDeviceAvailableMemory();
    virtual void SystemMessageBox(const CStrWChar& title,
                                  const CStrWChar& message) const;
    virtual CStrWChar GetDeviceModel() const;
    virtual unsigned char IsNetworkAvailable() const;
    virtual unsigned char CanDetectTimezone() const;
    virtual CStrWChar GetTimezone() const;
    virtual void UpdateAutoRotationOrientation();

    /** Move the application view into the view controller (auto-rotation switched on). */
    void AttachView() const;
    /** Take the application view back out of the view controller (auto-rotation switched off). */
    void DetachView() const;

    unsigned char m_appliedAutoRotationOrientationEnabled[AutoRotation_Count];   // 0x44 state of the last UpdateAutoRotationOrientation()
};

#endif
