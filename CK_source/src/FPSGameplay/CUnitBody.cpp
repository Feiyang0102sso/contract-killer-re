// CUnitBody.cpp - model, animations, movement and picking of a unit body.
// Function order follows the original debug map (0xda164..0xdd238).
#include "CUnitBody.h"

#include <math.h>
#include <string.h>
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "AnimatableSwerveObject.h"
#include "AnimationImpl.h"
#include "AnimationTransition.h"
#include "App.h"
#include "CBH_PersonType.h"
#include "CBH_Player.h"
#include "CDH_SoundManager.h"
#include "CFontMgr.h"
#include "CGameAIMap.h"
#include "CRenderSurface_OGLES_Targetable.h"
#include "CSquad.h"
#include "CSwerveGame.h"
#include "CStdUtil_Cocoa.h"
#include "CUnit.h"
#include "CVisualCharacterType.h"
#include "DGHelper.h"
#include "ICRenderSurface.h"

/** Readback surface for distance-tag glyphs. It uses the currently bound framebuffer. */
class CSur : public CRenderSurface_OGLES_Targetable {
};

namespace {
const int CharacterFadeMs = 300;
const float DegreesPerRadian = 57.29582595825195f;
const float RadiansPerDegree = 0.0174532778f;
const float FeetPerMeter = 3.28f;
const unsigned int DistanceTagRefreshMs = 500;
const EPersonAnimType FallForward = static_cast<EPersonAnimType>(14);
const EPersonAnimType FallBackward = static_cast<EPersonAnimType>(15);
const EPersonAnimType Tranquillized = static_cast<EPersonAnimType>(17);

App* GetApp()
{
    return static_cast<App*>(WindowApp::m_instance);
}

int WrapDegrees(int degrees)
{
    degrees %= 360;
    if (degrees < 0) {
        degrees += 360;
    }
    return degrees;
}

void BlockAngles(bool* blocked, int angle, int halfWidth)
{
    for (int offset = -halfWidth; offset <= halfWidth; ++offset) {
        blocked[WrapDegrees(angle + offset)] = true;
    }
}

}

// Internal linkage (static), not the anonymous namespace: the original symbol is __ZL16SetLayerForChild.
static void SetLayerForChild(SwerveHelper::Node node, int layer)
{
    typedef ::Appearance Appearance;
    typedef Sprite3D Sprite;
    typedef int (*SetLayer)(IQueryInterface*, int);
    // The original checks only the node and the casts, never the appearances.
    if (node != NULL) {
        SwerveHelper::Mesh mesh(node);
        if (mesh != NULL) {
            int count = mesh.getSubmeshCount();
            for (int index = 0; index < count; ++index) {
                Appearance appearance = mesh.getAppearance(index);
                CKMethod<SetLayer>(appearance.m_interface, 0x50)(appearance.m_interface, layer);
                mesh.setAppearance(index, appearance);
            }
        }
        Sprite sprite(node);
        if (sprite != NULL) {
            typedef int (*GetAppearance)(IQueryInterface*, IAppearanceInterface**);
            typedef int (*SetAppearance)(IQueryInterface*, IAppearanceInterface*);
            IAppearanceInterface* interface;
            CKMethod<GetAppearance>(sprite.m_interface, 0xbc)(sprite.m_interface, &interface);
            Appearance appearance(interface);
            CKMethod<SetLayer>(appearance.m_interface, 0x50)(appearance.m_interface, layer);
            CKMethod<SetAppearance>(sprite.m_interface, 0xc0)(sprite.m_interface, appearance.appearance());
        }
        SwerveHelper::Group group(node);
        if (group != NULL) {
            int count = group.getChildCount();
            for (int index = 0; index < count; ++index) {
                SetLayerForChild(group.getChild(index), layer);
            }
        }
    }
}

CUnitBody::CUnitBody(const CBH_PersonType* type, const CGameAIMap* map)
    : m_cullSphere(NULL), m_unit(NULL),
      m_zoneIntersection(CSwerve::GetInstance()->m_swerve), m_spreadIntersection(CSwerve::GetInstance()->m_swerve),
      m_skin(NULL), m_transition(NULL), m_type(type), m_map(map),
      m_pulsar(NULL), m_health(NULL), m_shotFlash(NULL),
      m_animationType(static_cast<EPersonAnimType>(23)), m_cellId(-1), m_pointId(-1), m_moved(true),
      m_speed(0.0f), m_yaw(0.0f), m_shotDirection(static_cast<EPersonAnimShotDirectionType>(1)),
      m_attitude(static_cast<EPersonAnimShotAttitudeType>(1)),
      m_showBlood(false), m_pickDistance(0.0f), m_field178(0), m_distanceTagValue(0), m_distanceTagTime(0)
{
    m_hasTag = false;
    m_hasKnownTag = false;
    m_visible = false;
    m_showDistanceTag = false;
    m_model = m_type->m_visualCharacter->GetCharacterModelCopy();
    m_cullSphere = new FrustumCulling::CullSphere(m_model, 1.0f);
    m_barMount = Group(m_model.find(DGHelper::getSwerveID("down_barTransform")));
    InitBodyZones();
    InitHealthBar();
    InitWeapon();
    // The original clears 0x70..0xcb with a libc memset after the init calls.
    for (unsigned int index = 0; index < sizeof(m_reserved70); ++index) {
        m_reserved70[index] = 0;
    }
    m_staticObstacles.ensureCapacity(10);
    m_dynamicObstacles.ensureCapacity(10);
}

