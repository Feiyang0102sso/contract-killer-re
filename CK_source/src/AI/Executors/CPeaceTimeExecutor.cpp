#include "CPeaceTimeExecutor.h"

#include "App.h"
#include "CBH_PersonType.h"
#include "CBH_Player.h"
#include "CAlertExecutor.h"
#include "CDH_SoundManager.h"
#include "CMovementExecutor.h"
#include "CPeaceIdleExecutor.h"
#include "CStdUtil_Cocoa.h"
#include "CSwerveGame.h"
#include "CUnit.h"

bool CPeaceTimeExecutor::m_alertSoundPlayed;

CPeaceTimeExecutor::CPeaceTimeExecutor(CUnitMind* mind)
    : IExecutor(mind), m_alertTime(0), m_idleNext(true), m_alerted(false), m_tryAttackPoint(false), m_ticks(0)
{
    m_alertSoundPlayed = false;
}

void CPeaceTimeExecutor::Tick(int)
{
    CSwerveGame* game = CSwerveGame::m_pInstance;
    if (m_ticks < 2) {
        ++m_ticks;
    }
    if (game == NULL || m_mind == NULL || (!game->m_missionAlert && game->m_peaceTime)) {
        return;
    }
    if (!m_alerted && (game->m_missionAlert || m_mind->GetUnit()->m_type != UnitCivilian)) {
        if (m_ticks < 2) {
            Finish(EXECUTOR_RESULT_STUN_OVER);
            return;
        }
        CancelChildren();
        m_alerted = true;
        m_alertTime = CStdUtil_Cocoa::GetUpTimeMS();
        AddAlertExecutor(m_mind);
    }
    if (!m_alertSoundPlayed && CStdUtil_Cocoa::GetUpTimeMS() - m_alertTime >= 801) {
        PlayAlertSound();
        m_alertSoundPlayed = true;
    }
}

void CPeaceTimeExecutor::PlayAlertSound()
{
    if (CBH_Player::GetInstance()->IsSpeedMode()) {
        return;
    }
    CUnit* unit = m_mind->GetUnit();
    const char* sound = "KEYSET_SOUND_CIVIC_ALERT";
    if (unit->m_type != UnitCivilian) {
        sound = "KEYSET_SOUND_WOMEN_ALERT";
        if (unit->m_body.m_type->m_visualCharacter->m_isMan) {
            sound = "KEYSET_SOUND_BAD_GUY_ALERT";
        }
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_soundManager->PlayRandomSound(sound, true);
}

void CPeaceTimeExecutor::OnReceiveDamage()
{
    Finish(EXECUTOR_RESULT_STUN_OVER);
}

void CPeaceTimeExecutor::OnSeeUnit(const CUnit* unit)
{
    if (unit != NULL && unit->m_invulnerable) {
        Finish(EXECUTOR_RESULT_STUN_OVER);
    }
}

void CPeaceTimeExecutor::OnChildFinished(EExecutosResult result)
{
    if (m_alerted) {
        Finish(EXECUTOR_RESULT_STUN_OVER);
    } else if (result == EXECUTOR_RESULT_SUCCESS) {
        ChooseFutherAction();
    } else {
        SetPeaceIdleBehaviour();
    }
}

void CPeaceTimeExecutor::ChooseFutherAction()
{
    bool idle = m_idleNext;
    m_idleNext = !m_idleNext;
    if (idle) {
        SetPeaceIdleBehaviour();
    } else {
        MoveToNewPoint();
    }
}

void CPeaceTimeExecutor::OnStart()
{
    ChooseFutherAction();
}

void CPeaceTimeExecutor::MoveToNewPoint()
{
    CUnitBody* body = m_mind->GetUnit()->GetBody();
    const CGameAIMap* map = body->m_map;
    if (map == NULL) {
        SetPeaceIdleBehaviour();
        return;
    }
    int destination = body->m_pointId;
    int camera = CSwerveGame::m_pInstance->GetCurrCamId();
    if (m_tryAttackPoint) {
        map->FindAnotherAttackPoint(camera, &destination, body->m_pointId);
    } else {
        map->FindAnotherPoint(&destination, body->m_pointId);
    }
    m_tryAttackPoint = !m_tryAttackPoint;
    AddMovementExecutor(m_mind, body->m_cellId, destination, false);
}

void CPeaceTimeExecutor::SetPeaceIdleBehaviour()
{
    AddPeaceIdleExecutor(m_mind);
}

const wchar_t* CPeaceTimeExecutor::GetName() const
{
    return L"CPeaceTimeExecutor";
}

void AddPeaceTimeExecutor(CUnitMind* mind)
{
    if (mind != NULL) {
        CPeaceTimeExecutor* executor = new CPeaceTimeExecutor(mind);
        mind->AddExecutor(executor);
        executor->OnStart();
    }
}

typedef char AssertPeaceTimeSize[sizeof(CPeaceTimeExecutor) == 0x18 ? 1 : -1];
