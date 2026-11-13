// CNGSOfferManager.cpp - the gServe offer and incentive state machine.
#include "CNGSOfferManager.h"
#include "CNGS.h"
#include "CNGSHeader.h"
#include "CNGSLocalUser.h"
#include "CNGSLoginFlow.h"
#include "CNGSOfferDelegate.h"
#include "CNGSRemoteUserList.h"
#include "CNGSServerRequest.h"
#include "CNGSSocialInterface.h"
#include "CNGSURLMgr.h"
#include "CNGSUtil.h"
#include "CNGS_Platform.h"
#include "CNotificationHandler.h"
#include "CObjectMap.h"
#include "CObjectMapArray.h"
#include "CObjectMapObject.h"

template <class Receiver>
class CNGSServerRequestCompleteFunctor : public CNGSServerRequestFunctor {
public:
    typedef void (Receiver::*Callback)(CObjectMap*, CNGSServerRequestFunctor*);

    CNGSServerRequestCompleteFunctor(Receiver* receiver, Callback callback)
        : m_receiver(receiver), m_callback(callback)
    {
    }

    virtual void requestCompleteCallback(CObjectMap* response)
    {
        if (m_callback) {
            (m_receiver->*m_callback)(response, this);
        }
    }

private:
    Receiver* m_receiver;
    Callback m_callback;
};

typedef CNGSServerRequestCompleteFunctor<COfferManager> OfferRequestComplete;

COfferManager::COfferManager()
    : CSingleton(ClassId_COfferManager)
{
    m_notificationHandler = NULL;
    m_offersLoaded = false;
    m_state = STATE_IDLE;
    m_selectedOfferIndex = 0;
    m_invitesSent = 0;
    m_inviteDelegate = new InviteDelegate;
    m_featuredGameDelegate = new FeaturedGameDelegate;
}

COfferManager::~COfferManager()
{
    resetOffers();
    resetUnconsumedIncentives();
    delete m_inviteDelegate;
    delete m_featuredGameDelegate;
}

unsigned char COfferManager::resetOffers()
{
    for (int index = 0; index < m_offers.Size(); ++index) {
        CDataOffer* offer = m_offers[index];
        if (offer != NULL) {
            switch (offer->m_featureType) {
            case CDataOffer::FEATURE_TYPE_FEATURED_GAME:
                delete static_cast<CFeaturedInstallOffer*>(offer);
                break;
            case CDataOffer::FEATURE_TYPE_INVITE:
                delete static_cast<CInviteOffer*>(offer);
                break;
            default:
                delete offer;
                break;
            }
            m_offers[index] = NULL;
        }
    }
    m_offers.Clear();
    m_state = STATE_IDLE;
    m_offersLoaded = false;
    return 1;
}

void COfferManager::resetUnconsumedIncentives()
{
    for (int index = 0; index < m_unconsumedIncentives.Size(); ++index) {
        if (m_unconsumedIncentives[index] != NULL) {
            delete m_unconsumedIncentives[index];
            m_unconsumedIncentives[index] = NULL;
        }
    }
    m_unconsumedIncentives.Clear();
    m_rewardedData.m_userGameRewardIds.Clear();
}

void COfferManager::registerNotificationHandler(CNotificationHandler* handler)
{
    m_notificationHandler = handler;
}

void COfferManager::initializeGluOffers()
{
    if (m_offersLoaded) {
        CNGSLoginFlow::GetInstance()->OnEvent(LoginFlowSuccess, "HandleInitializeGluOffers");
    } else if (CNGS::GetInstance()->GetLocalUser()->isValid()) {
        getOffersForGame();
    } else if (m_notificationHandler != NULL) {
        m_notificationHandler->HandleInitializeGluOffers(false, 2);
    }
}

void COfferManager::getOffersForGame()
{
    m_state = STATE_LOADING_OFFERS;
    COfferDataGetOffers data;
    sendRequest(GOFFER_SOURCE_GAME_OFFERS, &data,
        new OfferRequestComplete(this, &COfferManager::handleResponse_offers));
}

