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
const char* const FailureMessages[] = {
    NULL,
    "IDS_BOUNTY_DEATH",
    "IDS_BOUNTY_KILL_INSTEAD_TRANQ",
    "IDS_BOUNTY_TRANQ_INSTEAD_KILL_BOSS",
    "IDS_BOUNTY_TRANQ_INSTEAD_KILL_HENCHMAN",
    "IDS_BOUNTY_BOSS_RUNAWAY",
    "IDS_BOUNTY_CIVILIAN_KILLED",
    "IDS_BOUNTY_OUT_OF_AMMO",
    "IDS_BOUNTY_DEFEAT_MOVING_CAMERA_TIME_OUT",
    "IDS_BOUNTY_DEFEAT_RUINED_SILENCE"
};


void RemoveUnitReference(Vector<CUnit*>& units, CUnit* unit)
{
    for (int index = units.size() - 1; index >= 0; --index) {
        if (units[index] == unit) {
            units.removeElementAt(index);
        }
    }
}

CDialogWindow* CreateMissionDialog(const CMission::Dialog& entry, const CMission& mission,
                                   const CBH_PersonType* boss, bool upperCase)
{
    bool isMan = true;
    if (boss != NULL && boss->m_visualCharacter != NULL) {
        isMan = boss->m_visualCharacter->m_isMan;
    }
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetExtraImageFromImageID(entry.m_image);
    XString title = entry.m_title;
    if (upperCase) {
        title = title.Upper();
    }
    dialog->SetTitleText(title);
    dialog->SetMessageText(CGPSHuntingItem::ParametrizeTextWithParams(entry.m_text, mission.m_bossName, isMan));
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    return dialog;
}
}

CSwerveGame* CSwerveGame::m_pInstance;

/** Store link of the Facebook stories. */
static const wchar_t GSERV_FB_LINK[] = L"http://gcs.glu.com/gcs/iphone/iphone.jsp?t=bounty&src=facebook&p=iphone";

/** Camera HUD surfaces of the downtown map (filled by the static initializer, released by DeleteScene). */
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
int canShoot;
CStrWChar hpText;

CSwerveGame::CSwerveGame(ICRenderSurface* surface, signed char cameraId)
    : Window(false), m_tagImages(8), m_finishing(false), m_huntStart(0), m_huntCounter(0),
      m_challengeCheckTime(CStdUtil_Cocoa::GetTimeSeconds()),
      m_previousAutoAimHorizontal(0.0f), m_previousAutoAimVertical(0.0f), m_camera(NULL),
      m_collisionIntersection(CSwerve::GetInstance()->m_swerve),
      m_coverAnimation(NULL), m_resultScreen(NULL), m_resultsQueued(false), m_unitInfo(NULL),
      m_multiplier(3), m_multiplierEnabled(false),
      m_ambushDialogShown(false), m_reviveOffered(false), m_waitingForAmmo(false), m_inGameShopOpen(false),
      m_randomWeaponIntroduced(false),
      m_hitechVisor(false), m_bossCreated(false), m_bossRunningAway(false), m_bossEscapePopupTime(0),
      m_delayedResult(BountyResultNoDelayedResult), m_resultDelay(0), m_reviveShieldMs(0), m_environment(NULL),
      m_squad(NULL), m_slowMode(false), m_peaceTime(true), m_bossMustEscape(false), m_missionAlert(false),
      m_spawnedMinions(0), m_squadType(NULL),
      m_peaceElapsed(0), m_warElapsed(0), m_aimedUnit(NULL), m_aimedTimeMs(0), m_missedShots(0), m_stunMs(0),
      m_collision(NULL), m_grenadeLauncher(NULL), m_cursor(NULL), m_fpsCursor(NULL),
      m_scopeCursor(NULL), m_sceneReady(false), m_hud(NULL), m_cameraCount(0),
      m_loadingSurface(surface),
      m_loadingFadeMs(1500), m_cameraId(cameraId), m_field394(-1), m_stateDelay(-1), m_field39c(-1),
      m_exitAfterDialogs(false), m_dialogRepeatMs(0), m_missionElapsedMs(0), m_coverDuration(0),
      m_usedHudButton(false), m_usedSteadyAim(false), m_dragging(false), m_pointerX(0), m_pointerY(0), m_specialWeapon(NULL), m_specialWeaponState(0),
      m_pendingMovingMinions(0), m_previousMissionWeapon(NULL), m_previousMissionAmmo(0),
      m_finalFOV(0.0f), m_fovSpeed(0.0f), m_fovDurationMs(0), m_fovElapsedMs(0)
{
    m_explosionAnimation[0] = NULL;
    m_explosionEffectIndex = 0;
    for (int index = 0; index < 10; ++index) {
        m_bodyBlastAnimations[index] = NULL;
    }
    m_bodyBlastIndex = 0;
    m_loadedMissionWeapons[0] = NULL;
    m_loadedMissionWeapons[1] = NULL;
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_multiplierTimeMs = app->m_huntingInfo->m_assignedData.m_tripleMultiplierSeconds * 1000;
    m_pInstance = this;
    CApplet::m_pCore->SetMultipleTouchEnabled(true);
    m_player = CBH_Player::GetInstance();
    m_player->m_gameStats.RegisterHuntStart();
    app->m_helper->initSwerveGame(this);
    m_units.ensureCapacity(10);
    m_slowSpeedMultiplier = app->m_helper->m_config.GetIntegerValueByToken("GO_PILL_SPEED_MULTIPLIER", true);
    if (m_slowSpeedMultiplier < 1) {
        m_slowSpeedMultiplier = 1;
    }
    m_medkitsBeforeHunt = m_player->m_medkits;
    m_dialogQueue = new CWindowQueue();
    AddToFront(m_dialogQueue);
}

CSwerveGame::~CSwerveGame()
{
    CleanUp();
    m_tagImages.DeleteContents();
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_helper->m_swerveGame == this) {
        app->m_helper->SetSwerveGamePtr(NULL);
        CApplet::m_pCore->SetMultipleTouchEnabled(false);
    }
    if (m_pInstance == this) {
        m_pInstance = NULL;
    }
}

void CSwerveGame::Init()
{
    m_playerInjured = false;
    m_shopDamageHintShown = false;
    m_player->ResetBeforeHunting();
    m_player->m_field39 = false;
    m_player->m_fieldc0 = false;
    m_levelBeforeHunt = m_player->GetLevel();
    CApplet::m_pCore->SetTickRate(10);
    ChangeState(Loading, STATE_REPLACE);
    Activate();
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_player->m_challengeStats->RegisterHuntStart(app->m_huntingInfo->m_assignedData.m_locationName);
    m_multiplierEnabled = app->m_huntingInfo->m_assignedData.m_multiplierCountdown;
    app->m_trickShotManager->ResetEvents();
    app->m_trickShotManager->SetKillCount(0);
    if (!m_multiplierEnabled) {
        m_multiplier = 1;
    }
}

void CSwerveGame::Shutdown()
{
    m_resources.removeAllKeysets();
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_soundManager->StopAllSoundsAndRelease();
}

void CSwerveGame::CleanUp()
{
    m_particles.Free();
    delete m_explosionAnimation[0];
    m_explosionAnimation[0] = NULL;
    m_explosionEffect[0].Clear();
    m_explosionEffectIndex = 0;
    if (m_unitInfo != NULL) {
        m_unitInfo->Close();
        m_unitInfo = NULL;
    }
    m_aimedUnit = NULL;
    m_aimedTimeMs = 0;
    m_previousAutoAimHorizontal = 0.0f;
    m_previousAutoAimVertical = 0.0f;
    if (m_specialWeapon != NULL) {
        m_player->SetEstimatedWeapon(m_previousMissionWeapon, m_previousMissionAmmo);
        m_specialWeapon->unfetchWeaponModel();
        m_specialWeapon->releaseWeaponModel();
        m_specialWeapon = NULL;
    }
    for (int slot = 0; slot < 2; ++slot) {
        if (m_loadedMissionWeapons[slot] != NULL) {
            m_loadedMissionWeapons[slot]->releaseWeaponModel();
            m_loadedMissionWeapons[slot] = NULL;
        }
    }
    DeleteCursors();
    if (IsChildExisting(m_hud)) {
        if (m_hud->HasUsedSteadyAim()) {
            App* app = static_cast<App*>(WindowApp::m_instance);
            ++app->m_helper->GetPlayerData()->m_field60[1];
        }
        Remove(m_hud);
        m_hud->DeleteChilds();
        delete m_hud;
    }
    m_hud = NULL;
    delete m_camera;
    m_camera = NULL;
    delete m_coverAnimation;
    m_coverAnimation = NULL;
    delete m_environment;
    m_environment = NULL;
    delete m_grenadeLauncher;
    m_grenadeLauncher = NULL;
    delete m_collision;
    m_collision = NULL;
    SAFE_DELETE_VECTOR(m_units);
    SAFE_DELETE_VECTOR(m_objects);
    m_pickedUnits.removeAllElements();
    m_blastUnits.removeAllElements();
    delete m_squad;
    m_squad = NULL;
    m_collisionGeometry.Clear();
    m_cameraHud.Clear();
    m_skyBox.m_skyMesh.Clear();
    m_skyBox.m_skyWorld.Clear();
    m_skyBox.m_world.Clear();
    m_world.Clear();
    m_sceneReady = false;
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CHuntingInfo::SAssignedData& assignment = app->m_huntingInfo->m_assignedData;
    CLocation* const* location = app->m_locationManager->m_locations.Get(assignment.m_locationName);
    if (location == NULL || *location == NULL) {
        return;
    }
    XString::AnsiString resource((*location)->m_m3g.c_str());
    m_world = World(App::LoadObject3D(resource));
    if (m_world != NULL) {
        FixWorldPosition();
        DefineCollisionGeometry();
        if (App::m_platformType == 0) {
            SwitchOffEyeCandyGeometry();
        }
    }
    if (assignment.m_ambientSound != NULL) {
        app->m_soundManager->PrecacheSound(assignment.m_ambientSound);
    }
}