void CUnitBody::InitBodyZones()
{
    if (m_type != NULL && m_model != NULL) {
        m_headZone = Group(m_model.find(DGHelper::getSwerveID("head_kill_zoneTransform")));
        m_headArmorZone = Group(m_model.find(DGHelper::getSwerveID("head_armor_zoneTransform")));
        m_vitalZone = Group(m_model.find(DGHelper::getSwerveID("body_vital_zoneTransform")));
        m_bodyArmorZone = Group(m_model.find(DGHelper::getSwerveID("body_armor_zoneTransform")));
        if (m_headZone != NULL) {
            m_headZone.setRenderingEnable(false);
        }
        if (m_headArmorZone != NULL) {
            m_headArmorZone.setRenderingEnable(false);
        }
        if (m_vitalZone != NULL) {
            m_vitalZone.setRenderingEnable(false);
        }
        if (m_bodyArmorZone != NULL) {
            m_bodyArmorZone.setRenderingEnable(false);
            m_bodyArmorZone.setPickingEnable(m_type->m_visualCharacter->m_isWoman);
        }
    }
}

void CUnitBody::InitHealthBar()
{
    SwerveHelper::Object3D bar = App::LoadObject3D("BIN_CHARACTER_HEALTH_BAR_M3G");
    m_healthBar = Group(bar.find(DGHelper::getSwerveID("centerTransform")));
    m_barAnimations.LoadFrom(bar);
    m_barAnimations.StopAll();
    bar.Clear();
    m_pulsar = m_barAnimations.Anim(SwerveHelper::Id("pulsar"));
    m_health = m_barAnimations.Anim(SwerveHelper::Id("health"));
}

void CUnitBody::InitWeapon()
{
    if (m_type->m_weapon != NULL) {
        m_weapon = m_type->m_weapon->GetModelCopy();
        if (m_weapon != NULL) {
            m_weaponAnimations.LoadFrom(m_weapon);
            m_weaponAnimations.StopAll();
            m_shotFlash = m_weaponAnimations.Anim(SwerveHelper::Id("shotflash"));
        }
    }
}

CUnitBody::~CUnitBody()
{
    Cleanup();
}

void CUnitBody::Cleanup()
{
    if (m_showDistanceTag) {
        UnsetDistanceTag();
    }
    UnbindFromWorld();
    if (m_cullSphere != NULL) {
        delete m_cullSphere;
        m_cullSphere = NULL;
    }
    m_healthBar.Clear();
    m_barMount.Clear();
    m_model.Clear();
    m_type->m_visualCharacter->ReleaseCharacterModelCopy();
    if (m_weapon != NULL) {
        m_weapon.Clear();
        m_type->m_weapon->ReleaseModelCopy();
    }
    m_headZone.Clear();
    m_headArmorZone.Clear();
    m_vitalZone.Clear();
    m_bodyArmorZone.Clear();
    m_zoneIntersection.Clear();
    m_knownIcon.Clear();
    m_groundShadow.Clear();
    if (m_skin != NULL) {
        delete m_skin;
        m_skin = NULL;
    }
    if (m_transition != NULL) {
        delete m_transition;
        m_transition = NULL;
    }
}

void CUnitBody::Init(CUnit* unit, int point)
{
    m_unit = unit;
    LoadSkin();
    m_transition = new AnimationTransition(m_skin, this);
    BindToWorld();
    BindWeaponToHand();
    BindShadow();
    // Build the distance tag glyph cache when the visor first needs it and a target is bound.
    SetToPoint(point);
    if (point != -1) {
        SetExactlyToPoint();
        m_moved = true;
        World blood = World(App::LoadObject3D("BIN_M3G_HIT_EFFECT_BLOOD"));
        m_blood = Node(blood.find(SwerveHelper::Id("dummy_bloodTransform")));
        m_blood.setRenderingEnable(false);
        m_blood.setPickingEnable(false);
        if (unit->m_squad != NULL) {
            unit->m_squad->NoticeUnitParams(unit);
        }
    }
}

void CUnitBody::LoadSkin()
{
    GetApp()->m_helper->CreateAnimatableObject(m_model, &m_skin, CVisualCharacterType::m_commonMapResourceID);
}

void CUnitBody::BindToWorld()
{
    Group world(GetApp()->m_helper->m_swerveGame->m_world.find(DGHelper::getSwerveID("worldTransform")));
    world.addChild(m_model);
}

void CUnitBody::BindWeaponToHand()
{
    if (m_weapon != NULL) {
        Group hand(m_model.find(DGHelper::getSwerveID("Player_parent_weaponR_Transform")));
        if (hand != NULL) {
            hand.addChild(m_weapon);
            ShowWeapon(false);
        }
    }
}

void CUnitBody::BindShadow()
{
    SwerveHelper::Object3D shadow = App::LoadObject3D("BIN_M3G_EFFECT_SHADOW_SYSTEM");
    if (shadow != NULL) {
        m_groundShadow = Group(shadow.find(SwerveHelper::Id("rootTransform")));
        if (m_groundShadow != NULL) {
            m_barMount.addChild(m_groundShadow);
        }
    }
}

void CUnitBody::SetToPoint(int point)
{
    if (point == -1) {
        return;
    }
    if (m_map != NULL) {
        m_pointId = point;
        m_cellId = m_map->GetCellIdByPointId(point);
        m_position = m_map->GetMapPointPostition(point);
    }
    m_moved = true;
}

