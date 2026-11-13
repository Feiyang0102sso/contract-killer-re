// CSwerveGame.cpp - mission window and result handling from the CK main program.
#include "CSwerveGame.h"
#include "CDH_PlayerData.h"

#include "App.h"
#include "AppSettings.h"
#include "Ballistics.h"
#include "CAchievementManager.h"
#include "CAvatarManager.h"
#include "CBH_Math.h"
#include "CBH_Player.h"
#include "CBH_PersonType.h"
#include "CBH_PersonTypeManager.h"
#include "CVehicleType.h"
#include "CNGSUser.h"
#include "CNGS_Platform.h"
#include "CGServeMessageManager.h"
#include "CNGSSocialMessage.h"
#include "CBH_SurfaceManager.h"
#include "CStrWChar.h"
#include "AdBannerSizes.h"
#include "AnimationImpl.h"
#include "CVehicleManager.h"
#include "ICStdUtil.h"
#include "CRandGen.h"
#include "CBountyResultScreen.h"
#include "CChallenge.h"
#include "CChallengeManager.h"
#include "CChallengeStats.h"
#include "CDH_Animation.h"
#include "CDH_BasicGameData.h"
#include "CDH_SoundManager.h"
#include "CDH_Weapon.h"
#include "CDH_WeaponsManager.h"
#include "CFpsCursor.h"
#include "CDialogWindow.h"
#include "CExplosiveObject.h"
#include "CFont.h"
#include "CFontMgr.h"
#include "CGameAnalytics.h"
#include "CGameObjectManager.h"
#include "CGPSHuntingItem.h"
#include "CGServeHelper.h"
#include "CIngameShop.h"
#include "CIntroduceItemWindow.h"
#include "CLocation.h"
#include "CLocationManager.h"
#include "CLevelUpWindow.h"
#include "CLoadingScreen.h"
#include "CMission.h"
#include "CMissionManager.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSRemoteUser.h"
#include "CNGSRemoteUserList.h"
#include "CNationalityType.h"
#include "CObjectMapValue.h"
#include "CProgressSystem.h"
#include "CScriptType.h"
#include "CSquad.h"
#include "CSquadType.h"
#include "CSquadTypeManager.h"
#include "CStdUtil_Cocoa.h"
#include "CTrickShotManager.h"
#include "CTutorialManager.h"
#include "CUnitInfoScreen.h"
#include "CUtility.h"
#include "CWaitWindow.h"
#include "CResourceManager.h"
#include "CNetworkAvailability.h"
#include "ICSystemFont.h"
#include "ICDebug.h"
#include "SScreenAdaptingConstant.h"
#include "CWindowQueue.h"
#include "dgCamera.h"
#include "DGCursor.h"
#include "DGHelper.h"
#include "DGScopeCursor.h"
#include "DGWorld.h"
#include "GamePlayHUD.h"
#include "ICCore.h"
#include "SG_Instance.h"

#include <math.h>
#include <stdio.h>

namespace {
enum MissionState { Loading = 0, CameraIntro = 1, WeaponIntro = 2, Playing = 4, Results = 5 };
const int BuyVehicleMinigun = static_cast<int>(0x8f81ee9cu);
const int DeclineVehicleMinigun = static_cast<int>(0xf8f91f94u);
const int VehicleMinigunId = 36;
const float UnitIdentificationRange = 350.0f;
const int AcceptTargetResult = 0x6e7d5207;
const int AcceptRevengeResult = 0x6e7d5208;
const int CloseTargetResult = 0x21204844;
const unsigned int TargetMessageDurationSeconds = 604800;



}

CSwerveGame* CSwerveGame::m_pInstance;

/** Store link of the Facebook stories. */
static const wchar_t GSERV_FB_LINK[] = L"http://gcs.glu.com/gcs/iphone/iphone.jsp?t=bounty&src=facebook&p=iphone";

/** Camera HUD surfaces of the downtown map (filled by the static initializer, released by the original DeleteScene). */
struct DowntownCameraSurfaces {
    DowntownCameraSurfaces()
    {
        m_names[0] = "IDB_HUD_DOWNTOWN_01_CAMERA_01";
        m_names[1] = "IDB_HUD_DOWNTOWN_01_CAMERA_02";
        m_names[2] = "IDB_HUD_DOWNTOWN_01_CAMERA_03";
        m_names[3] = "IDB_HUD_DOWNTOWN_01_CAMERA_04";
    }

    const char* m_names[4];
};
static DowntownCameraSurfaces s_downtownCameraSurfaces;

/** Globals of this unit in the original (_canShoot, _hpText); their users are not recovered yet. */
bool canShoot;
CStrWChar hpText;

CSwerveGame::CSwerveGame(ICRenderSurface* surface, signed char cameraId)
    : Window(false), m_tagImages(8), m_finishing(false), m_bossSpawned(false), m_fieldB2(false), m_waitWindow(NULL),
      m_playerInjured(false), m_medkitsBeforeHunt(0), m_huntStart(0), m_huntCounter(0),
      m_challengeCheckTime(CStdUtil_Cocoa::GetTimeSeconds()),
      m_previousAutoAimHorizontal(0.0f), m_previousAutoAimVertical(0.0f), m_dialogQueue(NULL), m_camera(NULL),
      m_collisionIntersection(CSwerve::GetInstance()->m_swerve),
      m_coverAnimation(NULL), m_resultScreen(NULL), m_resultsQueued(false), m_field1cd(true), m_unitInfo(NULL),
      m_multiplier(3),
      m_multiplierTimeMs(static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_tripleMultiplierSeconds * 1000),
      m_multiplierEnabled(false), m_inScopeRoutine(false), m_inScopeRoutineDone(false),
      m_ambushDialogShown(false), m_reviveOffered(false), m_waitingForAmmo(false), m_inGameShopOpen(false),
      m_randomWeaponIntroduced(false), m_hitechVisor(false), m_bossCreated(false), m_bossRunningAway(false),
      m_failResultPending(false), m_bossEscapePopupTime(0),
      m_delayedResult(BountyResultNoDelayedResult), m_resultDelay(0), m_playTimeMs(0), m_reviveShieldMs(0),
      m_viewport(), m_environment(NULL), m_field240(3),
      m_squad(NULL), m_slowMode(false), m_peaceTime(true), m_preSimulatingUnits(false), m_bossMustEscape(false), m_missionAlert(false),
      m_spawnedMinions(0), m_squadType(NULL),
      m_peaceElapsed(0), m_warElapsed(0), m_aimedUnit(NULL), m_aimedTimeMs(0), m_missedShots(0), m_stunMs(0),
      m_collision(NULL), m_grenadeLauncher(NULL), m_queueStartsPending(0), m_switchScopeWhenReady(false),
      m_cursor(NULL), m_fpsCursor(NULL), m_scopeCursor(NULL), m_field2e0(), m_field2f8(false),
      m_sceneReady(false), m_field35c(false),
      m_loadingSurface(surface), m_loadingFadeMs(1500), m_cameraId(cameraId), m_field394(-1), m_stateDelay(-1), m_field39c(-1),
      m_field3b0(false), m_exitAfterDialogs(false), m_hideFpsCursor(false), m_useWeaponModel(true),
      m_slowSpeedMultiplier(0), m_missionElapsedMs(0), m_coverDuration(0), m_field406(false), m_field408(0),
      m_explosionEffectIndex(0), m_bodyBlastIndex(0), m_shopDamageHintShown(false),
      m_specialWeapon(NULL), m_specialWeaponState(0),
      m_pendingMovingMinions(0), m_finalFOV(0.0f), m_fovSpeed(0.0f), m_fovDurationMs(0), m_fovElapsedMs(0)
{
    m_pInstance = this;
    for (int pointer = 0; pointer < TouchCount; ++pointer) {
        m_pointerX[pointer] = 0;
        m_pointerY[pointer] = 0;
        m_pointerDown[pointer] = false;
    }
    CApplet::m_pCore->SetMultipleTouchEnabled(true);
    CBH_Player::GetInstance()->m_gameStats.RegisterHuntStart();
    CBH_Player::GetInstance()->m_challengeStats->RegisterHuntStart(
        static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_locationName);
    static_cast<App*>(WindowApp::m_instance)->m_helper->initSwerveGame(this);
    m_player = CBH_Player::GetInstance();
    m_units.ensureCapacity(10);
    int slowMultiplier = static_cast<App*>(WindowApp::m_instance)->m_helper->m_config.GetIntegerValueByToken(
        "GO_PILL_SPEED_MULTIPLIER", true);
    if (slowMultiplier < 1) {
        slowMultiplier = 1;
    }
    m_slowSpeedMultiplier = slowMultiplier;
    m_medkitsBeforeHunt = CBH_Player::GetInstance()->m_medkits;
    m_cameraHud.Clear();
    m_dialogQueue = new CWindowQueue();
    AddToFront(m_dialogQueue);
    m_loadedMissionWeapons[0] = NULL;
    m_loadedMissionWeapons[1] = NULL;
}

CSwerveGame::~CSwerveGame()
{
    CApplet::m_pCore->SetMultipleTouchEnabled(false);
    CleanUp();
    m_pInstance = NULL;
}

void CSwerveGame::Init()
{
    CBH_Player::GetInstance()->ResetBeforeHunting();
    m_hud = new GamePlayHUD();
    AddToFront(m_hud);
    if (App::m_platformType == 0) {
        m_useWeaponModel = false;
        m_hud->m_scopeButtonAlways = false;
    }
    CApplet::m_pCore->SetTickRate(10);
    ChangeState(Loading, STATE_REPLACE);
    Activate();
    if (static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_multiplierCountdown) {
        if (!m_multiplierEnabled) {
            m_hud->ShowCountdownTimer(true);
        }
        m_multiplierEnabled = true;
    } else {
        m_multiplier = 1;
    }
    m_levelBeforeHunt = CBH_Player::GetInstance()->m_level;
    m_fbLoginAction = 0;
}

void CSwerveGame::Shutdown()
{
    m_resources.removeAllKeysets();
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_soundManager->StopAllSoundsAndRelease();
}

void CSwerveGame::CleanUp()
{
    DeleteScene();
    if (m_loadingSurface != NULL) {
        CApplet::m_pApp->m_resourceManager->ReleaseResource("gps_scr_shot");
        m_loadingSurface = NULL;
    }
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->StopAllSoundsAndRelease();
    m_particles.Free();
    SG_Instance* spriteInstance = static_cast<App*>(WindowApp::m_instance)->m_sgInstance;
    spriteInstance->DumpArchetypeCharacter(9, 0);
    spriteInstance->DumpArchetypeCharacter(0, 0);
    m_world.Clear();
    m_collisionIntersection.Clear();
    m_collisionGeometry.Clear();
    if (m_specialWeapon != NULL) {
        static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->release3dResources(m_specialWeapon->m_id);
        m_specialWeapon = NULL;
    } else {
        static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->release3dResources(
            static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true)->m_id);
        static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->release3dResources(
            static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true)->m_id);
    }
    if (m_waitWindow != NULL) {
        m_waitWindow->CloseSafely();
    }
    CBH_Player::GetInstance()->SetEstimatedWeapon(NULL, 0);
    m_challengeDialogs.removeAllElements();
    m_tagImages.DeleteContents();
}

void CSwerveGame::onStartClosing()
{
    CleanUp();
}

void CSwerveGame::Activate()
{
    // restore the HUD visibility saved by Deactivate
    if (m_hudVisibleStack.size() != 0) {
        int last = m_hudVisibleStack.size() - 1;
        bool visible = m_hudVisibleStack[last];
        m_hudVisibleStack.removeElementAt(last);
        if (visible) {
            m_hud->Show();
        }
    }
    if (m_cursor != NULL) {
        m_cursor->PointerBreak();
    }
}

void CSwerveGame::Deactivate()
{
    bool visible = m_hud->IsEnabled();
    m_hudVisibleStack.addElement(visible);
    m_hud->Hide(true);
    if (m_cursor != NULL) {
        m_cursor->PointerBreak();
    }
}

void CSwerveGame::Build()
{
}

void CSwerveGame::LoadLocation()
{
    CLocation* location = static_cast<App*>(WindowApp::m_instance)->m_locationManager->GetLocation(
        static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_locationName);
    if (location == NULL) {
        return;
    }
    m_world = World(App::LoadObject3D(XString::AnsiString(location->m_m3g.c_str())));
    FixWorldPosition();
    DefineCollisionGeometry();
    if (App::m_platformType == 0) {
        SwitchOffEyeCandyGeometry();
    }
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound(
        static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_ambientSound);
}

void CSwerveGame::SwitchOffEyeCandyGeometry()
{
    if (m_world == NULL) {
        return;
    }
    // low end devices: detach the light sources and the eye candy from the scene
    CUnitBody::Group lightSource(m_world.find(DGHelper::getSwerveID("LightSourceTransform")));
    CUnitBody::Group eyeCandy(m_world.find(DGHelper::getSwerveID("EyeCandyTransform")));
    if (lightSource != NULL) {
        CUnitBody::Group parent(lightSource.getParent());
        if (parent != NULL) {
            parent.removeChild(lightSource);
        }
    }
    if (eyeCandy != NULL) {
        CUnitBody::Group parent(eyeCandy.getParent());
        if (parent != NULL) {
            parent.removeChild(eyeCandy);
        }
    }
    lightSource.Clear();
    eyeCandy.Clear();
}

void CSwerveGame::FixWorldPosition()
{
    // the original does not test the node
    SwerveHelper::Transformable root(m_world.find(DGHelper::getSwerveID("worldTransform")));
    float origin[3] = {0.0f, 0.0f, 0.0f};
    root.setTranslation(origin);
}

void CSwerveGame::DefineCollisionGeometry()
{
    if (m_world == NULL) {
        return;
    }
    m_collisionGeometry = CUnitBody::Group(m_world.find(DGHelper::getSwerveID("firegeometryTransform")));
    if (m_collisionGeometry != NULL) {
        m_collisionGeometry.setPickingEnable(true);
        m_collisionGeometry.setRenderingEnable(false);
    }
}

void CSwerveGame::LoadAIMap()
{
    m_aiMap.Init(m_world);
    m_pathCalculator.Init(&m_aiMap);
}

void CSwerveGame::LoadCollision()
{
    if (m_collisionGeometry != NULL) {
        Node& node = m_collisionGeometry;
        m_collision = new CCollision(node, 30);
        m_grenadeLauncher = new GrenadeLauncher(m_collision, m_units, "BIN_M3G_ROCKET");
    }
}

void CSwerveGame::LoadObjects()
{
    CreateObjectImpl("barrel_%02dTransform", "barrel");
    CreateObjectImpl("citybarrel_%02dTransform", "citybarrel");
    CreateObjectImpl("gas_%02dTransform", "gasholder");
}

void CSwerveGame::CreateObjectImpl(const char* pattern, const char* type)
{
    if (pattern == NULL || type == NULL) {
        return;
    }
    // the level marks the objects with groups named <pattern> 01, 02, ...
    SwerveHelper::Group point;
    int index = 1;
    do {
        char name[60];
        ICStdUtil::SPrintF(name, pattern, index);
        point = SwerveHelper::Group(m_world.find(DGHelper::getSwerveID(name)));
        if (point == NULL) {
            return;
        }
        index++;
        IGameObject* object = static_cast<App*>(WindowApp::m_instance)->m_gameObjectManager->CreateObjectByName(XString(type));
        object->Bind();
        object->SetToPos(point);
        m_objects.addElement(object);
    } while (point != NULL);
}

void CSwerveGame::CreateUnitByType(const CBH_PersonType* type, EUnitMindType mindType,
                                   const XString& name, bool scripted, CSquad* squad)
{
    if (type == NULL) {
        return;
    }
    int point;
    const CScriptType* script = NULL;
    if (scripted) {
        script = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_scriptType;
    }
    if (script != NULL) {
        point = script->m_spawnPointId;
    } else if (mindType == UnitCivilian) {
        point = m_aiMap.GetAnyPathPointID();
    } else {
        point = m_aiMap.GetAnySwarmPointID();
    }
    CUnit* unit = new CUnit(type, &m_pathCalculator, &m_aiMap);
    if (squad != NULL) {
        if (mindType == UnitWarrior) {
            squad->AddMember(unit);
        } else if (mindType == UnitBoss) {
            squad->AddLeader(unit);
        }
    }
    unit->Init(mindType, point, scripted, squad != NULL);
    unit->m_name = name;
    m_units.addElement(unit);
    if (mindType == UnitBoss) {
        m_bossCreated = true;
    }
}

void CSwerveGame::CreateOriginalBoss()
{
    if (!m_bossSpawned) {
        const CBH_PersonType* type = GetPersonaForBoss();
        if (type != NULL) {
            CreateUnitByType(type, UnitBoss, static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_bossName,
                             false, m_squad);
            m_bossSpawned = true;
        }
    }
}

void CSwerveGame::CreateScriptedBoss()
{
    if (static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_scriptType != NULL && !m_bossSpawned) {
        const CBH_PersonType* type = GetPersonaForBoss();
        if (type != NULL) {
            CreateUnitByType(type, UnitBoss, static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_bossName,
                             true, m_squad);
            m_bossSpawned = true;
        }
    }
}

