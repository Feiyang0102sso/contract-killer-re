// CNGSOfferRequest.cpp - JSON payloads of CK's offers and incentives.
#include "CNGSOfferRequest.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CObjectMapArray.h"
#include "CObjectMapObject.h"
#include "CUtil.h"
#include "ICStdUtil.h"

COfferDataBase::COfferDataBase()
{
}

COfferDataBase::~COfferDataBase()
{
}

void COfferDataBase::generateBaseJSON(CObjectMapObject& object) const
{
    char senderId[256] = {0};
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    ICStdUtil::SPrintF(senderId, "%i", user->GetClientID());
    object.addEntry(CStrWChar(L"senderIdentifier"), new CObjectMapString(CStrWChar(senderId)));
}

COfferDataSetInvitedFriends::COfferDataSetInvitedFriends()
    : m_offerItemId(-1), m_friendStartIndex(0), m_friendEndIndex(1000)
{
}

COfferDataSetInvitedFriends::~COfferDataSetInvitedFriends()
{
}

CObjectMapObject* COfferDataSetInvitedFriends::generateJSON() const
{
    CObjectMapObject* result = new CObjectMapObject;
    generateBaseJSON(*result);
    CObjectMapArray* networks = new CObjectMapArray;
    for (int index = 0; index < m_socialNetworkTypeIds.Size(); ++index) {
        char networkId[256] = {0};
        ICStdUtil::SPrintF(networkId, "%i", m_socialNetworkTypeIds[index]);
        networks->addEntry(new CObjectMapString(CStrWChar(networkId)));
    }
    result->addEntry(CStrWChar(L"socialNetworkTypeIds"), networks);
    result->addEntry(CStrWChar(L"friendStartIndex"), new CObjectMapInt(m_friendStartIndex));
    result->addEntry(CStrWChar(L"friendEndIndex"), new CObjectMapInt(m_friendEndIndex));
    result->addEntry(CStrWChar(L"offerItemId"), new CObjectMapInt(m_offerItemId));
    return result;
}

COfferDataSetSelected::COfferDataSetSelected()
    : m_offerItemId(-1), m_sourceGameId(-1)
{
}

COfferDataSetSelected::~COfferDataSetSelected()
{
}

CObjectMapObject* COfferDataSetSelected::generateJSON() const
{
    CObjectMapObject* result = new CObjectMapObject;
    generateBaseJSON(*result);
    result->addEntry(CStrWChar(L"featureType"), new CObjectMapInt(m_featureType));
    result->addEntry(CStrWChar(L"offerItemId"), new CObjectMapInt(m_offerItemId));
    result->addEntry(CStrWChar(L"sourceGameId"), new CObjectMapInt(m_sourceGameId));
    return result;
}

CInviteOfferDataSetSelected::CInviteOfferDataSetSelected()
{
}

CObjectMapObject* CInviteOfferDataSetSelected::generateJSON() const
{
    CObjectMapObject* result = COfferDataSetSelected::generateJSON();
    for (int network = 0; network < SOCIAL_NETWORK_COUNT; ++network) {
        CObjectMapArray* friends = new CObjectMapArray;
        const CLinkList& ids = m_invitees[network].m_list;
        for (CLinkListNode* node = ids.GetHead(); node != NULL; node = node->GetNext()) {
            const CStrWChar* id = static_cast<const CStrWChar*>(node->GetData());
            if (id != NULL) {
                friends->addEntry(new CObjectMapString(*id));
            }
        }
        if (friends->m_values.Size() == 0) {
            delete friends;
            continue;
        }
        CObjectMapObject* networkData = new CObjectMapObject;
        networkData->addEntry(CStrWChar(L"socialNetworkTypeId"), new CObjectMapInt(network));
        networkData->addEntry(CStrWChar(L"friendIds"), friends);
        result->addEntry(CStrWChar(L"friendsPerSocialNetworkType"), networkData);
    }
    return result;
}

void CInviteOfferDataSetSelected::resetInvitees()
{
    for (int network = 0; network < SOCIAL_NETWORK_COUNT; ++network) {
        m_invitees[network].Clear();
    }
}

void CInviteOfferDataSetSelected::addInvitee(int socialNetworkType, CStrWChar friendId)
{
    if (socialNetworkType >= 0 && socialNetworkType < SOCIAL_NETWORK_COUNT) {
        m_invitees[socialNetworkType].Add(friendId);
    }
}

COfferDataGetIncentives::COfferDataGetIncentives()
    : m_socialNetworkTypeId(-1)
{
}

COfferDataGetIncentives::~COfferDataGetIncentives()
{
    m_socialNetworkTypeId = -1;
}

CObjectMapObject* COfferDataGetIncentives::generateJSON() const
{
    CObjectMapObject* result = new CObjectMapObject;
    generateBaseJSON(*result);
    result->addEntry(CStrWChar(L"socialNetworkTypeId"), new CObjectMapInt(m_socialNetworkTypeId));
    return result;
}

COfferDataSetRewarded::COfferDataSetRewarded()
{
}

COfferDataSetRewarded::~COfferDataSetRewarded()
{
}

CObjectMapObject* COfferDataSetRewarded::generateJSON() const
{
    CObjectMapObject* result = new CObjectMapObject;
    generateBaseJSON(*result);
    CObjectMapArray* rewards = new CObjectMapArray;
    for (int index = 0; index < m_userGameRewardIds.Size(); ++index) {
        rewards->addEntry(new CObjectMapInt(m_userGameRewardIds[index]));
    }
    result->addEntry(CStrWChar(L"userGameRewardIds"), rewards);
    return result;
}

COfferDataSetPurchased::COfferDataSetPurchased()
    : m_offerCategoryId(-1)
{
}

CObjectMapObject* COfferDataSetPurchased::generateJSON() const
{
    CObjectMapObject* result = new CObjectMapObject;
    generateBaseJSON(*result);
    result->addEntry(CStrWChar(L"offerCategoryId"), new CObjectMapInt(m_offerCategoryId));
    result->addEntry(CStrWChar(L"offerItemId"), new CObjectMapInt(m_offerItemId));
    return result;
}

COfferDataSetPurchasedInstall::COfferDataSetPurchasedInstall()
    : m_socialNetworkTypeId(-1)
{
    m_socialNetworkUserId = L"";
}

CObjectMapObject* COfferDataSetPurchasedInstall::generateJSON() const
{
    CObjectMapObject* result = COfferDataSetPurchased::generateJSON();
    result->addEntry(CStrWChar(L"socialNetworkTypeId"), new CObjectMapInt(m_socialNetworkTypeId));
    result->addEntry(CStrWChar(L"socialNetworkUserId"), new CObjectMapString(m_socialNetworkUserId));
    return result;
}

CObjectMapObject* COfferDataGetOffers::generateJSON() const
{
    CObjectMapObject* result = new CObjectMapObject;
    generateBaseJSON(*result);
    int width;
    int height;
    CUtil_Cpp::getDeviceScreen(&width, &height);
    result->addEntry(CStrWChar(L"screenWidth"), new CObjectMapInt(width));
    result->addEntry(CStrWChar(L"screenHeight"), new CObjectMapInt(height));
    return result;
}