unsigned char COfferManager::isInitialized()
{
    return m_offersLoaded;
}

unsigned char COfferManager::wasPreviouslyInvited(int socialNetworkType, const CStrWChar* friendId)
{
    return m_inviteDelegate->m_offer->wasPreviouslyInvited(socialNetworkType, *friendId);
}

unsigned char COfferManager::isAlreadyPlayingGame(int socialNetworkType, const CStrWChar* friendId)
{
    unsigned char playing = 0;
    if (CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0]->findRemoteUserBySNID(socialNetworkType, *friendId) !=
        NULL) {
        playing = 1;
    }
    return playing;
}

int COfferManager::queueInvite(int socialNetworkType, const CStrWChar* friendId, unsigned char flag)
{
    if (m_state == STATE_IDLE) {
        return 1;
    }
    if (m_state != STATE_READY) {
        return 3;
    }
    if (m_selectedOfferIndex == -1) {
        return 4;
    }
    return m_inviteDelegate->queueInvite(socialNetworkType, friendId, flag);
}

int COfferManager::sendQueuedInvitations(const CStrWChar* message, const CStrWChar* link, const CStrWChar* name,
                                         const CStrWChar* caption, const CStrWChar* description, const CStrWChar* picture,
                                         const CStrWChar* privacy)
{
    if (m_state == STATE_IDLE) {
        return 1;
    }
    if (m_state != STATE_READY) {
        return 3;
    }
    if (m_selectedOfferIndex == -1) {
        return 4;
    }
    if (message == NULL) {
        return 5;
    }
    m_state = STATE_EXECUTING;
    m_inviteDelegate->setInviteParams(message, link, name, caption, description, picture, privacy);
    return m_inviteDelegate->execute();
}

void COfferManager::handleResponse_offers(CObjectMap* response, CNGSServerRequestFunctor*)
{
    CStrWChar error(L"failed");
    if (CNGSHeader::checkResponseOk(response, error) && extractOffersFromResponse(response)) {
        getDetailsForOffer(0);
    } else if (m_notificationHandler != NULL) {
        m_notificationHandler->HandleInitializeGluOffers(false, 11);
    }
}

unsigned char COfferManager::selectOffer(int index)
{
    // no range check in the original; each case fills the delegate's selection and sends it
    m_state = STATE_SELECTING;
    CDataOffer* offer = m_offers[index];
    switch (offer->m_featureType) {
    case CDataOffer::FEATURE_TYPE_INVITE:
        m_inviteDelegate->m_selectedData->m_offerItemId = offer->m_offerItemId;
        m_inviteDelegate->m_selectedData->m_sourceGameId = offer->m_sourceGameId;
        m_inviteDelegate->m_selectedData->m_featureType = CDataOffer::FEATURE_TYPE_INVITE;
        sendRequest(SERVICE_OFFER_SELECTED, m_inviteDelegate->m_selectedData,
            new OfferRequestComplete(this, &COfferManager::handleResponse_select));
        break;
    case CDataOffer::FEATURE_TYPE_FEATURED_GAME:
        m_featuredGameDelegate->m_selectedData->m_offerItemId = offer->m_offerItemId;
        m_featuredGameDelegate->m_selectedData->m_sourceGameId = offer->m_sourceGameId;
        m_featuredGameDelegate->m_selectedData->m_featureType = CDataOffer::FEATURE_TYPE_FEATURED_GAME;
        sendRequest(SERVICE_OFFER_SELECTED, m_featuredGameDelegate->m_selectedData,
            new OfferRequestComplete(this, &COfferManager::handleResponse_select));
        break;
    default:
        break;
    }
    return 1;
}

