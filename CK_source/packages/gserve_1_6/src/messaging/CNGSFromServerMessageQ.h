// CNGSFromServerMessageQ.h - the messages the server pushed to the local user: polls them
// ("resources/message/poll"), keeps the valid ones and acknowledges the handled ones
// ("resources/message/ack"). Ids still to acknowledge survive restarts in "CNG_IdList.dat".
// (b023x; created as PROVISIONAL by b016x) CK size is 0x78.
#ifndef CNGSFROMSERVERMESSAGEQ_H
#define CNGSFROMSERVERMESSAGEQ_H

#include <stddef.h>
#include "CNGSServerObject.h"
#include "CNGSServerRequest.h"
#include "CNGSFromServerMessage.h"
#include "TCVector.h"
#include "CHash.h"

class CObjectMap;
class CObjectMapValue;
class CObjectMapArray;
class CObjectMapObject;
class CNGSFromServerMessageQFunctor;
class CNGSPushNotificationDetails;

/** {"id": n} objects of `ids` in a new array (the caller owns it). */
CObjectMapArray* GetObjectMapRepresentationOfIdList(TCVector<long long>& ids);

class CNGSFromServerMessageQ : public CNGSServerObject {
public:
    CNGSFromServerMessageQ();
    virtual ~CNGSFromServerMessageQ();

    /** Poll the server for new messages; 1 when the request went out. */
    virtual unsigned char SyncAllFromServer(CNGSNotifyFunctor* notify);
    virtual unsigned char ForceAckWithNotify(CNGSNotifyFunctor* notify);

    /** Forget the messages and the ids to acknowledge. */
    void Reset();
    /** Read the ids left unacknowledged by the last run; returns whether the file was read. */
    unsigned char LoadMessagesToAckFromFileSystem();
    CObjectMapObject* CreateGiftMsgObject(const CStrWChar& text, unsigned char deductOnReceipt,
                                         long long itemType, long long quantity, long long commonCurrency,
                                         long long rareCurrency, long long experience);
    CObjectMapObject* CreateAwardMsgObject(long long itemType, long long quantity, long long commonCurrency,
                                          long long rareCurrency, long long experience);
    /** Message described by one envelope of a poll reply (NULL when unusable or handled already). */
    CNGSFromServerMessage* CreateMessageFromObjectEnvelopeObject(CObjectMapValue* envelopeValue);
    /** Queue `messageId` for acknowledgement and send the acks when idle. */
    void AckMessageById(long long messageId);
    /** The message was handled: drop it and acknowledge it; false when unknown. */
    bool MarkMessageForAck(long long messageId);
    /** Send the queued acknowledgements; returns the request id (0 when nothing went out). */
    int AckMessages(CNGSNotifyFunctor* notify);
    CNGSFromServerMessage* GetMessageById(long long messageId);
    unsigned int CheckForEnvelopesFromServer();
    /** Ids of the messages matching the filters (0 = any); returns how many were added. */
    int GetAllMessagesByUserAndType(TCVector<long long>& ids, long long senderId,
                                    long long type, CNGSFromServerMessage::CNGS_MSG_SUBTYPE subType);
    void HandleCheckForEnvelopesFromServerResponse(CObjectMap* response, CNGSFromServerMessageQFunctor* functor);
    unsigned int AcknowledgeMessageList(CObjectMapArray* list);
    void HandleAcknowledgeEnvelopesFromServerResponse(CObjectMap* response, CNGSFromServerMessageQFunctor* functor);
    CObjectMapObject* CreateEnvelopeObject(int recipientId, CObjectMapValue* content, unsigned int duration,
                                          long long type, unsigned char push, CNGSPushNotificationDetails* details);
    void PostEnvelopes(TCVector<CObjectMapObject*>& envelopes);
    void HandlePostEnvelopeResponse(CObjectMap* response, CNGSFromServerMessageQFunctor* functor);
    int WriteMessageIdsToAckToFS();
    /** Show the first prompt message (type 300) of the authenticated local user. */
    unsigned char ProcessPromptMessages();

    int m_unused18;                                                         // +0x18 never touched here
    TCVector<CNGSFromServerMessage*> m_messages;  // +0x1c valid messages
    TCVector<long long> m_messageIdsToAck;        // +0x34
    int m_acksInFlight;                                                     // +0x4c ids of the outstanding ack request
    CHash m_messagesById;                  // +0x50 low word of the id -> message
    CHash m_queuedAckIds;                  // +0x64 ids stored in the ack list file
};

/** Routes a poll / ack reply to a CNGSFromServerMessageQ member function. */
class CNGSFromServerMessageQFunctor : public CNGSServerRequestFunctor {
public:
    typedef void (CNGSFromServerMessageQ::*Callback)(CObjectMap* response, CNGSFromServerMessageQFunctor* functor);

    CNGSFromServerMessageQFunctor(CNGSFromServerMessageQ* target, Callback callback)
        : m_target(target)
        , m_callback(callback)
        , m_context(NULL)
    {
    }

    virtual ~CNGSFromServerMessageQFunctor() {}

    virtual void requestCompleteCallback(CObjectMap* response);

    CNGSFromServerMessageQ* m_target;   // +0x08
    Callback m_callback;                // +0x0c
    void* m_context;                    // +0x14 NULL
};

#endif
