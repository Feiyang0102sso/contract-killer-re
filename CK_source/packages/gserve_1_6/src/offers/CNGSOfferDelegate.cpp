// CNGSOfferDelegate.cpp - client side steps of the gOffer offers (invites, featured game).
#include "CNGSOfferDelegate.h"
#include "CNGSOfferManager.h"
#include "CNGSOfferRequest.h"
#include "CNGSDataOffer.h"
#include "CNGS_Platform.h"
#include "CNGSSocialInterface.h"



/** `text` when it is not empty, NULL otherwise (optional arguments of postMessage). */
static inline const CStrWChar* NonEmpty(const CStrWChar& text)
{
    // ?: keeps the original select (it eq; movs rX, #0)
    return text.GetLength() == 0 ? NULL : &text;
}

PostMessageDelegate::PostMessageDelegate()
{
    m_posting = 0;
    m_networkType = -1;
}

void PostMessageDelegate::PostMessage()
{
    if (m_posting) {
        return;
    }
    CNGSSocialInterface* social = CNGS_Platform::GetInstance()->getSocialInterface(m_networkType);
    social->postMessage(NonEmpty(m_message), &m_recipientId, NonEmpty(m_link), NonEmpty(m_name),
                        NonEmpty(m_caption), NonEmpty(m_description), NonEmpty(m_picture), NonEmpty(m_privacy));
    m_posting = 1;
}

void PostMessageDelegate::handlePostMessage(unsigned char success)
{
    m_posting = 0;
}

InviteDelegate::InviteDelegate()
{
    m_state = STATE_IDLE;
    m_offer = NULL;
    m_selectedData = new CInviteOfferDataSetSelected();
}

InviteDelegate::~InviteDelegate()
{
    if (m_selectedData != NULL) {
        delete m_selectedData;
        m_selectedData = NULL;
    }
}

void InviteDelegate::handlePostMessage(unsigned char success)
{
    m_posting = 0;
    if (m_state != STATE_POSTING) {
        return;
    }
    if (success) {
        ((CInviteOfferDataSetSelected*)m_selectedData)->addInvitee(m_networkType, m_recipientId);
        m_offer->addPreviouslyInvitedFriend(m_networkType, m_recipientId);
        m_invitesSent++;
    }
    sendNextInvite();
}

void InviteDelegate::sendNextInvite()
{
    m_networkType = m_offer->getPotentialInvitee(&m_recipientId);
    if (m_networkType != -1) {
        m_state = STATE_POSTING;
        PostMessage();
    } else {
        // Nobody left: report how many invites went out (7 = none at all).
        m_state = STATE_READY;
        int status = 0;
        if (m_invitesSent < 1) {
            status = 7;
        }
        COfferManager::GetInstance()->finishedExecuting(1, status, m_invitesSent);
    }
}

int InviteDelegate::queueInvite(int socialNetworkType, const CStrWChar* friendId, unsigned char repeat)
{
    if ((m_state != STATE_READY && m_state != 2) || m_offer == NULL) {
        return 10;
    }
    if (COfferManager::GetInstance()->isAlreadyPlayingGame(socialNetworkType, friendId)) {
        return 9;
    }
    if (!repeat && m_offer->wasPreviouslyInvited(socialNetworkType, *friendId)) {
        return 8;
    }
    if (m_offer->getNumQueuedInviteesInSN(socialNetworkType) >= 100) {
        return 12;
    }
    m_offer->addPotentialInvitee(socialNetworkType, *friendId);
    m_invitesRequested = 0;
    for (int network = 0; network < CInviteOffer::SOCIAL_NETWORK_COUNT; ++network) {
        m_invitesRequested += m_offer->getNumQueuedInviteesInSN(network);
    }
    return 0;
}

namespace {
const wchar_t* InviteText(const CStrWChar* text)
{
    if (text != NULL) {
        return text->GetWCharPtr();
    }
    return L"";
}
}

void InviteDelegate::setInviteParams(const CStrWChar* message, const CStrWChar* link, const CStrWChar* name,
                                     const CStrWChar* caption, const CStrWChar* description,
                                     const CStrWChar* picture, const CStrWChar* privacy)
{
    m_name = InviteText(name);
    m_caption = InviteText(caption);
    m_message = InviteText(message);
    m_link = InviteText(link);
    m_description = InviteText(description);
    m_picture = InviteText(picture);
    m_privacy = InviteText(privacy);
}

int InviteDelegate::execute()
{
    int result = 3;
    if (m_state == STATE_READY && !m_posting) {
        result = 6;
        if (m_invitesRequested >= 1) {
            m_state = STATE_RUNNING;
            ((CInviteOfferDataSetSelected*)m_selectedData)->resetInvitees();
            m_invitesRequested = 0;
            m_invitesSent = 0;
            sendNextInvite();
            result = 0;
        }
    }
    return result;
}

FeaturedGameDelegate::FeaturedGameDelegate()
{
    m_state = STATE_READY;
    m_value10 = 0;
    m_selectedData = new COfferDataSetSelected();
    m_busy = 0;
}

FeaturedGameDelegate::~FeaturedGameDelegate()
{
    if (m_selectedData != NULL) {
        delete m_selectedData;
        m_selectedData = NULL;
    }
}

int FeaturedGameDelegate::execute()
{
    int result = 3;
    if (m_state == STATE_READY && !m_busy) {
        m_state = STATE_READY;
        m_selectedData->m_featureType = CDataOffer::FEATURE_TYPE_FEATURED_GAME;
        COfferManager::GetInstance()->finishedExecuting(2, 0, 0);
        result = 0;
    }
    return result;
}
