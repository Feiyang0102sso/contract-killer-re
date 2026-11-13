#undef memset
#include <string.h>

#include "CTrickShotManager.h"

#include "App.h"
#include "CBH_Player.h"
#include "CChallengeStats.h"
#include "CDH_SoundManager.h"
#include "CGameAnalytics.h"
#include "CPopupController.h"

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
    } else if (count >= 2) {
        RegisterEvent(DoubleKill);
    } else {
        return;
    }
    ++CBH_Player::GetInstance()->m_challengeStats->m_multiKills;
}

void CTrickShotManager::RegisterEvent(EEvent event)
{
    if (static_cast<unsigned int>(event) > 10) {
        return;
    }
    m_events[event] = true;
    ShowMessage(GetEventName(event), GetEventMultiplier(event));
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
    } else {
        return;
    }
    ++CBH_Player::GetInstance()->m_challengeStats->m_multiKills;
}

void CTrickShotManager::ShowMessage(const XString& name, int multiplier)
{
    XString points = XString::Format(Window::ResString("IDS_POPUP_XP_MULTIPLIER").c_str(), multiplier);
    static_cast<App*>(WindowApp::m_instance)->m_popupController->AddTrickShotPopup(name, points);
}

XString CTrickShotManager::GetEventName(EEvent event)
{
    const char* name;
    switch (event) {
    case RunningRed:
        name = "IDS_GAME_PLAY_TRICKSHOT_RUNNING_RED";
        break;
    case BullsEye:
        name = "IDS_GAME_PLAY_TRICKSHOT_BULLS_EYE";
        break;
    case NapTime:
        name = "IDS_GAME_PLAY_TRICKSHOT_NAP_TIME";
        break;
    case Revenge:
        name = "IDS_GAME_PLAY_TRICKSHOT_REVENGE";
        break;
    case LongShot:
        name = "IDS_GAME_PLAY_TRICKSHOT_LONGSHOT";
        break;
    case SeekingBullet:
        name = "IDS_GAME_PLAY_TRICKSHOT_SEEKING_BULLET";
        break;
    case FastShot:
        name = "IDS_GAME_PLAY_TRICKSHOT_FAST_SHOT";
        break;
    case DoubleKill:
        name = "IDS_GAME_PLAY_TRICKSHOT_DOUBLE_KILL";
        break;
    case TripleKill:
        name = "IDS_GAME_PLAY_TRICKSHOT_TRIPLE_KILL";
        break;
    case DoubleExplosionKill:
        name = "IDS_GAME_PLAY_TRICKSHOT_DOUBLE_KILL_BY_EXPLOSION";
        break;
    case TripleExplosionKill:
        name = "IDS_GAME_PLAY_TRICKSHOT_TRIPLE_KILL_BY_EXPLOSION";
        break;
    default:
        name = "IDS_GAME_PLAY_TRICKSHOT_RUNNING_RED";
        break;
    }
    return Window::ResString(name);
}

int CTrickShotManager::GetMultiplier()
{
    // Original: kill multiplier * explosion multiplier * every registered event's multiplier.
    int killMultiplier = 3;
    if (m_killCount < 3) {
        killMultiplier = 1;
        if (m_killCount >= 2) {
            killMultiplier = 2;
        }
    }
    int explosionMultiplier = 3;
    if (m_explosionKillCount < 3) {
        explosionMultiplier = 1;
        if (m_explosionKillCount == 2) {
            explosionMultiplier = 2;
        }
    }
    int multiplier = explosionMultiplier * killMultiplier;
    for (unsigned int index = 0; index < EventCount; ++index) {
        multiplier = GetEventMultiplier(index) * multiplier;
    }
    return multiplier;
}

typedef char CKTrickShotManagerSize[sizeof(CTrickShotManager) == 0x14 ? 1 : -1];
