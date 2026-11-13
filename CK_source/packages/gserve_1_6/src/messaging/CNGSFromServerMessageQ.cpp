// CNGSFromServerMessageQ.cpp - server message queue of the local user: message poll, ack
// bookkeeping (persisted in "CNG_IdList.dat") and the prompt messages.
// The original clears its file buffers with the C library memset, not np_memset.
#undef memset
#include <string.h>
#include "CNGSFromServerMessageQ.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSJSONParser.h"
#include "CObjectMap.h"
#include "CObjectMapObject.h"
#include "CObjectMapArray.h"
#include "CNotificationHandler.h"
#include "CFileUtil_gServe.h"
#include "CArrayInputStream.h"
#include "CArrayOutputStream.h"
#include "ICFileMgr.h"
#include "CNGSTokens.h"
#include "CNGSUtil.h"
#include "CNGSPushNotificationDetails.h"


/** Name of the file with the ids still to acknowledge. */
static CStrWChar kMessageIdListFile("CNG_IdList.dat");

CNGSFromServerMessageQ::CNGSFromServerMessageQ()
{
    m_messages.Clear();
    m_messageIdsToAck.Clear();
    m_messagesById.Init(10, 10, 1);
    m_queuedAckIds.Init(10, 10, 1);
    m_acksInFlight = 0;
}

void CNGSFromServerMessageQ::Reset()
{
    m_messages.Clear();
    m_messageIdsToAck.Clear();
    m_messagesById.RemoveAll();
    m_queuedAckIds.RemoveAll();
    m_acksInFlight = 0;
}

unsigned char CNGSFromServerMessageQ::LoadMessagesToAckFromFileSystem()
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    if (user == NULL) {
        return 0;
    }
    if (!user->m_credentials.isValid()) {
        return 0;
    }

    unsigned char data[0x1000];
    memset(data, 0, sizeof(data));
    const wchar_t* fileName = kMessageIdListFile.GetWCharPtr();
    unsigned char loaded = CFileUtil_gServe::ReadApplicationDataFile(fileName, data, sizeof(data),
                                                                     user->m_credentials.getSubDirectoryPath().GetWCharPtr());
    if (!loaded) {
        return 0;
    }

    CArrayInputStream stream;
    if (stream.Open(data, sizeof(data))) {
        wchar_t text[0x1001];
        int length = stream.ReadJMUtf(text, 0x1001);
        text[length] = 0;
        if (length > 0) {
            CStrWChar json(text);
            // the decoded tree is not freed (as in the original)
            CObjectMapValue* value = CNGSJSONParser::decodeValue(json);
            if (value != NULL && value->getType() == CObjectMapValue::TYPE_ARRAY) {
                CObjectMapArray* list = (CObjectMapArray*)value;
                int count = list->m_values.Size();
                m_messageIdsToAck.Clear();
                if (count > 0) {
                    for (int index = 0; index < count; index++) {
                        CObjectMapValue* entry = list->m_values[index];
                        if (entry != NULL && entry->getType() == CObjectMapValue::TYPE_OBJECT) {
                            CObjectMapValue* idValue = ((CObjectMapObject*)entry)->getEntry(CStrWChar(NGS_ID_TOKEN));
                            if (idValue != NULL && idValue->getType() == CObjectMapValue::TYPE_INT) {
                                long long messageId = ((CObjectMapInt*)idValue)->m_value;
                                unsigned int key = (unsigned int)messageId;
                                if (!m_queuedAckIds.Find(key, NULL)) {
                                    m_messageIdsToAck.Add(messageId);
                                    m_queuedAckIds.Insert(key, (void*)key);
                                }
                            }
                        }
                    }
                }
                CNGSUtil::DebugLog("CNGSFromServerMessageQ::LoadMessagesToAckFromFileSystem found %d messages waiting to be acknowledged ",
                                  count);
                if (count > 0) {
                    CNGS::GetInstance()->GetLocalUser()->m_notificationHandler->HandleOldUnackedMessages();
                }
            } else {
                WriteMessageIdsToAckToFS();
            }
        }
    }
    return loaded;
}

CNGSFromServerMessageQ::~CNGSFromServerMessageQ()
{
}

