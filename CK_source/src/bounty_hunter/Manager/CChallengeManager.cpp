#include "CChallengeManager.h"

#include "App.h"
#include "CBH_Player.h"
#include "CChallenge.h"
#include "CChallengeStats.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSSession.h"
#include "CSaveManager.h"
#include "CStdUtil_Cocoa.h"
#include "CXmlHelper.h"
#include "ResMap.h"
#include <stdlib.h>

namespace {
const int SecondsPerDay = 86400;
const int ChallengeEpoch = 1312934400;
const int ChallengeTimeOffset = 32400;
}

CChallengeManager::CChallengeManager()
    : m_lastUpdateSecond(0), m_activeCount(0), m_reserved(0), m_finishTime(0), m_lastDay(0)
{
    ParseConfig();
    Load();
}

CChallengeManager::~CChallengeManager()
{
    SAFE_DELETE_VECTOR(m_challenges);
}

void CChallengeManager::ParseConfig()
{
    char* text = static_cast<App*>(WindowApp::m_instance)->InitResMap()->loadText(
        XString(L"res/common/configs/challenges.xml"));
    TiXmlDocument document;
    document.Parse(text, NULL, TIXML_ENCODING_UNKNOWN);
    if (text != NULL) {
        free(text);
    }
    // Original: the root node is not checked.
    TiXmlNode* root = document.FirstChild("challenges");
    for (TiXmlElement* element = root->FirstChildElement("challenge"); element != NULL;
         element = element->NextSiblingElement("challenge")) {
        CChallenge* challenge = new CChallenge(element);
        m_challenges.addElement(challenge);
        if (m_lastDay < challenge->m_day) {
            m_lastDay = challenge->m_day;
        }
    }
}

void CChallengeManager::Load()
{
    TiXmlElement* root = static_cast<App*>(WindowApp::m_instance)->m_saveManager->GetRootElement();
    // Original bug: the next section is looked up among the root's siblings, not the section's.
    for (TiXmlElement* section = root->FirstChildElement("challenges"); section != NULL;
         section = root->NextSiblingElement("challenges")) {
        m_activeCount = CXmlHelper::GetAttributeValueOrUseDefault(section, "active_challenges_count", XString(0)).ToInt();
        m_finishTime = CXmlHelper::GetAttributeValueOrUseDefault(section, "challenges_finish_time", XString(0)).ToInt();
        for (int index = 0; index < m_challenges.size(); ++index) {
            m_challenges[index]->Load(section);
        }
    }
}

void CChallengeManager::Update()
{
    // the original only checks the active challenges (no per-second throttle here)
    for (int index = 0; index < m_challenges.size(); ++index) {
        if (m_challenges[index]->m_active) {
            m_challenges[index]->CheckCompletion();
        }
    }
}

Vector<CChallenge*> CChallengeManager::GetActiveChallenges()
{
    CheckChallengesFinishByTime();
    Vector<CChallenge*> active;
    for (int index = 0; index < m_challenges.size(); ++index) {
        CChallenge* challenge = m_challenges[index];
        if (challenge->m_active) {
            challenge->CheckOverride();
            active.addElement(challenge);
        }
    }
    return active;
}

void CChallengeManager::CheckChallengesFinishByTime()
{
    // the original compares unsigned and deactivates the expired challenges here
    unsigned int now = GetTimeSec();
    if (now == 0) {
        return;
    }
    if (m_activeCount != 0) {
        if (m_activeCount < 1 || now <= static_cast<unsigned int>(m_finishTime)) {
            return;
        }
        for (int index = 0; index < m_challenges.size(); ++index) {
            if (m_challenges[index]->m_active) {
                m_challenges[index]->m_active = false;
            }
        }
    }
    m_finishTime = now + SecondsPerDay - now % SecondsPerDay;
    StartNewChallenges(now);
}

void CChallengeManager::Save()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CSaveManager* save = app->m_saveManager;
    TiXmlElement* section = CXmlHelper::ObtainElement(save->GetRootElement(), XString("challenges"));
    section->Clear();
    section->SetAttribute("active_challenges_count", m_activeCount);
    section->SetAttribute("challenges_finish_time", m_finishTime);
    for (int index = 0; index < m_challenges.size(); ++index) {
        if (m_challenges[index]->m_active) {
            m_challenges[index]->Save(section);
        }
    }
    save->SaveOnNextUpdate(false);
}

int CChallengeManager::GetTimeSec()
{
    if (!App::IsNetTimeAvailable()) {
        return 0;
    }
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    if (!user->isAuthenticated(2) && !user->isAuthenticated(1)) {
        return 0;
    }
    return user->GetSession()->getNetworkCurrentTimeSeconds() - ChallengeTimeOffset;
}

int CChallengeManager::DayForTime(int time) const
{
    // the original does not check the epoch
    if (m_lastDay == 0) {
        return 0;
    }
    return (time - ChallengeEpoch) / SecondsPerDay % (m_lastDay + 1);
}

int CChallengeManager::GetCurrentChallengeDay()
{
    return DayForTime(GetTimeSec());
}

void CChallengeManager::StartNewChallenges(int time)
{
    // the original only resets the challenge stats (no hunt re-registration, no player save)
    CBH_Player::GetInstance()->m_challengeStats->ResetStats();
    int day = DayForTime(time);
    Vector<CChallenge*> active;
    GetChallengesForDay(day, active);
    // the original stores the finish time inside the loop (only reached when a challenge starts)
    for (int index = 0; index < active.size(); ++index) {
        m_finishTime = (time / SecondsPerDay + 1) * SecondsPerDay;
        active[index]->Start(m_finishTime);
    }
    m_activeCount = active.size();
    Save();
}

void CChallengeManager::GetChallengesForDay(int day, Vector<CChallenge*>& challenges)
{
    for (int index = 0; index < m_challenges.size(); ++index) {
        CChallenge* challenge = m_challenges[index];
        if (challenge->m_day == day) {
            challenges.addElement(challenge);
        }
    }
}

CChallenge* CChallengeManager::GetChallengeById(int id)
{
    CheckChallengesFinishByTime();
    if (id > m_challenges.size()) {
        return NULL;
    }
    for (int index = 0; index < m_challenges.size(); ++index) {
        CChallenge* challenge = m_challenges[index];
        if (challenge->m_id == id) {
            challenge->CheckOverride();
            return challenge;
        }
    }
    return NULL;
}

bool CChallengeManager::IsChallengeActive(int id)
{
    // the original neither refreshes the timers nor checks for NULL here
    return GetChallengeById(id)->m_active;
}

int CChallengeManager::GetChallengesFinishTime()
{
    // the original reads the finish time of the first active challenge
    int finishTime = 0;
    Vector<CChallenge*> active = GetActiveChallenges();
    if (active.size() > 0) {
        finishTime = active[0]->m_finishTime;
    }
    return finishTime;
}

typedef char CKChallengeManagerSize[sizeof(CChallengeManager) == 0x24 ? 1 : -1];