void CSwerveGame::InitUnits()
{
    if (static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_fielda1) {
        m_squad = new CSquad();
    }
    const CHuntingInfo::SAssignedData& assignment = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData;
    if (assignment.m_scriptType == NULL && !assignment.m_bossSpawnsLast) {
        CreateOriginalBoss();
    }
    m_squadType = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_squadType;
    if (m_squadType != NULL) {
        m_spawnedMinions = 0;
        for (int index = 0; index < m_squadType->m_minions.size(); ++index) {
            const XString& name = m_squadType->GetMiniomNameByIndex(index);
            if (!(XString("") == name)) {
                CreateUnitByName(name, UnitWarrior, m_squad);
                m_spawnedMinions++;
            }
        }
        if (CanSpawnCivilians()) {
            CreateCivilians();
        }
    }
}

void CSwerveGame::CreateUnitByName(const XString& name, EUnitMindType mindType, CSquad* squad)
{
    CBH_PersonType* type = static_cast<App*>(WindowApp::m_instance)->InitPersonTypeManager()->GetType(name);
    if (type != NULL && type->m_visualCharacter != NULL) {
        CreateUnitByType(type, mindType, type->m_nationality->GetRandomFullName(type->m_visualCharacter->m_isMan),
                         false, squad);
    }
}

bool CSwerveGame::CanSpawnCivilians()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CLocation* location = app->m_locationManager->GetLocation(app->m_huntingInfo->m_assignedData.m_locationName);
    if (location != NULL && !location->m_spawnCivilians) {
        return false;
    }
    for (int index = 0; index < m_units.size(); ++index) {
        if (!m_units[index]->m_body.m_type->m_spawnCivilians) {
            return false;
        }
    }
    return true;
}

void CSwerveGame::CreateCivilians()
{
    const CSquadType* civilians = static_cast<App*>(WindowApp::m_instance)->InitSquadTypeManager()->GetType(XString("CivilianSquad"));
    if (civilians != NULL && civilians->m_minions.size() > 0) {
        int count = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_civilianCount;
        for (int index = 0; index < count; ++index) {
            const XString& name = civilians->GetMiniomName();
            if (!(XString("") == name)) {
                CreateUnitByName(name, UnitCivilian, NULL);
            }
        }
    }
}

void CSwerveGame::PutAllSquadsInPlace()
{
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit != NULL && unit->m_squad != NULL) {
            vec3 position = unit->m_squad->GetLocalSquadPosition(unit);
            CUnitBody* body = unit->GetBody();
            if (body == NULL) {
                return;
            }
            body->m_position = position;
        }
    }
}

int CSwerveGame::GetAlivaUnitByType(EUnitMindType type)
{
    int alive = 0;
    if (static_cast<unsigned int>(type) >= 3) {
        return alive;
    }
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        // the original tests only dead / tranquillized here (not CanBeHandled)
        if (unit != NULL && !unit->m_dead && !unit->m_tranquillized) {
            if (unit->m_type == type) {
                ++alive;
            }
        }
    }
    return alive;
}

void CSwerveGame::UpdateUnits(int elapsed)
{
    int count = m_units.size();
    float tagX = m_cursor->GetTagX();
    float tagY = m_cursor->GetTagY();
    float distance = GetDistanceToCollision(tagX, tagY);
    CUnit* aimedUnit = NULL;
    for (int index = 0; index < count; ++index) {
        CUnit* unit = m_units[index];
        if (unit != NULL) {
            unit->ResetPickInfo();
            if (unit->CanBeHandled()) {
                unit->m_body.CheckVisibility(m_camera->m_frustum);
                if (m_cursor != NULL) {
                    CUnitBody::Camera camera = m_world.getActiveCamera();
                    if (unit->Pick(tagX, tagY, distance, camera)) {
                        aimedUnit = unit;
                    }
                }
                AvoidCollision(unit);
                if (unit->m_type == UnitBoss) {
                    // the popup "boss is running away" is shown while a living boss escapes
                    m_bossRunningAway = !unit->m_dead && !unit->m_invulnerable && unit->m_runningAway;
                }
                CheckVisibility(unit);
                unit->Tick(elapsed);
            }
        }
    }
    UpdateAutoAim(elapsed);
    if (aimedUnit != NULL && aimedUnit == m_aimedUnit) {
        m_aimedTimeMs += elapsed;
        if (m_aimedTimeMs > static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->GetTaggingTimeMS()) {
            m_aimedTimeMs = 0;
            ShowUnitInfo(aimedUnit, 0);
        }
    } else {
        m_aimedTimeMs = 0;
    }
    m_aimedUnit = aimedUnit;
}

void CSwerveGame::UpdatePopups(int elapsed)
{
    if (elapsed > 0 && m_bossRunningAway) {
        unsigned int now = CStdUtil_Cocoa::GetUpTimeMS();
        if (now - m_bossEscapePopupTime >= 3001) {
            WindowApp::HandleTunnelCommand(-0x628818b5, 0, 0, 0);
            m_bossEscapePopupTime = now;
        }
    }
}

void CSwerveGame::AvoidCollision(CUnit* unit)
{
    if (unit != NULL) {
        for (int index = 0; index < m_units.size(); ++index) {
            CUnit* other = m_units[index];
            if (other != NULL) {
                bool handled = other->CanBeHandled();
                if (other != unit && handled) {
                    unit->m_body.AvoidCollision(&other->m_body);
                }
            }
        }
    }
}

void CSwerveGame::UpdateObjects(int elapsed)
{
    for (int index = 0; index < m_objects.size(); ++index) {
        m_objects[index]->Tick(elapsed);
    }
}

void CSwerveGame::UnitGarbageCollector()
{
    // move the removed units behind the kept ones, then delete them
    int count = m_units.size();
    int end = count;
    int index = 0;
    while (index < end) {
        CUnit* unit = m_units[index];
        if (unit == NULL) {
            // original bug: the index is not advanced, a NULL slot loops forever
            continue;
        }
        if (!unit->m_removed) {
            index++;
            continue;
        }
        if (unit->m_type == UnitBoss && !static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->IsAmbush()) {
            QueueResult(BountyResultBossEscaped);
        }
        m_aiMap.UnregisterUnitBody(&unit->m_body);
        if (end > 1 && index < end - 1) {
            CUnit* kept = m_units[end - 1];
            m_units[end - 1] = m_units[index];
            m_units[index] = kept;
        }
        end--;
    }
    if (end < count) {
        for (int removed = end; removed < count; ++removed) {
            delete m_units[removed];
        }
        if (end < count) {
            m_units.setSize(end);
        }
    }
}

void CSwerveGame::UpdateEnvironment(int elapsed, bool)
{
    if (elapsed > 0) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->InitAchievementManager()->Update();
    }
    m_missionElapsedMs += elapsed;
    m_world.animate(m_missionElapsedMs);
    m_environment->update();
    m_camera->Update(elapsed);
    m_particles.Update(static_cast<float>(elapsed));
}

void CSwerveGame::CheckVisibility(CUnit* unit)
{
    const float reactionDistanceSq = 49.0f;
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* other = m_units[index];
        if (other != unit && other != NULL) {
            vec3 offset = unit->m_body.m_position - other->m_body.m_position;
            if (offset.LengthSqr() <= reactionDistanceSq) {
                unit->m_mind.OnSeeUnit(other);
            }
        }
    }
}

void CSwerveGame::UpdateAutoAim(int elapsed)
{
    if (m_pointerDown[0]) {
        return;
    }
    CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon();
    if (weapon == NULL) {
        return;
    }
    if (!weapon->m_autoAim && !weapon->m_cyberScopeEquipped) {
        return;
    }
    vec3 cameraPosition = GetCurrentCameraPos();
    float cameraX = cameraPosition.m_x;
    float cameraY = cameraPosition.m_y;
    float cameraZ = cameraPosition.m_z;
    vec3 viewDirection = m_camera->GetViewDirection();
    // pick the unit closest to the view direction (angles in degrees, 5 degrees at most)
    CUnit* target = NULL;
    float bestHorizontal = 5.0f;
    float bestVertical = 5.0f;
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit != NULL && unit->CanBeHandled() && !unit->m_invulnerable) {
            vec3 aimPoint = unit->m_body.GetAutoAimPointPosition();
            float dx = cameraX - aimPoint.m_x;
            float dy = cameraY - aimPoint.m_y;
            float dz = cameraZ - aimPoint.m_z;
            float inverseLength = MathLib::InvSqrt(dx * dx + dy * dy + dz * dz);
            float inverseViewLength = MathLib::InvSqrt(viewDirection.m_x * viewDirection.m_x
                                                       + viewDirection.m_y * viewDirection.m_y
                                                       + viewDirection.m_z * viewDirection.m_z);
            float vertical = MathLib::ArcSin(dz * inverseLength - viewDirection.m_z * inverseViewLength);
            float horizontal = MathLib::ArcTan(dx, dy)
                - MathLib::ArcTan(viewDirection.m_x, viewDirection.m_y);
            if (vertical * vertical + horizontal * horizontal
                < bestVertical * bestVertical + bestHorizontal * bestHorizontal) {
                bestHorizontal = horizontal;
                bestVertical = vertical;
                target = unit;
            }
        }
    }
    if (target == NULL) {
        m_previousAutoAimHorizontal = 0.0f;
        m_previousAutoAimVertical = 0.0f;
        return;
    }
    float speed = elapsed * 0.005f;
    float horizontalStep = speed * fabsf(bestHorizontal);
    if (fabsf(horizontalStep) < 0.01f) {
        horizontalStep = 0.0f;
    }
    if (fabsf(bestHorizontal) >= horizontalStep) {
        horizontalStep += m_previousAutoAimHorizontal * -0.05f;
        float delta = horizontalStep;
        if (bestHorizontal > 0.0f) {
            delta = -horizontalStep;
        }
        m_camera->SetAngleH(m_camera->m_field0c + delta, true);
    }
    float verticalStep = speed * fabsf(bestVertical);
    if (fabsf(verticalStep) < 0.01f) {
        verticalStep = 0.0f;
    }
    if (fabsf(bestVertical) >= verticalStep) {
        verticalStep += m_previousAutoAimVertical * -0.05f;
        float delta = verticalStep;
        if (bestVertical > 0.0f) {
            delta = -verticalStep;
        }
        m_camera->SetAngleV(m_camera->m_field1c + delta, true);
    }
    m_previousAutoAimHorizontal = horizontalStep;
    m_previousAutoAimVertical = verticalStep;
}

void CSwerveGame::UpdateHitechVisor()
{
    if (CBH_Player::GetInstance()->m_hitechVisorEquipped && !m_hitechVisor) {
        // visor just put on: tag and reveal every unit
        for (int index = 0; index < m_units.size(); ++index) {
            CUnit* unit = m_units[index];
            if (unit != NULL) {
                unit->m_body.ShowDistanceTag(true);
                unit->m_body.SetAsKnown();
            }
        }
    } else if (!CBH_Player::GetInstance()->m_hitechVisorEquipped && m_hitechVisor) {
        // visor taken off: hide the distance tags
        for (int index = 0; index < m_units.size(); ++index) {
            CUnit* unit = m_units[index];
            if (unit != NULL) {
                unit->m_body.ShowDistanceTag(false);
            }
        }
    }
    m_hitechVisor = CBH_Player::GetInstance()->m_hitechVisorEquipped;
}

CUnit* CSwerveGame::GetUnitByGUID(int guid)
{
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit != NULL && unit->m_guid == guid) {
            return unit;
        }
    }
    return NULL;
}

void CSwerveGame::ShowUnitInfo(CUnit* unit, int startingDamage)
{
    if (m_unitInfo != NULL && m_unitInfo->m_state != 3) {
        if (m_unitInfo->m_guid == unit->m_guid) {
            m_unitInfo->FlushBeforeCloseTimer();
            return;
        }
        m_unitInfo->Close();
    }
    float distance = GetDistanceToUnit(unit);
    // only the target boss of a contract (or a revenge) is revealed by name;
    // the original reads the hunting info again after the name compare
    bool reveal = false;
    if (unit->m_name == static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_bossName) {
        const CHuntingInfo::SAssignedData& assignment = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData;
        reveal = assignment.m_hasTarget || assignment.m_revenge;
    }
    m_unitInfo = new CUnitInfoScreen(this, unit, distance, UnitIdentificationRange, true, startingDamage, reveal);
    AddToFront(m_unitInfo);
    unit->m_body.OnTagged(distance < UnitIdentificationRange);
}

void CSwerveGame::ShowUnitKillScreen(CUnit* unit)
{
    if (m_unitInfo != NULL) {
        m_unitInfo->Close();
    }
    float distance = GetDistanceToUnit(unit);
    m_unitInfo = new CUnitInfoScreen(this, unit, distance, UnitIdentificationRange, false, 0, false);
    AddToFront(m_unitInfo);
}

void CSwerveGame::OnUnitInfoScreenClosed()
{
    m_unitInfo = NULL;
}

void CSwerveGame::DrawVisorDistances(ICGraphics2d& graphics)
{
    if (!m_hitechVisor || m_camera == NULL) {
        return;
    }
    CFont* font = CFontMgr::GetInstance()->GetFont(17);
    if (font == NULL) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    DGCamera::Transform view = app->m_helper->m_transform;
    if (!m_world.getTransformTo(m_camera->m_camera, view)) {
        return;
    }
    DGCamera::Transform projection(CSwerve::GetInstance()->m_swerve);
    m_camera->m_camera.getProjection(projection);
    int width = app->m_helper->m_screenWidth;
    int height = app->m_helper->m_screenHeight;
    XString measure(CUtility::GetString("IDS_GAME_PLAY_MEASURE").GetWCharPtr());
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit == NULL || !unit->m_body.m_showDistanceTag || !unit->CanBeHandled()
            || unit->m_body.m_distanceTag != NULL) {
            continue;
        }
        const vec3& position = unit->m_body.m_position;
        float point[4] = {position.m_x, position.m_y, position.m_z + unit->m_body.GetHeight(), 1.0f};
        view.transform(4, point);
        float distance = sqrtf(point[0] * point[0] + point[1] * point[1] + point[2] * point[2]);
        projection.transform(4, point);
        if (point[3] <= 0.0f || fabsf(point[0]) > point[3] || fabsf(point[1]) > point[3]) {
            continue;
        }
        int x = static_cast<int>((point[0] / point[3] + 1.0f) * 0.5f * width);
        int y = static_cast<int>((1.0f - point[1] / point[3]) * 0.5f * height);
        float obstacleDistance = GetDistanceToCollision(x, y);
        if (obstacleDistance > 0.0f && obstacleDistance < distance - unit->m_body.GetRadius()) {
            continue;
        }
        XString text = XString::Format(L"%d %ls", static_cast<int>(distance * 3.28f), measure.c_str());
        DrawFontText(graphics, font, text.c_str(), x, y, ALIGN_HCENTER | ALIGN_BOTTOM);
    }
}

void CSwerveGame::UnregisterAllDeadBodies()
{
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit != NULL && (unit->m_dead || unit->m_tranquillized)) {
            m_aiMap.UnregisterUnitBody(unit->GetBody());
        }
    }
}

float CSwerveGame::GetDistanceToCollision(float x, float y)
{
    // nearest hit of the screen point on the collision geometry and on the vehicles, -1 when nothing
    CUnitBody::Camera camera = m_world.getActiveCamera();
    DGHelper* helper = static_cast<App*>(WindowApp::m_instance)->m_helper;
    int screenWidth = helper->m_screenWidth;
    int screenHeight = helper->m_screenHeight;
    float relativeX = x / screenWidth;
    float relativeY = y / screenHeight;
    float distance;
    if (!m_collisionGeometry.pick(-1, relativeX, relativeY, camera, m_collisionIntersection)) {
        distance = -1.0f;
    } else {
        if (m_collisionIntersection == NULL) {
            return -1.0f;
        }
        distance = m_collisionIntersection.getDistance();
    }
    for (int index = 0; index < m_vehicles.size(); ++index) {
        if (m_vehicles[index] != NULL) {
            if (m_vehicles[index].pick(-1, relativeX, relativeY, camera, m_collisionIntersection)) {
                float vehicleDistance = m_collisionIntersection.getDistance();
                if (distance < 0.0f || vehicleDistance < distance) {
                    distance = vehicleDistance;
                }
            }
        }
    }
    return distance;
}

vec3 CSwerveGame::GetCurrentCameraPos()
{
    vec3 position(0.0f, 0.0f, 0.0f);
    // the camera node of the current point: translation column of its world transform
    XString name = XString::Format(L"Camera%02d", m_cameraId + 1);
    ::Camera camera(m_world.find(SwerveHelper::Id(XString::AnsiString(name.c_str()))));
    if (camera != NULL) {
        ::Transform transform(CSwerve::GetInstance()->m_swerve);
        camera.getTransformTo(m_world, transform);
        float matrix[16];
        transform.get(matrix);
        position.m_x = matrix[3];
        position.m_y = matrix[7];
        position.m_z = matrix[11];
    }
    return position;
}

float CSwerveGame::GetDistanceToUnit(CUnit* unit)
{
    if (unit == NULL) {
        return 0.0f;
    }
    vec3 delta = GetCurrentCameraPos() - unit->m_body.m_position;
    return DGHelper::sqrt(delta.LengthSqr());
}