CObjectMapObject* CNGSFromServerMessageQ::CreateGiftMsgObject(const CStrWChar& text, unsigned char deductOnReceipt,
                                                             long long itemType, long long quantity,
                                                             long long commonCurrency, long long rareCurrency,
                                                             long long experience)
{
    CObjectMapObject* message = new CObjectMapObject();
    CObjectMapObject* gift = new CObjectMapObject();
    gift->addEntry(CStrWChar("Award"),
                   CreateAwardMsgObject(itemType, quantity, commonCurrency, rareCurrency, experience));
    if (deductOnReceipt) {
        CObjectMapObject* receipt = new CObjectMapObject();
        receipt->addEntry(CStrWChar("Award"), CreateAwardMsgObject(itemType, -quantity, -commonCurrency,
                                                                   -rareCurrency, 0));
        gift->addEntry(CStrWChar("OnReceipt"), receipt);
    }
    if (text.GetWCharPtr() != NULL) {
        message->addEntry(CStrWChar("MessageText"), new CObjectMapString(text));
    }
    message->addEntry(CStrWChar("Gift"), gift);
    return message;
}

CObjectMapObject* CNGSFromServerMessageQ::CreateAwardMsgObject(long long itemType, long long quantity,
                                                              long long commonCurrency, long long rareCurrency,
                                                              long long experience)
{
    CObjectMapObject* award = new CObjectMapObject();
    if (itemType != 0) {
        CObjectMapObject* item = new CObjectMapObject();
        item->addEntry(CStrWChar("ItemType"), new CObjectMapInt(itemType));
        item->addEntry(CStrWChar("Quantity"), new CObjectMapInt(quantity));
        award->addEntry(CStrWChar("Item"), item);
    }
    if (commonCurrency != 0) {
        award->addEntry(CStrWChar("CurrencyCommon"), new CObjectMapInt(commonCurrency));
    }
    if (rareCurrency != 0) {
        award->addEntry(CStrWChar("CurrencyRare"), new CObjectMapInt(rareCurrency));
    }
    if (experience != 0) {
        award->addEntry(CStrWChar("Experience"), new CObjectMapInt(experience));
    }
    return award;
}