void COfferManager::handleResponse_select(CObjectMap* response, CNGSServerRequestFunctor*)
{
    COffer offer;
    unsigned char success = parseResponse(response, offer);
    if (offer.m_featureType == CDataOffer::FEATURE_TYPE_FEATURED_GAME) {
        if (success) {
            setPurchasedOffer(3, offer.m_offerItemId);
        } else {
            m_state = STATE_READY;
            if (m_notificationHandler != NULL) {
                m_notificationHandler->HandleFeaturedGameSelected(false, 11);
            }
        }
    } else if (offer.m_featureType == CDataOffer::FEATURE_TYPE_INVITE) {
        if (success && m_selectedOfferIndex >= 0 && m_selectedOfferIndex < m_offers.Size() &&
            m_offers[m_selectedOfferIndex]->hasFriendInviteIncentive() != -1) {
            setPurchasedOffer(1, offer.m_offerItemId);
            return;
        }
        m_state = STATE_READY;
        if (m_notificationHandler != NULL) {
            int count = 11;
            if (success) {
                count = m_invitesSent;
            }
            m_notificationHandler->HandleSendInvite(success, count);
        }
    }
}

unsigned char COfferManager::parseResponse(CObjectMap* response, COffer& offer)
{
    if (response == NULL) {
        return 0;
    }
    CObjectMapValue* value = response->getDataAt(CStrWChar(L"content"), CStrWChar(L" "));
    if (value == NULL || value->getType() != CObjectMapValue::TYPE_OBJECT) {
        return 0;
    }
    CObjectMapObject* content = static_cast<CObjectMapObject*>(value);
    CObjectMapValue* success = content->getEntry(CStrWChar(L"success"));
    if (success == NULL || success->getType() != CObjectMapValue::TYPE_BOOL ||
        !static_cast<CObjectMapBool*>(success)->m_value) {
        return 0;
    }

    value = content->getEntry(CStrWChar(L"featureType"));
    if (value != NULL && value->getType() == CObjectMapValue::TYPE_INT) {
        offer.m_featureType = static_cast<CObjectMapInt*>(value)->m_value;
    }
    value = content->getEntry(CStrWChar(L"offerItemId"));
    if (value != NULL && value->getType() == CObjectMapValue::TYPE_INT) {
        offer.m_offerItemId = static_cast<CObjectMapInt*>(value)->m_value;
    }
    value = content->getEntry(CStrWChar(L"offerCategoryId"));
    if (value != NULL && value->getType() == CObjectMapValue::TYPE_INT) {
        offer.m_offerCategoryId = static_cast<CObjectMapInt*>(value)->m_value;
    }
    return 1;
}

void COfferManager::setPurchasedOffer(int offerCategoryId, int offerItemId)
{
    m_state = STATE_PURCHASING;
    OfferRequestComplete* complete = new OfferRequestComplete(this, &COfferManager::handleResponse_purchase);
    if (offerCategoryId == 2) {
        COfferDataSetPurchasedInstall data;
        data.m_offerItemId = offerItemId;
        data.m_offerCategoryId = offerCategoryId;
        sendRequest(SERVICE_OFFER_PURCHASED, &data, complete);
    } else {
        COfferDataSetPurchased data;
        data.m_offerItemId = offerItemId;
        data.m_offerCategoryId = offerCategoryId;
        sendRequest(SERVICE_OFFER_PURCHASED, &data, complete);
    }
}

void COfferManager::finishedExecuting(int type, int status, int count)
{
    if (m_state != STATE_EXECUTING) {
        return;
    }
    if (type == CDataOffer::FEATURE_TYPE_FEATURED_GAME) {
        if (status == 0) {
            selectOffer(m_selectedOfferIndex);
            return;
        }
        if (m_notificationHandler != NULL) {
            m_notificationHandler->HandleFeaturedGameSelected(false, 0);
        }
    } else if (type == CDataOffer::FEATURE_TYPE_INVITE) {
        if (status == 0) {
            m_invitesSent = count;
            selectOffer(m_selectedOfferIndex);
            return;
        }
        if (m_notificationHandler != NULL) {
            m_notificationHandler->HandleSendInvite(false, 0);
        }
    } else {
        return;
    }
    m_state = STATE_READY;
}