void CSwerveGame::SetTagImage(const XString& glyph, unsigned char* pixels, int width, int height)
{
    // the original simply overwrites an older image of the same glyph (no delete)
    STagImage* image = new STagImage();
    image->m_width = width;
    image->m_height = height;
    image->m_pixels = pixels;
    m_tagImages.Set(glyph, image);
}

const STagImage* CSwerveGame::GetTagImage(const XString& glyph)
{
    // the original reads a NULL default through a pointer when the glyph is unknown
    STagImage* missing = NULL;
    STagImage** image = m_tagImages.Get(glyph);
    if (image == NULL) {
        image = &missing;
    }
    return *image;
}

void CSwerveGame::LoadWeapon()
{
    static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->load3dResources(
        static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true)->m_id, true);
    static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->load3dResources(
        static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true)->m_id, true);
}

void CSwerveGame::LoadSounds()
{
    if (CBH_Player::GetInstance()->IsArmorAvailable()) {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound("IDM_ARMOR_HIT");
    }
    if (CBH_Player::GetInstance()->IsSpeedModeAvailable()) {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound("IDM_SLOW_START");
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound("IDM_SLOW_END");
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound("IDM_SLOW_GUNSHOT");
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound("IDM_HEART_BEAT");
    }
    if (CBH_Player::GetInstance()->m_ultrasoundAttemptCount > 0) {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound("IDM_ULTRASOUND_START");
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound("IDM_ULTRASOUND_LOOP");
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound("IDM_ULTRASOUND_END");
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheKeyset("KEYSET_SOUND_UNIT_RECEIVE_ULTRASOUND");
    }
    if (m_objects.size() > 0) {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound("IDM_BARREL_EXPLOSION");
    }
    // original bug: the first weapon is tested twice (the second one is never checked)
    if (static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true)->m_projectileType != 0
        || static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true)->m_projectileType != 0) {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound("IDM_GRENADE_EXPLOSION");
    }
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound("IDM_CHARACTER_BULLET_CLOSE");
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound("IDM_CHARACTER_ENEMY_WOUND_MAN_01");
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound("IDM_SPECIAL_DUCK");
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound("IDM_SPECIAL_AP_MEDKIT");
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound("IDM_HEADSHOT");
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound("IDM_MEN_HIT1_37");
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheSound("IDM_SPECIAL_REVIVE");
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheKeyset("KEYSET_SOUND_WOMEN_HIT_FULL_HEALTH");
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheKeyset("KEYSET_SOUND_WOMEN_HIT_LOW_HEALTH");
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheKeyset("KEYSET_SOUND_MEN_HIT_FULL_HEALTH");
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheKeyset("KEYSET_SOUND_MEN_HIT_LOW_HEALTH");
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheKeyset("KEYSET_SOUND_WOMEN_ALERT");
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheKeyset("KEYSET_SOUND_BAD_GUY_ALERT");
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheKeyset("KEYSET_SOUND_MAN_EXPLODE");
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheKeyset("KEYSET_SOUND_MAN_FIRE");
    if (CanSpawnCivilians()) {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PrecacheKeyset("KEYSET_SOUND_CIVIC_ALERT");
    }
}

void CSwerveGame::setCamera(int cameraId)
{
    char name[20];
    ICStdUtil::SPrintF(name, "Camera%02d", cameraId + 1);
    DGCamera::Camera camera(m_world.find(DGHelper::getSwerveID(name)));
    if (camera != ::Object3D()) {
        m_cameraId = cameraId;
        m_world.setActiveCamera(camera);
        DGCamera::Camera active = m_world.getActiveCamera();
        DGHelper* helper = static_cast<App*>(WindowApp::m_instance)->m_helper;
        helper->SetCameraSize(active, helper->m_screenWidth, helper->m_screenHeight, CAMERA_SIZE_SET_ASPECT);
    }
    if (m_camera != NULL) {
        delete m_camera;
        m_camera = NULL;
    }
    const CHuntingInfo::SAssignedData& assignment = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData;
    float horizontalLimit = assignment.m_cameraHorizontalLimit;
    float verticalLimit = assignment.m_cameraVerticalLimit;
    m_camera = new DGCamera(m_world.getActiveCamera(), horizontalLimit, verticalLimit);
    canShoot = false;
}

int CSwerveGame::GetCurrCamId()
{
    return m_cameraId;
}

int CSwerveGame::getCamerasCount()
{
    // the level names its cameras Camera01, Camera02, ...
    int count = 0;
    DGCamera::Camera camera;
    char name[60];
    do {
        ICStdUtil::SPrintF(name, "Camera%02d", count + 1);
        camera = DGCamera::Camera(m_world.find(DGHelper::getSwerveID(name)));
        if (camera != ::Object3D()) {
            count++;
        }
    } while (camera != ::Object3D());
    return count;
}

void CSwerveGame::setCursor(DGCursor* cursor)
{
    DGCursor* previous = NULL;
    if (m_cursor != NULL) {
        m_cursor->DeInit();
        previous = m_cursor;
    }
    if (previous == cursor) {
        return;
    }
    m_cursor = cursor;
    cursor->Init();
}

void CSwerveGame::InitScope()
{
    CDH_Weapon* first = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
    CDH_Weapon* second = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
    if (first == NULL || second == NULL) {
        return;
    }
    // load the scope sprites (archetype 9) and the cross sprites (archetype 0) right away
    SG_Instance* sprites = static_cast<App*>(WindowApp::m_instance)->m_sgInstance;
    unsigned char finished = 0;
    sprites->QueueArchetypeCharacter(9, 0);
    while (sprites->LoadQueued(1000, finished)) {
    }
    sprites->QueueArchetypeCharacter(0, 0);
    while (sprites->LoadQueued(1000, finished)) {
    }
}

void CSwerveGame::CreateCursors()
{
    m_fpsCursor = new CFPSCursor();
    m_scopeCursor = new DGScopeCursor();
    m_scopeCursor->setPosition(0, 0, true);
    m_scopeCursor->setConstraints(-10, -10, 10, 10);
}

void CSwerveGame::DeleteCursors()
{
    // the original clears each pointer inside its check
    if (m_fpsCursor != NULL) {
        delete m_fpsCursor;
        m_fpsCursor = NULL;
    }
    if (m_scopeCursor != NULL) {
        delete m_scopeCursor;
        m_scopeCursor = NULL;
    }
    // the original only forgets the active cursor (no DeInit)
    m_cursor = NULL;
}

void CSwerveGame::UseScope(bool active)
{
    if (active) {
        m_player->m_challengeStats->RegisterPlayerEnterScope();
        setCursor(m_scopeCursor);
    } else {
        setCursor(m_fpsCursor);
    }
}

void CSwerveGame::CreateCameraAnimation()
{
    m_coverAnimation = new CDH_Animation(m_world);
    if (m_coverAnimation != NULL) {
        m_coverAnimation->addController(XString("anim_hide01"));
        m_coverDuration = m_coverAnimation->getControllerBaseDuration(0);
    }
}

bool CSwerveGame::HasCoverAnimation() const
{
    return m_coverAnimation != NULL && m_coverDuration > 0;
}

void CSwerveGame::CreateCameraHud3D()
{
    const CHuntingInfo::SAssignedData& assignment = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData;
    if (assignment.m_cameraDurationMs <= 0 || assignment.m_cameraHUDModel.ByteSize() == 0) {
        return;
    }
    // the moving camera HUD model hangs below dynamic_camera_hudTransform of the level
    SwerveHelper::Object3D model = App::LoadObject3D(XString::AnsiString(assignment.m_cameraHUDModel.c_str()));
    m_cameraHud = SwerveHelper::Group(model.find(DGHelper::getSwerveID("dummy_hudTransform")));
    SwerveHelper::Group mount(m_world.find(DGHelper::getSwerveID("dynamic_camera_hudTransform")));
    mount.addChild(m_cameraHud);
}

void CSwerveGame::ChangeCover()
{
    if (m_coverAnimation == NULL) {
        return;
    }
    if (m_camera == NULL || m_hud == NULL) {
        return;
    }
    int introTime = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_introTimeMs;
    if (introTime >= 0 && m_missionElapsedMs < introTime) {
        return;
    }
    if (m_player->IsNeedHide()) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        CGameAnalytics::logCoverUsed(app->m_huntingInfo->m_assignedData.m_missionName,
                                     app->m_helper->GetGameData()->GetActiveWeapon()->m_id);
    }
    bool speedMode = CBH_Player::GetInstance()->IsSpeedMode();
    int duration = m_coverDuration;
    if (speedMode) {
        duration = duration / m_slowSpeedMultiplier;
    }
    m_coverAnimation->setControllerDuration(0, duration);
    if (m_player->IsNeedHide()) {
        m_camera->centerView(duration);
    } else {
        m_camera->returnToSavedView(duration);
    }
    m_hud->EnableDockCoverButton(false);
    bool leaving = !m_player->IsNeedHide();
    m_coverAnimation->setCurrentController(m_missionElapsedMs, 0, leaving);
    m_player->StartCover();
    m_fieldB2 = true;
    CBH_Player::GetInstance()->m_gameStats.AddCoversUsed(1);
    m_hud->ChangeDockCoverButtonState();
}

void CSwerveGame::onWeaponSwitch()
{
    if (m_cursor != NULL) {
        m_cursor->onWeaponSwitch();
    }
}

bool CSwerveGame::IsIntroTime() const
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int duration = app->m_huntingInfo->m_assignedData.m_introTimeMs;
    return duration >= 0 && m_missionElapsedMs < duration;
}

bool CSwerveGame::IsGamePaused() const
{
    return m_currentState != Playing;
}

void CSwerveGame::SetSlowMode(bool enabled)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_huntingInfo->m_assignedData.m_cameraDurationMs < 1) {
        m_slowMode = enabled;
    }
}

void CSwerveGame::OnCreateState(Window** window, int state)
{
    m_stateDelay = -1;
    if (state == WeaponIntro) {
        CreateCursors();
        setCursor(m_fpsCursor);
        static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->fetchWeaponModel();
        m_stateDelay = 1000;
    } else if (state == Playing) {
        m_hud->Show();
    } else if (state == Loading) {
        CLoadingScreen* loading = new CLoadingScreen(this);
        loading->AddLoadingStep(&CSwerveGame::LoadWeapon);
        loading->AddLoadingStep(&CSwerveGame::LoadLocation);
        loading->AddLoadingStep(&CSwerveGame::LoadCollision);
        loading->AddLoadingStep(&CSwerveGame::LoadObjects);
        loading->AddLoadingStep(&CSwerveGame::LoadSkyBox);
        loading->AddLoadingStep(&CSwerveGame::LoadAIMap);
        loading->AddLoadingStep(&CSwerveGame::LoadSounds);
        loading->AddLoadingStep(&CSwerveGame::CreateExplosionEffects);
        loading->AddLoadingStep(&CSwerveGame::CreateBodyBlastEffects);
        loading->AddLoadingStep(&CSwerveGame::CreateVehicles);
        loading->AddLoadingStep(&CSwerveGame::CreateCameraAnimation);
        loading->AddLoadingStep(&CSwerveGame::CreateCameraHud3D);
        loading->AddLoadingStep(&CSwerveGame::InitUnits);
        loading->AddLoadingStep(&CSwerveGame::InitScope);
        loading->AddLoadingStep(&CSwerveGame::PreliminaryUnitsSumulation);
        if (loading != NULL) {
            *window = loading;
            return;
        }
    }
    *window = new Window(false);
}

void CSwerveGame::OnStateInterrupt(int state, Window* window)
{
    switch (state) {
    case Loading:
        initHunting();
        m_huntStart = CStdUtil_Cocoa::GetTimeSeconds();
        m_huntCounter = 0;
        break;
    case 3: {
        int result = window->GetInterrupt();
        if (result == 1) {
            SetInterrupt(1);
        } else if (result == 2) {
            Activate();
            PopState();
        }
        break;
    }
    case Playing:
        m_hud->Hide(false);
        break;
    case WeaponIntro:
        ChangeState(Playing, STATE_GOTO);
        break;
    case 6:
        if (window->GetInterrupt() == 2) {
            PopState();
        }
        Activate();
        break;
    }
}

void CSwerveGame::OnCommand(Event& event)
{
    if (!m_sceneReady) {
        return;
    }
    if (m_hud->m_tutorialHints.size() != 0) {
        return;
    }
    switch (event.m_id) {
    case 0x3f972120:
        m_field35c = true;
        event.Clear();
        break;
    case static_cast<int>(0x972028c1u):
        event.Clear();
        break;
    case static_cast<int>(0x9720a3cbu):
        m_field406 = true;
        event.Clear();
        break;
    case 0x7214ff0d:
        m_field406 = false;
        event.Clear();
        break;
    case 0x2074d009:
        event.Clear();
        break;
    case 0x7e4d23f0:
        // in-game shop closed
        if (m_inGameShopOpen) {
            m_waitingForAmmo = false;
            m_inGameShopOpen = false;
        }
        event.Clear();
        break;
    case 0x49444b61: {
        static_cast<App*>(WindowApp::m_instance)->m_timeScale = 1.0f;
        CIngameShop* shop = new CIngameShop();
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(shop);
        shop->SetTab(CIngameShop::Bank);
        m_inGameShopOpen = true;
        event.Clear();
        break;
    }
    case static_cast<int>(0xb717ef9fu):
        // level up post declined
        m_levelUpPosts.removeElementAt(0);
        // fall through
    case 0x385a651b:
    case static_cast<int>(0x9f523292u):
        if (m_queueStartsPending > 0) {
            m_queueStartsPending--;
            m_dialogQueue->Start();
        }
        event.Clear();
        break;
    case 0x1105a3a5:
        event.Clear();
        if (!m_exitAfterDialogs) {
            m_exitAfterDialogs = true;
            CGameAnalytics::logMissionFailed(static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_missionName, 40);
            FinishGameWithResult(BountyResultPending, true);
            AddLevelUpDialogs();
            AddCompletedChallengesDialogs();
        }
        break;
    case static_cast<int>(0xd6cad5a6u):
        // a final dialog closed
        event.Clear();
        if (m_dialogQueue->IsEmpty()) {
            if (m_failResultPending) {
                WindowApp::HandleTunnelCommand(0x1105a3a5, 0, 0, 0);
            } else {
                ShowResultScreen(true);
            }
        }
        break;
    case static_cast<int>(0xa728ca73u): {
        // Facebook post succeeded
        if (m_waitWindow == NULL) {
            return;
        }
        m_waitWindow->Close();
        m_waitWindow = NULL;
        CTextDialog* dialog = new CTextDialog();
        dialog->SetTitleText(ResString("IDS_FACEBOOK_POST_SUCCESS_TITLE"));
        dialog->SetMessageText(ResString("IDS_FACEBOOK_POST_SUCCESS_MESSAGE"), NULL);
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        dialog->SetButtonsCommands(m_fbPostSuccessCommand, -1, -1);
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
        break;
    }
    case 0x70b135b5: {
        // Facebook post failed
        if (m_waitWindow == NULL) {
            return;
        }
        m_waitWindow->Close();
        m_waitWindow = NULL;
        CTextDialog* dialog = new CTextDialog();
        dialog->SetTitleText(ResString("IDS_FACEBOOK_POST_FAIL_TITLE"));
        dialog->SetMessageText(ResString("IDS_FACEBOOK_POST_FAIL_MESSAGE"), NULL);
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        dialog->SetButtonsCommands(m_fbPostFailCommand, -1, -1);
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(dialog);
        break;
    }
    case 0x6e7d5204: {
        // post the level up story
        if (CNGS::GetInstance()->GetLocalUser()->isAuthenticated(SOCIAL_NETWORK_FACEBOOK)) {
            int level = m_levelUpPosts[0];
            m_levelUpPosts.removeElementAt(0);
            PostFacebookMessage(level);
            CWaitWindow* waitWindow = new CWaitWindow();
            m_waitWindow = waitWindow;
            waitWindow->SetBackgroundColor(0x64000000);
            static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(m_waitWindow);
            m_fbPostSuccessCommand = static_cast<int>(0xd6cad5a6u);
            m_fbPostFailCommand = static_cast<int>(0xd6cad5a6u);
        } else {
            m_fbLoginAction = 1;
            static_cast<App*>(WindowApp::m_instance)->StartLogin(App::LoginFacebook, true);
        }
        break;
    }
    case AcceptTargetResult:
        SendMessageToTarget(true);
        WindowApp::HandleTunnelCommand(CloseTargetResult, 0, 0, 0);
        break;
    case AcceptRevengeResult:
        SendMessageToTarget(false);
        WindowApp::HandleTunnelCommand(CloseTargetResult, 0, 0, 0);
        break;
    case 0x6e7d5205:
    case 0x6e7d5206: {
        // post the target killed (5205) / avenged (5206) story
        bool authenticated = CNGS::GetInstance()->GetLocalUser()->isAuthenticated(SOCIAL_NETWORK_FACEBOOK);
        int id = event.m_id;
        if (authenticated) {
            int friendLength = CBH_Player::GetInstance()->m_missionBriefTargetFB.GetLength();
            CBH_Player* player = CBH_Player::GetInstance();
            if (id == 0x6e7d5205) {
                if (friendLength > 0) {
                    PostFacebookMessageRandomFriend(player->m_missionBriefTargetFB);
                } else if (player->m_targetFriendId > 0) {
                    PostFacebookMessageTargettedMission(true);
                    SendMessageToTarget(true);
                } else {
                    PostFacebookMessage(-1);
                }
            } else {
                if (friendLength > 0) {
                    PostFacebookMessageRandomFriend(player->m_missionBriefTargetFB);
                } else if (player->m_targetFriendId > 0) {
                    PostFacebookMessageTargettedMission(false);
                    SendMessageToTarget(false);
                } else {
                    PostFacebookMessage(-1);
                }
            }
            CWaitWindow* waitWindow = new CWaitWindow();
            m_waitWindow = waitWindow;
            waitWindow->SetBackgroundColor(0x64000000);
            static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(m_waitWindow);
            m_fbPostSuccessCommand = CloseTargetResult;
            m_fbPostFailCommand = CloseTargetResult;
        } else {
            if (id == 0x6e7d5205) {
                m_fbLoginAction = 2;
            } else {
                m_fbLoginAction = 3;
            }
            static_cast<App*>(WindowApp::m_instance)->StartLogin(App::LoginFacebook, true);
        }
        break;
    }
    case 3: {
        CDialogWindow* dialog = new CDialogWindow(true);
        dialog->SetButtons(CDialogWindow::BUTTON_OK);
        dialog->SetMessageText(XString("Save game data conflict occured. The game will now exit. Please re-launch the game."), NULL);
        dialog->SetTitleText("Ok");
        dialog->SetButtonsCommands(0x42650932, -1, -1);
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddToFront(dialog);
        static_cast<App*>(WindowApp::m_instance)->m_timeScale = 0.0f;
        break;
    }
    case 0x224665c1:
        // revive accepted
        CBH_Player::GetInstance()->Revive();
        m_reviveOffered = true;
        ChangeState(Playing, STATE_REPLACE);
        ChangeCover();
        m_hud->PlayReviveEffect(0.0f);
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_PHOENIX_USE", false, false);
        break;
    case 0x7c73577:
        // revive declined
        m_reviveOffered = true;
        ChangeState(Playing, STATE_REPLACE);
        break;
    case BuyVehicleMinigun: {
        CItem* minigun = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->getItemById(VehicleMinigunId);
        if (CBH_Player::GetInstance()->m_credits >= minigun->getPriceCredits()) {
            m_specialWeaponState = 1;
            CGameAnalytics::logVehicleMissionsMinigun(true);
        } else {
            ShowMinigunNotEnoughMoneyDialog();
        }
        break;
    }
    case DeclineVehicleMinigun:
        CGameAnalytics::logVehicleMissionsMinigun(false);
        break;
    case -0x3ca147:
        static_cast<App*>(WindowApp::m_instance)->m_timeScale = 1.0f;
        m_hud->BlinkIngameShop(5000);
        break;
    case static_cast<int>(0xb8418e92u): {
        static_cast<App*>(WindowApp::m_instance)->m_timeScale = 1.0f;
        CIngameShop* shop = new CIngameShop();
        static_cast<App*>(WindowApp::m_instance)->m_mainLayer->AddModal(shop);
        shop->SetTab(CIngameShop::Ammo);
        break;
    }
    case 0x118aa4cc:
        static_cast<App*>(WindowApp::m_instance)->m_timeScale = 1.0f;
        FinishGameWithResult(BountyResultNoAmmo, true);
        break;
    }
}