CNGSFromServerMessage* CNGSFromServerMessageQ::CreateMessageFromObjectEnvelopeObject(CObjectMapValue* envelopeValue)
{
    if (envelopeValue == NULL || envelopeValue->getType() != CObjectMapValue::TYPE_OBJECT) {
        CNGSUtil::DebugLog("Invalid envelope");
        return NULL;
    }
    CObjectMapObject* envelope = (CObjectMapObject*)envelopeValue;
    CStrWChar senderGamertag("");

    long long messageId;
    CObjectMapValue* idValue = envelope->getEntry(CStrWChar(NGS_ID_TOKEN));
    if (idValue == NULL || idValue->getType() != CObjectMapValue::TYPE_INT) {
        CNGSUtil::DebugLog("Invalid message id");
        messageId = 0;
    } else {
        messageId = ((CObjectMapInt*)idValue)->m_value;
    }

    long long senderId;
    CObjectMapValue* senderValue = envelope->getEntry(CStrWChar("senderid"));
    if (senderValue == NULL || senderValue->getType() != CObjectMapValue::TYPE_INT) {
        CNGSUtil::DebugLog("Invalid sender");
        senderId = 0;
    } else {
        senderId = ((CObjectMapInt*)senderValue)->m_value;
    }

    CObjectMapValue* gamertagValue = envelope->getEntry(CStrWChar("senderGamertag"));
    if (gamertagValue == NULL || gamertagValue->getType() != CObjectMapValue::TYPE_STRING) {
        CNGSUtil::DebugLog("Invalid sender nickname");
    } else {
        senderGamertag = CStrWChar(((CObjectMapString*)gamertagValue)->m_value);
    }

    long long type;
    CObjectMapValue* typeValue = envelope->getEntry(CStrWChar("type"));
    if (typeValue == NULL || typeValue->getType() != CObjectMapValue::TYPE_INT) {
        CNGSUtil::DebugLog("Invalid type");
        type = 0;
    } else {
        type = ((CObjectMapInt*)typeValue)->m_value;
    }

    long long recipientId;
    CObjectMapValue* recipientValue = envelope->getEntry(CStrWChar("recipientid"));
    if (recipientValue == NULL || recipientValue->getType() != CObjectMapValue::TYPE_INT) {
        CNGSUtil::DebugLog("Invalid recipient");
        recipientId = 0;
    } else {
        recipientId = ((CObjectMapInt*)recipientValue)->m_value;
    }

    CObjectMapValue* textValue = envelope->getEntry(CStrWChar("message"));
    if (textValue == NULL || textValue->getType() != CObjectMapValue::TYPE_STRING) {
        CNGSUtil::DebugLog("Invalid message contents");
        return NULL;
    }
    CStrWChar rawText(((CObjectMapString*)textValue)->m_value);
    CStrWChar text;

    // turn the escaped quotes \" back into "
    int quote = rawText.Find(L"\\\"", 0);
    if (quote < 0) {
        text = rawText;
    } else {
        int start = 0;
        if (quote == 0) {
            start = 1;
            quote = rawText.Find(L"\\\"", 1);
        }
        while (quote >= 0) {
            text += rawText.GetSubString(start, quote - 1);
            start = quote + 1;
            quote = rawText.Find(L"\\\"", quote + 2);
        }
        text += rawText.GetSubString(start, rawText.GetLength() - 1);
    }
    CNGSUtil::DebugLog("Message Received, Contents =");
    CNGSUtil::DebugLog(CNGSUtil::WStrToCStr(text).GetCharPtr());

    CNGSFromServerMessage* message = NULL;
    switch (type) {
    case 40:
    case 41:
    case 100:
    case 300:
        CNGSUtil::DebugLog("JSON Message received!!!");
        message = CNGSMessageJSONBase::CreateFromString(text);
        if (message == NULL) {
            return NULL;
        }
        message->Init(messageId, type, senderId, recipientId, senderGamertag, text);
        break;
    case 11:
    case 20:
        CNGSUtil::DebugLog("Text Message received!!!");
        message = new CNGSFromServerMessage();
        message->Init(messageId, type, senderId, recipientId, senderGamertag, text);
        break;
    case 30:
        // open the URL right away; nothing is kept
        CNGSUtil::DebugLog("Launch URL Message !!!");
        CApplet::m_pCore->LaunchURL(text.GetWCharPtr(), 0);
        AckMessageById(messageId);
        break;
    default:
        if (type >= 1000) {
            // game specific messages
            CNGSUtil::DebugLog("Custom Message received!!!");
            message = new CNGSFromServerMessage();
            message->Init(messageId, type, senderId, recipientId, senderGamertag, text);
        } else {
            // unknown type: acknowledge and drop it
            CNGSUtil::DebugLog("Unrecognized/Unknown Message received!!!");
            AckMessageById(messageId);
        }
        break;
    }
    return message;
}

void CNGSFromServerMessageQ::AckMessageById(long long messageId)
{
    unsigned int key = (unsigned int)messageId;
    if (!m_queuedAckIds.Find(key, NULL)) {
        m_messageIdsToAck.Add(messageId);
        m_queuedAckIds.Insert(key, (void*)key);
        WriteMessageIdsToAckToFS();
    }
    if (isReady() && m_writeRequestsOutstanding == 0) {
        AckMessages(NULL);
    }
}

unsigned char CNGSFromServerMessageQ::MarkMessageForAck(long long messageId)
{
    CNGSUtil::DebugLog("MarkMessageForAck, message = %d", (int)messageId);
    CNGSFromServerMessage* message = NULL;
    m_messagesById.Find((unsigned int)messageId, (void**)&message);
    if (message != NULL) {
        m_messagesById.Remove((unsigned int)message->m_messageId);
        m_messages.Remove(message);
        AckMessageById(message->m_messageId);
        CNGSUtil::DebugLog("Message %ld marked to Ack, %ld messages to be Acked",
                          (long)messageId, (long)m_messageIdsToAck.Size());
        return true;
    }
    return false;
}

int CNGSFromServerMessageQ::AckMessages(CNGSNotifyFunctor* notify)
{
    if (!isReady() || m_writeRequestsOutstanding != 0 || m_readNotify != NULL) {
        return 0;
    }
    m_readNotify = notify;
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    int sent = 0;
    if (isReady() && m_writeRequestsOutstanding == 0 && user != NULL && m_acksInFlight == 0
        && m_messageIdsToAck.Size() > 0) {
        CNGSUtil::DebugLog("Acking Messages");
        sent = AcknowledgeMessageList(GetObjectMapRepresentationOfIdList(m_messageIdsToAck));
        if (sent > 0) {
            m_acksInFlight = m_messageIdsToAck.Size();
            AddReadRequestOutstanding();
        }
    }
    return sent;
}

