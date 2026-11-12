// CFacebookInterface.h - Facebook implementation of CNGSSocialInterface (sizeof 0x48, no own fields):
// m_nativeManager is the CFacebookManager doing the Facebook SDK work.
// Owned by social/ios/CFacebookInterface.mm (b027, written by b027x). Objective-C++ only.
#ifndef CFACEBOOKINTERFACE_H
#define CFACEBOOKINTERFACE_H

#import <Foundation/Foundation.h>
#include "CNGSSocialInterface.h"

class CFacebookInterface : public CNGSSocialInterface {
public:
    CFacebookInterface();
    virtual ~CFacebookInterface();

    /** Converts the app id to UTF-8 and creates the Facebook session object (the secret is unused). */
    virtual void setAppID(CStrWChar appId, CStrWChar appSecret);
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
    virtual void resume();
    virtual void postMessage(CNGSSocialMessage* message);
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
    virtual unsigned char requestFriendPicWithID(const CStrWChar& id,
                                                 const CStrWChar* type);
    virtual unsigned char getFriendPicWithID(const CStrWChar& id, const void*& data, unsigned int& size);
    virtual void freeFriendPicWithID(const CStrWChar& id);
    virtual void freeAllFriendPics();
    virtual unsigned char LaunchExternalApp();

    /** The application was opened with `url` (Facebook login redirect). */
    BOOL handleOpenURL(NSURL* url);
};

#endif