void CSwerveGame::SendMessageToTarget(bool firstTarget)
{
    // tell the targetted friend: "tk" target killed (1001) / "rk" revenge (1004)
    CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
    for (int index = 0; index < friends->m_users.Size(); ++index) {
        CNGSRemoteUser* user = friends->m_users[index];
        if (user->GetClientID() == CBH_Player::GetInstance()->m_targetFriendId) {
            unsigned int now = CStdUtil_Cocoa::GetTimeSeconds();
            if (firstTarget) {
                XString text = XString::Format(L"tk:%d", now);
                CObjectMapString payload(CStrWChar(text.c_str()));
                user->SendMessage(&payload, TargetMessageDurationSeconds, 1001, false, NULL);
            } else {
                XString text = XString::Format(L"rk:%d", now);
                CObjectMapString payload(CStrWChar(text.c_str()));
                user->SendMessage(&payload, TargetMessageDurationSeconds, 1004, false, NULL);
            }
        }
    }
}

void CSwerveGame::OnPause()
{
    if (m_currentState != Loading) {
        WindowApp::HandleEvent(Event::COMMAND, 0x121683a7, 0, 0, 0);
    }
}

// Original 0x169618 (sketch from the decompilation): only while the scene is ready and no
// tutorial hint is queued; every pointer event is consumed.
void CSwerveGame::OnPointerEvent(Event& event)
{
    if (m_sceneReady && m_hud->m_tutorialHints.size() == 0) {
        unsigned int pointer = event.m_param3;
        if (pointer > 9) {
            return;
        }
        int x = event.m_param1;
        int y = event.m_param2;
        if (event.m_type == Event::POINTER_RELEASED) {
            HandleTouchReleased(x, y, pointer);
        } else if (event.m_type == Event::POINTER_DRAGGED) {
            HandleTouchDragged(x, y, pointer);
        } else if (event.m_type == Event::POINTER_PRESSED) {
            HandleTouchPressed(x, y, pointer);
        }
        event.Clear();
    }
}

// Original 0x169488: the HUD result 4 is the medkit button (the HUD itself does not heal).
bool CSwerveGame::HandleTouchPressed(int x, int y, int pointer)
{
    int control = m_hud->PointerPressed(x, y, pointer);
    if (control == 4) {
        CBH_Player::GetInstance()->RestoreHealth();
    } else if (control == 2) {
        if (static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->m_shootType != 2) {
            m_cursor->Fire();
        }
    } else if (control == -1 && m_cursor != NULL) {
        if (pointer < 1) {
            if (!m_player->IsInHide()) {
                m_cursor->PointerPressed(x, y);
            }
        } else {
            if (!static_cast<App*>(WindowApp::m_instance)->m_settings->IsFireButtonEnabled()) {
                m_cursor->Fire();
            }
        }
    }
    m_pointerDown[pointer] = true;
    m_pointerX[pointer] = x;
    m_pointerY[pointer] = y;
    return true;
}

void CSwerveGame::HandleTouchReleased(int x, int y, int pointer)
{
    m_hud->CheckContollStateOnPointerReleased(x, y, pointer);
    m_pointerDown[pointer] = false;
    m_pointerX[pointer] = 0;
    m_pointerY[pointer] = 0;
    if (m_cursor != NULL) {
        m_cursor->PointerReleased();
    }
}

void CSwerveGame::HandleTouchDragged(int x, int y, int pointer)
{
    if (!CBH_Player::GetInstance()->IsInHide()) {
        if (!CBH_Player::GetInstance()->IsInProgress()) {
            if (y > m_height) {
                y = 0;
            }
            m_hud->CheckContollStateOnPointerMove(x, y, m_pointerX[pointer], m_pointerY[pointer], pointer);
            if (m_pointerDown[pointer]) {
                if (m_cursor != NULL && !m_slowMode) {
                    int deltaY = y - m_pointerY[pointer];
                    int deltaX = x - m_pointerX[pointer];
                    m_cursor->PointerMove(x, y, deltaX, deltaY);
                }
                m_pointerX[pointer] = x;
                m_pointerY[pointer] = y;
            }
        }
    }
}

void CSwerveGame::Update()
{
    UpdateChallenges();
    Window::Update();
    App* app = static_cast<App*>(WindowApp::m_instance);
    int elapsed = app->m_updateFrameTime;
    int introTime = app->m_huntingInfo->m_assignedData.m_introTimeMs;
    // the dock cover button stays hidden during the intro time
    m_hud->SetDockCoverButtonVisibility(introTime >= 0 && m_missionElapsedMs < introTime);
    if (m_player->IsInProgress() && m_coverAnimation != NULL
        && m_coverAnimation->isAnimationComplete(m_missionElapsedMs)) {
        m_hud->EnableDockCoverButton(true);
        m_player->EndCover();
    }
    if (m_stateDelay >= 0) {
        m_stateDelay -= elapsed;
        if (m_stateDelay < 0) {
            fireInterrupt();
        }
    }
    if (!m_sceneReady) {
        return;
    }
    AmbientSoundsUpdate(elapsed);
    // CBountyResultScreen::m_accepted (private, +0xf8): the player closed the result screen
    if (!m_exitAfterDialogs || !m_dialogQueue->IsEmpty()) {
        if (m_resultScreen == NULL || !*(reinterpret_cast<bool*>(m_resultScreen) + 0xf8)) {
            if (m_dialogQueue->IsEmpty() && m_exitAfterDialogs) {
                m_dialogQueue->Start();
            }
            switch (m_currentState) {
            case Results:
                if (m_dialogQueue->IsEmpty()) {
                    ChangeState(Playing, STATE_REPLACE);
                }
                break;
            case Loading:
                break;
            case CameraIntro: {
                int fade = m_loadingFadeMs - elapsed;
                if (fade < 0) {
                    fade = 0;
                }
                m_loadingFadeMs = fade;
                UpdateEnvironment(elapsed, false);
                m_stateDelay = 0;
                break;
            }
            case 3:
            case 6:
                UpdateEnvironment(elapsed, false);
                break;
            case Playing:
                if (m_resultScreen != NULL) {
                    break;
                }
                CheckDialogConditions();
                // fall through
            default:
                HuntingUpdate(elapsed);
                break;
            }
            canShoot = true;
            return;
        }
    }
    SetInterrupt(1);
}

void CSwerveGame::HuntingUpdate(int elapsed)
{
    TryToSwitchWeapon();
    UpdateSpawingNewMinionForMovingCamera();
    UpdateHitechVisor();
    if (!static_cast<App*>(WindowApp::m_instance)->m_tutorialManager->IsPlayerInTutorial() && static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_cameraDurationMs < 1) {
        if (CBH_Player::GetInstance()->IsMissionVersionNew() || CBH_Player::GetInstance()->m_newMissionVersion) {
            CBH_Player::GetInstance()->ChangeMissionVersion();
            CDialogWindow* dialog = new CDialogWindow(true);
            dialog->SetTitleText("IDS_DIALOG_MISSION_NEW_IN_VER_150_TITLE");
            dialog->SetMessageText(ResString("IDS_DIALOG_MISSION_NEW_IN_VER_150"), NULL);
            dialog->SetButtons(CDialogWindow::BUTTON_OK);
            dialog->SetButtonsCommands(-0x3ca147, -1, -1);
            static_cast<App*>(WindowApp::m_instance)->m_timeScale = 0.0f;
            m_parent->AddModal(dialog);
        }
    }
    if (CBH_Player::GetInstance()->m_hp < CBH_Player::GetInstance()->GetMaxHp() && !m_shopDamageHintShown) {
        m_shopDamageHintShown = true;
        m_hud->BlinkIngameShop(2000);
    }
    int worldElapsed = elapsed;
    if (CBH_Player::GetInstance()->IsSpeedMode()) {
        worldElapsed = elapsed / m_slowSpeedMultiplier;
    }
    m_explosionAnimation[0]->animate(m_missionElapsedMs);
    UpdateGameFinishing(elapsed);
    CBH_Player::GetInstance()->GamePlayTick(elapsed);
    int repeat = m_dialogRepeatMs - elapsed;
    if (repeat < 0) {
        repeat = 0;
    }
    m_dialogRepeatMs = repeat;
    if (!m_bossCreated && static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_bossSpawnsLast
        && GetAlivaUnitByType(UnitWarrior) == 0) {
        WindowApp::HandleTunnelCommand(static_cast<int>(0xd46374d1u), 0, 0, 0);
        if (static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_scriptType != NULL) {
            CreateScriptedBoss();
        } else {
            CreateOriginalBoss();
        }
    }
    DefineWinnerSide();
    UpdateMissionCameraTimer();
    UpdateMultiplierTimer(elapsed);
    if (static_cast<App*>(WindowApp::m_instance)->m_settings->IsFireButtonEnabled() && m_hud->m_fireHeld) {
        if (static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->m_shootType == 2) {
            if (static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->IsReadyForShot()) {
                m_cursor->Fire();
            }
        }
    }
    if (m_cursor != NULL) {
        if (static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->IsReadyForAutoShot()) {
            Shot(m_cursor->GetAimX(), m_cursor->GetAimY());
        }
    }
    // leave the scope while reloading, return to it afterwards
    if (static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->IsReloading() && !m_inScopeRoutine) {
        m_inScopeRoutine = true;
        if (m_hud->m_scoped) {
            m_hud->switchScope();
            m_inScopeRoutineDone = true;
        }
    } else if (!static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->IsReloading() && m_inScopeRoutine) {
        if (!m_hud->m_scoped && m_inScopeRoutineDone) {
            m_hud->switchScope();
            m_inScopeRoutineDone = false;
        }
        m_inScopeRoutine = false;
    }
    if (m_switchScopeWhenReady) {
        if (static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->IsReadyForUse()) {
            m_hud->switchScope();
            m_switchScopeWhenReady = false;
        }
    }
    if (m_reviveShieldMs > 0) {
        m_reviveShieldMs -= elapsed;
    } else {
        m_playTimeMs += elapsed;
        m_huntCounter += worldElapsed;
        bool shotEffect = m_camera->IsShotEffectInProgress();
        UpdateFOV(elapsed);
        UpdateWarTime(worldElapsed);
        UpdatePeaceTime(worldElapsed);
        int stun = m_stunMs - worldElapsed;
        if (stun < 0) {
            stun = 0;
        }
        m_stunMs = stun;
        if (m_grenadeLauncher != NULL) {
            m_grenadeLauncher->Simulate(worldElapsed * 0.001f);
        }
        UpdateUnits(worldElapsed);
        UpdateObjects(worldElapsed);
        UnitGarbageCollector();
        UpdateEnvironment(worldElapsed, false);
        UpdatePopups(elapsed);
        if (!shotEffect) {
            static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->update(elapsed);
            if (m_cursor != NULL && m_currentState == Playing) {
                m_cursor->update(elapsed);
            }
        }
    }
}

void CSwerveGame::ShowObjective()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CHuntingInfo::SAssignedData& assignment = app->m_huntingInfo->m_assignedData;
    if (m_player->m_missionBriefImage == NULL) {
        return;
    }

    CDialogWindow* dialog;
    if (assignment.m_hasTarget) {
        CBeforeTargettingDialog* target;
        if (m_player->m_targetFriendId >= 0) {
            target = new CBeforeTargettingDialog(m_player->m_targetFriendId);
        } else {
            target = new CBeforeTargettingDialog(m_player->m_missionBriefTargetFB);
        }
        target->SetMessageText(m_player->m_missionBriefText);
        dialog = target;
    } else {
        dialog = new CBeforeHuntDialog();
        dialog->SetMessageText(m_player->m_missionBriefText);
    }
    dialog->SetExtraImageFromImageID(m_player->m_missionBriefImage);
    dialog->SetTitleText(m_player->m_missionBriefTitle);
    dialog->SetMissionRewards(assignment.m_moneyReward, assignment.m_experienceReward,
                              m_player->m_missionBriefEnergy, assignment.m_hasTarget);
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    m_dialogQueue->Add(dialog);
    m_dialogQueue->Start();
}

void CSwerveGame::ShowReturnToGpsConfirmation()
{
    if (!m_sceneReady || m_finishing) {
        return;
    }
    CTextDialog* dialog = new CTextDialog();
    dialog->SetTitleText("IDS_DIALOG_TITLE_WARNING");
    dialog->SetMessageText(ResString("IDS_RETURN_TO_GPS_TEXT"));
    dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_IGNORE);
    dialog->SetButtonsCommands(0x1105a3a5, -1, -1);
    m_dialogQueue->Add(dialog);
    m_dialogQueue->Start();
}

void CSwerveGame::AmbientSoundsUpdate(int)
{
    if (static_cast<App*>(WindowApp::m_instance)->m_timeScale == 0.0f) {
        return;
    }
    const char* ambient = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_ambientSound;
    if (CBH_Player::GetInstance()->IsSpeedMode()) {
        // slow motion: heart beat instead of the ambient sound
        if (!static_cast<App*>(WindowApp::m_instance)->m_soundManager->IsPlaying("IDM_HEART_BEAT")) {
            static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_HEART_BEAT", false, false);
        }
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->Pause(ambient);
        return;
    }
    if (!static_cast<App*>(WindowApp::m_instance)->m_soundManager->IsPlaying(ambient) || static_cast<App*>(WindowApp::m_instance)->m_soundManager->IsPaused(ambient)) {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound(ambient, true, false);
    }
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->Stop("IDM_HEART_BEAT");
}

void CSwerveGame::AddStunTimerMS(int elapsed)
{
    if (elapsed > 0) {
        m_stunMs += elapsed;
    }
}