unsigned char CNGSFromServerMessageQ::ForceAckWithNotify(CNGSNotifyFunctor* notify)
{
    if (!isReady() || m_writeRequestsOutstanding != 0 || m_readNotify != NULL) {
        return 0;
    }
    m_readNotify = notify;
    return AcknowledgeMessageList(GetObjectMapRepresentationOfIdList(m_messageIdsToAck));
}

CNGSFromServerMessage* CNGSFromServerMessageQ::GetMessageById(long long messageId)
{
    void* found = NULL;
    m_messagesById.Find((unsigned int)messageId, &found);
    return (CNGSFromServerMessage*)found;
}

unsigned char CNGSFromServerMessageQ::SyncAllFromServer(CNGSNotifyFunctor* notify)
{
    unsigned char sent = 0;
    if (isReady() && m_writeRequestsOutstanding == 0) {
        m_readNotify = notify;
        sent = CheckForEnvelopesFromServer() != 0;
    }
    return sent;
}

unsigned int CNGSFromServerMessageQ::CheckForEnvelopesFromServer()
{
    CObjectMap* message = new CObjectMap();
    message->beginObject();
    InsertHeaderIntoMessage(message, "message poll");
    message->beginObject(CStrWChar(NGS_USER_TOKEN));
    message->addEntry(CStrWChar(NGS_ID_TOKEN), CObjectMapInt(CNGS::GetInstance()->GetLocalUser()->GetClientID()));
    message->endObject();
    message->endObject();

    unsigned int messageId = SendMessageToServer(message, "resources/message/poll",
        new CNGSFromServerMessageQFunctor(this, &CNGSFromServerMessageQ::HandleCheckForEnvelopesFromServerResponse));
    if (messageId != 0) {
        AddReadRequestOutstanding();
    }
    return messageId;
}

int CNGSFromServerMessageQ::GetAllMessagesByUserAndType(TCVector<long long>& ids, long long senderId, long long type,
                                                        CNGSFromServerMessage::CNGS_MSG_SUBTYPE subType)
{
    int count = 0;
    if (type != 0) {
        for (int index = 0; index < m_messages.Size(); index++) {
            CNGSFromServerMessage* message = m_messages[index];
            if (message->m_type == type && (senderId == 0 || message->m_senderId == senderId)
                && (subType == CNGSFromServerMessage::CNGS_MSG_SUBTYPE_ANY || message->m_subType == subType)) {
                long long messageId = message->m_messageId;
                ids.Add(messageId);
                count++;
            }
        }
    } else if (senderId == 0) {
        // no filter: every message
        for (int index = 0; index < m_messages.Size(); index++) {
            long long messageId = m_messages[index]->m_messageId;
            ids.Add(messageId);
            count++;
        }
    } else {
        for (int index = 0; index < m_messages.Size(); index++) {
            CNGSFromServerMessage* message = m_messages[index];
            if (message->m_senderId == senderId && (type == 0 || message->m_type == type)) {
                long long messageId = message->m_messageId;
                count++;
                ids.Add(messageId);
            }
        }
    }
    return count;
}