void COfferManager::handlePostMessage(unsigned char success)
{
    m_inviteDelegate->handlePostMessage(success);
}

void COfferManager::handleResponse_purchase(CObjectMap* response, CNGSServerRequestFunctor*)
{
    m_state = STATE_READY;
    COffer offer;
    unsigned char success = parseResponse(response, offer);
    if (m_notificationHandler == NULL) {
        return;
    }
    if (offer.m_offerCategoryId == 3) {
        int result = 11;
        if (success) {
            result = 0;
        }
        m_notificationHandler->HandleFeaturedGameSelected(success, result);
    } else if (offer.m_offerCategoryId == 1) {
        int result = 11;
        if (success) {
            result = m_invitesSent;
        }
        m_notificationHandler->HandleSendInvite(success, result);
    }
}

void COfferManager::setPurchasedOfferInstall(int socialNetworkType)
{
    CNGSSocialInterface* social = CNGS_Platform::GetInstance()->getSocialInterface(socialNetworkType);
    if (social == NULL) {
        return;
    }
    CStrWChar playerId = social->getPlayerID();
    COfferDataSetPurchasedInstall data;
    data.m_offerItemId = -1;
    data.m_socialNetworkTypeId = socialNetworkType;
    data.m_socialNetworkUserId = playerId;
    data.m_offerCategoryId = 2;
    sendRequest(SERVICE_OFFER_PURCHASED, &data,
        new OfferRequestComplete(this, &COfferManager::handleResponse_purchase));
}

unsigned char COfferManager::getUnconsumedIncentives()
{
    if (m_state == STATE_READY && CNGS::GetInstance()->GetLocalUser()->isValid()) {
        m_state = STATE_GETTING_INCENTIVES;
        COfferDataGetIncentives data;
        sendRequest(SERVICE_INCENTIVE, &data,
            new OfferRequestComplete(this, &COfferManager::handleResponse_getIncentive));
        return 1;
    }
    if (m_notificationHandler != NULL) {
        m_notificationHandler->HandleIncentive(false, 0);
    }
    return 0;
}

unsigned int COfferManager::sendRequest(int service, const COfferDataBase* data, CNGSServerRequestFunctor* functor)
{
    CNGSHeader header;
    header.m_carrier = L"itunes";
    CObjectMapObject root;
    root.addEntry(CStrWChar(L"header"), header.createObjectRepresentation(1));
    root.addEntry(CStrWChar(L"content"), data->generateJSON());
    CStrChar url = CNGSURLMgr::GetInstance()->getGOfferURL(static_cast<eGOfferServiceType>(service));
    CNGSServerRequest* request = new CNGSServerRequest(url.GetCharPtr(), new CObjectMap(&root), functor, 0, 1);
    return request->m_messageId;
}

static CObjectMapObject* FindOfferContent(CObjectMap* response)
{
    if (response != NULL) {
        CObjectMapValue* value = response->getDataAt(CStrWChar(L"content"), CStrWChar(L" "));
        if (value != NULL && value->getType() == CObjectMapValue::TYPE_OBJECT) {
            CObjectMapObject* content = static_cast<CObjectMapObject*>(value);
            CObjectMapValue* success = content->getEntry(CStrWChar(L"success"));
            if (success != NULL && success->getType() == CObjectMapValue::TYPE_BOOL &&
                static_cast<CObjectMapBool*>(success)->m_value) {
                return content;
            }
        }
    }
    return NULL;
}

static CObjectMapArray* FindOfferArray(CObjectMapObject& object, const wchar_t* key)
{
    CObjectMapValue* entry = object.getEntry(CStrWChar(key));
    if (entry != NULL && entry->getType() == CObjectMapValue::TYPE_ARRAY) {
        return static_cast<CObjectMapArray*>(entry);
    }
    return NULL;
}