void CSwerveGame::Shot(int x, int y)
{
    if (m_finishing) {
        return;
    }
    if (m_hideFpsCursor) {
        static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_FIRE_EMPTY", false, false);
        return;
    }
    CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon();
    if (weapon == NULL) {
        return;
    }
    if (!weapon->IsReadyForShot() || m_switchScopeWhenReady) {
        return;
    }
    if (!weapon->MakeShot()) {
        return;
    }
    m_camera->Shot(1.2f, 200);
    CBH_Player::GetInstance()->m_challengeStats->RegisterShot(weapon);
    if (weapon->m_projectileType != 0) {
        LaunchGrenade(*weapon);
        return;
    }
    m_environment->shot(x, y);
    m_pickedUnits.setSize(0);
    CBH_Player::GetInstance()->m_gameStats.AddShotsNumber(1);
    // the units were picked by UpdateUnits; a spread weapon picks again
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit != NULL && unit->CanBeHandled()) {
            CUnitBody::Camera camera = m_world.getActiveCamera();
            m_cursor->GetTagX();
            m_cursor->GetTagY();
            bool picked;
            if (weapon->m_spread) {
                picked = unit->SpreadPick(camera, m_collisionGeometry);
            } else {
                EHitType hit;
                picked = unit->GetPickInfo(&hit);
            }
            if (picked) {
                m_pickedUnits.addElement(unit);
            }
        }
    }
    HandleGameObjectShot(weapon, x, y);
    SortUnitPickDistance(m_pickedUnits);
    CBH_Player* player = CBH_Player::GetInstance();
    int count = m_pickedUnits.size();
    int headshotsBefore = player->m_gameStats.m_headshotsNumber;
    if (weapon->m_tranquillizer && count > 1) {
        count = 1;
    }
    int kills = 0;
    bool hitAnyone = false;
    for (int index = 0; index < count; ++index) {
        CUnit* target = m_pickedUnits[index];
        if (target == NULL) {
            return;
        }
        EHitType hit;
        target->GetPickInfo(&hit);
        static_cast<App*>(WindowApp::m_instance)->m_trickShotManager->ResetEvents();
        target->CalcDamageAndKillingPosibility(hit, weapon);
        hitAnyone |= DefineShotProbability(hit, weapon, target);
        kills += target->m_invulnerable;
    }
    static_cast<App*>(WindowApp::m_instance)->m_trickShotManager->SetKillCount(kills);
    ShowLeftUnitPopUp(kills);
    if (!hitAnyone) {
        player->m_gameStats.RegisterShootMissed();
        m_missedShots++;
        CBH_Player::GetInstance()->m_challengeStats->m_headshotsInRow = 0;
    }
    player->m_gameStats.RegisterMultiKill(kills);
    if (IsOnBoat()) {
        int headshots = player->m_gameStats.m_headshotsNumber - headshotsBefore;
        if (headshots > 0) {
            player->m_gameStats.RegisterHeadshotsOnBoat(headshots);
        }
    }
}

void CSwerveGame::LaunchGrenade(const CDH_Weapon& weapon)
{
    if (m_grenadeLauncher == NULL) {
        return;
    }
    vec3 forward = m_camera->GetViewDirection();
    forward.Normalize();
    vec3 position = GetCurrentCameraPos();
    if (!m_hud->m_scoped) {
        vec3 right = vec3(0.0f, 0.0f, 1.0f).Cross(forward);
        right.Normalize();
        vec3 up = forward.Cross(right);
        up.Normalize();
        position = position + right * weapon.m_projectileOffsetX + up * weapon.m_projectileOffsetY;
    }
    m_grenadeLauncher->AddProjectile(position, forward * -33.0f, 1.0f, weapon);
}

void CSwerveGame::ShowLeftUnitPopUp(int defeated)
{
    if (defeated <= 0) {
        return;
    }
    // the original counts the units of the helper's running game
    CSwerveGame* game = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame;
    int warriors = game->GetAlivaUnitByType(UnitWarrior);
    int bosses = game->GetAlivaUnitByType(UnitBoss);
    int remaining = game->m_pendingMovingMinions + bosses + warriors;
    if (static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_bossSpawnsLast) {
        if (!m_bossSpawned) {
            remaining++;
        }
    }
    // original quirk: with the special weapon the defeated units are not subtracted
    bool show = true;
    if (m_specialWeapon == NULL) {
        remaining = remaining - defeated;
        show = remaining >= 0;
    }
    if (show) {
        WindowApp::HandleTunnelCommand(-0x5220a649, remaining, 0, 0);
    }
}

void CSwerveGame::BlastGrenade(const vec3& position, const vec3& normal, const CDH_Weapon& weapon)
{
    if (m_environment != NULL) {
        m_environment->AddBlast(position, normal, 1.0f);
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_soundManager->PlaySound("IDM_GRENADE_EXPLOSION", false, false);
    Explode(position, weapon.damage(), 5, &weapon);
}

void CSwerveGame::PlayInjureEffect(CUnit* unit, bool armor)
{
    if (unit == NULL) {
        return;
    }
    vec3 towardCamera = GetCurrentCameraPos() - unit->m_body.m_position;
    towardCamera.m_z = 0.0f;
    towardCamera = towardCamera * MathLib::InvSqrt(towardCamera.LengthSqr());
    vec3 view = m_camera->GetViewDirection();
    view.m_z = 0.0f;
    view = view * MathLib::InvSqrt(view.LengthSqr());
    if (view.Dot(towardCamera) >= 0.1f) {
        if (m_hud != NULL) {
            // the HUD gets the side of the shooter (sine of the angle), never the armor flag
            float side = view.m_x * towardCamera.m_y - view.m_y * towardCamera.m_x;
            m_hud->PlayEnjureEffect(side, false);
        }
        if (!armor) {
            m_playerInjured = true;
        }
    }
}

void CSwerveGame::CreateExplosionEffects()
{
    CUnitBody::Group worldTransform(m_world.find(DGHelper::getSwerveID("worldTransform")));
    SwerveHelper::Object3D effect = App::LoadObject3D("BIN_M3G_HIT_EFFECT_BLAST");
    m_explosionEffect[0] = CUnitBody::Node(effect.find(SwerveHelper::Id("dummy_blastTransform")));
    if (m_explosionEffect[0] != NULL) {
        worldTransform.addChild(m_explosionEffect[0]);
        m_explosionAnimation[0] = new CDH_Animation(m_explosionEffect[0]);
        m_explosionAnimation[0]->addController(XString("blast"));
    }
}

void CSwerveGame::PlayInternalExplosion(const vec3& position, const vec3& normal, float scale)
{
    if (m_explosionAnimation[m_explosionEffectIndex] == NULL || m_explosionEffect[m_explosionEffectIndex] == NULL) {
        return;
    }
    SwerveHelper::Transform transform = SwerveHelper::NewTransform();
    transform.setIdentity();
    vec3 up(normal.m_y, normal.m_z, normal.m_x);
    Angles angles(normal, up);
    Quaternion quaternion;
    angles.ToQuaternion(quaternion);
    const float translation[3] = { position.m_x, position.m_y, position.m_z };
    const float rotation[4] = { quaternion.m_w, quaternion.m_x, quaternion.m_y, quaternion.m_z };
    const float scales[3] = { scale, scale, scale };
    transform.postTranslate(translation);
    transform.postRotate(rotation);
    transform.postScale(scales);
    m_explosionEffect[m_explosionEffectIndex].setTransform(transform);
    m_explosionAnimation[m_explosionEffectIndex]->setCurrentController(m_missionElapsedMs, 0, false);
    m_explosionEffectIndex = 0;
}

void CSwerveGame::OnExplosionOccured(CExplosiveObject* object)
{
    if (object != NULL) {
        Explode(object->GetWorldPosition(), object->m_damage, object->m_damageRadius, NULL);
    }
}

void CSwerveGame::HandleGameObjectShot(CDH_Weapon* weapon, float x, float y)
{
    if (weapon == NULL) {
        return;
    }
    for (int index = 0; index < m_objects.size(); ++index) {
        IGameObject* object = m_objects[index];
        if (object != NULL && object->CanBeHandled()) {
            DGCamera::Camera camera = m_world.getActiveCamera();
            bool picked = object->Pick(x, y, 1000000.0f, camera);
            if (picked) {
                object->ReceivedDamage(weapon->damage(), 0);
            }
        }
    }
}

void CSwerveGame::Explode(const vec3& position, int damage, int radius, const CDH_Weapon* weapon)
{
    m_blastUnits.setSize(0);
    float radiusSquared = static_cast<float>(radius * radius);
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit != NULL && !unit->m_dead && !unit->m_invulnerable) {
            if ((unit->m_body.m_position - position).LengthSqr() <= radiusSquared) {
                unit->CalcDamageAndKillingPosibility(damage, weapon);
                m_blastUnits.addElement(unit);
            }
        }
    }
    int kills = 0;
    if (m_blastUnits.size() > 0) {
        for (int index = 0; index < m_blastUnits.size(); ++index) {
            CUnit* unit = m_blastUnits[index];
            if (unit != NULL && unit->CanBeHandled()) {
                kills += unit->m_fatalDamage;
            }
        }
        if (kills > 0) {
            CBH_Player::GetInstance()->m_gameStats.RegisterKillsByExplosion(kills);
            ShowLeftUnitPopUp(kills);
        }
    }
    static_cast<App*>(WindowApp::m_instance)->m_trickShotManager->SetKillByExplosionCount(kills);
    for (int index = 0; index < m_blastUnits.size(); ++index) {
        CUnit* unit = m_blastUnits[index];
        if (unit != NULL && unit->CanBeHandled()) {
            unit->ReceivedDamage(false);
        }
    }
    for (int index = 0; index < m_objects.size(); ++index) {
        IGameObject* object = m_objects[index];
        if (object != NULL && object->CanBeHandled()) {
            if ((object->GetWorldPosition() - position).LengthSqr() < radiusSquared) {
                object->ReceivedDamage(damage, 90);
            }
        }
    }
}

bool CSwerveGame::DefineShotProbability(EHitType hit, const CDH_Weapon* weapon, CUnit* unit)
{
    if (weapon == NULL || unit == NULL) {
        return false;
    }
    // The weapon table uses feet and the scene positions are in metres.
    const float FeetSquaredPerMetreSquared = 10.7584f;
    // the original asks the running game of the helper for the camera position
    vec3 cameraPosition = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->GetCurrentCameraPos();
    float dx = cameraPosition.m_x - unit->m_body.m_position.m_x;
    float dy = cameraPosition.m_y - unit->m_body.m_position.m_y;
    float dz = cameraPosition.m_z - unit->m_body.m_position.m_z;
    float distanceSqr = dx * dx + dy * dy + dz * dz;
    if (distanceSqr * FeetSquaredPerMetreSquared > weapon->effectiveRange() * weapon->effectiveRange()) {
        // out of range: a hit in 4 % of the shots only
        if (!CBH_Math::GetProbabiliyOutcome(4)) {
            return false;
        }
        ICDebug::LogMessage("out-of-range hit (%d%% chance): hit_type(%d), unit(%p), weapon(%p)", 4, hit, unit, weapon);
        if (unit->m_fatalDamage) {
            static_cast<App*>(WindowApp::m_instance)->m_trickShotManager->RegisterEvent(CTrickShotManager::LongShot);
            CBH_Player::GetInstance()->m_gameStats.RegisterOutOfRangeKill();
            CBH_Player::GetInstance()->m_challengeStats->m_killsOutOfRange++;
        }
    }
    unit->m_damageType = hit;
    unit->ReceivedDamage(false);
    return true;
}

void CSwerveGame::UpdatePeaceTime(int elapsed)
{
    // the original reads the app's hunting info again for every test
    if (!m_peaceTime || static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->IsAmbush()) {
        return;
    }
    if (!static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_multiplierCountdown) {
        return;
    }
    if (CBH_Player::GetInstance()->IsNeedHide()) {
        m_peaceElapsed += elapsed;
    }
    if (m_peaceElapsed > static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_alertSeconds * 1000) {
        m_peaceTime = false;
    }
}

void CSwerveGame::UpdateWarTime(int elapsed)
{
    if (!m_multiplierEnabled) {
        return;
    }
    m_warElapsed += elapsed;
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CHuntingInfo::SAssignedData& assignment = app->m_huntingInfo->m_assignedData;
    int duration = assignment.m_cameraDurationMs;
    int limit = assignment.m_escapeSeconds * 1000;
    if (duration > 0) {
        limit = duration;
    }
    if (m_warElapsed > limit) {
        CBH_Player::GetInstance()->m_gameStats.RegisterWarTimerEnd();
        m_bossMustEscape = true;
    }
    // remaining time, clamped to [0, limit]
    int remaining = limit - m_warElapsed;
    int shown = 0;
    if (remaining >= 0) {
        shown = remaining;
        if (remaining > limit) {
            shown = limit;
        }
    }
    m_hud->SetCountdownTimerValue(shown);
    m_hud->SetCountdownTimerLimit(limit);
}

inline void CSwerveGame::QueueResult(EBountyResult result)
{
    if (m_delayedResult == BountyResultNoAmmo || m_delayedResult == BountyResultNoDelayedResult) {
        m_delayedResult = result;
        m_resultDelay = 3000;
        m_finishing = true;
    }
}

void CSwerveGame::RegisterUnitNeutralization(EUnitMindType type)
{
    if (type == UnitCivilian && static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_field9c) {
        QueueResult(BountyResultCivilianKilled);
        return;
    }
    if (m_field240 == 3 && type != UnitBoss
        && static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_bossFirst
        && !static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->IsAmbush()) {
        m_bossMustEscape = true;
        return;
    }
    if (type != UnitCivilian) {
        return;
    }
    CHuntingInfo* hunt = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo;
    if (hunt->m_assignedData.m_field9b) {
        hunt->ReduceRewardMoney(hunt->m_assignedData.m_field84);
    }
}

void CSwerveGame::DefineWinnerSide()
{
    int cameraDuration = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_cameraDurationMs;
    if (cameraDuration > 0 && m_missionElapsedMs > cameraDuration) {
        CBH_Player::GetInstance()->m_gameStats.RegisterWarTimerEnd();
        FinishGameWithResult(BountyResultCameraTimeOut, true);
        return;
    }
    if (CBH_Player::GetInstance()->m_hp <= 0) {
        if (!m_reviveOffered && CBH_Player::GetInstance()->CanRevive()) {
            ShowReviveDialog();
            return;
        }
        FinishGameWithResult(BountyResultLose, true);
        return;
    }
    int alive = 0;
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit == NULL || unit->m_type == UnitCivilian) {
            continue;
        }
        if (!unit->m_dead && !unit->m_tranquillized) {
            alive++;
        }
        CHuntingInfo* hunt = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo;
        if (unit->m_dead && hunt->m_assignedData.m_tranquillize && unit->m_type == UnitBoss) {
            QueueResult(BountyResultKilledInsteadOfTranquillized);
            return;
        }
        if (!hunt->m_assignedData.m_tranquillize && unit->m_type == UnitBoss && unit->m_tranquillized) {
            QueueResult(BountyResultBossTranquillizedInsteadOfKilled);
            return;
        }
        if (unit->m_type == UnitWarrior) {
            if (!hunt->m_assignedData.m_bossWins && unit->m_tranquillized) {
                QueueResult(BountyResultHenchmanTranquillizedInsteadOfKilled);
                return;
            }
        } else if (unit->m_type == UnitBoss && hunt->m_assignedData.m_bossWins) {
            if (unit->m_dead || unit->m_tranquillized) {
                // a silent kill is captured only when nobody is fighting any more
                bool fighting = false;
                if (hunt->SilentKill() && !static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->IsAmbush()) {
                    for (int other = 0; other < m_units.size(); ++other) {
                        CUnit* defender = m_units[other];
                        if (defender != NULL && defender->CanBeHandled()
                            && defender->m_mind.HasExecutor(L"CWarTimeExecutor")
                            && !defender->m_mind.HasExecutor(L"CDeathExecutor")) {
                            fighting = true;
                            break;
                        }
                    }
                }
                if (!fighting) {
                    QueueResult(BountyResultCapture);
                    return;
                }
            }
        }
    }
    if (alive == 0 && m_pendingMovingMinions == 0) {
        static_cast<App*>(WindowApp::m_instance)->InitProgressSystem()->RegisterGameEvent(
            static_cast<CProgressSystem::EGameEvent>(3), 0, -1);
        QueueResult(BountyResultWin);
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CAmmo* firstAmmo = app->m_helper->GetGameData()->GetWeapon01(true)->GetActiveAmmo();
    const CAmmo* secondAmmo = app->m_helper->GetGameData()->GetWeapon02(true)->GetActiveAmmo();
    int firstCount = CBH_Player::GetInstance()->GetAvailableAmmo(firstAmmo, true);
    int secondCount = CBH_Player::GetInstance()->GetAvailableAmmo(secondAmmo, true);
    if (secondCount + firstCount < 1 && app->m_timeScale != 0.0f) {
        // out of ammo: offer the shop (ammo tab) or finish the hunt
        CDialogWindow* dialog = new CDialogWindow(true);
        dialog->SetMessageText(ResString("IDS_DIALOG_BUY_MORE_AMMO"), NULL);
        dialog->SetButtons(static_cast<CDialogWindow::EDialogButton>(14), CDialogWindow::BUTTON_IGNORE);
        dialog->SetButtonsCommands(static_cast<int>(0xb8418e92u), 0x118aa4cc, -1);
        app->m_mainLayer->AddModal(dialog);
        app->m_timeScale = 0.0f;
        return;
    }
    if (!app->m_huntingInfo->SilentKill()) {
        return;
    }
    if (app->m_huntingInfo->IsAmbush()) {
        return;
    }
    if (!m_missionAlert) {
        return;
    }
    QueueResult(BountyResultRuinedSilence);
}