void CUnitBody::Tick(int elapsed)
{
    if (m_map != NULL) {
        m_map->RegisterStaticCollision(this);
    }
    AvoidObstacles();
    UpdateAngle(elapsed);
    if (m_moved) {
        UpdatePosition(elapsed);
    }
    UpdateHealthBar();
    UpdateAnimation(elapsed);
    if (m_showDistanceTag) {
        UpdateDistanceTag();
    }
    if (m_showBlood) {
        m_bloodTimeMs += WindowApp::m_instance->m_updateFrameTime;
        if (m_bloodTimeMs > 400) {
            m_showBlood = false;
            m_blood.setRenderingEnable(false);
        }
    }
}

void CUnitBody::AvoidObstacles()
{
    if (m_staticObstacles.size() == 0 && m_dynamicObstacles.size() == 0) {
        return;
    }
    bool blocked[360];
    memset(blocked, 0, sizeof(blocked));
    for (int index = 0; index < m_dynamicObstacles.size(); ++index) {
        vec3 direction = m_dynamicObstacles[index].m_position - m_position;
        BlockAngles(blocked, static_cast<int>(GetYawAngle(direction)), 44);
    }
    for (int index = 0; index < m_staticObstacles.size(); ++index) {
        const SStaticObstacle& obstacle = m_staticObstacles[index];
        vec3 edge = obstacle.m_points[1] - obstacle.m_points[0];
        vec3 middle = (obstacle.m_points[0] + obstacle.m_points[1]) * 0.5f;
        vec3 outward(edge.m_y, -edge.m_x, 0.0f);
        if (outward.Dot(middle - m_position) < 0.0f) {
            outward = outward * -1.0f;
        }
        BlockAngles(blocked, static_cast<int>(GetYawAngle(outward)), 88);
    }
    m_staticObstacles.setSize(0);
    m_dynamicObstacles.setSize(0);
    if (m_speed <= 0.0f) {
        return;
    }
    int heading = WrapDegrees(static_cast<int>(GetYawAngle(m_direction)));
    if (!blocked[heading]) {
        return;
    }
    for (int degrees = 1; degrees < 180; ++degrees) {
        if (!blocked[WrapDegrees(heading + degrees)]) {
            AdditionalRotate(static_cast<float>(degrees));
            return;
        }
        if (!blocked[WrapDegrees(heading - degrees)]) {
            AdditionalRotate(-static_cast<float>(degrees));
            return;
        }
    }
}

void CUnitBody::UpdateAngle(int)
{
    if (m_direction.m_x == 0.0f && m_direction.m_y == 0.0f) {
        return;
    }
    float yaw = GetYawAngle(m_direction);
    float delta = yaw - m_yaw;
    if (delta > 180.0f) {
        delta -= 360.0f;
    } else if (delta < -180.0f) {
        delta += 360.0f;
    }
    if (fabsf(delta) > 5.0f) {
        m_yaw += delta * 0.5f;
    } else {
        m_yaw = yaw;
    }
    m_moved = true;
}

void CUnitBody::UpdatePosition(int elapsed)
{
    CalculateStep(elapsed);
    if (m_model == NULL) {
        return;
    }
    SwerveHelper::Transform transform(CSwerve::GetInstance()->m_swerve);
    transform.postTranslate(&m_position.m_x);
    float orientation[4] = {m_yaw, 0.0f, 0.0f, 1.0f};
    transform.postRotate(orientation);
    m_model.setTransform(transform);
    m_moved = false;
}