void CNGSFromServerMessageQ::HandleCheckForEnvelopesFromServerResponse(CObjectMap* response, CNGSFromServerMessageQFunctor* functor)
{
    CNGSUtil::DebugLog("HandleCheckForEnvelopesFromServerResponse");
    if (WasErrorInResponse(response, functor, "HandleCheckForEnvelopesFromServerResponse") == 0) {
        CObjectMapValue* listValue = response->getDataAt(CStrWChar("list"), CStrWChar(L" "));
        if (listValue == NULL || listValue->getType() != CObjectMapValue::TYPE_ARRAY) {
            CNGSUtil::DebugLog("Invalid Envelope List");
        } else {
            CObjectMapArray* list = (CObjectMapArray*)listValue;
            int count = list->m_values.Size();
            unsigned char newMessages = 0;
            if (count > 0) {
                CNGSUtil::DebugLog("got something");
                for (int index = 0; index < count; index++) {
                    CNGSFromServerMessage* message = CreateMessageFromObjectEnvelopeObject(list->m_values[index]);
                    // a message already known is skipped (and not freed, as in the original)
                    if (message != NULL && !m_messagesById.Find((unsigned int)message->m_messageId, NULL)) {
                        int timeState = message->CheckTime();
                        if (timeState < 0) {
                            // not valid yet: dropped
                            CNGSUtil::DebugLog("Message %ld not yet ready to deliver", message->m_messageId);
                            delete message;
                        } else if (timeState >= 1) {
                            // expired: acknowledge and drop it
                            CNGSUtil::DebugLog("Message %ld expired", message->m_messageId);
                            CNGSUtil::DebugLog("Marking message %ld for later Ack, message index = %d", message->m_messageId,
                                               index);
                            long long messageId = message->m_messageId;
                            m_messageIdsToAck.Add(messageId);
                            delete message;
                        } else {
                            m_messagesById.Insert((unsigned int)message->m_messageId, message);
                            m_messages.Add(message);
                            newMessages = 1;
                        }
                    }
                }
            }

            CNotificationHandler* handler = CNGS::GetInstance()->GetLocalUser()->m_notificationHandler;
            handler->HandleReceivedEnvelope();
            if (newMessages) {
                handler->HandleNewMessagesReceived();
            }
        }
    }
    CompleteReadRequestOutstanding();
}

CObjectMapArray* GetObjectMapRepresentationOfIdList(TCVector<long long>& ids)
{
    int count = ids.Size();
    CObjectMapArray* list = new CObjectMapArray();
    for (int index = 0; index < count; index++) {
        CObjectMapObject* entry = new CObjectMapObject();
        entry->addEntry(CStrWChar(NGS_ID_TOKEN), new CObjectMapInt(ids[index]));
        list->addEntry(entry);    // b147: inline addEntry (parameter slot in the original frame)
    }
    return list;
}

unsigned int CNGSFromServerMessageQ::AcknowledgeMessageList(CObjectMapArray* list)
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    unsigned int sent = 0;
    if (isReady() && m_writeRequestsOutstanding == 0 && user != NULL) {
        CObjectMapObject* message = new CObjectMapObject();
        InsertHeaderIntoMessageObject(message, "message ack", 0);
        message->addEntry(CStrWChar("messages"), list);
        message->addEntry(CStrWChar(NGS_USER_TOKEN), user->GetUserObject());
        sent = SendMessageObjectToServer(message, "resources/message/ack",
            new CNGSFromServerMessageQFunctor(this, &CNGSFromServerMessageQ::HandleAcknowledgeEnvelopesFromServerResponse));
    }
    return sent;
}

void CNGSFromServerMessageQ::HandleAcknowledgeEnvelopesFromServerResponse(CObjectMap* response, CNGSFromServerMessageQFunctor* functor)
{
    // the caller name differs from the function name in the original
    if (WasErrorInResponse(response, functor, "HandleAcknowledgeFromServerEnvelopesResponse") == 0) {
        CNGSUtil::DebugLog("HandleAcknowledgeFromServerEnvelopesResponse");
        // drop the acknowledged ids from the front
        int remaining = m_messageIdsToAck.Size() - m_acksInFlight;
        for (int index = 0; index < remaining; index++) {
            m_queuedAckIds.Remove((unsigned int)m_messageIdsToAck[index]);
            m_messageIdsToAck[index] = m_messageIdsToAck[m_acksInFlight + index];
        }
        m_messageIdsToAck.EnsureCapacity(remaining);
        m_messageIdsToAck.m_size = remaining;
        m_acksInFlight = 0;
    }
    WriteMessageIdsToAckToFS();
    CompleteReadRequestOutstanding();
}

CObjectMapObject* CNGSFromServerMessageQ::CreateEnvelopeObject(int recipientId, CObjectMapValue* content,
                                                               unsigned int duration, long long type,
                                                               unsigned char push, CNGSPushNotificationDetails* details)
{
    CObjectMapObject* envelope = new CObjectMapObject();
    envelope->addEntry(CStrWChar("recipientid"), new CObjectMapInt(recipientId));
    envelope->addEntry(CStrWChar("duration"), new CObjectMapInt(duration));
    envelope->addEntry(CStrWChar("type"), new CObjectMapInt(type));
    envelope->addEntry(CStrWChar("pushok"), new CObjectMapInt(push));
    if (push && details != NULL) {
        envelope->addEntry(CStrWChar("notification"), details->GetObjectMapRepresentation());
    }
    CStrWChar text = CNGSJSONParser::encodeValue(content);
    envelope->addEntry(CStrWChar("message"), new CObjectMapString(text));
    return envelope;
}

