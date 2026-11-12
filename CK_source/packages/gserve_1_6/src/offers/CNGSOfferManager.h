// CNGSOfferManager.h - gOffer client: offer selection / purchase requests and the unconsumed
// incentives of the local user. Owner: offers/CNGSOfferManager.cpp (b028x; started as a b016x
// PROVISIONAL that only held the class id).
#ifndef CNGSOFFERMANAGER_H
#define CNGSOFFERMANAGER_H

#include <stddef.h>
#include "CStringToKey.h"
#include "CSingleton.h"
#include "TCVector.h"
#include "CNGSDataOffer.h"
#include "CNGSDataIncentive.h"
#include "CNGSOfferRequest.h"

// Computed by every includer at static initialization (CNGS.o and CNGSOfferDelegate.o keep it).
static unsigned int ClassId_COfferManager = CStringToKey("COfferManager", 0);

class CObjectMap;
class CNGSServerRequestFunctor;
class CNotificationHandler;
class InviteDelegate;
class FeaturedGameDelegate;

/** Result of a select / purchase reply (COfferManager::parseResponse). */
class COffer {
public:
    COffer()
        : m_featureType(0)
        , m_offerItemId(-1)
        , m_offerCategoryId(0)
    {
    }

    int m_featureType;          // +0x00 "featureType"
    int m_offerItemId;          // +0x04 "offerItemId"
    int m_offerCategoryId;      // +0x08 "offerCategoryId"
};

class COfferManager : public CSingleton {
public:
    /** m_state values seen in the offers code. */
    enum {
        STATE_IDLE = 0,
        STATE_LOADING_OFFERS = 1,
        STATE_LOADING_INVITEES = 2,
        STATE_READY = 3,
        STATE_EXECUTING = 4,
        STATE_SELECTING = 5,
        STATE_PURCHASING = 6,
        STATE_GETTING_INCENTIVES = 7
    };

    /** gOffer services (CNGSURLMgr::getGOfferURL). */
    enum {
        SERVICE_OFFER_SELECTED = 1,
        SERVICE_OFFER_PURCHASED = 2,
        SERVICE_INCENTIVE = 4
    };

    COfferManager();
    virtual ~COfferManager();

    /** The registered instance, created on first use. */
    static inline COfferManager* GetInstance()
    {
        // The id is read before CApplet::m_pApp (verified: CNGSOfferDelegate.o load order).
        unsigned int classId = ClassId_COfferManager;
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(classId, &found);
        COfferManager* instance = (COfferManager*)found;
        if (instance == NULL) {
            instance = new COfferManager();
        }
        return instance;
    }

    /** Delete every offer; back to STATE_IDLE. Returns 1. */
    unsigned char resetOffers();
    /** Delete every unconsumed incentive and forget the rewarded ids. */
    void resetUnconsumedIncentives();
    void registerNotificationHandler(CNotificationHandler* handler);
    void initializeGluOffers();
    void getOffersForGame();
    unsigned char isInitialized();
    unsigned char wasPreviouslyInvited(int socialNetworkType, const CStrWChar* friendId);
    /** The friend is already a player of the game (in the friends-with-avatar list). */
    unsigned char isAlreadyPlayingGame(int socialNetworkType, const CStrWChar* friendId);
    /** Queue a friend for the selected invite offer; 1 not initialized, 3 busy, 4 no offer selected. */
    int queueInvite(int socialNetworkType, const CStrWChar* friendId, unsigned char flag);
    /** Post the queued invitations (5: no message). */
    int sendQueuedInvitations(const CStrWChar* message, const CStrWChar* link, const CStrWChar* name,
                              const CStrWChar* caption, const CStrWChar* description, const CStrWChar* picture,
                              const CStrWChar* privacy);
    void handleResponse_offers(CObjectMap* response, CNGSServerRequestFunctor* functor);
    unsigned char extractOffersFromResponse(CObjectMap* response);
    void getDetailsForOffer(int index);
    void getPreviouslyInvitedFriendsForOffer(int index, int start);
    void handleResponse_invitedFriends(CObjectMap* response, CNGSServerRequestFunctor* functor);
    unsigned char extractInvitedFriendsFromResponse(CObjectMap* response, int* page);
    /** Send the "offer selected" request for offer `index` (invite or featured game). */
    unsigned char selectOffer(int index);
    void handleResponse_select(CObjectMap* response, CNGSServerRequestFunctor* functor);
    /** Read featureType / offerItemId / offerCategoryId of a successful reply into `offer`. */
    unsigned char parseResponse(CObjectMap* response, COffer& offer);
    void setPurchasedOffer(int offerCategoryId, int offerItemId);
    /** A delegate finished (`type`: 1 invite, 2 featured game; `status` 0 = ok). */
    void finishedExecuting(int type, int status, int count);
    /** A social network post finished (forwarded to the invite delegate). */
    void handlePostMessage(unsigned char success);
    void handleResponse_purchase(CObjectMap* response, CNGSServerRequestFunctor* functor);
    void setPurchasedOfferInstall(int socialNetworkType);
    /** Ask the server for the unconsumed incentives (only when STATE_READY and logged in). */
    unsigned char getUnconsumedIncentives();
    void handleResponse_getIncentive(CObjectMap* response, CNGSServerRequestFunctor* functor);
    /** Append the incentives of a get-incentives reply to `incentives`; 1 when the reply was a success. */
    unsigned char extractIncentivesFromResponse(CObjectMap* response,
                                                TCVector<CDataIncentive*>& incentives);
    bool queueUnconsumedIncentiveRewarded(int index);
    void allUnconsumedIncentivesRewarded();
    void handleResponse_setRewardedOffer(CObjectMap* response, CNGSServerRequestFunctor* functor);
    /** Reconstructed (dead-stripped in the shipped binary): reward ids of the unconsumed incentives -> m_rewardedData. */
    void collectRewardedIncentives();
    /** Select the offer with offerItemId `offerId`; 0 when unknown. */
    unsigned char setOfferId(int offerId);
    unsigned char setOfferIndex(int index);
    int getFriendInviteIncentiveQuantityForOffer();
    CStrWChar* getFriendInviteIncentiveUnitForOffer();
    int getFriendInstallIncentiveQuantityForOffer();
    CStrWChar* getFriendInstallIncentiveUnitForOffer();
    unsigned char offerHasFriendInviteIncentive();
    /** Send `data` to gOffer service `service`; returns the message id. */
    unsigned int sendRequest(int service, const COfferDataBase* data, CNGSServerRequestFunctor* functor);

    CNotificationHandler* m_notificationHandler;                    // +0x08
    InviteDelegate* m_inviteDelegate;                               // +0x0c owned
    FeaturedGameDelegate* m_featuredGameDelegate;                   // +0x10 owned
    int m_value14;                                                  // +0x14 not used by this unit
    TCVector<CDataOffer*> m_offers;        // +0x18 owned
    unsigned char m_offersLoaded;                                   // +0x30
    int m_state;                                                    // +0x34 STATE_*
    int m_invitesSent;                                              // +0x38 reported by HandleSendInvite
    int m_selectedOfferIndex;                                       // +0x3c
    TCVector<CDataIncentive*> m_unconsumedIncentives;    // +0x40 owned
    COfferDataSetRewarded m_rewardedData;                           // +0x58
};

#endif