void CSwerveGame::SwitchOffEyeCandyGeometry()
{
    const char* const groupNames[] = {"LightSourceTransform", "EyeCandyTransform"};
    for (unsigned int index = 0; index < sizeof(groupNames) / sizeof(groupNames[0]); ++index) {
        CUnitBody::Group group(m_world.find(DGHelper::getSwerveID(groupNames[index])));
        if (group != NULL) {
            CUnitBody::Group parent(group.getParent());
            if (parent != NULL) {
                parent.removeChild(group);
            }
        }
    }
}

void CSwerveGame::FixWorldPosition()
{
    SwerveHelper::Transformable root(m_world.find(DGHelper::getSwerveID("worldTransform")));
    if (root != NULL) {
        float origin[3] = {0.0f, 0.0f, 0.0f};
        root.setTranslation(origin);
    }
}

void CSwerveGame::DefineCollisionGeometry()
{
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
    if (m_world != NULL) {
        CreateObjectImpl("barrel_%02dTransform", "barrel");
        CreateObjectImpl("citybarrel_%02dTransform", "citybarrel");
        CreateObjectImpl("gas_%02dTransform", "gasholder");
    }
}

void CSwerveGame::CreateObjectImpl(const char* pattern, const char* type)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    XString objectName(type);
    for (int index = 1;; ++index) {
        char name[32];
        snprintf(name, sizeof(name), pattern, index);
        SwerveHelper::Group point(m_world.find(DGHelper::getSwerveID(name)));
        if (point == NULL) {
            break;
        }
        IGameObject* object = app->m_gameObjectManager->CreateObjectByName(objectName);
        if (object == NULL) {
            break;
        }
        object->SetToPos(point);
        object->Bind();
        m_objects.addElement(object);
    }
}

void CSwerveGame::CreateUnitByType(const CBH_PersonType* type, EUnitMindType mindType,
                                   const XString& name, bool scripted, CSquad* squad)
{
    if (type == NULL) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CScriptType* script = app->m_huntingInfo->m_assignedData.m_scriptType;
    int point = -1;
    if (scripted && script != NULL) {
        point = script->m_spawnPointId;
    } else if (mindType == UnitCivilian) {
        point = m_aiMap.GetAnyPathPointID();
    } else {
        point = m_aiMap.GetAnySwarmPointID();
    }
    if (point < 0 || point >= m_aiMap.m_specialPoints.size()) {
        return;
    }
    CUnit* unit = new CUnit(type, &m_pathCalculator, &m_aiMap);
    if (unit->m_body.m_model == NULL) {
        delete unit;
        return;
    }
    if (squad != NULL) {
        if (mindType == UnitBoss) {
            squad->AddLeader(unit);
        } else if (mindType == UnitWarrior) {
            squad->AddMember(unit);
        }
    }
    unit->Init(mindType, point, scripted, squad != NULL);
    if (squad != NULL) {
        squad->NoticeUnitParams(unit);
    }
    unit->m_name = name;
    m_units.addElement(unit);
    if (mindType == UnitBoss) {
        m_bossCreated = true;
    }
}

void CSwerveGame::CreateOriginalBoss()
{
    if (!m_bossCreated) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        const CHuntingInfo::SAssignedData& assignment = app->m_huntingInfo->m_assignedData;
        CreateUnitByType(assignment.m_bossType, UnitBoss, assignment.m_bossName, false, m_squad);
    }
}

void CSwerveGame::CreateScriptedBoss()
{
    if (!m_bossCreated) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        const CHuntingInfo::SAssignedData& assignment = app->m_huntingInfo->m_assignedData;
        if (assignment.m_scriptType != NULL) {
            CreateUnitByType(assignment.m_bossType, UnitBoss, assignment.m_bossName, true, m_squad);
        }
    }
}

void CSwerveGame::InitUnits()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CHuntingInfo::SAssignedData& assignment = app->m_huntingInfo->m_assignedData;
    if (assignment.m_fielda1) {
        m_squad = new CSquad();
    }
    if (assignment.m_scriptType == NULL && !assignment.m_bossSpawnsLast) {
        CreateOriginalBoss();
    }
    m_squadType = assignment.m_squadType;
    if (m_squadType != NULL) {
        const Vector<XString>& minions = m_squadType->m_minions;
        for (int index = 0; index < minions.size(); ++index) {
            const XString& name = m_squadType->GetMiniomNameByIndex(index);
            if (name.Length() != 0) {
                CreateUnitByName(name, UnitWarrior, m_squad);
                ++m_spawnedMinions;
            }
        }
        if (CanSpawnCivilians()) {
            CreateCivilians();
        }
    }
}

void CSwerveGame::CreateUnitByName(const XString& name, EUnitMindType mindType, CSquad* squad)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CBH_PersonType* person = app->InitPersonTypeManager()->GetType(name);
    if (person != NULL && person->m_visualCharacter != NULL) {
        XString displayName;
        if (person->m_nationality != NULL) {
            displayName = person->m_nationality->GetRandomFullName(person->m_visualCharacter->m_isMan);
        }
        CreateUnitByType(person, mindType, displayName, false, squad);
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CSquadType* civilians = app->InitSquadTypeManager()->GetType(XString("CivilianSquad"));
    if (civilians == NULL || civilians->m_minions.size() == 0) {
        return;
    }
    int count = app->m_huntingInfo->m_assignedData.m_civilianCount;
    for (int index = 0; index < count; ++index) {
        const XString& name = civilians->GetMiniomName();
        if (name.Length() != 0) {
            CreateUnitByName(name, UnitCivilian, NULL);
        }
    }
}

void CSwerveGame::PutAllSquadsInPlace()
{
    if (m_squad == NULL || m_squad->m_leader == NULL) {
        return;
    }
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit->m_squad != NULL && unit != m_squad->m_leader) {
            unit->m_body.m_position = m_squad->GetLocalSquadPosition(unit);
            unit->m_body.m_cellId = m_squad->m_leader->m_body.m_cellId;
            unit->m_body.UpdatePosition(0);
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
    CUnit* aimedUnit = NULL;
    m_bossRunningAway = false;
    float aimX = 0.0f;
    float aimY = 0.0f;
    float obstacleDistance = -1.0f;
    bool canTag = m_camera != NULL && m_cursor != NULL && m_currentState == Playing
        && !IsIntroTime() && !m_finishing && !m_player->IsInHide() && !m_player->IsInProgress()
        && !m_hud->IsModelAnimationActive();
    if (canTag) {
        aimX = m_cursor->GetTagX();
        aimY = m_cursor->GetTagY();
        obstacleDistance = GetDistanceToCollision(aimX, aimY);
    }
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit == NULL) {
            continue;
        }
        if (m_hitechVisor && unit->CanBeHandled() && !unit->m_body.m_showDistanceTag) {
            unit->m_body.ShowDistanceTag(true);
            unit->m_body.SetAsKnown();
        }
        if (m_camera != NULL) {
            unit->m_body.CheckVisibility(m_camera->m_frustum);
        }
        unit->ResetPickInfo();
        if (canTag && unit->CanBeHandled()) {
            EHitType hit;
            unit->Pick(aimX, aimY, obstacleDistance, m_camera->m_camera);
            if (unit->GetPickInfo(&hit)
                && (aimedUnit == NULL || unit->m_body.m_pickDistance < aimedUnit->m_body.m_pickDistance)) {
                aimedUnit = unit;
            }
        }
        if (unit->CanBeHandled()) {
            AvoidCollision(unit);
            CheckVisibility(unit);
            if (unit->m_type == UnitBoss && unit->m_runningAway) {
                m_bossRunningAway = true;
            }
        }
        unit->Tick(elapsed);
        if (unit->m_squad != NULL) {
            unit->m_squad->NoticeUnitParams(unit);
        }
    }
    if (canTag) {
        UpdateAutoAim(elapsed);
    }
    if (aimedUnit != m_aimedUnit || aimedUnit == NULL || !aimedUnit->CanBeHandled()) {
        m_aimedUnit = aimedUnit;
        m_aimedTimeMs = 0;
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_aimedTimeMs += elapsed;
    if (m_aimedTimeMs > app->InitProgressSystem()->GetTaggingTimeMS()) {
        m_aimedTimeMs = 0;
        ShowUnitInfo(aimedUnit, 0);
    }
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
            if (other != NULL && other != unit && other->CanBeHandled()) {
                unit->m_body.AvoidCollision(&other->m_body);
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    int index = 0;
    while (index < m_units.size()) {
        CUnit* unit = m_units[index];
        if (unit != NULL && !unit->m_removed) {
            ++index;
            continue;
        }

        if (unit != NULL) {
            if (unit->m_type == UnitBoss && !app->m_huntingInfo->IsAmbush()) {
                QueueResult(BountyResultBossEscaped);
            }
            m_aiMap.UnregisterUnitBody(unit->GetBody());
            if (unit->m_squad != NULL) {
                unit->m_squad->LeaveSquad(unit);
                unit->m_squad->m_members.Remove(HashTableKey<const CUnit*>(unit));
            }
            if (m_aimedUnit == unit) {
                m_aimedUnit = NULL;
                m_aimedTimeMs = 0;
            }
            RemoveUnitReference(m_pickedUnits, unit);
            RemoveUnitReference(m_blastUnits, unit);
        }

        const int last = m_units.size() - 1;
        m_units[index] = m_units[last];
        m_units.setSize(last);
        delete unit;
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
        if (other != NULL && other != unit) {
            vec3 offset = other->m_body.m_position - unit->m_body.m_position;
            if (offset.LengthSqr() <= reactionDistanceSq) {
                unit->m_mind.OnSeeUnit(other);
            }
        }
    }
}

void CSwerveGame::UpdateAutoAim(int elapsed)
{
    if (m_dragging || m_camera == NULL) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
    if (weapon == NULL || (!weapon->m_autoAim && !weapon->m_cyberScopeEquipped)) {
        return;
    }
    vec3 cameraPosition = GetCurrentCameraPos();
    vec3 cameraDirection = m_camera->GetViewDirection();
    float cameraLength = sqrtf(cameraDirection.LengthSqr());
    if (cameraLength <= 0.0f) {
        return;
    }
    float cameraHorizontal = MathLib::ArcTan(cameraDirection.m_x, cameraDirection.m_y);
    float bestError = 50.0f;
    float horizontal = 0.0f;
    float vertical = 0.0f;
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit == NULL || !unit->CanBeHandled() || unit->m_invulnerable || unit->m_body.m_headZone == NULL) {
            continue;
        }
        vec3 direction = cameraPosition - unit->m_body.GetAutoAimPointPosition();
        float length = sqrtf(direction.LengthSqr());
        if (length <= 0.0f || cameraDirection.Dot(direction) <= 0.0f) {
            continue;
        }
        float horizontalError = MathLib::ArcTan(direction.m_x, direction.m_y) - cameraHorizontal;
        if (horizontalError > 180.0f) {
            horizontalError -= 360.0f;
        } else if (horizontalError < -180.0f) {
            horizontalError += 360.0f;
        }
        float verticalError = MathLib::ArcSin(direction.m_z / length - cameraDirection.m_z / cameraLength);
        float error = horizontalError * horizontalError + verticalError * verticalError;
        if (error < bestError) {
            bestError = error;
            horizontal = horizontalError;
            vertical = verticalError;
        }
    }
    float previousHorizontal = m_previousAutoAimHorizontal;
    float previousVertical = m_previousAutoAimVertical;
    if (fabsf(previousHorizontal) < 0.01f) {
        previousHorizontal = 0.0f;
    }
    if (fabsf(previousVertical) < 0.01f) {
        previousVertical = 0.0f;
    }
    float horizontalStep = elapsed * 0.005f * fabsf(horizontal) - 0.05f * previousHorizontal;
    float verticalStep = elapsed * 0.005f * fabsf(vertical) - 0.05f * previousVertical;
    if (horizontal != 0.0f && horizontalStep > 0.0f && fabsf(horizontal) >= previousHorizontal) {
        if (horizontal > 0.0f) {
            horizontalStep = -horizontalStep;
        }
        m_camera->SetAngleH(m_camera->m_field0c + horizontalStep, true);
    }
    if (vertical != 0.0f && verticalStep > 0.0f && fabsf(vertical) >= previousVertical) {
        if (vertical > 0.0f) {
            verticalStep = -verticalStep;
        }
        m_camera->SetAngleV(m_camera->m_field1c + verticalStep, true);
    }
    m_previousAutoAimHorizontal = horizontal;
    m_previousAutoAimVertical = vertical;
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
    if (m_collisionGeometry == NULL || m_collisionIntersection == NULL || m_camera == NULL) {
        return -1.0f;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    float relativeX = x / app->m_helper->m_screenWidth;
    float relativeY = y / app->m_helper->m_screenHeight;
    if (m_collisionGeometry.pick(-1, relativeX, relativeY, m_camera->m_camera, m_collisionIntersection)) {
        return m_collisionIntersection.getDistance();
    }
    return -1.0f;
}