void CSwerveGame::ShowReviveDialog()
{
    ChangeState(Results, STATE_REPLACE);
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetMessageText(ResString("IDS_WANT_TO_REVIVE"), NULL);
    dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_IGNORE);
    dialog->SetButtonsCommands(0x224665c1, 0x07c73577, -1);
    m_dialogQueue->Add(dialog);
    m_dialogQueue->Start();
    m_dialogQueue->Start();
    ChangeState(Results, STATE_REPLACE);
}

bool CSwerveGame::HasMissionAmmo()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_BasicGameData* data = app->m_helper->GetGameData();
    CDH_Weapon* weapons[] = {data->GetWeapon01(true), data->GetWeapon02(true)};
    for (unsigned int index = 0; index < sizeof(weapons) / sizeof(weapons[0]); ++index) {
        CDH_Weapon* weapon = weapons[index];
        if (weapon != NULL && (weapon->m_ammo > 0 || m_player->GetAvailableAmmo(weapon->GetActiveAmmo(), true) > 0)) {
            return true;
        }
    }
    return false;
}

void CSwerveGame::ShowSpecialWeaponOffer()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_huntingInfo->m_assignedData.m_cameraDurationMs <= 0) {
        return;
    }
    CDH_Weapon* weapon = app->InitWeaponManager()->GetWeaponById(VehicleMinigunId);
    if (weapon == NULL) {
        return;
    }
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetTitleText(weapon->m_name);
    dialog->SetMessageText(weapon->m_description);
    dialog->SetExtraImageFromImageID("IDB_WEAPON_MENU_MINI_GUN_ICON");
    dialog->AddDownTextWithIcon(XString(weapon->getPriceCredits()), "IDB_SHOP_CREDITS", false, false,
                                static_cast<eGameFont>(8));
    dialog->SetButtons(CDialogWindow::BUTTON_BUY, CDialogWindow::BUTTON_IGNORE);
    dialog->SetButtonsCommands(BuyVehicleMinigun, DeclineVehicleMinigun, -1);
    m_dialogQueue->Add(dialog);
    m_dialogQueue->Start();
}

void CSwerveGame::ShowMinigunNotEnoughMoneyDialog()
{
    CTextDialog* dialog = new CTextDialog();
    dialog->SetTitleText(ResString("IDS_SHOP_NOT_ENOUGH_CREDITS_TITLE"));
    dialog->SetMessageText(ResString("IDS_SHOP_NOT_ENOUGH_CREDITS_TEXT"), NULL);
    dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_BUY);
    dialog->SetButtonsCommands(0, 0x49444b61, -1);
    m_dialogQueue->Add(dialog);
    m_dialogQueue->Start();
    m_dialogQueue->Start();
}

void CSwerveGame::onSwitchToScope()
{
    CBH_Player::GetInstance()->m_challengeStats->RegisterPlayerEnterScope();
    m_hud->OnScopeSwitched();
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    if (weapon != NULL && !weapon->m_scopeAttached) {
        // weapons without a scope only zoom the camera or hide the cross
        if (weapon->m_zoomWithoutScope) {
            if (m_camera != NULL) {
                m_camera->SetFOV(weapon->m_zoomFovMax);
            }
        } else if (m_useWeaponModel) {
            m_cursor->ShowCross(false);
        }
        return;
    }
    m_hud->SetZoomSliderEnable(true);
    // The FPS cursor reports screen pixels; the float overload converts them to 20.12 fixed point.
    m_scopeCursor->setPosition(static_cast<float>(m_fpsCursor->GetAimX()),
                               static_cast<float>(m_fpsCursor->GetAimY()), true);
    setCursor(m_scopeCursor);
}

void CSwerveGame::onSwitchFromScope()
{
    m_cursor->ShowCross(true);
    m_hud->OnScopeSwitched();
    if (m_camera != NULL) {
        m_camera->RestoreOriginalFOV();
    }
}

void CSwerveGame::onScopeInStart()
{
    if (m_camera == NULL) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
    if (weapon == NULL || weapon->m_scopeAttached) {
        return;
    }
    if (!weapon->m_zoomWithoutScope) {
        return;
    }
    SetFinalFOV(weapon->m_zoomFovMax, weapon->m_scopeTime);
}

void CSwerveGame::SetFinalFOV(float fov, int durationMs)
{
    if (m_camera == NULL || durationMs < 1) {
        return;
    }
    // FOV moves linearly from the current value to fov in durationMs (speed per second)
    float current = m_camera->GetFOV();
    m_finalFOV = fov;
    m_fovSpeed = (fov - current) / (float)durationMs * 1000.0f;
    m_fovDurationMs = durationMs;
    m_fovElapsedMs = 0;
}

void CSwerveGame::UpdateFOV(int elapsed)
{
    if (m_fovDurationMs > 0 && m_camera != NULL) {
        float step = m_fovSpeed * elapsed / 1000.0f;
        m_camera->SetFOV(m_camera->GetFOV() + step);
        m_fovElapsedMs += elapsed;
        if (m_fovElapsedMs >= m_fovDurationMs) {
            m_fovDurationMs = 0;
        }
    }
}

void CSwerveGame::onScopeOutStart()
{
    if (m_camera == NULL) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
    if (weapon == NULL) {
        return;
    }
    // the original leaves a scoped weapon alone here
    if (weapon->m_scopeAttached) {
        return;
    }
    if (!weapon->m_zoomWithoutScope) {
        return;
    }
    m_camera->SetFOV(weapon->m_zoomFovMax);
    SetFinalFOV(m_camera->m_defaultFov, weapon->m_scopeTime);
}

void CSwerveGame::ResetInScopeRoutine()
{
    m_inScopeRoutine = false;
    m_inScopeRoutineDone = false;
}

void CSwerveGame::TryToSwitchWeapon()
{
    // the original only checks the dialog queue and the weapon, and always arms the pending minions
    if (m_specialWeaponState != 1 || !m_dialogQueue->IsEmpty()) {
        return;
    }
    CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon();
    if (weapon != NULL && weapon->IsReadyForUse()) {
        if (m_hud != NULL) {
            m_hud->switchWeapons();
        }
    }
    int count = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_additionalMinionCount;
    if (count < 0) {
        count = 0;
    }
    m_pendingMovingMinions = count;
    m_specialWeaponState = 2;
}

void CSwerveGame::OnWeaponHided()
{
    TakeSpecialWeapon();
}

void CSwerveGame::OnWeaponShown()
{
    ReleaseOldWeapon();
}

void CSwerveGame::TakeSpecialWeapon()
{
    if (m_hud == NULL || m_specialWeaponState != 2) {
        return;
    }
    // keep the mission weapons to release their models later (ReleaseOldWeapon)
    m_loadedMissionWeapons[0] = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
    m_loadedMissionWeapons[1] = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
    CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(VehicleMinigunId);
    if (weapon == NULL) {
        return;
    }
    CBH_Player::GetInstance()->ReduceCredits(weapon->getPriceCredits());
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_MENU_PURCHASE_COMPLETE", false, false);
    weapon->loadWeaponModel(false);
    m_hud->CleanBulletIcon();
    m_hud->CleanWeaponImage();
    m_specialWeapon = weapon;
    m_specialWeaponState = 3;
    m_hud->ReInitWeaponButton();
    m_hud->ReInitBulletIcon(m_specialWeapon);
}

void CSwerveGame::ReleaseOldWeapon()
{
    if (m_specialWeaponState != 3) {
        return;
    }
    if (m_loadedMissionWeapons[0] != NULL) {
        static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->release3dResources(m_loadedMissionWeapons[0]->m_id);
    }
    if (m_loadedMissionWeapons[1] != NULL) {
        static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->release3dResources(m_loadedMissionWeapons[1]->m_id);
    }
    m_specialWeaponState = 4;
}

bool CSwerveGame::IsRequiredSpawnNewMinion() const
{
    int alive = 0;
    for (int index = 0; index < m_units.size(); ++index) {
        const CUnit* unit = m_units[index];
        if (unit != NULL && !unit->m_dead && !unit->m_tranquillized) {
            if (unit->m_type == UnitWarrior) {
                alive++;
            }
        }
    }
    return alive < m_spawnedMinions;
}

void CSwerveGame::SpawnNewMinion()
{
    if (m_squadType != NULL && m_squadType->m_minions.size() > 0) {
        const XString& name = m_squadType->GetMiniomName();
        if (!(name == XString(""))) {
            CBH_PersonType* type = static_cast<App*>(WindowApp::m_instance)->InitPersonTypeManager()->GetType(name);
            if (type != NULL && type->m_visualCharacter != NULL) {
                int point = m_aiMap.GetAnySwarmPointID();
                if (point != -1) {
                    CUnit* unit = new CUnit(type, &m_pathCalculator, &m_aiMap);
                    unit->Init(UnitWarrior, point, false, false);
                    unit->m_name = type->m_nationality->GetRandomFullName(type->m_visualCharacter->m_isMan);
                    m_units.addElement(unit);
                }
            }
        }
    }
}

void CSwerveGame::UpdateSpawingNewMinionForMovingCamera()
{
    if (m_specialWeapon != NULL && IsRequiredSpawnNewMinion() && m_pendingMovingMinions > 0) {
        SpawnNewMinion();
        --m_pendingMovingMinions;
    }
}

bool CSwerveGame::IsOnBoat() const
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    return app->m_huntingInfo->m_assignedData.m_locationName == XString("Bridge")
        && (m_cameraId == 0 || m_cameraId == 4);
}

void CSwerveGame::ShowAmmoDialog()
{
    m_waitingForAmmo = true;
    m_hud->ResetFireButton();
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetMessageText(ResString("IDS_DIALOG_BUY_MORE_AMMO"));
    dialog->SetButtons(CDialogWindow::BUTTON_AMMO, CDialogWindow::BUTTON_IGNORE);
    dialog->SetButtonsCommands(-0x47be716e, 0x118aa4cc, -1);
    m_dialogQueue->Add(dialog);
    m_dialogQueue->Start();
    static_cast<App*>(WindowApp::m_instance)->m_timeScale = 0.0f;
}

void CSwerveGame::OpenIngameShop()
{
    if (m_inGameShopOpen || m_hud == NULL) {
        return;
    }
    m_inGameShopOpen = true;
    m_hud->ResetFireButton();
    if (m_cursor != NULL) {
        m_cursor->PointerBreak();
    }
    memset(m_pointerDown, 0, sizeof(m_pointerDown));
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_mainLayer->AddModal(new CIngameShop());
}

void CSwerveGame::Paint(ICGraphics2d& graphics)
{
    if (!m_sceneReady) {
        return;
    }
    switch (m_currentState) {
    case CameraIntro:
        RunPaint3D(graphics);
        if (m_loadingFadeMs > 0 && m_loadingSurface != NULL) {
            App* app = static_cast<App*>(WindowApp::m_instance);
            app->m_helper->DrawImage(m_loadingSurface, 0, 0, m_loadingFadeMs / 1500.0f, 0, 0, 0.0f, 0);
        }
        break;
    case Playing:
    case Results:
        RunPaint3D(graphics);
        if (m_cursor != NULL && (m_hud->m_enabled || m_hud->m_scoped)) {
            if (m_hideFpsCursor && m_cursor == m_fpsCursor) {
                m_cursor->Hide();
                m_hud->Hide(true);
            } else {
                m_cursor->draw();
            }
        }
        break;
    case WeaponIntro:
    case 3:
    case 6:
        RunPaint3D(graphics);
        break;
    }
}

void CSwerveGame::Paint3D(Graphics3D& graphics)
{
    RenderEnvironment(graphics);
    m_environment->paintHitEffects(graphics);
    // The revive effect is a child window of the HUD; it is not painted here.
    if (m_useWeaponModel) {
        if (!m_hud->isWeaponModelVisible()
            && static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->m_scopeAttached) {
            return;
        }
        if (m_slowMode) {
            return;
        }
        static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->paintWeaponModel(graphics);
    }
}

void CSwerveGame::RenderEnvironment(Graphics3D& graphics)
{
    m_skyBox.Update();
    m_skyBox.Render(graphics);
    App::RenderWorld(graphics, m_world);
    if (m_grenadeLauncher != NULL) {
        m_grenadeLauncher->Paint3D(graphics);
    }
}

void CSwerveGame::LoadSkyBox()
{
    m_skyBox.Init(m_world);
}

void CSwerveGame::FinishGameWithResult(EBountyResult result, bool force)
{
    if (m_resultsQueued) {
        return;
    }
    m_hud->SetEnabled(false);
    if (CBH_Player::GetInstance()->IsSpeedMode()) {
        CBH_Player::GetInstance()->StopConsumeReactixControl();
    }
    CBH_Player::GetInstance()->m_challengeStats->RegisterHuntEnd(result);
    GatherChallengeStats(result);
    // a cyber scope is spent by the hunt (or unequipped when none is left)
    CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon01(true);
    if (weapon != NULL && weapon->m_cyberScopeEquipped) {
        if (CBH_Player::GetInstance()->m_cyberScopesCount == 0) {
            weapon->SetCyberScopeEquiped(false);
        } else {
            CBH_Player::GetInstance()->ReduceCyberScopeCount(1);
        }
    }
    weapon = static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->GetWeapon02(true);
    if (weapon != NULL && weapon->m_cyberScopeEquipped) {
        if (CBH_Player::GetInstance()->m_cyberScopesCount == 0) {
            weapon->SetCyberScopeEquiped(false);
        } else {
            CBH_Player::GetInstance()->ReduceCyberScopeCount(1);
        }
    }
    if (result == BountyResultNoAmmo) {
        CBH_Player::GetInstance()->m_gameStats.AddNoAmmoFailBounties(1);
    } else if (result == BountyResultBossEscaped) {
        CBH_Player::GetInstance()->m_gameStats.AddBossEscapedFailBounties(1);
    }
    if (!m_finishing || force) {
        m_finishing = true;
        CHuntingInfo* hunt = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo;
        hunt->SetBountyResult(result);
        if (result != BountyResultPending && !m_resultsQueued) {
            bool win = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->IsPlayerWin();
            CBH_Player* player = CBH_Player::GetInstance();
            if (win) {
                player->AddXp(static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_experienceReward,
                              GAIN_SOURCE_MISSION);
                ShowMissionFinalDialogs();
            } else {
                if (player->m_hp == 0) {
                    CBH_Player::GetInstance()->m_gameStats.AddWoundedFailBounties(1);
                }
                ShowFailResultDialog();
            }
        }
    }
}

void CSwerveGame::GatherChallengeStats(EBountyResult result)
{
    if (result != BountyResultWin && result != BountyResultCapture) {
        return;
    }
    // the original reads the clock and ignores it
    CStdUtil_Cocoa::GetTimeSeconds();
    double seconds = static_cast<double>(static_cast<unsigned int>(m_huntCounter)) / 1000.0;
    CBH_Player::GetInstance()->m_challengeStats->RegisterMissionCompletionTime(static_cast<int>(seconds));
    int medkits = CBH_Player::GetInstance()->m_medkits;
    if (m_medkitsBeforeHunt == medkits) {
        CBH_Player::GetInstance()->m_challengeStats->AddMissionsWithoutMedkitsUsed(1);
    }
    CBH_Player::GetInstance()->m_challengeStats->m_completedBounties++;
    if (!m_playerInjured) {
        CBH_Player::GetInstance()->m_challengeStats->m_missionsWithoutInjury++;
    }
    CHuntingInfo* hunt = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo;
    bool countdownWithoutAlert = hunt->m_assignedData.m_multiplierCountdown && hunt->m_assignedData.m_alertSeconds == 0;
    if (m_missedShots == 0 && (hunt->IsAmbush() | countdownWithoutAlert) == true) {
        CBH_Player::GetInstance()->m_challengeStats->m_ambushWithoutMisses++;
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_cameraDurationMs > 0) {
        CBH_Player::GetInstance()->m_challengeStats->m_movingCameraWins++;
    }
    const CHuntingInfo::SAssignedData& assignment = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData;
    if (assignment.m_weaponGated || assignment.m_requiredWeaponId >= 0) {
        CBH_Player::GetInstance()->m_challengeStats->m_weaponGatedWins++;
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_missionAlert) {
        return;
    }
    CBH_Player::GetInstance()->m_challengeStats->m_missionsWithoutAlert++;
}

void CSwerveGame::UpdateChallenges()
{
    // completed challenges are collected here; AddCompletedChallengesDialogs queues them at the results
    static_cast<App*>(WindowApp::m_instance)->m_challengeManager->Update();
    Vector<CChallenge*> active = static_cast<App*>(WindowApp::m_instance)->m_challengeManager->GetActiveChallenges();
    bool addedDialog = false;
    for (int index = 0; index < active.size(); ++index) {
        CChallenge* challenge = active[index];
        if (static_cast<unsigned int>(challenge->m_completedAt) > m_challengeCheckTime) {
            CTextDialog* dialog = new CTextDialog();
            dialog->SetTitleText("IDS_CHALLENGE_COMPLETE_TITLE");
            XString reward = challenge->GetRewardString(challenge->GetSoloReward());
            XString text = XString::Format(ResString("IDS_CHALLENGE_COMPLETED_SOLO_INGAME").c_str(),
                                           challenge->m_name.c_str(), reward.c_str());
            dialog->SetMessageText(text);
            dialog->SetButtons(CDialogWindow::BUTTON_OK);
            m_challengeDialogs.addElement(dialog);
            if (challenge->GetX1Reward()->m_awarded) {
                CChallengeCompletedDialog* friendDialog = CheckForFriendCompleteChallenge(challenge);
                if (friendDialog != NULL) {
                    m_challengeDialogs.addElement(friendDialog);
                }
            }
            addedDialog = true;
        }
    }
    if (addedDialog) {
        m_challengeCheckTime = CStdUtil_Cocoa::GetTimeSeconds();
    }
}

