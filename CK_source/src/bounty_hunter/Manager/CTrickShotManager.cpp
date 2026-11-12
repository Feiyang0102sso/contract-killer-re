#include "CTrickShotManager.h"

#include "App.h"
#include "CBH_Player.h"
#include "CChallengeStats.h"
#include "CDH_SoundManager.h"
#include "CGameAnalytics.h"
#include "CPopupController.h"

namespace {
const int EventMultipliers[] = { 2, 3, 2, 2, 3, 2, 3, 2, 3, 2, 3 };
}

CTrickShotManager::CTrickShotManager() : m_killCount(0), m_explosionKillCount(0)
{
    ResetEvents();
}

void CTrickShotManager::ResetEvents()
{
    memset(m_events, 0, sizeof(m_events));
}

void CTrickShotManager::SetKillCount(int count)
{
    m_killCount = count;
    m_explosionKillCount = 0;
    if (count >= 3) {
        RegisterEvent(TripleKill);
    } else if (count == 2) {
        RegisterEvent(DoubleKill);
    }
    if (count >= 2) {
        ++CBH_Player::GetInstance()->m_challengeStats->m_multiKills;
    }
}

void CTrickShotManager::RegisterEvent(EEvent event)
{
    if (static_cast<unsigned int>(event) >= EventCount) {
        return;
    }
    m_events[event] = true;
    ShowMessage(GetEventName(event), EventMultipliers[event]);
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_soundManager->PlayRandomSound("KEYSET_SOUND_TRICKSHOT_ACHIEVED", false);
    CGameAnalytics::logTrickShotEarned(GetEventName(event));
}

void CTrickShotManager::SetKillByExplosionCount(int count)
{
    m_killCount = 0;
    m_explosionKillCount = count;
    if (count >= 3) {
        RegisterEvent(TripleExplosionKill);
    } else if (count == 2) {
        RegisterEvent(DoubleExplosionKill);
    }
    if (count >= 2) {
        ++CBH_Player::GetInstance()->m_challengeStats->m_multiKills;
    }
}

void CTrickShotManager::ShowMessage(const XString& name, int multiplier)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    XString format = Window::ResString("IDS_POPUP_XP_MULTIPLIER");
    XString points = XString::Format(format.c_str(), multiplier);
    app->m_popupController->AddTrickShotPopup(name, points);
}

XString CTrickShotManager::GetEventName(EEvent event)
{
    const char* name = "IDS_GAME_PLAY_TRICKSHOT_RUNNING_RED";
    switch (event) {
    case BullsEye: name = "IDS_GAME_PLAY_TRICKSHOT_BULLS_EYE"; break;
    case NapTime: name = "IDS_GAME_PLAY_TRICKSHOT_NAP_TIME"; break;
    case Revenge: name = "IDS_GAME_PLAY_TRICKSHOT_REVENGE"; break;
    case LongShot: name = "IDS_GAME_PLAY_TRICKSHOT_LONGSHOT"; break;
    case SeekingBullet: name = "IDS_GAME_PLAY_TRICKSHOT_SEEKING_BULLET"; break;
    case FastShot: name = "IDS_GAME_PLAY_TRICKSHOT_FAST_SHOT"; break;
    case DoubleKill: name = "IDS_GAME_PLAY_TRICKSHOT_DOUBLE_KILL"; break;
    case TripleKill: name = "IDS_GAME_PLAY_TRICKSHOT_TRIPLE_KILL"; break;
    case DoubleExplosionKill: name = "IDS_GAME_PLAY_TRICKSHOT_DOUBLE_KILL_BY_EXPLOSION"; break;
    case TripleExplosionKill: name = "IDS_GAME_PLAY_TRICKSHOT_TRIPLE_KILL_BY_EXPLOSION"; break;
    default: break;
    }
    return Window::ResString(name);
}

int CTrickShotManager::GetMultiplier()
{
    int multiplier = 1;
    if (m_killCount >= 3 || m_explosionKillCount >= 3) {
        multiplier = 3;
    } else if (m_killCount == 2 || m_explosionKillCount == 2) {
        multiplier = 2;
    }
    for (int index = 0; index < EventCount; ++index) {
        if (m_events[index]) {
            multiplier *= EventMultipliers[index];
        }
    }
    return multiplier;
}

typedef char CKTrickShotManagerSize[sizeof(CTrickShotManager) == 0x14 ? 1 : -1];
