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
    TiXmlDocument document = CXmlHelper::Parse(L"res/common/configs/challenges.xml");
    TiXmlElement* root = document.FirstChildElement("challenges");
    if (root == NULL) {
        return;
    }
    for (TiXmlElement* element = root->FirstChildElement("challenge"); element != NULL;
         element = element->NextSiblingElement("challenge")) {
        CChallenge* challenge = new CChallenge(element);
        m_challenges.addElement(challenge);
        if (challenge->m_day > m_lastDay) {
            m_lastDay = challenge->m_day;
        }
    }
}

void CChallengeManager::Load()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    TiXmlElement* root = app->m_saveManager->GetRootElement();
    TiXmlElement* section = root->FirstChildElement("challenges");
    if (section == NULL) {
        return;
    }
    m_finishTime = CXmlHelper::GetIntAttributeValueOrUseDefault(section, "challenges_finish_time", 0);
    m_activeCount = 0;
    for (int index = 0; index < m_challenges.size(); ++index) {
        CChallenge* challenge = m_challenges[index];
        challenge->Load(section);
        if (challenge->m_active) {
            ++m_activeCount;
        }
    }
}

void CChallengeManager::Update()
{
    unsigned int now = CStdUtil_Cocoa::GetTimeSeconds();
    if (now == m_lastUpdateSecond) {
        return;
    }
    m_lastUpdateSecond = now;
    CheckChallengesFinishByTime();
    for (int index = 0; index < m_challenges.size(); ++index) {
        m_challenges[index]->CheckCompletion();
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
    int now = GetTimeSec();
    if (now == 0 || (m_activeCount > 0 && now <= m_finishTime)) {
        return;
    }
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
    if (time < ChallengeEpoch || m_lastDay == 0) {
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
    for (int index = 0; index < m_challenges.size(); ++index) {
        m_challenges[index]->m_active = false;
    }
    Vector<CChallenge*> active;
    GetChallengesForDay(DayForTime(time), active);
    m_finishTime = (time / SecondsPerDay + 1) * SecondsPerDay;
    for (int index = 0; index < active.size(); ++index) {
        active[index]->Start(m_finishTime);
    }
    m_activeCount = active.size();
    CBH_Player* player = CBH_Player::GetInstance();
    CChallengeStats* stats = player->m_challengeStats;
    bool huntActive = stats->m_huntActive;
    XString location = stats->m_location;
    stats->ResetStats();
    if (huntActive) {
        stats->RegisterHuntStart(location);
    }
    Save();
    player->Save();
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
    for (int index = 0; index < m_challenges.size(); ++index) {
        if (m_challenges[index]->m_id == id) {
            m_challenges[index]->CheckOverride();
            return m_challenges[index];
        }
    }
    return NULL;
}

bool CChallengeManager::IsChallengeActive(int id)
{
    CheckChallengesFinishByTime();
    CChallenge* challenge = GetChallengeById(id);
    return challenge != NULL && challenge->m_active;
}

int CChallengeManager::GetChallengesFinishTime()
{
    CheckChallengesFinishByTime();
    return m_finishTime;
}

typedef char CKChallengeManagerSize[sizeof(CChallengeManager) == 0x24 ? 1 : -1];