static int ReadOfferInt(CObjectMapObject& object, const wchar_t* key, int fallback)
{
    CObjectMapValue* entry = object.getEntry(CStrWChar(key));
    if (entry != NULL && entry->getType() == CObjectMapValue::TYPE_INT) {
        return static_cast<CObjectMapInt*>(entry)->m_value;
    }
    return fallback;
}

static const wchar_t* ReadOfferString(CObjectMapObject& object, const wchar_t* key, const wchar_t* fallback)
{
    CObjectMapValue* entry = object.getEntry(CStrWChar(key));
    if (entry != NULL && entry->getType() == CObjectMapValue::TYPE_STRING) {
        return static_cast<CObjectMapString*>(entry)->m_value.GetWCharPtr();
    }
    return fallback;
}

static void ReadIncentive(CObjectMapObject& record, CDataIncentive& incentive)
{
    incentive.m_quantity = ReadOfferInt(record, L"quantity", 0);
    incentive.m_unitOfMeasurement = ReadOfferString(record, L"unitOfMeasurement", L"");
    incentive.m_userGameRewardId = ReadOfferInt(record, L"userGameRewardId", 0);
    incentive.m_offeredGameName = ReadOfferString(record, L"offeredGameName", L"");
    incentive.m_offerCategoryId = ReadOfferInt(record, L"offerCategoryId", 0);
    incentive.m_offerCategoryName = ReadOfferString(record, L"offerCategoryName", L"");
}

unsigned char COfferManager::extractOffersFromResponse(CObjectMap* response)
{
    CObjectMapObject* content = FindOfferContent(response);
    if (content == NULL) {
        return 0;
    }
    CObjectMapArray* offers = FindOfferArray(*content, L"offer");
    if (offers == NULL) {
        return 1;
    }
    for (int index = 0; index < offers->m_values.Size(); ++index) {
        CObjectMapValue* value = offers->m_values[index];
        if (value == NULL || value->getType() != CObjectMapValue::TYPE_OBJECT) {
            continue;
        }
        CObjectMapObject& record = *static_cast<CObjectMapObject*>(value);
        int feature = ReadOfferInt(record, L"featureType", 0);
        CDataOffer* offer;
        if (feature == CDataOffer::FEATURE_TYPE_FEATURED_GAME) {
            CFeaturedInstallOffer* featured = new CFeaturedInstallOffer;
            const wchar_t* image = ReadOfferString(record, L"image", NULL);
            if (image != NULL) {
                CStrChar base64 = CNGSUtil::WStrToCStr(CStrWChar(image));
                if (base64.GetCharPtr() != NULL) {
                    featured->setImage(base64.GetCharPtr());
                }
            }
            offer = featured;
        } else if (feature == CDataOffer::FEATURE_TYPE_INVITE) {
            offer = new CInviteOffer;
        } else {
            continue;
        }

        offer->m_featureType = feature;
        offer->m_deepLink = ReadOfferString(record, L"deepLink", L"");
        offer->m_effectiveDate = ReadOfferString(record, L"effectiveDate", L"0");
        offer->m_gameId = ReadOfferInt(record, L"gameId", 0);
        offer->m_longDescription = ReadOfferString(record, L"longDescription", L"");
        offer->m_offerItemId = ReadOfferInt(record, L"offerItemId", 0);
        offer->m_shortDescription = ReadOfferString(record, L"shortDescription", L"");
        offer->m_sourceGameId = ReadOfferInt(record, L"sourceGameId", 0);

        CObjectMapArray* incentives = FindOfferArray(record, L"incentives");
        if (incentives != NULL) {
            for (int entry = 0; entry < incentives->m_values.Size(); ++entry) {
                CObjectMapValue* incentiveValue = incentives->m_values[entry];
                if (incentiveValue != NULL && incentiveValue->getType() == CObjectMapValue::TYPE_OBJECT) {
                    CDataIncentive incentive;
                    ReadIncentive(*static_cast<CObjectMapObject*>(incentiveValue), incentive);
                    offer->m_incentives.Add(incentive);
                }
            }
        }
        m_offers.Add(offer);
    }
    return 1;
}