vec3 CSwerveGame::GetCurrentCameraPos()
{
    vec3 position;
    if (m_camera != NULL) {
        DGCamera::Transform transform(CSwerve::GetInstance()->m_swerve);
        m_camera->m_camera.getTransformTo(m_world, transform);
        DGHelper::TransformPoint(transform, position);
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_BasicGameData* game = app->m_helper->GetGameData();
    CDH_WeaponsManager* weapons = app->InitWeaponManager();
    m_loadedMissionWeapons[0] = game->GetWeapon01(true);
    m_loadedMissionWeapons[1] = game->GetWeapon02(true);
    for (int slot = 0; slot < 2; ++slot) {
        CDH_Weapon* weapon = m_loadedMissionWeapons[slot];
        if (weapon != NULL) {
            weapons->load3dResources(weapon->m_id, true);
        }
    }
}

void CSwerveGame::LoadSounds()
{
    const char* sounds[] = {
        "IDM_ARMOR_HIT", "IDM_SLOW_START", "IDM_SLOW_END", "IDM_SLOW_GUNSHOT", "IDM_HEART_BEAT",
        "IDM_ULTRASOUND_START", "IDM_ULTRASOUND_LOOP", "IDM_ULTRASOUND_END", "IDM_BARREL_EXPLOSION",
        "IDM_GRENADE_EXPLOSION", "IDM_CHARACTER_BULLET_CLOSE", "IDM_CHARACTER_ENEMY_WOUND_MAN_01",
        "IDM_SPECIAL_DUCK", "IDM_SPECIAL_AP_MEDKIT", "IDM_HEADSHOT", "IDM_MEN_HIT1_37", "IDM_SPECIAL_REVIVE"
    };
    App* app = static_cast<App*>(WindowApp::m_instance);
    for (unsigned int index = 0; index < sizeof(sounds) / sizeof(sounds[0]); ++index) {
        app->m_soundManager->PrecacheSound(sounds[index]);
    }
}

void CSwerveGame::setCamera(int cameraId)
{
    if (m_world.m_interface == NULL) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    char name[20];
    snprintf(name, sizeof(name), "Camera%02d", cameraId + 1);
    DGCamera::Camera camera(m_world.find(DGHelper::getSwerveID(name)));
    if (camera.m_interface != NULL) {
        m_cameraId = cameraId;
        m_world.setActiveCamera(camera);
        app->m_helper->SetCameraSize(camera, app->m_helper->m_screenWidth,
                                     app->m_helper->m_screenHeight, CAMERA_SIZE_SET_ASPECT);
    } else {
        camera = m_world.getActiveCamera();
    }
    delete m_camera;
    m_camera = NULL;
    if (camera.m_interface != NULL) {
        const CHuntingInfo::SAssignedData& assignment = app->m_huntingInfo->m_assignedData;
        m_camera = new DGCamera(camera, assignment.m_cameraHorizontalLimit, assignment.m_cameraVerticalLimit);
    }
}

int CSwerveGame::GetCurrCamId()
{
    return m_cameraId;
}

int CSwerveGame::getCamerasCount()
{
    int count = 0;
    while (m_world.m_interface != NULL) {
        char name[60];
        snprintf(name, sizeof(name), "Camera%02d", count + 1);
        DGCamera::Camera camera(m_world.find(DGHelper::getSwerveID(name)));
        if (camera.m_interface == NULL) {
            break;
        }
        ++count;
    }
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    SG_Instance* sprites = app->m_sgInstance;
    const int archetypes[] = { 9, 0 };
    for (unsigned int index = 0; index < sizeof(archetypes) / sizeof(archetypes[0]); ++index) {
        sprites->QueueArchetypeCharacter(archetypes[index], 0);
        unsigned char finished = 0;
        while (sprites->LoadQueued(1000, finished)) {
        }
    }
}

void CSwerveGame::CreateCursors()
{
    m_fpsCursor = new CFPSCursor();
    m_scopeCursor = new DGScopeCursor();
    m_scopeCursor->setPosition(0, 0, true);
    m_scopeCursor->setConstraints(-10, -10, 10, 10);
    setCursor(m_fpsCursor);
}

void CSwerveGame::DeleteCursors()
{
    if (m_fpsCursor != NULL) {
        delete m_fpsCursor;
    }
    m_fpsCursor = NULL;
    if (m_scopeCursor != NULL) {
        delete m_scopeCursor;
    }
    m_scopeCursor = NULL;
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
    if (m_world == NULL) {
        return;
    }
    m_coverAnimation = new CDH_Animation(m_world);
    int controller = m_coverAnimation->addController(XString("anim_hide01"));
    if (controller >= 0) {
        m_coverDuration = m_coverAnimation->getControllerBaseDuration(controller);
    }
}

bool CSwerveGame::HasCoverAnimation() const
{
    return m_coverAnimation != NULL && m_coverDuration > 0;
}

void CSwerveGame::CreateCameraHud3D()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CHuntingInfo::SAssignedData& assignment = app->m_huntingInfo->m_assignedData;
    if (m_world == NULL || assignment.m_cameraDurationMs <= 0 || assignment.m_cameraHUDModel.Length() == 0) {
        return;
    }
    SwerveHelper::Group mount(m_world.find(DGHelper::getSwerveID("dynamic_camera_hudTransform")));
    if (mount != NULL) {
        XString::AnsiString resource(assignment.m_cameraHUDModel.c_str());
        SwerveHelper::Object3D model = App::LoadObject3D(resource);
        m_cameraHud = SwerveHelper::Group(SwerveHelper::Find(model, "dummy_hudTransform"));
        if (m_cameraHud != NULL) {
            mount.addChild(m_cameraHud);
        }
    }
}

void CSwerveGame::ChangeCover()
{
    if (!HasCoverAnimation() || m_camera == NULL || m_hud == NULL || IsIntroTime() || m_player->IsInProgress()) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    bool hiding = m_player->IsNeedHide();
    int duration = m_coverDuration;
    if (m_player->IsSpeedMode()) {
        duration /= m_slowSpeedMultiplier;
        if (duration < 1) {
            duration = 1;
        }
    }
    if (hiding) {
        CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
        if (weapon != NULL) {
            CGameAnalytics::logCoverUsed(app->m_huntingInfo->m_assignedData.m_missionName, weapon->GetId());
            if (m_cursor == m_scopeCursor) {
                m_hud->switchScope();
            }
        }
        m_camera->centerView(duration);
    } else {
        m_camera->returnToSavedView(duration);
    }
    if (m_cursor != NULL) {
        m_cursor->PointerBreak();
    }
    m_dragging = false;
    m_hud->ResetFireButton();
    m_coverAnimation->setControllerDuration(0, duration);
    m_coverAnimation->setCurrentController(m_missionElapsedMs, 0, !hiding);
    m_player->StartCover();
    m_hud->ChangeDockCoverButtonState();
    m_player->m_gameStats.AddCoversUsed(1);
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
    if (state == Loading) {
        CLoadingScreen* loading = new CLoadingScreen(this);
        loading->AddLoadingStep(&CSwerveGame::LoadLocation);
        loading->AddLoadingStep(&CSwerveGame::LoadAIMap);
        loading->AddLoadingStep(&CSwerveGame::LoadCollision);
        loading->AddLoadingStep(&CSwerveGame::LoadObjects);
        loading->AddLoadingStep(&CSwerveGame::CreateCameraAnimation);
        loading->AddLoadingStep(&CSwerveGame::CreateCameraHud3D);
        loading->AddLoadingStep(&CSwerveGame::LoadWeapon);
        loading->AddLoadingStep(&CSwerveGame::LoadSounds);
        loading->AddLoadingStep(&CSwerveGame::InitScope);
        *window = loading;
    } else {
        if (state == WeaponIntro) {
            App* app = static_cast<App*>(WindowApp::m_instance);
            CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
            if (weapon != NULL) {
                weapon->fetchWeaponModel();
            }
            m_stateDelay = 1000;
        }
        *window = new Window(false);
    }
}

