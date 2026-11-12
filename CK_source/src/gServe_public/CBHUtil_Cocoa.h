#ifndef CBHUTIL_COCOA_H
#define CBHUTIL_COCOA_H

class CBHUtil_Cocoa {
public:
    static void LaunchGameCenter();
    static bool IsGameCenterAuthenticated();
#ifdef __OBJC__
    static id GetRootViewController();
#else
    static void* GetRootViewController();  // C++ view of the same symbol (used by CFriendsWindow)
#endif
};

#endif