void COfferManager::getDetailsForOffer(int index)
{
    while (m_offers.Size() > index) {
        m_state = STATE_LOADING_INVITEES;
        if (m_offers[index]->m_featureType != CDataOffer::FEATURE_TYPE_FEATURED_GAME) {
            if (m_offers[index]->m_featureType == CDataOffer::FEATURE_TYPE_INVITE) {
                getPreviouslyInvitedFriendsForOffer(index, 0);
            }
            return;
        }
        ++index;
    }
    m_state = STATE_READY;
    m_offersLoaded = 1;
    m_inviteDelegate->m_state = COfferDelegate::STATE_READY;
    // no NULL check in the original
    m_notificationHandler->HandleInitializeGluOffers(1, 0);
}

void COfferManager::getPreviouslyInvitedFriendsForOffer(int index, int start)
{
    CInviteOffer* offer = static_cast<CInviteOffer*>(m_offers[index]);
    COfferDataSetInvitedFriends data;
    data.m_offerItemId = offer->m_offerItemId;
    data.m_friendStartIndex = start;
    data.m_friendEndIndex = start + 1000;
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    for (int network = 0; network < CInviteOffer::SOCIAL_NETWORK_COUNT; ++network) {
        bool populated = offer->isPreviouslyInvitedFriendsListPopulated(network);
        if (user->isAuthenticated(network) && !populated) {
            data.m_socialNetworkTypeIds.Add(network);
        }
    }
    sendRequest(GOFFER_INVITED_FRIENDS_FOR_GAME, &data,
        new OfferRequestComplete(this, &COfferManager::handleResponse_invitedFriends));
}

void COfferManager::handleResponse_invitedFriends(CObjectMap* response, CNGSServerRequestFunctor*)
{
    CStrWChar error(L"failed");
    if (CNGSHeader::checkResponseOk(response, error)) {
        int page[2] = {0, 0};
        if (extractInvitedFriendsFromResponse(response, page) && page[1] != 0) {
            getPreviouslyInvitedFriendsForOffer(page[0], page[1]);
        } else {
            getDetailsForOffer(page[0] + 1);
        }
    } else if (m_notificationHandler != NULL) {
        m_notificationHandler->HandleInitializeGluOffers(0, 11);
    }
}

unsigned char COfferManager::extractInvitedFriendsFromResponse(CObjectMap* response, int* page)
{
    page[0] = -1;
    page[1] = 0;
    CObjectMapObject* content = FindOfferContent(response);
    if (content == NULL) {
        return 0;
    }
    CObjectMapValue* value = content->getEntry(CStrWChar(L"enduser"));
    if (value == NULL || value->getType() != CObjectMapValue::TYPE_OBJECT) {
        return 0;
    }
    CObjectMapObject& endUser = *static_cast<CObjectMapObject*>(value);
    int offerId = ReadOfferInt(endUser, L"offerItemId", -1);
    CInviteOffer* offer = NULL;
    for (int index = 0; index < m_offers.Size(); ++index) {
        CDataOffer* candidate = m_offers[index];
        if (candidate->m_offerItemId == offerId && candidate->m_featureType == CDataOffer::FEATURE_TYPE_INVITE) {
            page[0] = index;
            offer = static_cast<CInviteOffer*>(candidate);
            break;
        }
    }
    if (offer == NULL) {
        return 0;
    }
    CObjectMapArray* networks = FindOfferArray(endUser, L"friends");
    if (networks == NULL) {
        return 1;
    }
    for (int index = 0; index < networks->m_values.Size(); ++index) {
        CObjectMapValue* value = networks->m_values[index];
        if (value == NULL || value->getType() != CObjectMapValue::TYPE_OBJECT) {
            continue;
        }
        CObjectMapObject& record = *static_cast<CObjectMapObject*>(value);
        int network = ReadOfferInt(record, L"socialNetworkTypeId", -1);
        if (network < 0 || network >= CInviteOffer::SOCIAL_NETWORK_COUNT) {
            continue;
        }
        CObjectMapArray* friends = FindOfferArray(record, L"friendIdentifiers");
        if (friends == NULL) {
            continue;
        }
        for (int entry = 0; entry < friends->m_values.Size(); ++entry) {
            CObjectMapValue* id = friends->m_values[entry];
            if (id != NULL && id->getType() == CObjectMapValue::TYPE_STRING) {
                offer->addPreviouslyInvitedFriend(network, static_cast<CObjectMapString*>(id)->m_value);
            }
        }
        if (friends->m_values.Size() >= 1000) {
            page[1] = offer->getNumInviteesInSN(network);
        } else {
            offer->setPreviouslyInvitedFriendsListPopulated(network, 1);
        }
    }
    return 1;
}