void CSwerveGame::OnStateInterrupt(int state, Window*)
{
    if (state == Loading) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        const CHuntingInfo::SAssignedData& assignment = app->m_huntingInfo->m_assignedData;
        m_peaceTime = !app->m_huntingInfo->IsAmbush();
        if (m_world.m_interface == NULL) {
            SetInterrupt(2);
            return;
        }
        m_environment = new DGWorld(m_world, NULL);
        CreateExplosionEffects();
        m_skyBox.Init(m_world);
        setCamera(assignment.m_cameraIndex);
        if (m_camera == NULL) {
            SetInterrupt(2);
            return;
        }
        app->m_helper->GetGameData()->SetActiveWeapon(0);
        CreateCursors();
        m_hud = new GamePlayHUD();
        AddToFront(m_hud);
        m_cameraCount = getCamerasCount();
        m_hud->setCamerasCount(m_cameraCount);
        InitUnits();
        if (!assignment.m_bossSpawnsLast) {
            CreateScriptedBoss();
            if (assignment.m_bossType != NULL && !m_bossCreated) {
                SetInterrupt(2);
                return;
            }
        }
        PutAllSquadsInPlace();
        m_player->AddHp(m_player->GetMaxHp());
        m_huntStart = CStdUtil_Cocoa::GetTimeSeconds();
        m_huntCounter = 0;
        m_sceneReady = true;
        if (assignment.m_ambientSound != NULL) {
            app->m_soundManager->PlaySound(assignment.m_ambientSound, true, false);
        }
        CGameAnalytics::logGameplayEnter(assignment.m_missionName, assignment.m_moneyReward,
                                         assignment.m_experienceReward);
        ChangeState(CameraIntro, STATE_REPLACE);
    } else if (state == WeaponIntro) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_helper->m_gamePlayManager.RestoreAmmo();
        ChangeState(Playing, STATE_GOTO);
        ShowSpecialWeaponOffer();
    }
}

void CSwerveGame::OnCommand(Event& event)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (event.m_id == BuyVehicleMinigun && m_specialWeaponState == 0 && !m_finishing) {
        CDH_Weapon* weapon = app->InitWeaponManager()->GetWeaponById(VehicleMinigunId);
        if (weapon != NULL) {
            if (m_player->m_credits >= weapon->getPriceCredits()) {
                m_specialWeaponState = 1;
                CGameAnalytics::logVehicleMissionsMinigun(true);
            } else {
                ShowMinigunNotEnoughMoneyDialog();
            }
        }
        event.Clear();
    } else if (event.m_id == DeclineVehicleMinigun) {
        CGameAnalytics::logVehicleMissionsMinigun(false);
        event.Clear();
    } else if (event.m_id == -0x3ca147) {
        m_hud->BlinkIngameShop(5000);
        event.Clear();
    } else if (event.m_id == static_cast<int>(0x811575c5u) && m_currentState == Playing && !m_finishing) {
        ChangeCover();
        event.Clear();
    } else if ((event.m_id == 0x2ebf5358 && m_currentState == Playing && !m_finishing)
               || (event.m_id == -0x47be716e && m_waitingForAmmo)) {
        OpenIngameShop();
        event.Clear();
    } else if (event.m_id == 0x118aa4cc && m_waitingForAmmo) {
        m_waitingForAmmo = false;
        app->m_timeScale = 1.0f;
        FinishGameWithResult(BountyResultNoAmmo, true);
        event.Clear();
    } else if (event.m_id == 0x7e4d23f0 && m_inGameShopOpen) {
        m_inGameShopOpen = false;
        if (m_waitingForAmmo) {
            m_waitingForAmmo = false;
            if (!HasMissionAmmo()) {
                FinishGameWithResult(BountyResultNoAmmo, true);
            }
        }
        event.Clear();
    } else if (event.m_id == 0x224665c1 && m_reviveOffered && m_player->m_hp <= 0) {
        m_player->Revive();
        if (HasCoverAnimation()) {
            ChangeCover();
        } else {
            m_reviveShieldMs = 1500;
        }
        m_hud->PlayReviveEffect(0.0f);
        app->m_soundManager->PlaySound("IDM_PHOENIX_USE", false, false);
        event.Clear();
    } else if (event.m_id == 0x07c73577 && m_reviveOffered && m_player->m_hp <= 0) {
        FinishGameWithResult(BountyResultLose, true);
        event.Clear();
    } else if ((event.m_id == AcceptTargetResult || event.m_id == AcceptRevengeResult)
               && m_resultsQueued && app->m_huntingInfo->IsPlayerWin()) {
        SendMessageToTarget(event.m_id == AcceptTargetResult);
        WindowApp::HandleTunnelCommand(CloseTargetResult, 0, 0, 0);
        event.Clear();
    } else if (event.m_id == 0x1105a3a5) {
        event.Clear();
        if (!m_exitAfterDialogs) {
            CGameAnalytics::logMissionFailed(app->m_huntingInfo->m_assignedData.m_missionName, 40);
            FinishGameWithResult(BountyResultPending, true);
        }
    }
}

void CSwerveGame::SendMessageToTarget(bool firstTarget)
{
    int friendId = m_player->m_targetFriendId;
    if (friendId <= 0) {
        return;
    }
    m_player->m_targetFriendId = -1;
    CNGSLocalUser* local = CNGS::GetInstance()->GetLocalUser();
    CNGSRemoteUser* target = local->m_remoteUserLists[0]->findRemoteUserByID(friendId);
    if (target == NULL) {
        return;
    }
    long long messageType = 1004;
    const wchar_t* format = L"rk:%d";
    if (firstTarget) {
        messageType = 1001;
        format = L"tk:%d";
    }
    XString text = XString::Format(format, CStdUtil_Cocoa::GetTimeSeconds());
    CObjectMapString payload(CStrWChar(text.c_str()));
    target->SendMessage(&payload, TargetMessageDurationSeconds, messageType, false, NULL);
}

void CSwerveGame::OnPause()
{
    if (m_currentState != Loading) {
        WindowApp::HandleEvent(Event::COMMAND, 0x121683a7, 0, 0, 0);
    }
}

void CSwerveGame::OnPointerEvent(Event& event)
{
    if (event.m_type == Event::POINTER_RELEASED) {
        if (m_hud != NULL && HandleTouchReleased(event.m_param1, event.m_param2, event.m_param3)) {
            event.Clear();
        }
        return;
    }
    if (m_currentState != Playing || m_camera == NULL || m_finishing || m_waitingForAmmo || m_inGameShopOpen
        || IsIntroTime() || !m_dialogQueue->IsEmpty() || m_hud->IsModelAnimationActive()) {
        return;
    }
    bool handled = false;
    if (event.m_type == Event::POINTER_PRESSED) {
        handled = HandleTouchPressed(event.m_param1, event.m_param2, event.m_param3);
    } else if (event.m_type == Event::POINTER_DRAGGED) {
        handled = HandleTouchDragged(event.m_param1, event.m_param2, event.m_param3);
    }
    if (handled) {
        event.Clear();
    }
}

bool CSwerveGame::HandleTouchPressed(int x, int y, int pointer)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int control = m_hud->PointerPressed(x, y, pointer);
    if (m_player->IsInHide() || m_player->IsInProgress()) {
        return true;
    }
    if (control != -1) {
        if (control == 2 && m_cursor != NULL) {
            CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
            if (weapon != NULL && weapon->m_shootType != 2) {
                m_cursor->Fire();
            }
        }
        return true;
    }
    if (pointer != 0) {
        if (pointer > 0 && m_cursor != NULL && !app->m_settings->IsFireButtonEnabled()) {
            m_cursor->Fire();
        }
        return pointer > 0;
    }
    m_pointerX = x;
    m_pointerY = y;
    m_dragging = true;
    if (m_cursor != NULL) {
        m_cursor->PointerPressed(x, y);
    }
    return true;
}

bool CSwerveGame::HandleTouchReleased(int x, int y, int pointer)
{
    if (m_hud->IsFirePointer(pointer)) {
        m_hud->CheckContollStateOnPointerReleased(x, y, pointer);
        return true;
    }
    if (pointer != 0 || !m_dragging) {
        return false;
    }
    m_dragging = false;
    if (m_cursor != NULL) {
        m_cursor->PointerReleased();
    }
    return true;
}

bool CSwerveGame::HandleTouchDragged(int x, int y, int pointer)
{
    if (m_hud->IsFirePointer(pointer)) {
        m_hud->CheckContollStateOnPointerMove(x, y, 0, 0, pointer);
        return true;
    }
    if (m_player->IsInHide() || m_player->IsInProgress()) {
        return true;
    }
    if (pointer != 0 || !m_dragging) {
        return false;
    }
    int dx = x - m_pointerX;
    int dy = y - m_pointerY;
    if (m_cursor != NULL) {
        m_cursor->PointerMove(x, y, dx, dy);
    } else {
        App* app = static_cast<App*>(WindowApp::m_instance);
        float turn = m_camera->GetFOV() / app->m_screenHeight;
        if (dx >= 0) {
            m_camera->rotateLeft(dx * turn, false);
        } else {
            m_camera->rotateRight(-dx * turn, false);
        }
        if (dy >= 0) {
            m_camera->rotateUp(dy * turn, false);
        } else {
            m_camera->rotateDown(-dy * turn, false);
        }
    }
    m_pointerX = x;
    m_pointerY = y;
    return true;
}