CChallengeCompletedDialog* CSwerveGame::CheckForFriendCompleteChallenge(CChallenge* challenge)
{
    bool firstAwarded = challenge->GetX1Reward()->m_awarded;
    bool secondAwarded = challenge->GetX2Reward()->m_awarded;
    if (!firstAwarded && !secondAwarded) {
        return NULL;
    }
    XString title;
    XString message;
    XString friendName = challenge->GetFriendName(0);
    if (static_cast<unsigned int>(friendName.ByteSize()) < 4) {
        friendName = CGServeHelper::GetFriendNickname(challenge->GetFriendId(0));
    }
    if (firstAwarded && secondAwarded) {
        XString firstReward = challenge->GetRewardString(challenge->GetX1Reward());
        XString secondReward = challenge->GetRewardString(challenge->GetX2Reward());
        title = ResString("IDS_CHALLENGE_FRIEND_REWARD_MULTIPLE_TITLE");
        message = XString::Format(ResString("IDS_CHALLENGE_FRIEND_REWARD_MULTIPLE_TEXT").c_str(), friendName.c_str(), 2,
                                  challenge->m_name.c_str(), firstReward.c_str(), secondReward.c_str());
    } else {
        // original quirk: the first reward is shown even when only the second one was awarded
        title = ResString("IDS_CHALLENGE_FRIEND_REWARD_TITLE");
        XString reward = challenge->GetRewardString(challenge->GetX1Reward());
        message = XString::Format(ResString("IDS_CHALLENGE_FRIEND_REWARD_MESSAGE").c_str(), friendName.c_str(),
                                  challenge->m_name.c_str(), reward.c_str());
    }
    ICSystemFont* font = ICSystemFont::CreateInstance();
    font->Initialize(SScreenAdaptingConstant(12, 24)(), NULL, 1);
    CChallengeCompletedDialog* dialog = new CChallengeCompletedDialog();
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    dialog->SetTitleText(title);
    dialog->SetMessageText(message, font);
    dialog->SetSound("IDM_COOP_CHALLENGE");
    return dialog;
}

int CSwerveGame::AddLevelUpDialogs()
{
    int gained = CBH_Player::GetInstance()->m_level - m_levelBeforeHunt;
    int count = gained;
    if (count < 0) {
        count = 0;
    }
    if (count > 0) {
        CLevelUpWindow* dialog = NULL;
        for (int step = 1; step <= count; ++step) {
            dialog = AddLevelUpDialogInQueue(m_levelBeforeHunt + step);
            dialog->SetCloseCommand(static_cast<int>(0xb717ef9fu));
        }
        // the last level up dialog of a won hunt opens the result screen
        if (dialog != NULL && !m_exitAfterDialogs) {
            dialog->SetCloseCommand(static_cast<int>(0xd6cad5a6u));
        }
        if (count > 0) {
            static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_fieldc0 = true;
        }
    }
    return count;
}

CLevelUpWindow* CSwerveGame::AddLevelUpDialogInQueue(int level)
{
    XString message;
    message.Append(XString::Format(ResString("IDS_DIALOG_NEW_LEVEL").c_str(), level) + XString("\n"));
    message.Append(ResString("IDS_DIALOG_NEW_LEVEL_ENERGY") + XString("\n"));
    if (static_cast<App*>(WindowApp::m_instance)->InitAvatarManager()->IsNewAvatarUnlocked(level)) {
        message.Append(ResString("IDS_DIALOG_NEW_LEVEL_AVATAR_UNLOCKED") + XString("\n"));
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_informantUnlockOnLevelUp) {
        message.Append(ResString("IDS_DIALOG_NEW_INFORMANT_UNLOCKED") + XString("\n"));
    }
    Vector<XString> weapons = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetUnlockedWeapons(level);
    XString names;
    for (int index = 0; index < weapons.size(); ++index) {
        if (index > 0) {
            names.Append(XString(", "));
        }
        names.Append(weapons[index]);
    }
    if (names.ByteSize() != 0) {
        message.Append(XString::Format(ResString("IDS_DIALOG_NEW_LEVEL_WEAPON_UNLOCKED").c_str(), names.c_str()) + L"\n");
    }
    int healthNow = CBH_Player::GetInstance()->GetMaxHpForLevel(level);
    int healthBefore = CBH_Player::GetInstance()->GetMaxHpForLevel(level - 1);
    message.Append(XString("+") + XString((healthNow - healthBefore) / 1000) + XString(" ")
                   + ResString("IDS_DIALOG_LEVEL_UP_HP"));
    int energyNow = CBH_Player::GetInstance()->GetMaxEnergyForLevel(level);
    int energyBefore = CBH_Player::GetInstance()->GetMaxEnergyForLevel(level - 1);
    if (energyNow - energyBefore > 0) {
        message.Append(XString(" , ") + XString("+") + XString(energyNow - energyBefore) + XString(" ")
                       + ResString("IDS_DIALOG_LEVEL_UP_ENERGY") + XString("\n"));
    }
    // sharing needs a Facebook login, the finished tutorial and the network
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    bool loggedIn = user->isValid() && user->isAuthenticated(SOCIAL_NETWORK_FACEBOOK);
    int tutorialState = CBH_Player::GetInstance()->m_tutorialState;
    bool networkAvailable = static_cast<App*>(WindowApp::m_instance)->m_networkAvailability->IsAvailable();
    bool allowShare = loggedIn && tutorialState > 12 && networkAvailable;
    CLevelUpWindow* dialog = new CLevelUpWindow(message, allowShare);
    m_levelUpPosts.addElement(level);
    m_dialogQueue->Add(dialog);
    m_dialogQueue->Start();
    return dialog;
}

void CSwerveGame::ShowMissionFinalDialogs()
{
    int missionId = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_missionId;
    if (missionId >= 0) {
        CMission* mission = static_cast<App*>(WindowApp::m_instance)->m_missionManager->GetMissionById(missionId);
        for (int index = 0; index < mission->m_finalDialogs.size(); ++index) {
            CMission::Dialog entry = mission->m_finalDialogs[index];
            CDialogWindow* dialog = new CDialogWindow(true);
            dialog->SetExtraImageFromImageID(entry.m_image);
            dialog->SetTitleText(entry.m_title.Upper());
            CBH_PersonType* boss = static_cast<App*>(WindowApp::m_instance)->InitPersonTypeManager()->GetType(mission->m_boss);
            bool isMan = boss->m_visualCharacter->m_isMan;
            XString text = CGPSHuntingItem::ParametrizeTextWithParams(entry.m_text, mission->m_bossName, isMan);
            dialog->SetMessageText(text, NULL);
            dialog->SetButtons(CDialogWindow::BUTTON_OK);
            dialog->SetButtonsCommands(static_cast<int>(0xd6cad5a6u), -1, -1);
            m_dialogQueue->Add(dialog);
            m_dialogQueue->Start();
        }
    }
    AddCompletedChallengesDialogs();
    AddLevelUpDialogs();
    m_failResultPending = false;
    m_exitAfterDialogs = true;
    ShowResultScreen(false);
    ChangeState(Results, STATE_REPLACE);
    m_dialogQueue->Start();
    m_resultsQueued = true;
}

void CSwerveGame::CheckDialogConditions()
{
    if (!m_ambushDialogShown) {
        if (static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->IsAmbush()
            && static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_cameraDurationMs < 1) {
            ShowAmbushDialog();
            m_ambushDialogShown = true;
            return;
        }
    }
    int randomWeaponId = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_randomWeaponId;
    if (!m_randomWeaponIntroduced && randomWeaponId != -1) {
        m_randomWeaponIntroduced = true;
        CDH_Weapon* weapon = static_cast<App*>(WindowApp::m_instance)->InitWeaponManager()->GetWeaponById(randomWeaponId);
        if (weapon == NULL) {
            return;
        }
        CIntroduceItemWindow* introduction = new CIntroduceItemWindow(
            weapon, ResString("IDS_RANDOM_WEAPON_INTRO"), false, true);
        m_dialogQueue->Add(introduction);
        m_dialogQueue->Start();
        m_dialogQueue->Start();
        return;
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_missionId == -1) {
        return;
    }
    // mission dialog conditions: 1 boss below a quarter of his health, 2 scope used, 3 hunt started,
    // 4 every 30 seconds, 5 player hurt; the last one that holds wins
    int condition = -1;
    if (m_seenDialogConditions.indexOf(1) == -1) {
        for (int index = 0; index < m_units.size(); ++index) {
            CUnit* unit = m_units[index];
            if (unit != NULL && unit->m_type == UnitBoss) {
                if (static_cast<float>(unit->m_hp) < static_cast<float>(unit->m_maxHp) * 0.25f) {
                    condition = 1;
                }
            }
        }
    }
    if (m_seenDialogConditions.indexOf(2) == -1 && m_hud->m_scoped) {
        condition = 2;
    }
    if (m_seenDialogConditions.indexOf(3) == -1) {
        condition = 3;
    }
    bool repeatWaiting = m_seenDialogConditions.indexOf(4) != -1 && m_dialogRepeatMs != 0;
    if (!repeatWaiting) {
        condition = 4;
        if (m_seenDialogConditions.indexOf(4) != -1) {
            m_seenDialogConditions.removeElementAt(m_seenDialogConditions.indexOf(4));
        }
        m_dialogRepeatMs = 30000;
    }
    if (m_seenDialogConditions.indexOf(5) == -1) {
        if (CBH_Player::GetInstance()->m_hp < CBH_Player::GetInstance()->GetMaxHp()
            || CBH_Player::GetInstance()->m_fieldc0) {
            condition = 5;
        }
    }
    if (condition == -1) {
        return;
    }
    if (m_seenDialogConditions.indexOf(condition) != -1) {
        return;
    }
    m_seenDialogConditions.addElement(condition);
    ShowDialogs(condition);
}

void CSwerveGame::ShowAmbushDialog()
{
    ChangeState(Results, STATE_REPLACE);
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetTitleText(ResString("IDS_DIALOG_TITLE_AMBUSH"));
    dialog->SetMessageText(app->m_huntingInfo->m_assignedData.m_field6c);
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    m_dialogQueue->Add(dialog);
    m_dialogQueue->Start();
}

void CSwerveGame::ShowDialogs(int condition)
{
    ChangeState(Results, STATE_REPLACE);
    CMission* mission = static_cast<App*>(WindowApp::m_instance)->m_missionManager->GetMissionById(static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_missionId);
    for (int index = 0; index < mission->m_inMissionDialogs.size(); ++index) {
        CMission::Dialog entry = mission->m_inMissionDialogs[index];
        if (entry.m_condition == condition) {
            CDialogWindow* dialog = new CDialogWindow(true);
            dialog->SetExtraImageFromImageID(entry.m_image);
            XString title = entry.m_title;
            // the original sets the image twice
            dialog->SetExtraImageFromImageID(entry.m_image);
            dialog->SetTitleText(title);
            CBH_PersonType* boss = static_cast<App*>(WindowApp::m_instance)->InitPersonTypeManager()->GetType(mission->m_boss);
            bool isMan = boss->m_visualCharacter->m_isMan;
            XString text = CGPSHuntingItem::ParametrizeTextWithParams(entry.m_text, mission->m_bossName, isMan);
            dialog->SetMessageText(text, NULL);
            dialog->SetButtons(CDialogWindow::BUTTON_OK);
            m_dialogQueue->Add(dialog);
            m_dialogQueue->Start();
        }
    }
    m_dialogQueue->Start();
}

void CSwerveGame::ShowResultScreen(bool fadeIn)
{
    m_resultScreen = new CBountyResultScreen(fadeIn);
    m_dialogQueue->Add(m_resultScreen);
    m_resultsQueued = true;
    ChangeState(Results, STATE_REPLACE);
}

void CSwerveGame::ShowFailResultDialog()
{
    CTextDialog* dialog = new CTextDialog();
    dialog->SetTitleText("IDS_DIALOG_TITLE_FAIL");
    int result = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_result;
    XString message;
    switch (result) {
    case 1:
        message = ResString("IDS_BOUNTY_DEATH");
        break;
    case 2:
        message = ResString("IDS_BOUNTY_KILL_INSTEAD_TRANQ");
        break;
    case 3:
        message = ResString("IDS_BOUNTY_TRANQ_INSTEAD_KILL_BOSS");
        break;
    case 4:
        message = ResString("IDS_BOUNTY_TRANQ_INSTEAD_KILL_HENCHMAN");
        break;
    case 5:
        message = ResString("IDS_BOUNTY_BOSS_RUNAWAY");
        break;
    case 6:
        message = ResString("IDS_BOUNTY_CIVILIAN_KILLED");
        break;
    case 7:
        message = ResString("IDS_BOUNTY_OUT_OF_AMMO");
        break;
    case 8:
        message = ResString("IDS_BOUNTY_DEFEAT_MOVING_CAMERA_TIME_OUT");
        break;
    case 9:
        message = ResString("IDS_BOUNTY_DEFEAT_RUINED_SILENCE");
        break;
    }
    dialog->SetMessageText(message, NULL);
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    m_dialogQueue->Add(dialog);
    m_dialogQueue->Start();
    ChangeState(Results, STATE_REPLACE);
    m_resultsQueued = true;
    AddCompletedChallengesDialogs();
    AddLevelUpDialogs();
    m_exitAfterDialogs = true;
    m_dialogQueue->Start();
    CGameAnalytics::logMissionFailed(static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_missionName, result);
}

void CSwerveGame::UpdateGameFinishing(int elapsedMs)
{
    if (m_delayedResult != BountyResultNoDelayedResult) {
        m_resultDelay -= elapsedMs;
        if (m_resultDelay <= 0) {
            FinishGameWithResult(m_delayedResult, true);
            m_delayedResult = BountyResultNoDelayedResult;
        }
    }
}

void CSwerveGame::UpdateMultiplierTimer(int elapsedMs)
{
    if (m_slowMode) {
        return;
    }
    int multiplier = m_multiplier;
    if (multiplier == 1) {
        return;
    }
    m_multiplierTimeMs -= elapsedMs;
    if (m_multiplierTimeMs >= 0) {
        return;
    }
    // the current level ran out: drop one level (never below x1)
    int lowered = multiplier - 1;
    m_multiplierTimeMs = 0;
    if (lowered < 1) {
        lowered = 1;
    }
    if (lowered == 2) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        m_multiplierTimeMs = app->m_huntingInfo->m_assignedData.m_doubleMultiplierSeconds * 1000;
    }
    if (multiplier != lowered) {
        m_hud->SetCountdownTimerMultiplier(lowered);
    }
    m_multiplier = lowered;
}

void CSwerveGame::UpdateMissionCameraTimer()
{
    if (m_hud == NULL) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    int duration = app->m_huntingInfo->m_assignedData.m_cameraDurationMs;
    if (duration < 1) {
        return;
    }
    m_hud->SetMissionTimerValue(duration - m_missionElapsedMs);
}

typedef char AssertSwerveGameSize[sizeof(CSwerveGame) == 0x4f0 ? 1 : -1];

void CSwerveGame::AddCompletedChallengesDialogs()
{
    UpdateChallenges();
    for (int index = 0; index < m_challengeDialogs.size(); ++index) {
        CTextDialog* dialog = m_challengeDialogs[index];
        dialog->SetSound("IDM_COOP_CHALLENGE");
        m_dialogQueue->Add(dialog);
        m_dialogQueue->Start();
    }
}

CBH_PersonType* CSwerveGame::GetPersonaForBoss() const
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CHuntingInfo* hunt = app->m_huntingInfo;
    if (hunt->m_assignedData.m_hasTarget) {
        // a hunt with a target always fights the rival sniper
        CBH_PersonTypeManager* manager = app->InitPersonTypeManager();
        return manager->GetType(XString(L"RivalSniper"));
    }
    return hunt->m_assignedData.m_bossType;
}

void CSwerveGame::CleanupInternalEffects()
{
    if (m_explosionAnimation[0] != NULL) {
        delete m_explosionAnimation[0];
        m_explosionAnimation[0] = NULL;
    }
    m_explosionEffect[0].Clear();
    for (int index = 0; index < 10; ++index) {
        if (m_bodyBlastAnimations[index] != NULL) {
            delete m_bodyBlastAnimations[index];
            m_bodyBlastAnimations[index] = NULL;
        }
        m_bodyBlastEffects[index].Clear();
    }
}

void CSwerveGame::SortUnitPickDistance(Vector<CUnit*>& units)
{
    // insertion sort by pick distance, stops at the first empty slot
    for (int index = 0; index < units.size() && units[index] != NULL; ++index) {
        CUnit* unit = units[index];
        float distance = unit->m_body.m_pickDistance;
        int destination = index;
        while (destination > 0 && units[destination - 1]->m_body.m_pickDistance > distance) {
            units[destination] = units[destination - 1];
            --destination;
        }
        units[destination] = unit;
    }
}