void CUnitBody::UpdateHealthBar()
{
    if (m_unit == NULL || m_health == NULL || m_pulsar == NULL) {
        return;
    }
    if (m_unit->m_wounded && !m_pulsar->IsPlaying()) {
        CSwerveGame* game = GetApp()->m_helper->m_swerveGame;
        int time = game->m_missionElapsedMs;
        m_pulsar->StartAtTimeMS(time, 1.0f, time, 0.0f, 1.0f);
    }
    m_health->Start(0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
    m_health->SetPositionSec((1.0f - m_unit->m_healthFraction) * m_health->m_durationSec);
    m_health->Pause();
}

void CUnitBody::UpdateAnimation(int elapsed)
{
    if (m_transition != NULL) {
        m_transition->update(elapsed);
    }
    if (m_skin == NULL) {
        return;
    }
    int type = m_animationType;
    bool animate;
    if (type == FallForward || type == FallBackward) {
        animate = true;
    } else {
        animate = m_visible;
        if (type == Tranquillized) {
            animate = true;
        }
    }
    // CSwerveGame+0x24a suppresses the skin animation.
    if (!reinterpret_cast<const bool&>(GetApp()->m_helper->m_swerveGame->m_reserved24a) && animate) {
        // Low-end devices animate the skin every third frame only (idle excepted).
        if (App::m_platformType != 0 || (++m_field178 == 3 && (m_field178 = 0, type != 0))) {
            m_skin->animate();
        }
    }
    m_skin->advanceTime(static_cast<float>(elapsed));
}

void CUnitBody::OnPathComplete()
{
    m_unit->m_mind.OnPathComplete();
}

void CUnitBody::OnNeutralization()
{
    UnbindWeapon();
    UnbindAllIcons();
    ShowDistanceTag(false);
}

void CUnitBody::UnbindWeapon()
{
    if (m_weapon != NULL) {
        Group hand(m_model.find(DGHelper::getSwerveID("Player_parent_weaponR_Transform")));
        hand.removeChild(m_weapon);
    }
}

void CUnitBody::UnbindAllIcons()
{
    Group mount(m_model.find(DGHelper::getSwerveID(CVisualCharacterType::m_pBodyDummyName)));
    if (mount != NULL && m_knownIcon != NULL) {
        mount.removeChild(m_knownIcon);
        m_knownIcon.Clear();
    }
}

void CUnitBody::ShowDistanceTag(bool show)
{
    if (m_unit->m_dead) {
        return;
    }
    if (m_showDistanceTag && !show) {
        UnsetDistanceTag();
    } else {
        m_distanceTagValue = 0;
        m_distanceTagTime = 0;
    }
    m_showDistanceTag = show;
}

ICRenderSurface* CUnitBody::CreateDistanceTagSurface()
{
    const ICRenderSurface::InitParam surfaceDesc[] = {
        {ICRenderSurface::InitParam_ColorFormat, ColorFormat_A8B8G8R8},
        {ICRenderSurface::InitParam_Targetable, 1},
        {ICRenderSurface::InitParam_Width, 100},
        {ICRenderSurface::InitParam_Height, 100},
        {ICRenderSurface::InitParam_End, 0}
    };
    CSur* target = new CSur();
    target->CRenderSurface_OGLES_Texture::Initialize(surfaceDesc);
    return target;
}

void CUnitBody::InitDistanceTag()
{
    CSwerveGame* game = GetApp()->m_helper->m_swerveGame;
    CFont* font = CFontMgr::GetInstance()->GetFont(19);
    if (font == NULL || game->m_tagImages.m_count != 0 || ICGraphics::GetInstance()->GetTargetSurface() == NULL) {
        return;
    }
    ICRenderSurface* target = CreateDistanceTagSurface();
    if (target == NULL) {
        return;
    }
    const char* glyphs = "0123456789 ft";
    for (const char* current = glyphs; *current != '\0'; ++current) {
        char character[] = {*current, '\0'};
        XString glyph(character);
        unsigned char* pixels = NULL;
        int width = 0;
        int height = 0;
        SwerveHelper::DrawToArray(&pixels, &width, &height, target, glyph, font);
        if (pixels != NULL) {
            game->SetTagImage(glyph, pixels, width, height);
        }
    }
    delete target;
}

void CUnitBody::UpdateDistanceTag()
{
    if (!m_unit->CanBeHandled()) {
        UnsetDistanceTag();
        return;
    }
    unsigned int now = CStdUtil_Cocoa::GetUpTimeMS();
    if (m_distanceTagTime != 0 && now - m_distanceTagTime <= DistanceTagRefreshMs) {
        return;
    }
    CSwerveGame* game = GetApp()->m_helper->m_swerveGame;
    int distance = static_cast<int>(game->GetDistanceToUnit(m_unit) * FeetPerMeter);
    if (distance != m_distanceTagValue || m_distanceTag == NULL) {
        m_distanceTagValue = distance;
        m_distanceTagTime = now;
        SetDistanceTag(distance);
    }
}

void CUnitBody::CombineCharImages(unsigned char** pixels, int* width, int* height, const XString& text)
{
    *pixels = NULL;
    *width = 0;
    *height = 0;
    Vector<const STagImage*> glyphs;
    CSwerveGame* game = GetApp()->m_helper->m_swerveGame;
    int fullWidth = 0;
    int fullHeight = 0;
    for (int index = 0; index < text.Length(); ++index) {
        wchar_t character[] = {text.Char(index), 0};
        const STagImage* glyph = game->GetTagImage(XString(character));
        if (glyph == NULL) {
            return;
        }
        glyphs.addElement(glyph);
        fullWidth += glyph->m_width;
        if (fullHeight < glyph->m_height) {
            fullHeight = glyph->m_height;
        }
    }
    if (fullWidth == 0 || fullHeight == 0) {
        return;
    }
    unsigned char* image = new unsigned char[fullWidth * fullHeight * 4];
    memset(image, 0, fullWidth * fullHeight * 4);
    int x = 0;
    for (int index = 0; index < glyphs.size(); ++index) {
        const STagImage* glyph = glyphs[index];
        int rowBytes = glyph->m_width * 4;
        for (int row = 0; row < glyph->m_height; ++row) {
            memcpy(image + (row * fullWidth + x) * 4, glyph->m_pixels + row * rowBytes, rowBytes);
        }
        x += glyph->m_width;
    }
    *pixels = image;
    *width = fullWidth;
    *height = fullHeight;
}

void CUnitBody::SetDistanceTag(int distance)
{
    Group mount(m_model.find(DGHelper::getSwerveID(CVisualCharacterType::m_pBodyDummyName)));
    if (mount == NULL) {
        return;
    }
    InitDistanceTag();
    unsigned char* pixels = NULL;
    int width = 0;
    int height = 0;
    CombineCharImages(&pixels, &width, &height, XString::Format(L"%d ft", distance));
    if (pixels == NULL) {
        return;
    }
    Node sprite = SwerveHelper::CreateSprite(pixels, width, height);
    delete[] pixels;
    if (sprite == NULL) {
        return;
    }
    if (m_distanceTag != NULL) {
        mount.removeChild(m_distanceTag);
    }
    const float position[] = {0.0f, 0.0f, 3.2f};
    const float scale[] = {1.0f, 0.6f, 1.0f};
    sprite.setTranslation(position);
    sprite.setScale(scale);
    mount.addChild(sprite);
    m_distanceTag = sprite;
}

void CUnitBody::OnTagged(bool known)
{
    CUnit* unit = m_unit;
    if (unit == NULL) {
        return;
    }
    if (unit->CanBeHandled() == true && !unit->m_invulnerable && !m_hasKnownTag && (!m_hasTag || known)) {
        if (known) {
            UnbindAllIcons();
            BindKnownIcon();
            m_hasKnownTag = true;
        } else {
            BindUnknownIcon();
        }
        m_hasTag = true;
    }
}

void CUnitBody::BindKnownIcon()
{
    if (m_knownIcon != NULL) {
        return;
    }
    const char* resource;
    if (m_unit->m_type == UnitBoss) {
        resource = "BIN_CHARACTER_MARK_MIND_BOSS_M3G";
    } else if (m_unit->m_type == UnitWarrior) {
        resource = "BIN_CHARACTER_MARK_MIND_MINION_M3G";
    } else if (m_unit->m_type == UnitCivilian) {
        resource = "BIN_CHARACTER_MARK_MIND_CIVILIAN_M3G";
    }
    SwerveHelper::Object3D mark = App::LoadObject3D(resource);
    m_knownIcon = Group(mark.find(DGHelper::getSwerveID("dummy_markTransform")));
    mark.Clear();
    Group mount(m_model.find(DGHelper::getSwerveID(CVisualCharacterType::m_pBodyDummyName)));
    if (mount != NULL && m_knownIcon != NULL) {
        m_knownIcon.setPickingEnable(false);
        mount.addChild(m_knownIcon);
        // The original calls SetIconTransparencyParameters() after mounting the icon.
        SetIconTransparencyParameters();
    }
}

void CUnitBody::BindUnknownIcon()
{
    if (m_knownIcon != NULL) {
        return;
    }
    SwerveHelper::Object3D mark = App::LoadObject3D("BIN_CHARACTER_MARK_SIDE_UNKNOWN_M3G");
    m_knownIcon = Group(mark.find(DGHelper::getSwerveID("dummy_markTransform")));
    mark.Clear();
    Group mount(m_model.find(DGHelper::getSwerveID(CVisualCharacterType::m_pBodyDummyName)));
    if (mount != NULL && m_knownIcon != NULL) {
        m_knownIcon.setPickingEnable(false);
        mount.addChild(m_knownIcon);
        // Apply the same rendering layer to the unknown marker.
        SetIconTransparencyParameters();
    }
}

void CUnitBody::SetIconTransparencyParameters()
{
    if (m_knownIcon != NULL) {
        SetLayerForChild(m_knownIcon, 0);
    }
}

void CUnitBody::CheckVisibility(const FrustumCulling& frustum)
{
    m_visible = false;
    if (m_cullSphere == NULL) {
        return;
    }
    m_cullSphere->m_offset = reinterpret_cast<const swvVector3D&>(m_position);
    m_visible = frustum.CheckVisibility(*m_cullSphere);
}

void CUnitBody::SetAttitude(EPersonAnimShotAttitudeType attitude)
{
    m_attitude = attitude;
}

void CUnitBody::DefineCameraPitch(const vec3& direction)
{
    vec3 forward = direction * MathLib::InvSqrt(direction.LengthSqr());
    vec3 planar(direction.m_x, direction.m_y, 0.0f);
    planar = planar * MathLib::InvSqrt(planar.LengthSqr());
    m_shotDirection = static_cast<EPersonAnimShotDirectionType>(!(forward.Dot(planar) < 0.86f));
}

void CUnitBody::SetOrientationToPoint(const vec3& target, bool facing)
{
    vec3 direction = target - m_position;
    DefineCameraPitch(direction);
    vec3 planar(direction.m_x, direction.m_y, 0.0f);
    m_direction = planar * MathLib::InvSqrt(planar.LengthSqr());
    if (!facing) {
        m_direction.m_x = -m_direction.m_x;
        m_direction.m_y = -m_direction.m_y;
        m_direction.m_z = -m_direction.m_z;
    }
    m_moved = true;
}

void CUnitBody::SetExactlyToPoint()
{
    m_position = m_map->GetMapPointPostition(m_pointId);
}

void CUnitBody::SetMovementParam(const vec3& target, float speed)
{
    m_target = target;
    vec3 direction = target - m_position;
    m_speed = speed;
    m_direction = direction * MathLib::InvSqrt(direction.LengthSqr());
    m_moved = true;
}

void CUnitBody::AvoidCollision(CUnitBody* other)
{
    if (m_speed <= 0.0f) {
        return;
    }
    vec3 separation = other->m_position - m_position;
    float distanceSq = separation.LengthSqr();
    float radius = GetRadius() + other->GetRadius();
    if (distanceSq <= 0.0f || distanceSq >= radius * radius) {
        return;
    }
    float distance = sqrtf(distanceSq);
    float toward = m_direction.Dot(separation) / distance;
    if (toward <= 0.0f) {
        return;
    }
    if (distance <= 0.5f) {
        m_speed *= toward;
        return;
    }
    vec3 velocity = m_direction * m_speed - other->m_direction * other->m_speed;
    float cross = velocity.m_x * separation.m_y - velocity.m_y * separation.m_x;
    float along = velocity.m_x * separation.m_x + velocity.m_y * separation.m_y;
    if (along <= 0.0f) {
        return;
    }
    float relativeAngle = atan2f(cross, along) * DegreesPerRadian;
    float avoidanceAngle = atanf(radius / distance) * DegreesPerRadian;
    if (fabsf(relativeAngle) < avoidanceAngle) {
        if (relativeAngle < 0.0f) {
            AdditionalRotate(relativeAngle + avoidanceAngle + 0.5f);
        } else {
            AdditionalRotate(relativeAngle - avoidanceAngle - 0.5f);
        }
    }
}

void CUnitBody::AdditionalRotate(float degrees)
{
    float radians = degrees * 0.017453292f;
    float sine = static_cast<float>(sin(radians));
    float cosine = static_cast<float>(cos(radians));
    float x = m_direction.m_x;
    float y = m_direction.m_y;
    m_direction.m_x = x * cosine - y * sine;
    m_direction.m_y = x * sine + y * cosine;
    m_direction = m_direction * MathLib::InvSqrt(m_direction.LengthSqr());
}

float CUnitBody::GetRadius() const
{
    return 0.7f;
}

float CUnitBody::GetHeight() const
{
    return 1.7f;
}

void CUnitBody::AddStaticObstacle(const vec3& start, const vec3& end)
{
    SStaticObstacle obstacle;
    obstacle.m_points[0] = start;
    obstacle.m_points[1] = end;
    m_staticObstacles.addElement(obstacle);
}

float CUnitBody::GetYawAngle(const vec3& direction) const
{
    vec3 forward(0.0f, -1.0f, 0.0f);
    vec3 planar(direction.m_x, direction.m_y, 0.0f);
    planar = planar * MathLib::InvSqrt(planar.LengthSqr());
    float angle = static_cast<float>(acos(planar.Dot(forward)) * DegreesPerRadian);
    if (forward.Cross(planar).m_z < 0.0f) {
        angle = -angle;
    }
    return angle;
}

void CUnitBody::StopAnimation(const char* name)
{
    if (m_transition != NULL && name != NULL) {
        m_transition->stopAnimation(name, CharacterFadeMs);
    }
}

const char* CUnitBody::StartAnimation(EPersonAnimType type, bool loop)
{
    if (m_type != NULL && m_transition != NULL && static_cast<unsigned int>(type) < 23) {
        AnimationImpl* animation = m_type->m_visualCharacter->GetAnimation(type);
        if (animation != NULL) {
            m_transition->startAnimation(animation, 1.0f, loop, CharacterFadeMs, 1.0f, 0.0f);
            m_animationType = type;
            return animation->getName();
        }
    }
    return NULL;
}

const char* CUnitBody::PlayAfraidAnimation()
{
    if (m_unit->m_type == UnitBoss) {
        AnimationImpl* animation = m_type->m_visualCharacter->GetBossAfraidAnimation();
        if (animation == NULL) {
            return NULL;
        }
        m_transition->startAnimation(animation, 1.0f, false, CharacterFadeMs, 1.0f, 0.0f);
        m_animationType = static_cast<EPersonAnimType>(1);
        return animation->getName();
    }
    return StartAnimation(static_cast<EPersonAnimType>(1), false);
}

const char* CUnitBody::PlayDeathAnimation(const vec3& from)
{
    vec3 direction = m_direction * MathLib::InvSqrt(m_direction.LengthSqr());
    float facing = (from - m_position).Dot(direction);
    SetOrientationToPoint(from, facing > 0.0f);
    m_yaw = GetYawAngle(m_direction);
    if (m_groundShadow != NULL) {
        m_barMount.removeChild(m_groundShadow);
    }
    if (facing > 0.0f) {
        return StartAnimation(FallForward, false);
    }
    return StartAnimation(FallBackward, false);
}

const char* CUnitBody::PlayIdleAnimation(int index)
{
    if (m_type != NULL && m_transition != NULL) {
        AnimationImpl* animation = m_type->m_visualCharacter->GetIdleAnimation(index);
        if (animation != NULL) {
            m_transition->startAnimation(animation, 1.0f, false, CharacterFadeMs, 1.0f, 0.0f);
            m_animationType = PERSON_ANIM_TYPE_DEFAULT;
            return animation->getName();
        }
    }
    return NULL;
}

const char* CUnitBody::PlayTranquillizeAnimation()
{
    return StartAnimation(Tranquillized, false);
}

const char* CUnitBody::PlayShotAnimation()
{
    if (m_type != NULL && m_transition != NULL) {
        AnimationImpl* animation = m_type->m_visualCharacter->GetShotAnimation(
            m_attitude, m_shotDirection, static_cast<EPersonAnimWeaponType>(m_unit->m_weaponAnimType));
        if (animation != NULL) {
            m_transition->startAnimation(animation, 1.0f, false, CharacterFadeMs, 1.0f, 0.0f);
            m_animationType = static_cast<EPersonAnimType>(8);
            return animation->getName();
        }
    }
    return NULL;
}

const char* CUnitBody::PlayHideAnimation()
{
    if (m_type != NULL && m_transition != NULL) {
        AnimationImpl* animation = m_type->m_visualCharacter->GetHideAnimation(
            m_attitude, static_cast<EPersonAnimWeaponType>(m_unit->m_weaponAnimType));
        if (animation != NULL) {
            m_transition->startAnimation(animation, 1.0f, false, CharacterFadeMs, 1.0f, 0.0f);
            m_animationType = static_cast<EPersonAnimType>(9);
            return animation->getName();
        }
    }
    return NULL;
}

const char* CUnitBody::PlayUnhideAnimation()
{
    if (m_type != NULL && m_transition != NULL) {
        AnimationImpl* animation = m_type->m_visualCharacter->GetUnhideAnimation(
            m_attitude, static_cast<EPersonAnimWeaponType>(m_unit->m_weaponAnimType));
        if (animation != NULL) {
            m_transition->startAnimation(animation, 1.0f, false, CharacterFadeMs, 1.0f, 0.0f);
            m_animationType = static_cast<EPersonAnimType>(10);
            return animation->getName();
        }
    }
    return NULL;
}

const char* CUnitBody::PlayReloadAnimation(bool* available)
{
    *available = false;
    if (m_type != NULL && m_transition != NULL) {
        AnimationImpl* animation = m_type->m_visualCharacter->GetReloadAnimation(
            m_attitude, static_cast<EPersonAnimWeaponType>(m_unit->m_weaponAnimType));
        if (animation == NULL) {
            *available = false;
            return NULL;
        }
        m_transition->startAnimation(animation, 1.0f, false, CharacterFadeMs, 1.0f, 0.0f);
        m_animationType = static_cast<EPersonAnimType>(11);
        *available = true;
        return animation->getName();
    }
    return NULL;
}

const char* CUnitBody::PlayGetWeaponAnimation()
{
    if (m_type != NULL && m_transition != NULL) {
        AnimationImpl* animation = m_type->m_visualCharacter->GetTakeOutWeaponAnimation(
            static_cast<EPersonAnimWeaponType>(m_unit->m_weaponAnimType));
        if (animation != NULL) {
            m_transition->startAnimation(animation, 1.0f, false, CharacterFadeMs, 1.0f, 0.0f);
            m_animationType = static_cast<EPersonAnimType>(12);
            return animation->getName();
        }
    }
    return NULL;
}

bool CUnitBody::IsRunning() const
{
    return m_animationType == 4 || m_animationType == 5;
}

void CUnitBody::PlayWeaponSound()
{
    if (m_type != NULL && m_type->m_weapon != NULL) {
        if (CBH_Player::GetInstance()->IsSpeedMode() == true) {
            GetApp()->m_soundManager->PlaySound("IDM_SLOW_GUNSHOT", false, false);
        } else {
            GetApp()->m_soundManager->PlaySound(m_type->m_weapon->m_shootSound, false, false);
        }
    }
}

void CUnitBody::ShowWeapon(bool show)
{
    if (m_weapon != NULL) {
        m_weapon.setRenderingEnable(show);
    }
}

void CUnitBody::ShowBulletMuzzleFlash()
{
    if (m_shotFlash != NULL) {
        m_shotFlash->Start(0.0f, 1.0f, 0.0f, 1.0f, 1.0f);
    }
}

void CUnitBody::UnsetDistanceTag()
{
    if (m_distanceTag != NULL) {
        Group mount(m_model.find(DGHelper::getSwerveID(CVisualCharacterType::m_pBodyDummyName)));
        if (mount != NULL) {
            mount.removeChild(m_distanceTag);
            m_distanceTag.Clear();
        }
    }
}

void CUnitBody::UnbindFromWorld()
{
    Group world(GetApp()->m_helper->m_swerveGame->m_world.find(DGHelper::getSwerveID("worldTransform")));
    world.removeChild(m_model);
}

void CUnitBody::DefineBodyZones(EHitType* type, float x, float y, const Camera& camera)
{
    if (m_zoneIntersection == NULL) {
        return;
    }
    if ((m_bodyArmorZone != NULL && m_bodyArmorZone.pick(-1, x, y, camera, m_zoneIntersection)) ||
        (m_headArmorZone != NULL && m_headArmorZone.pick(-1, x, y, camera, m_zoneIntersection))) {
        *type = HitArmor;
    } else if (m_headZone != NULL && m_headZone.pick(-1, x, y, camera, m_zoneIntersection)) {
        *type = HitHead;
    } else if (m_vitalZone != NULL && m_vitalZone.pick(-1, x, y, camera, m_zoneIntersection)) {
        *type = HitVital;
    }
}

void CUnitBody::CalculateStep(int elapsed)
{
    if (m_speed > 0.0f) {
        vec3 step = m_direction * m_speed * static_cast<float>(elapsed) * 0.001f;
        m_speed = 0.0f;
        m_position = m_position + step;
    }
}

bool CUnitBody::Pick(float x, float y, float maximumDistance, Camera& camera, EHitType& type)
{
    if (m_model == NULL || m_spreadIntersection == NULL) {
        return false;
    }
    typedef int (*GetPicking)(IQueryInterface*, unsigned char*);
    unsigned char picking = 0;
    CKMethod<GetPicking>(m_model.m_interface, 0x94)(m_model.m_interface, &picking);
    m_model.setPickingEnable(true);
    DGHelper* helper = GetApp()->m_helper;
    float relativeX = x / helper->m_screenWidth;
    float relativeY = y / helper->m_screenHeight;
    bool hit = m_model.pick(-1, relativeX, relativeY, camera, m_spreadIntersection) != 0;
    if (hit) {
        m_pickDistance = m_spreadIntersection.getDistance();
        hit = maximumDistance <= 0.0f || m_pickDistance <= maximumDistance;
        if (hit) {
            m_spreadIntersection.getRay(m_pickRay);
            m_hitDistance = m_pickDistance;
            type = HitBody;
            DefineBodyZones(&type, relativeX, relativeY, camera);
        }
    }
    m_model.setPickingEnable(picking != 0);
    return hit;
}

bool CUnitBody::SpreadPick(Camera& camera, Group& collision, EHitType& type)
{
    if (m_model == NULL || m_spreadIntersection == NULL) {
        return false;
    }
    const float rays[][4] = {
        {0.0f, 0.0f, 0.0f, 0.0f},
        {1.5f, 1.0f, 0.0f, 0.0f}, {-1.5f, 1.0f, 0.0f, 0.0f},
        {1.5f, 0.0f, 1.0f, 0.0f}, {-1.5f, 0.0f, 1.0f, 0.0f}
    };
    typedef int (*GetPicking)(IQueryInterface*, unsigned char*);
    unsigned char previous = 0;
    CKMethod<GetPicking>(m_model.m_interface, 0x94)(m_model.m_interface, &previous);
    m_model.setPickingEnable(true);
    bool hit = false;
    m_pickDistance = -1.0f;
    for (unsigned int index = 0; index < sizeof(rays) / sizeof(rays[0]); ++index) {
        const float* ray = rays[index];
        if (PickWithAngleAndCollisionGeometry(camera, collision, ray[0], ray[1], ray[2], ray[3],
                                              m_spreadIntersection, type)) {
            hit = true;
        }
    }
    m_model.setPickingEnable(previous != 0);
    return hit;
}

bool CUnitBody::PickWithAngleAndCollisionGeometry(Camera& camera, Group& collision, float angle,
                                                 float axisX, float axisY, float axisZ,
                                                 RayIntersection intersection, EHitType& type)
{
    if (!SwerveHelper::PickWithAngle(m_model, camera, angle, axisX, axisY, axisZ, intersection)) {
        return false;
    }
    float distance = intersection.getDistance();
    if (m_pickDistance >= 0.0f && distance >= m_pickDistance) {
        return false;
    }
    float ray[6];
    intersection.getRay(ray);
    if (SwerveHelper::PickWithAngle(collision, camera, angle, axisX, axisY, axisZ, intersection)
        && intersection.getDistance() < distance) {
        return false;
    }
    memcpy(m_pickRay, ray, sizeof(m_pickRay));
    m_pickDistance = distance;
    m_hitDistance = distance;
    type = HitBody;
    if (SwerveHelper::PickWithAngle(m_bodyArmorZone, camera, angle, axisX, axisY, axisZ, intersection)
        || SwerveHelper::PickWithAngle(m_headArmorZone, camera, angle, axisX, axisY, axisZ, intersection)) {
        type = HitArmor;
    } else if (SwerveHelper::PickWithAngle(m_headZone, camera, angle, axisX, axisY, axisZ, intersection)) {
        type = HitHead;
    } else if (SwerveHelper::PickWithAngle(m_vitalZone, camera, angle, axisX, axisY, axisZ, intersection)) {
        type = HitVital;
    }
    return true;
}

vec3 CUnitBody::GetAutoAimPointPosition()
{
    CSwerveGame* game = GetApp()->m_helper->m_swerveGame;
    if (m_headZone != NULL) {
        SwerveHelper::Transform transform = GetApp()->m_helper->m_transform;
        if (m_headZone.getTransformTo(game->m_world, transform)) {
            return SwerveHelper::GetTranslation(transform);
        }
    }
    return m_position + vec3(0.0f, 0.0f, GetHeight());
}

void CUnitBody::eventListener(const AnimationEvent& event, AnimationInstance* instance)
{
    if (instance == NULL || instance->m_animation == NULL) {
        return;
    }
    if (event.m_id != AnimationEvent::EVENT_END) {
        return;
    }
    const char* name = instance->m_animation->getName();
    EPersonAnimType type = m_type->m_visualCharacter->GetAnimationTypeByName(name);
    m_unit->m_mind.OnAnimationComplete(type, name);
}

void CUnitBody::DrawHitEffect()
{
    if (m_blood.getParent() == NULL) {
        m_model.addChild(m_blood);
    }
    m_hitEffectTransform = SwerveHelper::NewTransform();
    m_hitEffectTransform.setIdentity();
    float position[3];
    position[0] = m_pickRay[0] + m_hitDistance * m_pickRay[3];
    position[1] = m_pickRay[1] + m_hitDistance * m_pickRay[4];
    position[2] = m_pickRay[2] + m_hitDistance * m_pickRay[5];
    m_hitEffectTransform.postTranslate(position);
    float rotation[4];
    rotation[0] = -MathLib::ArcTan(-m_pickRay[3], -m_pickRay[4]);
    rotation[1] = 0.0f;
    rotation[2] = 0.0f;
    rotation[3] = 1.0f;
    m_hitEffectTransform.postRotate(rotation);
    m_blood.setTransform(m_hitEffectTransform);
    m_blood.setRenderingEnable(true);
    m_showBlood = true;
    m_bloodTimeMs = 0;
}

void CUnitBody::SetAsKnown()
{
    if (!m_unit->m_dead) {
        BindKnownIcon();
    }
}

typedef char AssertUnitBodySize[sizeof(CUnitBody) == 0x184 ? 1 : -1];
