// AdColony entry points used by CK. Readiness depends on the loaded SDK.
#ifndef ADCOLONY_FACADE_H
#define ADCOLONY_FACADE_H

class AdColony_Facade {
public:
    class IVideoEndedCallbackDelegate {
    public:
        virtual void OnVideoEnded(bool completed, const char* zoneId) = 0;
    };

    static void addZoneId(const char* zoneId);
    static void init(const char* appId);
    static void initFromBundle();
    static void release();
    static bool isReady(const char* zoneId);
    static void showVideo(IVideoEndedCallbackDelegate* delegate, const char* zoneId,
                          bool showPrePopup, bool showPostPopup);
    static int getAwardAmount(const char* zoneId);
    static void cancelVideoCallback(IVideoEndedCallbackDelegate* delegate);
};

#endif