void CSwerveGame::Update()
{
    Window::Update();
    if (m_exitAfterDialogs && m_dialogQueue->IsEmpty()) {
        SetInterrupt(1);
        return;
    }
    if (!m_sceneReady) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    int elapsed = app->m_updateFrameTime;
    if (!m_dialogQueue->IsEmpty() || m_waitingForAmmo || m_inGameShopOpen) {
        return;
    }
    if (m_currentState == Results && m_ambushDialogShown && !m_resultsQueued && !m_finishing) {
        ChangeState(Playing, STATE_REPLACE);
        return;
    }
    UpdateGameFinishing(elapsed);
    if (m_currentState == CameraIntro) {
        m_loadingFadeMs -= elapsed;
        if (m_loadingFadeMs <= 0) {
            ChangeState(WeaponIntro, STATE_REPLACE);
        }
    } else if (m_currentState == WeaponIntro) {
        m_stateDelay -= elapsed;
        if (m_stateDelay <= 0) {
            fireInterrupt();
        }
    } else if (m_currentState == Playing && !m_finishing) {
        HuntingUpdate(elapsed);
        return;
    }
    UpdateScene(elapsed);
}

void CSwerveGame::HuntingUpdate(int elapsed)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    int worldElapsed = elapsed;
    if (ShowMissionVersionDialog()) {
        return;
    }
    if (!m_shopDamageHintShown && m_player->m_hp < m_player->GetMaxHp()) {
        m_shopDamageHintShown = true;
        m_hud->BlinkIngameShop(2000);
    }
    TryToSwitchWeapon();
    CheckDialogConditions();
    if (!m_dialogQueue->IsEmpty()) {
        return;
    }
    UpdateFOV(elapsed);
    UpdateMultiplierTimer(elapsed);
    if (m_reviveShieldMs > 0) {
        m_reviveShieldMs -= elapsed;
        if (m_reviveShieldMs <= 0) {
            m_player->m_field39 = false;
        }
    }
    if (!IsIntroTime()) {
        if (m_player->IsSpeedMode()) {
            worldElapsed /= m_slowSpeedMultiplier;
        }
        m_player->GamePlayTick(elapsed);
        m_stunMs -= worldElapsed;
        if (m_stunMs < 0) {
            m_stunMs = 0;
        }
        if (m_peaceTime) {
            UpdatePeaceTime(worldElapsed);
        } else {
            UpdateWarTime(worldElapsed);
        }
    }
    AmbientSoundsUpdate(elapsed);
    if (m_cursor != NULL) {
        m_cursor->update(elapsed);
    }
    CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
    if (weapon != NULL) {
        weapon->update(elapsed);
        if (!m_hud->IsModelAnimationActive() && m_cursor != NULL) {
            if ((m_hud->IsFireHeld() && weapon->m_shootType == 2) || weapon->IsReadyForAutoShot()) {
                m_cursor->Fire();
            }
        }
    }
    UpdateScene(worldElapsed);
    UpdatePopups(elapsed);
    if (!m_finishing && !IsIntroTime()) {
        DefineWinnerSide();
    }
}

void CSwerveGame::UpdateScene(int elapsed)
{
    if (m_camera != NULL && !m_finishing) {
        m_camera->m_wanderDisabled = m_hud != NULL && m_hud->IsSteadyAimActive();
    }
    UpdateHitechVisor();
    UpdateUnits(elapsed);
    if (m_currentState == Playing && !m_finishing) {
        UpdateChallenges();
    }
    if (m_currentState == Playing && !m_finishing && !IsIntroTime()) {
        UpdateSpawingNewMinionForMovingCamera();
    }
    UpdateObjects(elapsed);
    UnitGarbageCollector();
    if (m_grenadeLauncher != NULL) {
        m_grenadeLauncher->Simulate(elapsed * 0.001f);
    }
    int environmentElapsed = elapsed;
    if (m_finishing) {
        environmentElapsed = 0;
    }
    UpdateEnvironment(environmentElapsed, false);
    UpdateMissionCameraTimer();
    if (m_explosionAnimation[0] != NULL) {
        m_explosionAnimation[0]->animate(m_missionElapsedMs);
    }
    if (m_player->IsInProgress() && HasCoverAnimation()
        && m_coverAnimation->isAnimationComplete(m_missionElapsedMs)) {
        m_player->EndCover();
    }
}

bool CSwerveGame::ShowMissionVersionDialog()
{
    if (!m_player->IsMissionVersionNew() && !m_player->m_newMissionVersion) {
        return false;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (CTutorialManager::IsPlayerInTutorial() || app->m_huntingInfo->m_assignedData.m_cameraDurationMs > 0) {
        return false;
    }
    m_player->ChangeMissionVersion();
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetTitleText("IDS_DIALOG_MISSION_NEW_IN_VER_150_TITLE");
    dialog->SetMessageText(ResString("IDS_DIALOG_MISSION_NEW_IN_VER_150"));
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    dialog->SetButtonsCommands(-0x3ca147, -1, -1);
    m_dialogQueue->Add(dialog);
    m_dialogQueue->Start();
    return true;
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_timeScale == 0.0f) {
        return;
    }
    CDH_SoundManager* sound = app->m_soundManager;
    const char* ambient = app->m_huntingInfo->m_assignedData.m_ambientSound;
    const char* heartbeat = "IDM_HEART_BEAT";
    if (m_player->IsSpeedMode()) {
        if (!sound->IsPlaying(heartbeat)) {
            sound->PlaySound(heartbeat, false, false);
        }
        if (ambient != NULL && !sound->IsPaused(ambient)) {
            sound->Pause(ambient);
        }
        return;
    }
    if (ambient != NULL) {
        if (sound->IsPaused(ambient)) {
            sound->Resume(ambient);
        } else if (!sound->IsPlaying(ambient)) {
            sound->PlaySound(ambient, true, false);
        }
    }
    sound->Stop(heartbeat);
}

void CSwerveGame::AddStunTimerMS(int elapsed)
{
    if (elapsed > 0) {
        m_stunMs += elapsed;
    }
}

void CSwerveGame::Shot(int x, int y)
{
    if (m_currentState != Playing || m_finishing || m_camera == NULL || IsIntroTime()
        || m_player->IsInHide() || m_player->IsInProgress() || m_hud->IsModelAnimationActive()) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
    if (weapon == NULL || !weapon->IsReadyForShot() || !weapon->MakeShot()) {
        return;
    }
    CTrickShotManager* trickShots = app->m_trickShotManager;
    trickShots->ResetEvents();
    m_player->m_challengeStats->RegisterShot(weapon);
    if (weapon->m_projectileType != 0) {
        LaunchGrenade(*weapon);
        m_camera->Shot(1.2f, 200);
        m_player->m_gameStats.AddShotsNumber(1);
        return;
    }
    if (m_environment != NULL) {
        m_environment->shot(x, y);
    }
    HandleGameObjectShot(weapon, x, y);
    float distance = -1.0f;
    if (!weapon->m_spread) {
        distance = GetDistanceToCollision(x, y);
    }
    m_pickedUnits.setSize(0);
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit->CanBeHandled()) {
            bool picked;
            if (weapon->m_spread) {
                picked = unit->SpreadPick(m_camera->m_camera, m_collisionGeometry);
            } else {
                unit->Pick(x, y, distance, m_camera->m_camera);
                EHitType type;
                picked = unit->GetPickInfo(&type);
            }
            if (picked) {
                m_pickedUnits.addElement(unit);
            }
        }
    }
    m_camera->Shot(1.2f, 200);
    m_player->m_gameStats.AddShotsNumber(1);
    SortUnitPickDistance(m_pickedUnits);
    int headshotsBefore = m_player->m_gameStats.m_headshotsNumber;
    bool hitAnyone = false;
    int kills = 0;
    int count = m_pickedUnits.size();
    if (weapon->m_tranquillizer && count > 1) {
        count = 1;
    }
    for (int index = 0; index < count; ++index) {
        CUnit* target = m_pickedUnits[index];
        EHitType hit;
        target->GetPickInfo(&hit);
        trickShots->ResetEvents();
        target->CalcDamageAndKillingPosibility(hit, weapon);
        if (DefineShotProbability(hit, weapon, target)) {
            hitAnyone = true;
            if (target->m_dead) {
                ++kills;
            }
        }
    }
    trickShots->SetKillCount(kills);
    ShowLeftUnitPopUp(kills);
    if (!hitAnyone) {
        m_player->m_gameStats.RegisterShootMissed();
        ++m_missedShots;
    }
    m_player->m_gameStats.RegisterMultiKill(kills);
    int headshots = m_player->m_gameStats.m_headshotsNumber - headshotsBefore;
    if (headshots > 0 && IsOnBoat()) {
        m_player->m_gameStats.RegisterHeadshotsOnBoat(headshots);
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
    if (m_cursor != m_scopeCursor) {
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    int remaining = GetAlivaUnitByType(UnitWarrior) + GetAlivaUnitByType(UnitBoss);
    remaining += m_pendingMovingMinions;
    if (app->m_huntingInfo->m_assignedData.m_bossSpawnsLast && !m_bossCreated) {
        ++remaining;
    }
    WindowApp::HandleTunnelCommand(-0x5220a649, remaining, 0, 0);
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
            float angle = -90.0f;
            if (view.Cross(towardCamera).m_z >= 0.0f) {
                angle = 90.0f;
            }
            m_hud->PlayEnjureEffect(angle, armor);
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
    if (effect != NULL && worldTransform != NULL) {
        m_explosionEffect[0] = CUnitBody::Node(effect.find(SwerveHelper::Id("dummy_blastTransform")));
        if (m_explosionEffect[0] != NULL) {
            worldTransform.addChild(m_explosionEffect[0]);
            m_explosionAnimation[0] = new CDH_Animation(m_explosionEffect[0]);
            m_explosionAnimation[0]->addController(XString("blast"));
        }
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
    DGCamera::Camera camera = m_world.getActiveCamera();
    for (int index = 0; index < m_objects.size(); ++index) {
        IGameObject* object = m_objects[index];
        if (object->CanBeHandled() && object->Pick(x, y, 1000000.0f, camera)) {
            object->ReceivedDamage(weapon->damage(), 0);
        }
    }
}

void CSwerveGame::Explode(const vec3& position, int damage, int radius, const CDH_Weapon* weapon)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CTrickShotManager* trickShots = app->m_trickShotManager;
    trickShots->ResetEvents();
    m_blastUnits.setSize(0);
    float radiusSquared = static_cast<float>(radius * radius);
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (!unit->m_dead && !unit->m_invulnerable &&
            (unit->m_body.m_position - position).LengthSqr() <= radiusSquared) {
            unit->CalcDamageAndKillingPosibility(damage, weapon);
            m_blastUnits.addElement(unit);
        }
    }
    int kills = 0;
    for (int index = 0; index < m_blastUnits.size(); ++index) {
        CUnit* unit = m_blastUnits[index];
        if (unit->CanBeHandled() && unit->m_fatalDamage) {
            ++kills;
        }
    }
    trickShots->SetKillByExplosionCount(kills);
    for (int index = 0; index < m_blastUnits.size(); ++index) {
        CUnit* unit = m_blastUnits[index];
        if (unit->CanBeHandled()) {
            if (weapon == NULL && unit->m_fatalDamage && unit->m_type == UnitBoss) {
                ++m_player->m_challengeStats->m_bossKillsByExplosiveObject;
            }
            unit->ReceivedDamage(false);
        }
    }
    if (kills > 0) {
        m_player->m_gameStats.RegisterKillsByExplosion(kills);
        ShowLeftUnitPopUp(kills);
    }
    for (int index = 0; index < m_objects.size(); ++index) {
        IGameObject* object = m_objects[index];
        if (object->CanBeHandled() && (object->GetWorldPosition() - position).LengthSqr() < radiusSquared) {
            object->ReceivedDamage(damage, 90);
        }
    }
}