void CNGSFromServerMessageQ::PostEnvelopes(TCVector<CObjectMapObject*>& envelopes)
{
    CObjectMapArray* list = new CObjectMapArray();
    CObjectMapObject* message = new CObjectMapObject();
    AddWriteRequestOutstanding();
    for (int index = 0; index < envelopes.Size(); index++) {
        list->addEntry(envelopes[index]);
    }
    message->addEntry(CStrWChar("messages"), list);
    message->addEntry(CStrWChar(NGS_USER_TOKEN), CNGS::GetInstance()->GetLocalUser()->GetUserObject());
    InsertHeaderIntoMessageObject(message, "message post", 0);
    CObjectMap* map = new CObjectMap(message);
    CNGSFromServerMessageQFunctor* functor =
        new CNGSFromServerMessageQFunctor(this, &CNGSFromServerMessageQ::HandlePostEnvelopeResponse);
    SendMessageToServer(map, "resources/message/post", functor);
}

void CNGSFromServerMessageQ::HandlePostEnvelopeResponse(CObjectMap* response,
                                                       CNGSFromServerMessageQFunctor* functor)
{
    if (WasErrorInResponse(response, functor, "HandleAcknowledgeFromServerEnvelopesResponse") == 0) {
        CNGSUtil::DebugLog("HandleFromServerMessagesResponse Post Message response from server: %s",
                          CNGSUtil::WStrToCStr(CNGSJSONParser::encodeValue(response->getRoot())).GetCharPtr());
        CNGSUtil::DebugLog("HandleFromServerMessagesResponse");
    } else {
        CNGSUtil::DebugLog("HandleFromServerMessagesResponse ERROR!!!");
    }
    CompleteWriteRequestOutstanding();
}

int CNGSFromServerMessageQ::WriteMessageIdsToAckToFS()
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    if (user != NULL) {
        CObjectMapArray* list = GetObjectMapRepresentationOfIdList(m_messageIdsToAck);
        CStrWChar json = CNGSJSONParser::encodeValue(list);
        if (list != NULL) {
            delete list;
        }

        CArrayOutputStream stream;
        unsigned char data[0x1000];
        memset(data, 0, sizeof(data));
        if (json.GetLength() < sizeof(data) / sizeof(wchar_t) && stream.Open(data, sizeof(data))) {
            CStrWChar directory = CNGSUserCredentials::getFullDirectoryPathForSubdir(user->m_credentials.getSubDirectoryPath());
            if (ICFileMgr::GetInstance()->MkDir(directory.GetWCharPtr())) {
                stream.WriteJMUtf(json.GetWCharPtr());
                const wchar_t* fileName = kMessageIdListFile.GetWCharPtr();
                if (CFileUtil_gServe::WriteApplicationDataFile(fileName, data, sizeof(data),
                                                               user->m_credentials.getSubDirectoryPath().GetWCharPtr())) {
                    CNGSUtil::DebugLog("Stored message IDs for later ack");
                } else {
                    CNGSUtil::DebugLog("Failed storing message IDs for later ack");
                }
            }
        }
    }
    return 0;
}

unsigned char CNGSFromServerMessageQ::ProcessPromptMessages()
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    unsigned char result = 0;
    if (user != NULL && user->isAuthenticated(0)) {
        TCVector<long long> ids;
        if (GetAllMessagesByUserAndType(ids, 0, 300, CNGSFromServerMessage::CNGS_MSG_SUBTYPE_ANY) > 0) {
            result = GetMessageById(ids[0])->Apply();
        }
    }
    return result;
}

void CNGSFromServerMessageQFunctor::requestCompleteCallback(CObjectMap* response)
{
    // plain pointer-to-member test (see CNGSTimeFunctor)
    if (m_callback) {
        (m_target->*m_callback)(response, this);
    } else {
        CNGSUtil::DebugLog("No CNGSFromServerMessageQ response handler");
    }
}