void COfferManager::handleResponse_getIncentive(CObjectMap* response, CNGSServerRequestFunctor*)
{
    CStrWChar error(L"failed");
    int count = 0;
    if (CNGSHeader::checkResponseOk(response, error)) {
        resetUnconsumedIncentives();
        extractIncentivesFromResponse(response, m_unconsumedIncentives);
        count = m_unconsumedIncentives.Size();
    }
    if (m_notificationHandler != NULL) {
        m_notificationHandler->HandleIncentive(count > 0, count);
    }
    m_state = STATE_READY;
}

unsigned char COfferManager::extractIncentivesFromResponse(CObjectMap* response,
                                                           TCVector<CDataIncentive*>& incentives)
{
    CObjectMapObject* content = FindOfferContent(response);
    if (content == NULL) {
        return 0;
    }
    CObjectMapArray* records = FindOfferArray(*content, L"incentiveresult");
    if (records == NULL) {
        return 1;
    }
    for (int index = 0; index < records->m_values.Size(); ++index) {
        CObjectMapValue* value = records->m_values[index];
        if (value == NULL || value->getType() != CObjectMapValue::TYPE_OBJECT) {
            continue;
        }
        CObjectMapObject& record = *static_cast<CObjectMapObject*>(value);
        int network = ReadOfferInt(record, L"socialNetworkTypeId", -1);
        CNGSSocialInterface* social = CNGS_Platform::GetInstance()->getSocialInterface(network);
        if (social == NULL || !social->isUserAuthenticated()) {
            continue;
        }
        CDataIncentive* incentive = new CDataIncentive;
        ReadIncentive(record, *incentive);
        incentive->m_socialNetworkTypeId = network;
        incentive->m_incentiveCount = ReadOfferInt(record, L"incentiveCount", 1);
        const wchar_t* friendId = ReadOfferString(record, L"invitedFriendIdentifier", NULL);
        if (friendId != NULL) {
            incentive->m_invitedFriendName = social->getFriendNameFromSortedListWithId(CStrWChar(friendId));
        }
        incentives.Add(incentive);
    }
    return 1;
}

bool COfferManager::queueUnconsumedIncentiveRewarded(int index)
{
    // the original checks for a non-empty list, not for a negative index
    if (m_unconsumedIncentives.Size() > 0 && m_unconsumedIncentives.Size() > index) {
        m_rewardedData.m_userGameRewardIds.Add(m_unconsumedIncentives[index]->m_userGameRewardId);
        return true;
    }
    return false;
}

void COfferManager::collectRewardedIncentives()
{
    for (int index = 0; index < m_unconsumedIncentives.Size(); ++index) {
        queueUnconsumedIncentiveRewarded(index);
    }
}