bool CSwerveGame::DefineShotProbability(EHitType, const CDH_Weapon* weapon, CUnit* unit)
{
    if (weapon == NULL || unit == NULL) {
        return false;
    }
    // The weapon table uses feet and the scene positions are in metres.
    const float FeetSquaredPerMetreSquared = 10.7584f;
    vec3 delta = unit->m_body.m_position - GetCurrentCameraPos();
    float range = weapon->effectiveRange();
    if (range * range < delta.LengthSqr() * FeetSquaredPerMetreSquared) {
        if (!CBH_Math::GetProbabiliyOutcome(4)) {
            return false;
        }
        if (unit->m_fatalDamage) {
            App* app = static_cast<App*>(WindowApp::m_instance);
            app->m_trickShotManager->RegisterEvent(CTrickShotManager::LongShot);
            m_player->m_gameStats.RegisterOutOfRangeKill();
            ++m_player->m_challengeStats->m_killsOutOfRange;
        }
    }
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

void CSwerveGame::QueueResult(EBountyResult result)
{
    if (m_delayedResult == BountyResultNoDelayedResult || m_delayedResult == BountyResultNoAmmo) {
        m_delayedResult = result;
        m_resultDelay = 3000;
        m_finishing = true;
    }
}

void CSwerveGame::RegisterUnitNeutralization(EUnitMindType type)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CHuntingInfo* hunt = app->m_huntingInfo;
    const CHuntingInfo::SAssignedData& assignment = hunt->m_assignedData;
    if (type == UnitCivilian && assignment.m_field9c) {
        QueueResult(BountyResultCivilianKilled);
    } else if (type != UnitBoss && assignment.m_bossFirst && !hunt->IsAmbush()) {
        m_bossMustEscape = true;
    } else if (type == UnitCivilian && assignment.m_field9b) {
        hunt->ReduceRewardMoney(assignment.m_field84);
    }
}

void CSwerveGame::DefineWinnerSide()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    CHuntingInfo* hunt = app->m_huntingInfo;
    const CHuntingInfo::SAssignedData& assignment = hunt->m_assignedData;
    if (assignment.m_cameraDurationMs > 0 && m_missionElapsedMs >= assignment.m_cameraDurationMs) {
        m_player->m_gameStats.RegisterWarTimerEnd();
        FinishGameWithResult(BountyResultCameraTimeOut, true);
        return;
    }
    if (m_player->m_hp <= 0) {
        if (!m_reviveOffered && m_player->CanRevive()) {
            ShowReviveDialog();
        } else {
            FinishGameWithResult(BountyResultLose, true);
        }
        return;
    }
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit->m_type == UnitBoss) {
            if (unit->m_removed && !unit->m_dead && !unit->m_tranquillized) {
                QueueResult(BountyResultBossEscaped);
                return;
            }
            if (unit->m_dead && assignment.m_tranquillize) {
                QueueResult(BountyResultKilledInsteadOfTranquillized);
                return;
            }
            if (unit->m_tranquillized && !assignment.m_tranquillize) {
                QueueResult(BountyResultBossTranquillizedInsteadOfKilled);
                return;
            }
            if (assignment.m_bossWins && (unit->m_dead || unit->m_tranquillized)) {
                QueueResult(BountyResultCapture);
                return;
            }
        } else if (unit->m_type == UnitWarrior && unit->m_tranquillized && !assignment.m_bossWins) {
            QueueResult(BountyResultHenchmanTranquillizedInsteadOfKilled);
            return;
        }
    }
    int warriors = GetAlivaUnitByType(UnitWarrior);
    if (warriors == 0 && !m_bossCreated && assignment.m_bossSpawnsLast) {
        WindowApp::HandleTunnelCommand(-0x2b9c8b2f, 0, 0, 0);
        if (assignment.m_scriptType != NULL) {
            CreateScriptedBoss();
        } else {
            CreateOriginalBoss();
        }
    }
    if (warriors == 0 && m_pendingMovingMinions == 0 && GetAlivaUnitByType(UnitBoss) == 0
        && (m_bossCreated || !assignment.m_hasTarget)) {
        QueueResult(BountyResultWin);
    } else if (hunt->SilentKill() && !hunt->IsAmbush() && m_missionAlert) {
        QueueResult(BountyResultRuinedSilence);
    } else if (!HasMissionAmmo()) {
        ShowAmmoDialog();
    }
}

void CSwerveGame::ShowReviveDialog()
{
    m_reviveOffered = true;
    m_hud->ResetFireButton();
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetMessageText(ResString("IDS_WANT_TO_REVIVE"));
    dialog->SetButtons(CDialogWindow::BUTTON_OK, CDialogWindow::BUTTON_IGNORE);
    dialog->SetButtonsCommands(0x224665c1, 0x07c73577, -1);
    m_dialogQueue->Add(dialog);
    m_dialogQueue->Start();
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
    CDialogWindow* dialog = new CDialogWindow(true);
    dialog->SetTitleText("IDS_SHOP_NOT_ENOUGH_CREDITS_TITLE");
    dialog->SetMessageText(ResString("IDS_SHOP_NOT_ENOUGH_CREDITS_TEXT"));
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    m_dialogQueue->Add(dialog);
    m_dialogQueue->Start();
}

void CSwerveGame::onSwitchToScope()
{
    CBH_Player::GetInstance()->m_challengeStats->RegisterPlayerEnterScope();
    m_hud->OnScopeSwitched();
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
    if (weapon != NULL && !weapon->m_scopeAttached) {
        // weapons without a scope only zoom the camera or hide the cross
        if (weapon->m_zoomWithoutScope) {
            if (m_camera != NULL) {
                m_camera->SetFOV(weapon->m_zoomFovMax);
            }
        } else if (m_hideCrossInScope) {
            m_cursor->ShowCross(false);
        }
        return;
    }
    m_hud->SetZoomSliderEnable(true);
    // the scope cursor starts where the fps cursor aims
    m_scopeCursor->setPosition(m_fpsCursor->GetAimX(), m_fpsCursor->GetAimY(), true);
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
    m_finalFOV = fov;
    m_fovSpeed = (fov - m_camera->GetFOV()) / (float)durationMs * 1000.0f;
    m_fovDurationMs = durationMs;
    m_fovElapsedMs = 0;
}

