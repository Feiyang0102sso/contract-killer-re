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
#include "DGHelper.h"

bool CPeaceTimeExecutor::m_alertSoundPlayed;

CPeaceTimeExecutor::CPeaceTimeExecutor(CUnitMind* mind)
    : IExecutor(mind), m_idleNext(true), m_alerted(false), m_tryAttackPoint(false), m_ticks(0)
{
    // the original constructor does not set m_alertTime
    m_alertSoundPlayed = false;
}

void CPeaceTimeExecutor::Tick(int)
{
    // Original 0xd14fc: after adding the alert child it falls through to the alert sound check
    // (our old early finish of an animation-less alert child was not in the original).
    CSwerveGame* game = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame;
    bool missionAlert = game->m_missionAlert;
    bool peaceTime = game->m_peaceTime;
    int ticks = m_ticks;
    if (ticks < 2) {
        ticks++;
        m_ticks = ticks;
    }
    if ((missionAlert | !peaceTime) && m_mind != NULL) {
        bool unitIsHostile = m_mind->GetUnit()->m_type != UnitCivilian;
        if ((missionAlert | (unitIsHostile & !peaceTime)) && !m_alerted) {
            if (ticks >= 2) {
                CancelChildren();
                m_alerted = true;
                AddAlertExecutor(m_mind);
                m_alertTime = CStdUtil_Cocoa::GetUpTimeMS();
            } else {
                Finish(EXECUTOR_RESULT_STUN_OVER);
                return;
            }
        }
        unsigned int now = CStdUtil_Cocoa::GetUpTimeMS();
        if (!m_alertSoundPlayed) {
            if (now - m_alertTime < 801) {
                return;
            }
            PlayAlertSound();
            m_alertSoundPlayed = true;
        }
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
    // the original makes the call first and toggles the flag afterwards
    if (m_idleNext) {
        SetPeaceIdleBehaviour();
    } else {
        MoveToNewPoint();
    }
    m_idleNext = !m_idleNext;
}

void CPeaceTimeExecutor::OnStart()
{
    ChooseFutherAction();
}

void CPeaceTimeExecutor::MoveToNewPoint()
{
    // Original 0xd16a8: the walk starts from the cell of the current point (not the body's cell);
    // while units are pre-simulated only ordinary points are used.
    CUnit* unit = m_mind->GetUnit();
    if (unit == NULL || unit->GetBody() == NULL) {
        return;
    }
    CUnitBody* body = unit->GetBody();
    int point = body->m_pointId;
    int camera = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->GetCurrCamId();
    int destination;
    bool found;
    if (!m_tryAttackPoint || static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_preSimulatingUnits) {
        found = body->m_map->FindAnotherPoint(&destination, point);
    } else {
        found = body->m_map->FindAnotherAttackPoint(camera, &destination, point);
    }
    if (!found) {
        destination = point;
    }
    int cell = body->m_map->GetCellIdByPointId(point);
    AddMovementExecutor(m_mind, cell, destination, false);
    m_tryAttackPoint = !m_tryAttackPoint;
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
