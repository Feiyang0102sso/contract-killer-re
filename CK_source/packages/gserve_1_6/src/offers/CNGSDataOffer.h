// CNGSDataOffer.h - one gOffer offer and its incentives; CInviteOffer adds the per social
// network friend lists of the invite flow (already invited / still to invite).
// Field names follow the assignments in the offers response parser.
#ifndef CNGSDATAOFFER_H
#define CNGSDATAOFFER_H

#include "CStrWChar.h"
#include "TCVector.h"
#include "CNGSDataIncentive.h"
#include "CNGSFriendInviteeList.h"

class CDataOffer {
public:
    /** featureType values (m_featureType). */
    enum {
        FEATURE_TYPE_INVITE = 1,
        FEATURE_TYPE_FEATURED_GAME = 2
    };

    CDataOffer();
    ~CDataOffer();

    /** Index of the first incentive of category 1 (friend invite), -1 when none. */
    int hasFriendInviteIncentive();
    int hasFriendInstallIncentive();

    CStrWChar m_deepLink;      // +0x00
    CStrWChar m_effectiveDate; // +0x10
    int m_gameId;              // +0x20
    CStrWChar m_longDescription;  // +0x24
    int m_value34;                                              // +0x34 meaning unknown
    CStrWChar m_shortDescription; // +0x38
    int m_offerItemId;                                          // +0x48
    CStrWChar m_gameName;      // +0x4c
    int m_sourceGameId;                                         // +0x5c
    int m_featureType;                                          // +0x60 FEATURE_TYPE_*
    TCVector<CDataIncentive> m_incentives;    // +0x64
};

class CInviteOffer : public CDataOffer {
public:
    CInviteOffer();

    /** Social networks with friend lists. */
    enum {
        SOCIAL_NETWORK_COUNT = 6
    };

    /** Remember that `friendId` of network `socialNetworkType` was invited (empty ids are ignored). */
    void addPreviouslyInvitedFriend(int socialNetworkType, const CStrWChar& friendId);
    void addPotentialInvitee(int socialNetworkType, const CStrWChar& friendId);
    int getNumInviteesInSN(int socialNetworkType);
    int getNumQueuedInviteesInSN(int socialNetworkType);
    unsigned char wasPreviouslyInvited(int socialNetworkType, const CStrWChar* friendId);
    unsigned char wasPreviouslyInvited(int socialNetworkType, const CStrWChar& friendId);  // added by gserve (CK original signature)
    bool isPreviouslyInvitedFriendsListPopulated(int socialNetworkType);
    void setPreviouslyInvitedFriendsListPopulated(int socialNetworkType, unsigned char populated);
    /**
     * Take the first friend still to invite (lowest network first) into *friendId and drop it
     * from its list; returns its network, -1 when nobody is left.
     */
    int getPotentialInvitee(CStrWChar* friendId);

    CFriendInviteeList m_previouslyInvited[SOCIAL_NETWORK_COUNT];  // +0x7c
    CFriendInviteeList m_potentialInvitees[SOCIAL_NETWORK_COUNT];  // +0xf4
    unsigned int m_populatedNetworks;                              // +0x16c
};

class CFeaturedInstallOffer : public CDataOffer {
public:
    CFeaturedInstallOffer() : m_image(NULL), m_imageSize(0) {}
    ~CFeaturedInstallOffer();
    void setImage(const char* image);

    unsigned char* m_image;
    unsigned int m_imageSize;
};

#endif
