// CNGSDataOffer.cpp - gOffer offer records.
#include "CNGSDataOffer.h"
#include "CNGSUtil.h"
#include <string.h>

CDataOffer::CDataOffer()
    : m_gameId(0), m_value34(0), m_offerItemId(0), m_sourceGameId(0)
{
}


CDataOffer::~CDataOffer()
{
}

CInviteOffer::CInviteOffer()
    : m_populatedNetworks(0)
{
}

void CInviteOffer::addPreviouslyInvitedFriend(int socialNetworkType, const CStrWChar& friendId)
{
    if (friendId.GetLength() != 0) {
        m_previouslyInvited[socialNetworkType].Add(friendId);
    }
}

void CInviteOffer::addPotentialInvitee(int socialNetworkType, const CStrWChar& friendId)
{
    if (friendId.GetLength() != 0) {
        m_potentialInvitees[socialNetworkType].Add(friendId);
    }
}

int CInviteOffer::getNumInviteesInSN(int socialNetworkType)
{
    return m_previouslyInvited[socialNetworkType].Size();
}

int CInviteOffer::getNumQueuedInviteesInSN(int socialNetworkType)
{
    return m_potentialInvitees[socialNetworkType].Size();
}

unsigned char CInviteOffer::wasPreviouslyInvited(int socialNetworkType, const CStrWChar* friendId)
{
    if (friendId == NULL) {
        return 0;
    }
    return m_previouslyInvited[socialNetworkType].Exists(*friendId);
}

unsigned char CInviteOffer::wasPreviouslyInvited(int socialNetworkType, const CStrWChar& friendId)
{
    return m_previouslyInvited[socialNetworkType].Exists(friendId) != 0;
}

int CDataOffer::hasFriendInviteIncentive()
{
    for (int index = 0; index < m_incentives.Size(); index++) {
        if (m_incentives[index].m_offerCategoryId == 1) {
            return index;
        }
    }
    return -1;
}

int CDataOffer::hasFriendInstallIncentive()
{
    for (int index = 0; index < m_incentives.Size(); ++index) {
        if (m_incentives[index].m_offerCategoryId == 2) {
            return index;
        }
    }
    return -1;
}

bool CInviteOffer::isPreviouslyInvitedFriendsListPopulated(int socialNetworkType)
{
    // signed test as in the original (bit 31 counts as not populated)
    return ((int)m_populatedNetworks & (1 << socialNetworkType)) > 0;
}

void CInviteOffer::setPreviouslyInvitedFriendsListPopulated(int socialNetworkType, unsigned char populated)
{
    unsigned int network = 1U << socialNetworkType;
    if (populated) {
        m_populatedNetworks |= network;
    } else {
        m_populatedNetworks &= ~network;
    }
}

int CInviteOffer::getPotentialInvitee(CStrWChar* friendId)
{
    for (int network = 0; network < SOCIAL_NETWORK_COUNT; network++) {
        CFriendInviteeList& invitees = m_potentialInvitees[network];
        if (invitees.Size() > 0 && invitees.m_list.m_head != NULL) {
            CStrWChar* first = (CStrWChar*)invitees.m_list.m_head->m_data;
            *friendId = *first;
            invitees.RemoveElement(*friendId);
            return network;
        }
    }
    return -1;
}

CFeaturedInstallOffer::~CFeaturedInstallOffer()
{
    delete[] m_image;
}

void CFeaturedInstallOffer::setImage(const char* image)
{
    delete[] m_image;
    m_image = NULL;
    m_imageSize = strlen(image);
    if (m_imageSize > 0) {
        m_image = new unsigned char[m_imageSize];
        if (m_image != NULL) {
            CNGSUtil::Base64Decode(m_image, reinterpret_cast<const unsigned char*>(image), m_imageSize);
        }
    }
}