void COfferManager::allUnconsumedIncentivesRewarded()
{
    if (m_rewardedData.m_userGameRewardIds.Size() > 0) {
        sendRequest(GOFFER_OFFER_REWARDED, &m_rewardedData,
            new OfferRequestComplete(this, &COfferManager::handleResponse_setRewardedOffer));
    }
}

void COfferManager::handleResponse_setRewardedOffer(CObjectMap* response, CNGSServerRequestFunctor*)
{
    CStrWChar error(L"failed");
    CNGSHeader::checkResponseOk(response, error);
}

unsigned char COfferManager::setOfferId(int offerId)
{
    int found = -1;
    for (int index = 0; index < m_offers.Size(); index++) {
        if (m_offers[index]->m_offerItemId == offerId) {
            found = index;
            break;
        }
    }
    if (found == -1) {
        return 0;
    }
    m_selectedOfferIndex = found;
    CDataOffer* offer = m_offers[found];
    if (offer->m_featureType == 1) {
        m_inviteDelegate->m_offer = (CInviteOffer*)offer;
    }
    return 1;
}

unsigned char COfferManager::setOfferIndex(int index)
{
    if (index < 0 || index >= m_offers.Size()) {
        return 0;
    }
    m_selectedOfferIndex = index;
    CDataOffer* offer = m_offers[index];
    if (offer->m_featureType == 1) {
        m_inviteDelegate->m_offer = (CInviteOffer*)offer;
    }
    return 1;
}

int COfferManager::getFriendInviteIncentiveQuantityForOffer()
{
    int quantity = 0;
    if (m_selectedOfferIndex < m_offers.Size()) {
        CDataOffer* offer = m_offers[m_selectedOfferIndex];
        int incentive = -1;
        if (m_offers.Size() > m_selectedOfferIndex) {
            incentive = offer->hasFriendInviteIncentive();
        }
        if (incentive < offer->m_incentives.Size()) {
            CDataIncentive data = offer->m_incentives[incentive];
            quantity = data.m_quantity;
        }
    }
    return quantity;
}

CStrWChar* COfferManager::getFriendInviteIncentiveUnitForOffer()
{
    CStrWChar* unit = NULL;
    if (m_selectedOfferIndex < m_offers.Size()) {
        CDataOffer* offer = m_offers[m_selectedOfferIndex];
        int incentive = -1;
        if (m_offers.Size() > m_selectedOfferIndex) {
            incentive = offer->hasFriendInviteIncentive();
        }
        if (incentive < offer->m_incentives.Size()) {
            unit = &offer->m_incentives[incentive].m_unitOfMeasurement;
        }
    }
    return unit;
}

int COfferManager::getFriendInstallIncentiveQuantityForOffer()
{
    int quantity = 0;
    if (m_selectedOfferIndex < m_offers.Size()) {
        CDataOffer* offer = m_offers[m_selectedOfferIndex];
        int incentive = -1;
        if (m_offers.Size() > m_selectedOfferIndex) {
            incentive = offer->hasFriendInstallIncentive();
        }
        if (incentive < offer->m_incentives.Size()) {
            CDataIncentive data = offer->m_incentives[incentive];
            quantity = data.m_quantity;
        }
    }
    return quantity;
}

CStrWChar* COfferManager::getFriendInstallIncentiveUnitForOffer()
{
    CStrWChar* unit = NULL;
    if (m_selectedOfferIndex < m_offers.Size()) {
        CDataOffer* offer = m_offers[m_selectedOfferIndex];
        int incentive = -1;
        if (m_offers.Size() > m_selectedOfferIndex) {
            incentive = offer->hasFriendInstallIncentive();
        }
        if (incentive < offer->m_incentives.Size()) {
            unit = &offer->m_incentives[incentive].m_unitOfMeasurement;
        }
    }
    return unit;
}

unsigned char COfferManager::offerHasFriendInviteIncentive()
{
    if (m_selectedOfferIndex < m_offers.Size()) {
        return m_offers[m_selectedOfferIndex]->hasFriendInviteIncentive() > -1;
    }
    return 0;
}
