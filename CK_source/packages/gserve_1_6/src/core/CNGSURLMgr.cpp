// CNGSURLMgr.cpp - environment and service paths used by CK's gServe clients.
#include "CNGSURLMgr.h"
#include <string.h>

static const char kGOfferBase[] = "http://sc-goffer-stage:8080/gOffer/";
static const char kLeaderboardBase[] = "http://ng-leaderboard.glu.com/ngleaderboard/rest/";
static const char kAlternateLeaderboardBase[] = "http://ng-leaderboard.glu.com/rngleaderboard/est/";
static const char* const kOfferPaths[GOFFER_SERVICE_COUNT] = {
    "resources/offer/v1/sourcegameoffers",
    "resources/offer/v1/offerselected",
    "resources/offer/v1/offerpurchased",
    "resources/offer/v1/offerrewarded",
    "resources/offer/v1/incentive",
    "resources/offer/v1/offernetworks",
    "resources/offer/v1/sourcegameoffernetworks",
    "resources/offer/v1/invitedfriendsforgame"
};

CNGSURLMgr::CNGSURLMgr()
    : CSingleton(ClassId_CNGSURLMgr)
{
    memset(m_baseURLs, 0, sizeof(m_baseURLs));
    for (int environment = 0; environment < SERVER_ENV_COUNT; ++environment) {
        m_gOfferBaseURLs[environment] = new CStrChar(kGOfferBase);
        m_baseURLs[SERVER_BASE_URL_GOFFER][environment] = m_gOfferBaseURLs[environment]->GetCharPtr();
        m_baseURLs[SERVER_BASE_URL_LEADERBOARD][environment] = kLeaderboardBase;
    }
    m_baseURLs[SERVER_BASE_URL_LEADERBOARD][SERVER_ENV_3] = kAlternateLeaderboardBase;
    m_serverEnvironment = SERVER_ENV_1;
    for (int service = 0; service < GOFFER_SERVICE_COUNT; ++service) {
        m_gOfferServices[service] = kOfferPaths[service];
    }
    for (int path = 0; path < 6; ++path) {
        int service = path;
        if (path == 5) {
            service = GOFFER_INVITED_FRIENDS_FOR_GAME;
        }
        m_gOfferServicePaths[path] = new CStrChar(kOfferPaths[service]);
        m_gOfferServices[service] = m_gOfferServicePaths[path]->GetCharPtr();
    }
    m_leaderboardServices[0] = "leaderboard/v1/getrank";
    m_leaderboardServices[1] = "leaderboard/v1/getfriendsranks";
    m_leaderboardServices[2] = "leaderboard/v1/updatescore";
    m_leaderboardServices[3] = "leaderboard/v1/replacescore";
    m_leaderboardServices[4] = "leaderboard/v1/listranks";
    m_leaderboardServices[5] = NULL;
    m_leaderboardServices[6] = NULL;
    m_leaderboardServices[7] = "leaderboard/v1/lastscoreupdate";
}

CNGSURLMgr::~CNGSURLMgr()
{
    for (int environment = 0; environment < SERVER_ENV_COUNT; ++environment) {
        const char* base = m_baseURLs[SERVER_BASE_URL_GOFFER][environment];
        if (base != m_gOfferBaseURLs[environment]->GetCharPtr()) {
            delete[] base;
        }
        base = m_baseURLs[SERVER_BASE_URL_LEADERBOARD][environment];
        if (base != kLeaderboardBase && base != kAlternateLeaderboardBase) {
            delete[] base;
        }
        delete[] m_baseURLs[SERVER_BASE_URL_1][environment];
        delete m_gOfferBaseURLs[environment];
    }
    for (int path = 0; path < 6; ++path) {
        delete m_gOfferServicePaths[path];
    }
}

void CNGSURLMgr::setServerEnvironment(eServerEnvType environment)
{
    m_serverEnvironment = environment;
}

CStrChar CNGSURLMgr::getBaseURL(eServerBaseURLType type)
{
    // no range checks in the original
    CStrChar result;
    const char* url = m_baseURLs[type][m_serverEnvironment];
    if (url != NULL) {
        result = url;
    }
    return result;
}

CStrChar CNGSURLMgr::getGOfferURL(eGOfferServiceType service)
{
    CStrChar result = getBaseURL(SERVER_BASE_URL_GOFFER);
    result.Concatenate(m_gOfferServices[service]);
    return result;
}

void CNGSURLMgr::setBaseURL(eServerBaseURLType type, eServerEnvType environment, const char* url)
{
    if (type < 0 || type >= SERVER_BASE_URL_COUNT || environment < 0 ||
        environment >= SERVER_ENV_COUNT || url == NULL) {
        return;
    }
    char* copy = new char[strlen(url) + 1];
    if (copy == NULL) {
        return;
    }
    strcpy(copy, url);
    const char*& base = m_baseURLs[type][environment];
    if (base != m_gOfferBaseURLs[environment]->GetCharPtr() &&
        base != kLeaderboardBase && base != kAlternateLeaderboardBase) {
        delete[] base;
    }
    base = copy;
}