void CSwerveGame::UpdateFOV(int elapsed)
{
    if (m_camera == NULL || m_fovDurationMs <= 0 || elapsed <= 0) {
        return;
    }
    m_fovElapsedMs += elapsed;
    if (m_fovElapsedMs >= m_fovDurationMs) {
        m_camera->SetFOV(m_finalFOV);
        m_fovDurationMs = 0;
        return;
    }
    m_camera->SetFOV(m_camera->GetFOV() + m_fovSpeed * elapsed / 1000.0f);
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
    if (weapon->m_scopeAttached) {
        // Reloading can call ScopeOut directly, without going through the HUD button.
        m_hud->SetZoomSliderEnable(false);
        setCursor(m_fpsCursor);
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
    if (m_specialWeaponState != 1 || m_hud == NULL || m_hud->IsModelAnimationActive()
        || !m_dialogQueue->IsEmpty() || m_player->IsInHide() || m_player->IsInProgress()) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
    if (weapon == NULL || !weapon->IsReadyForUse()) {
        return;
    }
    if (m_cursor != NULL && m_cursor == m_scopeCursor) {
        m_hud->switchScope();
        return;
    }
    int count = app->m_huntingInfo->m_assignedData.m_additionalMinionCount;
    if (count > 0) {
        m_pendingMovingMinions = count;
    }
    m_hud->ResetFireButton();
    weapon->ResetAutoShot();
    m_hud->switchWeapons();
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    CDH_Weapon* weapon = app->InitWeaponManager()->GetWeaponById(VehicleMinigunId);
    if (weapon == NULL) {
        m_specialWeaponState = 0;
        return;
    }
    if (m_player->m_credits < weapon->getPriceCredits()) {
        m_specialWeaponState = 0;
        ShowMinigunNotEnoughMoneyDialog();
        return;
    }
    m_previousMissionWeapon = m_player->m_estimatedWeapon;
    m_previousMissionAmmo = m_player->m_estimatedAmmo;
    m_specialWeapon = weapon;
    m_player->ReduceCredits(weapon->getPriceCredits());
    m_player->SetEstimatedWeapon(weapon, -1);
    weapon->loadWeaponModel(false);
    weapon->restoreAmmo();
    app->m_soundManager->PlaySound("IDM_MENU_PURCHASE_COMPLETE", false, false);
    m_specialWeaponState = 3;
}

void CSwerveGame::ReleaseOldWeapon()
{
    if (m_specialWeaponState != 3) {
        return;
    }
    for (int slot = 0; slot < 2; ++slot) {
        if (m_loadedMissionWeapons[slot] != NULL) {
            m_loadedMissionWeapons[slot]->releaseWeaponModel();
            m_loadedMissionWeapons[slot] = NULL;
        }
    }
    m_specialWeaponState = 4;
}

bool CSwerveGame::IsRequiredSpawnNewMinion() const
{
    int alive = 0;
    for (int index = 0; index < m_units.size(); ++index) {
        const CUnit* unit = m_units[index];
        if (unit != NULL && unit->m_type == UnitWarrior && !unit->m_dead && !unit->m_tranquillized) {
            ++alive;
        }
    }
    return alive < m_spawnedMinions;
}

void CSwerveGame::SpawnNewMinion()
{
    if (m_squadType != NULL && m_squadType->m_minions.size() > 0) {
        const XString& name = m_squadType->GetMiniomName();
        if (name.Length() != 0) {
            CreateUnitByName(name, UnitWarrior, NULL);
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
    m_dragging = false;
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_mainLayer->AddModal(new CIngameShop());
}

void CSwerveGame::Paint(ICGraphics2d& graphics)
{
    if (!m_sceneReady) {
        return;
    }
    RunPaint3D(graphics);
    if (m_currentState == CameraIntro && m_loadingFadeMs > 0 && m_loadingSurface != NULL) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        app->m_helper->DrawImage(m_loadingSurface, 0, 0, m_loadingFadeMs / 1500.0f, 0, 0, 0.0f, 0);
    }
    if (m_cursor != NULL && m_currentState == Playing) {
        if (!m_hud->IsModelAnimationActive()) {
            DrawVisorDistances(graphics);
            m_cursor->draw();
        }
    }
}

void CSwerveGame::Paint3D(Graphics3D& graphics)
{
    RenderEnvironment(graphics);
    if (m_environment != NULL) {
        m_environment->paintHitEffects(graphics);
    }
    if (m_grenadeLauncher != NULL) {
        m_grenadeLauncher->Paint3D(graphics);
    }
    if (m_hud != NULL && m_hud->PaintReviveEffect(graphics)) {
        return;
    }
    if (m_currentState == Playing && m_cursor != m_scopeCursor && !m_slowMode && App::m_platformType != 0) {
        App* app = static_cast<App*>(WindowApp::m_instance);
        CDH_Weapon* weapon = app->m_helper->GetGameData()->GetActiveWeapon();
        if (weapon != NULL) {
            weapon->paintWeaponModel(graphics);
        }
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (m_player->IsSpeedMode()) {
        m_player->StopConsumeReactixControl();
    }
    CDH_BasicGameData* game = app->m_helper->GetGameData();
    CDH_Weapon* weapons[] = {game->GetWeapon01(true), game->GetWeapon02(true)};
    for (int slot = 0; slot < 2; ++slot) {
        CDH_Weapon* weapon = weapons[slot];
        if (weapon != NULL && weapon->m_cyberScopeEquipped) {
            if (m_player->m_cyberScopesCount == 0) {
                weapon->SetCyberScopeEquiped(false);
            } else {
                m_player->ReduceCyberScopeCount(1);
            }
        }
    }
    if (result == BountyResultBossEscaped) {
        m_player->m_gameStats.AddBossEscapedFailBounties(1);
    } else if (result == BountyResultNoAmmo) {
        m_player->m_gameStats.AddNoAmmoFailBounties(1);
    }
    if (m_finishing && !force) {
        return;
    }
    m_finishing = true;
    CHuntingInfo* hunt = app->m_huntingInfo;
    hunt->SetBountyResult(result);
    GatherChallengeStats(result);
    m_player->m_challengeStats->RegisterHuntEnd(result);
    if (result == BountyResultPending) {
        m_exitAfterDialogs = true;
        if (AddLevelUpDialogs() > 0) {
            m_dialogQueue->Start();
        }
    } else if (hunt->IsPlayerWin()) {
        m_player->AddXp(hunt->m_assignedData.m_experienceReward, GAIN_SOURCE_MISSION);
        ShowMissionFinalDialogs();
    } else {
        if (m_player->m_hp == 0) {
            m_player->m_gameStats.AddWoundedFailBounties(1);
        }
        ShowFailResultDialog();
    }
}

void CSwerveGame::GatherChallengeStats(EBountyResult result)
{
    CChallengeStats* stats = m_player->m_challengeStats;
    if (!stats->m_huntActive || (result != BountyResultWin && result != BountyResultCapture)) {
        return;
    }
    App* app = static_cast<App*>(WindowApp::m_instance);
    CHuntingInfo* hunt = app->m_huntingInfo;
    const CHuntingInfo::SAssignedData& assignment = hunt->m_assignedData;
    stats->RegisterMissionCompletionTime(m_missionElapsedMs / 1000);
    if (m_medkitsBeforeHunt == m_player->m_medkits) {
        stats->AddMissionsWithoutMedkitsUsed(1);
    }
    ++stats->m_completedBounties;
    if (!m_playerInjured) {
        ++stats->m_missionsWithoutInjury;
    }
    if (m_missedShots == 0 && (hunt->IsAmbush() || (assignment.m_multiplierCountdown && assignment.m_escapeSeconds == 0))) {
        ++stats->m_ambushWithoutMisses;
    }
    if (assignment.m_cameraDurationMs > 0) {
        ++stats->m_movingCameraWins;
    }
    if (assignment.m_weaponGated || assignment.m_requiredWeaponId >= 0) {
        ++stats->m_weaponGatedWins;
    }
    if (!m_missionAlert) {
        ++stats->m_missionsWithoutAlert;
    }
}

void CSwerveGame::UpdateChallenges()
{
    // completed challenges are collected here; AddCompletedChallengesDialogs queues them at the results
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_challengeManager->Update();
    Vector<CChallenge*> active = app->m_challengeManager->GetActiveChallenges();
    bool addedDialog = false;
    for (int index = 0; index < active.size(); ++index) {
        CChallenge* challenge = active[index];
        if (m_challengeCheckTime < static_cast<unsigned int>(challenge->m_completedAt)) {
            CTextDialog* dialog = new CTextDialog();
            dialog->SetTitleText("IDS_CHALLENGE_COMPLETE_TITLE");
            XString reward = challenge->GetRewardString(challenge->GetSoloReward());
            XString text = XString::Format(ResString("IDS_CHALLENGE_COMPLETED_SOLO_INGAME").c_str(),
                                           challenge->m_name.c_str(), reward.c_str());
            dialog->SetMessageText(text);
            dialog->SetButtons(CDialogWindow::BUTTON_OK);
            m_challengeDialogs.addElement(dialog);
            if (challenge->GetX1Reward()->m_awarded) {
                CTextDialog* friendDialog = CheckForFriendCompleteChallenge(challenge);
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
    const SChallengeReward* first = challenge->GetX1Reward();
    const SChallengeReward* second = challenge->GetX2Reward();
    if (!first->m_awarded) {
        return NULL;
    }

    XString friendName = challenge->GetFriendName(0);
    if (friendName.Length() == 0) {
        friendName = CGServeHelper::GetFriendNickname(challenge->GetFriendId(0));
    }
    XString firstReward = challenge->GetRewardString(first);
    CChallengeCompletedDialog* dialog = new CChallengeCompletedDialog();
    if (second->m_awarded) {
        XString format = ResString("IDS_CHALLENGE_FRIEND_REWARD_MULTIPLE_TEXT");
        XString secondReward = challenge->GetRewardString(second);
        dialog->SetTitleText("IDS_CHALLENGE_FRIEND_REWARD_MULTIPLE_TITLE");
        dialog->SetMessageText(XString::Format(format.c_str(), friendName.c_str(), 2,
            challenge->m_name.c_str(), firstReward.c_str(), secondReward.c_str()));
    } else {
        XString format = ResString("IDS_CHALLENGE_FRIEND_REWARD_MESSAGE");
        dialog->SetTitleText("IDS_CHALLENGE_FRIEND_REWARD_TITLE");
        dialog->SetMessageText(XString::Format(format.c_str(), friendName.c_str(),
            challenge->m_name.c_str(), firstReward.c_str()));
    }
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    dialog->SetSound("IDM_COOP_CHALLENGE");
    return dialog;
}

int CSwerveGame::AddLevelUpDialogs()
{
    int current = m_player->GetLevel();
    int count = 0;
    for (int level = m_levelBeforeHunt + 1; level <= current; ++level) {
        CLevelUpWindow* dialog = AddLevelUpDialogInQueue(level);
        int command = static_cast<int>(0xb717ef9fu);
        if (level == current && !m_exitAfterDialogs) {
            command = static_cast<int>(0xd6cad5a6u);
        }
        dialog->SetCloseCommand(command);
        ++count;
    }
    m_levelBeforeHunt = current;
    if (count > 0) {
        static_cast<App*>(WindowApp::m_instance)->m_huntingInfo->m_fieldc0 = true;
    }
    return count;
}

CLevelUpWindow* CSwerveGame::AddLevelUpDialogInQueue(int level)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    XString format = ResString("IDS_DIALOG_NEW_LEVEL");
    XString message = XString::Format(format.c_str(), level);
    message.Append(XString(L"\n"));
    message.Append(ResString("IDS_DIALOG_NEW_LEVEL_ENERGY"));
    if (app->InitAvatarManager()->IsNewAvatarUnlocked(level)) {
        message.Append(XString(L"\n"));
        message.Append(ResString("IDS_DIALOG_NEW_LEVEL_AVATAR_UNLOCKED"));
    }
    if (app->m_huntingInfo->m_assignedData.m_field70) {
        message.Append(XString(L"\n"));
        message.Append(ResString("IDS_DIALOG_NEW_INFORMANT_UNLOCKED"));
    }
    Vector<XString> weapons = app->InitWeaponManager()->GetUnlockedWeapons(level);
    if (weapons.size() > 0) {
        XString names;
        for (int index = 0; index < weapons.size(); ++index) {
            if (index > 0) {
                names.Append(XString(L", "));
            }
            names.Append(weapons[index]);
        }
        format = ResString("IDS_DIALOG_NEW_LEVEL_WEAPON_UNLOCKED");
        message.Append(XString(L"\n"));
        message.Append(XString::Format(format.c_str(), names.c_str()));
    }
    int health = m_player->GetMaxHpForLevel(level) - m_player->GetMaxHpForLevel(level - 1);
    message.Append(XString::Format(L"\n+%d %ls", health / 1000, ResString("IDS_DIALOG_LEVEL_UP_HP").c_str()));
    int energy = m_player->GetMaxEnergyForLevel(level) - m_player->GetMaxEnergyForLevel(level - 1);
    if (energy > 0) {
        message.Append(XString::Format(L", +%d %ls", energy, ResString("IDS_DIALOG_LEVEL_UP_ENERGY").c_str()));
    }
    CLevelUpWindow* dialog = new CLevelUpWindow(message, false);
    m_dialogQueue->Add(dialog);
    return dialog;
}

void CSwerveGame::ShowMissionFinalDialogs()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CHuntingInfo::SAssignedData& assignment = app->m_huntingInfo->m_assignedData;
    CMission* mission = app->m_missionManager->GetMissionById(assignment.m_missionId);
    if (mission != NULL) {
        for (int index = 0; index < mission->m_finalDialogs.size(); ++index) {
            const CMission::Dialog& entry = mission->m_finalDialogs[index];
            if ((entry.m_condition == 100 && !app->m_huntingInfo->m_bossTranquillized)
                || (entry.m_condition == 101 && app->m_huntingInfo->m_bossTranquillized)) {
                continue;
            }
            CDialogWindow* dialog = CreateMissionDialog(entry, *mission, assignment.m_bossType, true);
            dialog->SetButtonsCommands(-0x29352a5a, -1, -1);
            m_dialogQueue->Add(dialog);
        }
    }
    m_exitAfterDialogs = true;
    AddCompletedChallengesDialogs();
    AddLevelUpDialogs();
    ShowResultScreen(false);
    m_dialogQueue->Start();
}

void CSwerveGame::CheckDialogConditions()
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CHuntingInfo::SAssignedData& assignment = app->m_huntingInfo->m_assignedData;
    if (!m_ambushDialogShown && app->m_huntingInfo->IsAmbush() && assignment.m_cameraDurationMs <= 0) {
        m_ambushDialogShown = true;
        ShowAmbushDialog();
        return;
    }
    if (!m_randomWeaponIntroduced && assignment.m_randomWeaponId != -1) {
        m_randomWeaponIntroduced = true;
        CDH_Weapon* weapon = app->InitWeaponManager()->GetWeaponById(assignment.m_randomWeaponId);
        if (weapon != NULL) {
            CIntroduceItemWindow* introduction = new CIntroduceItemWindow(
                weapon, ResString("IDS_RANDOM_WEAPON_INTRO"), false, true);
            m_dialogQueue->Add(introduction);
            m_dialogQueue->Start();
        }
        return;
    }
    CMission* mission = app->m_missionManager->GetMissionById(assignment.m_missionId);
    if (mission == NULL || mission->m_inMissionDialogs.size() == 0) {
        return;
    }
    m_dialogRepeatMs -= app->m_updateFrameTime;
    bool bossWounded = false;
    for (int index = 0; index < m_units.size(); ++index) {
        CUnit* unit = m_units[index];
        if (unit->m_type == UnitBoss && unit->m_hp * 4 < unit->m_maxHp) {
            bossWounded = true;
            break;
        }
    }
    bool conditions[] = { false, bossWounded, m_cursor == m_scopeCursor, true,
                          m_dialogRepeatMs <= 0, m_player->m_hp < m_player->GetMaxHp() || m_player->m_fieldc0 };
    for (int condition = 5; condition > 0; --condition) {
        if (!conditions[condition]) {
            continue;
        }
        bool shown = false;
        for (int index = 0; index < m_seenDialogConditions.size(); ++index) {
            if (m_seenDialogConditions[index] == condition) {
                shown = true;
                break;
            }
        }
        if (!shown || condition == 4) {
            ShowDialogs(condition);
            if (condition == 4) {
                m_dialogRepeatMs = 30000;
            } else {
                m_seenDialogConditions.addElement(condition);
            }
            if (!m_dialogQueue->IsEmpty()) {
                return;
            }
        }
    }
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    const CHuntingInfo::SAssignedData& assignment = app->m_huntingInfo->m_assignedData;
    CMission* mission = app->m_missionManager->GetMissionById(assignment.m_missionId);
    if (mission == NULL) {
        return;
    }
    for (int index = 0; index < mission->m_inMissionDialogs.size(); ++index) {
        const CMission::Dialog& entry = mission->m_inMissionDialogs[index];
        if (entry.m_condition == condition) {
            m_dialogQueue->Add(CreateMissionDialog(entry, *mission, assignment.m_bossType, false));
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    EBountyResult result = app->m_huntingInfo->m_result;
    CDialogWindow* dialog = new CDialogWindow(false);
    dialog->SetTitleText("IDS_DIALOG_TITLE_FAIL");
    if (result >= BountyResultLose && result <= BountyResultRuinedSilence) {
        dialog->SetMessageText(ResString(FailureMessages[result]));
    }
    dialog->SetButtons(CDialogWindow::BUTTON_OK);
    m_dialogQueue->Add(dialog);
    AddCompletedChallengesDialogs();
    AddLevelUpDialogs();
    m_dialogQueue->Start();
    m_resultsQueued = true;
    m_exitAfterDialogs = true;
    ChangeState(Results, STATE_REPLACE);
    CGameAnalytics::logMissionFailed(app->m_huntingInfo->m_assignedData.m_missionName, result);
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
    m_reserved24a = 1;
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
    m_reserved24a = 0;
}

/** Set `fog` on every appearance of `mesh` (raw pointers, no reference counting, as in the original). */
void ApplyFog(Mesh mesh, Fog fog)
{
    IAppearanceInterface* appearance = NULL;
    mesh.getAppearance(0, &appearance);
    int index = 1;
    int result = 0;
    do {
        appearance->setFog(static_cast<IObject3DInterface*>(fog.m_interface));
        result = mesh.getAppearance(index, &appearance);
        index++;
    } while (result == 0);
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

void CSwerveGame::FixPlotUnitPositon()
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
        Group place = m_world.find(DGHelper::getSwerveID(name));
        if (place.m_interface == NULL) {
            return;
        }
        App* app = static_cast<App*>(WindowApp::m_instance);
        const CVehicleType* type = app->m_vehicleManager->GetRandomVehicle();
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

/** Build the 3D scene of the hunt (original @0x16d2c0, 912 B). Not used by the candidate's OnStateInterrupt(Loading) path. */
void CSwerveGame::CreateScene(bool reload)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_helper->GetGameData()->SetActiveWeapon(0);
    m_usedHudButton = false;
    m_usedSteadyAim = false;
    if (app->m_huntingInfo->IsAmbush()) {
        m_peaceTime = false;
    }
    m_environment = new DGWorld(m_world, NULL);
    m_viewport[0] = 0;
    m_viewport[1] = 0;
    m_viewport[2] = static_cast<short>(app->m_helper->m_screenWidth);
    m_viewport[3] = static_cast<short>(app->m_helper->m_screenHeight);
    SetRandomCamera();
    setCamera(app->m_huntingInfo->m_assignedData.m_cameraIndex);
    CBH_Player::GetInstance()->AddHp(CBH_Player::GetInstance()->GetMaxHp());
    CBH_Player::GetInstance()->m_fieldc0 = false;

    SwerveHelper::Object3D worldTransform = m_world.find(DGHelper::getSwerveID("worldTransform"));
    int rain = -1;
    app->m_helper->getUserParameter(worldTransform, "rain", &rain);

    // scene bounds: two dummies mark opposite corners, default is a 200 x 200 square
    SwerveHelper::Transform transform = SwerveHelper::NewTransform();
    vec3 firstCorner(0.0f, 0.0f, 0.0f);
    vec3 secondCorner(0.0f, 0.0f, 0.0f);
    SwerveHelper::Node firstDummy(m_world.find(DGHelper::getSwerveID("dummy_size_1Transform")));
    SwerveHelper::Node secondDummy(m_world.find(DGHelper::getSwerveID("dummy_size_2Transform")));
    if (firstDummy.m_interface != NULL && secondDummy.m_interface != NULL) {
        firstDummy.getTransformTo(m_world, transform);
        DGHelper::TransformPoint(transform, firstCorner);
        secondDummy.getTransformTo(m_world, transform);
        DGHelper::TransformPoint(transform, secondCorner);
        m_sceneMinX = secondCorner.m_x;
        if (firstCorner.m_x < secondCorner.m_x) {
            m_sceneMinX = firstCorner.m_x;
        }
        m_sceneMinY = secondCorner.m_y;
        if (firstCorner.m_y < secondCorner.m_y) {
            m_sceneMinY = firstCorner.m_y;
        }
        m_sceneMaxX = secondCorner.m_x;
        if (secondCorner.m_x < firstCorner.m_x) {
            m_sceneMaxX = firstCorner.m_x;
        }
        m_sceneMaxY = secondCorner.m_y;
        if (secondCorner.m_y < firstCorner.m_y) {
            m_sceneMaxY = firstCorner.m_y;
        }
    } else {
        m_sceneMinX = -100.0f;
        m_sceneMinY = -100.0f;
        m_sceneMaxX = 100.0f;
        m_sceneMaxY = 100.0f;
    }

    m_cameraCount = getCamerasCount();
    m_hud->setCamerasCount(m_cameraCount);
    m_sceneReady = true;
    // unknown: the original also sets byte 0x27d of the HUD when it exists
    FixPlotUnitPositon();
    const CHuntingInfo::SAssignedData& assignment = app->m_huntingInfo->m_assignedData;
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
    m_cameraId = 0;
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
    XString name = XString::Format(L"Camera%02d", m_cameraId + 1);
    XString::AnsiString ansiName(name.c_str());
    Camera camera(m_world.find(SwerveHelper::Id(ansiName)));
    m_world.setActiveCamera(camera);
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_helper->SetCameraSize(camera, app->m_helper->m_screenWidth, app->m_helper->m_screenHeight,
                                 CAMERA_SIZE_SET_ASPECT);
}

/**
 * Free the 3D scene (original @0x16dbe8). Recovered parts only: the release of four HUD
 * surfaces, the 0xf0 vector, the steady aim statistics (0x404/0x405), the AnimationImpl vector (0x364)
 * and the unit position HashTable (0x244) are not mapped in the candidate layout yet.
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
