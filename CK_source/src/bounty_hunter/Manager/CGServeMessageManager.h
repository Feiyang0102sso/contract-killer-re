// CGServeMessageManager.h - queue of outgoing posts waiting for a social network.
#ifndef CGSERVEMESSAGEMANAGER_H
#define CGSERVEMESSAGEMANAGER_H

#include "CNGSUserCredentials.h"
#include "TCVector.h"

class CNGSSocialMessage;

class CGServeMessageManager {
public:
    CGServeMessageManager();
    virtual ~CGServeMessageManager();

    void HandleUpdate();
    void AddMessage(CNGSSocialMessage* message, eSocialNetworkType network);

private:
    struct MessageItem {
        CNGSSocialMessage* m_message;
        eSocialNetworkType m_network;
    };

    TCVector<MessageItem> m_messages;
};

#endif