void CSwerveGame::DeleteVehicles()
{
    for (int index = 0; index < m_vehicles.size(); ++index) {
        m_vehicles[index].Clear();
        m_vehicleTypes[index]->ReleaseVehicleModelCopy();
    }
    m_vehicles.removeAllElements();
    m_vehicleTypes.removeAllElements();
}

void CSwerveGame::PreliminaryUnitsSumulation()
{
    // let the units walk for a while so the scene does not start frozen
    m_preSimulatingUnits = true;
    int count = m_units.size();
    for (int step = 0; step < 2000; ++step) {
        for (int index = 0; index < count; ++index) {
            CUnit* unit = m_units[index];
            if (unit != NULL) {
                int elapsed = CRandGen::GetInstance()->GetRandRange(0, 10) + 55;
                if (elapsed < 0) {
                    elapsed = 0;
                }
                unit->Tick(elapsed);
            }
        }
    }
    m_preSimulatingUnits = false;
}

/** Set `fog` on every appearance of `mesh`. */
void ApplyFog(Mesh mesh, Fog fog)
{
    int count = mesh.getSubmeshCount();
    for (int index = 0; index < count; ++index) {
        IAppearanceInterface* handle = NULL;
        mesh.getAppearance(index, &handle);
        Appearance appearance(handle);
        if (appearance != NULL) {
            appearance.setFog(fog);
        }
    }
}

void CSwerveGame::initHunting()
{
    CreateScene(false);
    // the app pointer is read again after CreateScriptedBoss (no cached local in the original)
    if (!static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_bossSpawnsLast) {
        CreateScriptedBoss();
    }
    static_cast<App*>(WindowApp::m_instance)->m_helper->m_gamePlayManager.RestoreAmmo();
    PutAllSquadsInPlace();
    ChangeState(WeaponIntro, STATE_GOTO);
    m_hitechVisor = CBH_Player::GetInstance()->m_hitechVisorEquipped;
    if (!m_hitechVisor) {
        return;
    }
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit != NULL) {
            unit->m_body.ShowDistanceTag(true);
            unit->m_body.SetAsKnown();
        }
    }
}

__attribute__((noinline)) void CSwerveGame::FixPlotUnitPositon()
{
    // move the boss (or else the first warrior) to an attack point of the current camera
    // the original reads the app and the hunting info again for every test
    if (static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_scriptType != NULL) {
        return;
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->IsAmbush()) {
        return;
    }
    if (static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_fielda1) {
        return;
    }
    CUnit* unit = NULL;
    for (int index = 0; index < m_units.size(); ++index) {
        if (m_units[index]->m_type == UnitBoss) {
            unit = m_units[index];
            break;
        }
    }
    if (unit == NULL) {
        for (int index = 0; index < m_units.size(); ++index) {
            if (m_units[index]->m_type == UnitWarrior) {
                unit = m_units[index];
                break;
            }
        }
    }
    if (unit == NULL) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    int point = m_aiMap.GetAnyAttackPointID(app->m_helper->m_swerveGame->m_cameraId);
    unit->m_mind.CancelAllExecutors();
    unit->m_body.SetToPoint(point);
    unit->InitBehaviour(false, false);
}

void CSwerveGame::disableSpritePicking(::Group group)
{
    if (group.m_interface == NULL) {
        return;
    }
    for (int index = 0; index < group.getChildCount(); ++index) {
        ::Sprite3D sprite = group.getChild(index);
        if (sprite.m_interface != NULL) {
            sprite.setPickingEnable(false);
        } else {
            disableSpritePicking(group.getChild(index));
        }
    }
}

/** Set `fog` on every mesh below `group` (sub groups first, then meshes). */
void SetChildsFog(Group group, Fog fog)
{
    if (group.m_interface == NULL) {
        return;
    }
    for (int index = 0; index < group.getChildCount(); ++index) {
        Group childGroup = group.getChild(index);
        if (childGroup.m_interface != NULL) {
            SetChildsFog(childGroup, fog);
        } else {
            Mesh mesh = group.getChild(index);
            if (mesh.m_interface != NULL) {
                ApplyFog(mesh, fog);
            }
        }
    }
}

void CSwerveGame::CreateVehicles()
{
    if (App::m_platformType == 0) {
        return;
    }
    m_vehicleTypes.ensureCapacity(10);
    m_vehicles.ensureCapacity(10);
    // the level marks parking places with groups named vehicle_01Transform, vehicle_02Transform, ...
    for (unsigned int number = 1; ; ++number) {
        char name[100];
        ICStdUtil::SPrintF(name, "vehicle_%02uTransform", number);
        SwerveHelper::Object3D found = m_world.find(DGHelper::getSwerveID(name));
        if (found == NULL) {
            return;
        }
        Group place(found);
        if (place.m_interface == NULL) {
            return;
        }
        const CVehicleType* type = static_cast<App*>(WindowApp::m_instance)->m_vehicleManager->GetRandomVehicle();
        if (type != NULL) {
            Group model = type->GetModelCopy();
            if (model.m_interface != NULL) {
                model.setPickingEnable(true);
                place.addChild(model);
                m_vehicleTypes.addElement(type);
                m_vehicles.addElement(model);
            }
        }
    }
}

/** Build the 3D scene of the hunt (original @0x16d2c0, 912 B). */
void CSwerveGame::CreateScene(bool reload)
{
    static_cast<App*>(WindowApp::m_instance)->m_helper->GetGameData()->SetActiveWeapon(0);
    m_usedHudButton = false;
    m_usedSteadyAim = false;
    if (static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->IsAmbush()) {
        m_peaceTime = false;
    }
    m_environment = new DGWorld(m_world, NULL);
    DGHelper* helper = static_cast<App*>(WindowApp::m_instance)->m_helper;
    int screenWidth = helper->m_screenWidth;
    int screenHeight = helper->m_screenHeight;
    m_viewport[0] = 0;
    m_viewport[1] = 0;
    m_viewport[2] = static_cast<short>(screenWidth);
    m_viewport[3] = static_cast<short>(screenHeight);
    SetRandomCamera();
    setCamera(static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData.m_cameraIndex);
    CBH_Player::GetInstance()->AddHp(CBH_Player::GetInstance()->GetMaxHp());
    CBH_Player::GetInstance()->m_fieldc0 = false;

    SwerveHelper::Object3D worldTransform = m_world.find(DGHelper::getSwerveID("worldTransform"));
    int rain = -1;
    static_cast<App*>(WindowApp::m_instance)->m_helper->getUserParameter(worldTransform, "rain", &rain);

    // scene bounds: two dummies mark opposite corners, default is a 200 x 200 square
    SwerveHelper::Transform transform(CSwerve::GetInstance()->m_swerve);
    vec3 firstCorner;
    vec3 secondCorner;
    SwerveHelper::Node firstDummy(m_world.find(DGHelper::getSwerveID("dummy_size_1Transform")));
    SwerveHelper::Node secondDummy(m_world.find(DGHelper::getSwerveID("dummy_size_2Transform")));
    bool hasBounds = firstDummy != ::Object3D() && secondDummy != ::Object3D();
    if (hasBounds) {
        firstDummy.getTransformTo(m_world, transform);
        DGHelper::TransformPoint(transform, firstCorner);
        secondDummy.getTransformTo(m_world, transform);
        DGHelper::TransformPoint(transform, secondCorner);
        m_sceneMin.m_x = secondCorner.m_x;
        if (firstCorner.m_x < secondCorner.m_x) {
            m_sceneMin.m_x = firstCorner.m_x;
        }
        m_sceneMin.m_y = secondCorner.m_y;
        if (firstCorner.m_y < secondCorner.m_y) {
            m_sceneMin.m_y = firstCorner.m_y;
        }
        m_sceneMax.m_x = secondCorner.m_x;
        if (secondCorner.m_x < firstCorner.m_x) {
            m_sceneMax.m_x = firstCorner.m_x;
        }
        m_sceneMax.m_y = secondCorner.m_y;
        if (secondCorner.m_y < firstCorner.m_y) {
            m_sceneMax.m_y = firstCorner.m_y;
        }
    } else {
        m_sceneMin.m_x = -100.0f;
        m_sceneMin.m_y = -100.0f;
        m_sceneMax.m_x = 100.0f;
        m_sceneMax.m_y = 100.0f;
    }

    m_cameraCount = getCamerasCount();
    m_hud->setCamerasCount(m_cameraCount);
    m_sceneReady = true;
    if (m_hud != NULL) {
        m_hud->m_commandsReady = true;
    }
    FixPlotUnitPositon();
    const CHuntingInfo::SAssignedData& assignment = static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_assignedData;
    CGameAnalytics::logGameplayEnter(assignment.m_missionName, assignment.m_moneyReward,
                                     assignment.m_experienceReward);
    // `reload` is not read by the original body
}

void CSwerveGame::CreateBodyBlastEffects()
{
    CUnitBody::Group worldTransform(m_world.find(DGHelper::getSwerveID("worldTransform")));
    SwerveHelper::Object3D effect = App::LoadObject3D("BIN_M3G_EFFECT_BODY_BLAST_SYSTEM");
    // the original looks the same dummy up ten times in one loaded object
    for (int index = 0; index < 10; ++index) {
        m_bodyBlastEffects[index] = CUnitBody::Node(effect.find(SwerveHelper::Id("dummy_h_blastTransform")));
        if (m_bodyBlastEffects[index] != NULL) {
            worldTransform.addChild(m_bodyBlastEffects[index]);
            m_bodyBlastAnimations[index] = new CDH_Animation(m_bodyBlastEffects[index]);
            m_bodyBlastAnimations[index]->addController(XString("blast"));
        }
    }
}

void CSwerveGame::PlayInternalBodyBlast(const vec3& position)
{
    if (m_bodyBlastAnimations[m_bodyBlastIndex] == NULL || m_bodyBlastEffects[m_bodyBlastIndex] == NULL) {
        return;
    }
    float translation[3];
    SwerveHelper::Transform transform = SwerveHelper::NewTransform();
    transform.setIdentity();
    float x = position.m_x;
    float y = position.m_y;
    float z = position.m_z;
    translation[0] = x;
    translation[1] = y;
    translation[2] = z;
    transform.postTranslate(translation);
    m_bodyBlastEffects[m_bodyBlastIndex].setTransform(transform);
    m_bodyBlastAnimations[m_bodyBlastIndex]->setCurrentController(m_missionElapsedMs, 0, false);
    m_bodyBlastIndex = (m_bodyBlastIndex + 1) % 10;
}

void CSwerveGame::SetRandomCamera()
{
    CUnitBody::Group root(m_world.find(SwerveHelper::Id("RootNode")));
    disableSpritePicking(root);
    // the level fog lives on the appearance named "fog"; spread it over the whole scene
    Appearance fogHolder(m_world.find(SwerveHelper::Id("fog")));
    if (fogHolder.m_interface != NULL) {
        Fog fog = fogHolder.getFog();
        if (root.m_interface != NULL && fog.m_interface != NULL) {
            SetChildsFog(root, fog);
        }
    }
    // The original starts at Camera01 before selecting the mission's camera.
    XString name = XString::Format(L"Camera%02d", 1);
    XString::AnsiString ansiName(name.c_str());
    Camera camera(m_world.find(SwerveHelper::Id(ansiName)));
    if (camera == NULL) {
        return;
    }
    m_cameraId = 0;
    m_world.setActiveCamera(camera);
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_helper->SetCameraSize(camera, app->m_helper->m_screenWidth, app->m_helper->m_screenHeight,
                                 CAMERA_SIZE_SET_ASPECT);
}

/**
 * Free the 3D scene (original @0x16db90). The original also released four
 * downtown HUD surfaces. Their load path and the unit position table still need mapping.
 */
void CSwerveGame::DeleteScene()
{
    // the original reads WindowApp::m_instance again for every use
    for (int index = 0; index < 4; ++index) {
        static_cast<App*>(WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(s_downtownCameraSurfaces.m_names[index]);
    }
    m_vectorF0.removeAllElements();
    if (m_usedHudButton) {
        static_cast<App*>(WindowApp::m_instance)->m_helper->GetPlayerData()->m_field60[0]++;
    }
    if (m_usedSteadyAim) {
        static_cast<App*>(WindowApp::m_instance)->m_helper->GetPlayerData()->m_field60[1]++;
    }
    m_sceneReady = false;
    SAFE_DELETE_VECTOR(m_units);
    SAFE_DELETE_VECTOR(m_objects);
    DeleteCursors();
    if (m_grenadeLauncher != NULL) {
        delete m_grenadeLauncher;
        m_grenadeLauncher = NULL;
    }
    if (m_collision != NULL) {
        delete m_collision;
        m_collision = NULL;
    }
    if (m_camera != NULL) {
        delete m_camera;
        m_camera = NULL;
    }
    if (m_environment != NULL) {
        delete m_environment;
        m_environment = NULL;
    }
    if (m_coverAnimation != NULL) {
        delete m_coverAnimation;
        m_coverAnimation = NULL;
    }
    SAFE_DELETE_VECTOR(m_sceneAnimations);
    CleanupInternalEffects();
    DeleteVehicles();
    m_cameraHud.Clear();
    if (m_squad != NULL) {
        delete m_squad;
        m_squad = NULL;
    }
}

bool CSwerveGame::PostFacebookMessage(int level)
{
    if (!CNGS::GetInstance()->GetLocalUser()->isAuthenticated(SOCIAL_NETWORK_FACEBOOK)) {
        return false;
    }
    CNGSSocialMessage* message = CNGSSocialMessage::CreateInstance(SOCIAL_NETWORK_FACEBOOK);
    XString text = XString::Format(ResString("IDS_FB_SHARE_LEVEL_UP").c_str(), level);
    message->SetParam("message", text.c_str());
    message->SetParam("picture", L"http://gcs.glu.com/gcs/images/ContractKiller_114icon.jpg");
    message->SetParam("name", ResString("IDS_FB_LINK_DESC").c_str());
    message->SetParam("link", GSERV_FB_LINK);
    message->SetParam("caption", ResString("IDS_FB_CAPTION").c_str());
    message->SetParam("description", ResString("IDS_FB_DESCRIPTION").c_str());
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_messageManager->AddMessage(message, SOCIAL_NETWORK_FACEBOOK);
    CGameAnalytics::logPostFbWall(XString("Level Up"));
    return true;
}

void CSwerveGame::PostFacebookMessageRandomFriend(CStrWChar friendId)
{
    // the original asks for the Facebook interface and ignores the result
    CNGS_Platform::GetInstance()->getSocialInterface(SOCIAL_NETWORK_FACEBOOK);
    XString text = ResString("IDS_FB_SHARE_KILLED");
    CNGSSocialMessage* message = CNGSSocialMessage::CreateInstance(SOCIAL_NETWORK_FACEBOOK);
    message->SetParam("target_id", friendId.GetWCharPtr());
    message->SetParam("message", text.c_str());
    message->SetParam("name", ResString("IDS_FB_LINK_DESC").c_str());
    message->SetParam("link", GSERV_FB_LINK);
    message->SetParam("caption", ResString("IDS_FB_CAPTION").c_str());
    message->SetParam("description", ResString("IDS_FB_DESCRIPTION").c_str());
    message->SetParam("picture", L"http://gcs.glu.com/gcs/images/ContractKiller_114icon.jpg");
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_messageManager->AddMessage(message, SOCIAL_NETWORK_FACEBOOK);
    CGameAnalytics::logPostFbWall(XString("Random Friend Target"));
}

void CSwerveGame::PostFacebookMessageTargettedMission(bool killed)
{
    CNGSRemoteUserList* friends = CNGS::GetInstance()->GetLocalUser()->m_remoteUserLists[0];
    for (int index = 0; index < friends->m_users.Size(); ++index) {
        CNGSRemoteUser* user = friends->m_users[index];
        if (user->GetClientID() != CBH_Player::GetInstance()->m_targetFriendId) {
            continue;
        }
        CNGSSocialMessage* message = CNGSSocialMessage::CreateInstance(SOCIAL_NETWORK_FACEBOOK);
        XString text("");
        if (killed) {
            text = ResString("IDS_FB_SHARE_KILLED");
        } else {
            XString format = ResString("IDS_FB_SHARE_AVENGED");
            text = XString::Format(format.c_str(), user->GetNickName().GetWCharPtr());
        }
        CStrWChar targetId;
        if (user != NULL) {
            targetId = user->m_credentials.getIDForSocialNetwork(SOCIAL_NETWORK_FACEBOOK);
            if (targetId.GetLength() >= 0) {
                message->SetParam("target_id", targetId.GetWCharPtr());
                message->SetParam("message", text.c_str());
                message->SetParam("name", ResString("IDS_FB_LINK_DESC").c_str());
                message->SetParam("link", GSERV_FB_LINK);
                message->SetParam("caption", ResString("IDS_FB_CAPTION").c_str());
                message->SetParam("description", ResString("IDS_FB_DESCRIPTION").c_str());
                message->SetParam("picture", L"http://gcs.glu.com/gcs/images/ContractKiller_114icon.jpg");
                App* app = static_cast<App*>(WindowApp::m_instance);
                app->m_messageManager->AddMessage(message, SOCIAL_NETWORK_FACEBOOK);
                if (killed) {
                    CGameAnalytics::logPostFbWall(XString("Friend Target"));
                } else {
                    CGameAnalytics::logPostFbWall(XString("Friend Revenge"));
                }
            }
        }
    }
}
