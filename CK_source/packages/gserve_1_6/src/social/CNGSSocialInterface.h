// CNGSSocialInterface.h - base of the social network interfaces (Game Center, Facebook).
// Owned by social/CNGSSocialInterface.cpp (b027, written by b027x). The base implements every
// slot as a no-op / empty result; the platform interfaces override what their network supports.
// sizeof 0x48 (CFacebookInterface and CGameCenterInterface add no fields).
#ifndef CNGSSOCIALINTERFACE_H
#define CNGSSOCIALINTERFACE_H

#include "CStrWChar.h"
#include "CHash.h"
#include "TCVector.h"

class CNGSSocialMessage;
class CNGSUser;

/**
 * One friend of the social network, kept sorted by name in CNGSSocialInterface::m_friendsInfos.
 * No user-declared constructor (TCVector creates its elements with new[]).
 */
struct FriendInfo {
    CStrWChar m_name;     // +0x00 display name
    CStrWChar m_id;       // +0x10 network user id
};

class CNGSSocialInterface {
public:
    CNGSSocialInterface();
    virtual ~CNGSSocialInterface();

    virtual void setAppID(CStrWChar appId, CStrWChar appSecret);
    virtual unsigned char isUserAuthenticated();
    virtual void setUserAuthenticated(unsigned char authenticated);
    /** Server attribute name of the user id of network `network` (SocialNetwork_NGS_ID_TOKEN). */
    virtual const char* getAttributeName(int network);
    /** Server attribute name of the alias of network `network` (SocialNetwork_NGS_ALIAS_TOKEN). */
    virtual const char* getUserAliasAttributeName(int network);
    virtual void login();
    virtual void finishLogin();
    virtual void logout();
    virtual void finishLogout();
    virtual void invalidate();
    virtual void loadFriends();
    virtual void finishLoadFriends();
    virtual int getConnectionState();
    virtual int getPostConnectionState();
    virtual CStrWChar getPlayerID();
    virtual CStrWChar getPlayerAlias();
    virtual CStrWChar getPlayerEmail();
    virtual int getFriendsCount();
    virtual CStrWChar getFriendIDAtIndex(int index);
    virtual CStrWChar getFriendNameFromSortedListAtIndex(int index);
    virtual CStrWChar getFriendIDFromSortedListAtIndex(int index);
    virtual CStrWChar getFriendNameFromSortedListWithId(const CStrWChar& id);
    virtual int getLoadFriendState();
    /** The application came back to the foreground. */
    virtual void resume();
    virtual void reportScore(int score, const CStrWChar& category);
    virtual void finishReportScore();
    virtual void showLeaderboard(void* viewController);
    virtual void showAchievement(void* viewController);
    virtual void showFriendRequestScreen(void* viewController);
    virtual void reportAchievement(int percent, const CStrWChar& identifier);
    virtual void finishReportAchievement();
    virtual void postMessage(CNGSSocialMessage* message);
    /** Post to `targetUser` (NULL: own wall). */
    virtual void postMessage(const CStrWChar* text, const CNGSUser* targetUser,
                             const CStrWChar* link,
                             const CStrWChar* name,
                             const CStrWChar* picture);
    virtual void postMessage(const CStrWChar* text,
                             const CStrWChar* targetId,
                             const CStrWChar* link,
                             const CStrWChar* name,
                             const CStrWChar* caption,
                             const CStrWChar* description,
                             const CStrWChar* picture,
                             const CStrWChar* privacy);
    /** The post finished: tell the local user. */
    virtual void finishPostMessage(unsigned char success);
    /** The last post had no target user. */
    virtual unsigned char isMessageToAll();
    virtual unsigned char requestFriendPicWithID(const CStrWChar& id,
                                                 const CStrWChar* type);
    virtual unsigned char getFriendPicWithID(const CStrWChar& id, const void*& data, unsigned int& size);
    virtual void freeFriendPicWithID(const CStrWChar& id);
    virtual void freeAllFriendPics();
    /** A friend picture arrived (or failed): tell the local user. */
    virtual void finishLoadFriendPic(unsigned char success);
    /** Open the network's own app (Game Center / Facebook); 1 when it was opened, 0 in the base. */
    virtual unsigned char LaunchExternalApp();

    /** Associate / validate the gServe user after the network login (`previousId`: id saved before). */
    void handlePostUserLogin(int network, CStrWChar& previousId);
    void resetSaveFriendsData();
    TCVector<FriendInfo>* getSocialNetworkFriendsInfos();

    void* m_nativeManager;                                          // +0x04 CFacebookManager* / CGameCenterManager* (ObjC)
    int m_loadFriendState;                                          // +0x08 0 none, 1 loaded, 2 failed
    CHash m_savedFriendsData;       // +0x0c
    unsigned char m_userAuthenticated;                              // +0x20
    unsigned char m_loginReadPending;                               // +0x21 a read request of the local user is open for login()
    int m_reserved24;                                               // +0x24 not used by the social code
    int m_socialNetworkType;                                        // +0x28 eSocialNetworkType, -1 in the base
    unsigned char m_messageHasTarget;                               // +0x2c the last post went to a user
    TCVector<FriendInfo> m_friendsInfos;   // +0x30 sorted by name
};

#endif
