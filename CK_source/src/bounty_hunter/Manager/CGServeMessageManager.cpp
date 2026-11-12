#include "CGServeMessageManager.h"

#include "CNGS_Platform.h"
#include "CNGSSocialInterface.h"
#include "CNGSSocialMessage.h"

typedef char GServeMessageManagerSizeCheck[sizeof(CGServeMessageManager) == 0x1c ? 1 : -1];

CGServeMessageManager::CGServeMessageManager()
{
}

CGServeMessageManager::~CGServeMessageManager()
{
    for (int index = 0; index < m_messages.Size(); index++) {
        delete m_messages[index].m_message;
    }
}

void CGServeMessageManager::HandleUpdate()
{
    if (m_messages.Size() > 0) {
        MessageItem entry = m_messages[0];
        CNGSSocialInterface* network = CNGS_Platform::GetInstance()->getSocialInterface(entry.m_network);
        if (network->getPostConnectionState() != 1) {
            network->postMessage(entry.m_message);
            delete entry.m_message;
            m_messages.RemoveAt(0);
        }
    }
}

void CGServeMessageManager::AddMessage(CNGSSocialMessage* message, eSocialNetworkType network)
{
    MessageItem entry;
    entry.m_message = message;
    entry.m_network = network;
    m_messages.Add(entry);
}
