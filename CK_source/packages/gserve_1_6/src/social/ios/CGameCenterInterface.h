// CGameCenterInterface.h - Game Center implementation of CNGSSocialInterface (sizeof 0x48, no own fields).
// Owner: social/ios/CGameCenterInterface.mm (b028x; started as a b016x PROVISIONAL).
// m_nativeManager is the CGameCenterManager (Objective-C, CGameCenterManager.h) doing the GameKit work.
#ifndef CGAMECENTERINTERFACE_H
#define CGAMECENTERINTERFACE_H

#include "CNGSSocialInterface.h"

class CGameCenterInterface : public CNGSSocialInterface {
public:
    CGameCenterInterface();
    virtual ~CGameCenterInterface();

    virtual void login();
    virtual void finishLogin();
    virtual void logout();
    virtual void finishLogout();
    virtual void invalidate();
    virtual void loadFriends();
    virtual void finishLoadFriends();
    virtual int getConnectionState();
    virtual CStrWChar getPlayerID();
    virtual CStrWChar getPlayerAlias();
    virtual int getFriendsCount();
    virtual CStrWChar getFriendIDAtIndex(int index);
    virtual void reportScore(int score, const CStrWChar& category);
    virtual void finishReportScore();
    virtual void showLeaderboard(void* viewController);
    virtual void showAchievement(void* viewController);
    virtual void showFriendRequestScreen(void* viewController);
    virtual void reportAchievement(int percent, const CStrWChar& identifier);
    virtual void finishReportAchievement();
    virtual unsigned char requestFriendPicWithID(const CStrWChar& id,
                                                 const CStrWChar* type);
    virtual unsigned char getFriendPicWithID(const CStrWChar& id, const void*& data, unsigned int& size);
    virtual void freeFriendPicWithID(const CStrWChar& id);
    virtual void freeAllFriendPics();
    virtual unsigned char LaunchExternalApp();

    /** The Game Center player changed (authentication notification). */
    virtual void handleNotification();
    /** The Game Center friends changed (player data notification). */
    virtual void handleFriendsNotification();
};

#endif
