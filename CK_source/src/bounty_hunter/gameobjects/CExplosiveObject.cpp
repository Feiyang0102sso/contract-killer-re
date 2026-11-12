#include "CExplosiveObject.h"

#include "App.h"
#include "CDH_Animation.h"
#include "CDH_SoundManager.h"
#include "CSwerve.h"
#include "CSwerveGame.h"
#include "CVisualObjectManager.h"
#include "CXmlHelper.h"
#include "DGHelper.h"

CExplosiveObject::CExplosiveObject(const TiXmlNode* config)
    : m_maxHp(0), m_hp(0), m_damage(0), m_damageRadius(0), m_explosionDelay(0),
      m_removeAfterExplosion(false), m_exploded(false), m_pendingExplosion(false),
      m_animationComplete(false), m_animation(NULL)
{
    m_damage = CXmlHelper::GetAttributeValue(config, "damage").ToInt();
    m_damageRadius = CXmlHelper::GetAttributeValue(config, "damage_radius").ToInt();
    m_maxHp = CXmlHelper::GetAttributeValue(config, "hp").ToInt();
    m_hp = m_maxHp;
    m_name = CXmlHelper::GetAttributeValue(config, "name");
    m_pickableName = CXmlHelper::GetAttributeValue(config, "pickable_dummy_name");
    m_removeAfterExplosion = CXmlHelper::GetAttributeValue(config, "delete_after_exposion").ToBool();
    m_resource = CXmlHelper::GetAttributeValue(config, "modelRes");
    XString::AnsiString resource(m_resource.c_str());
    const char* model = resource;
    LoadModel(model, CXmlHelper::GetAttributeValue(config, "dummy_name"));
}

CExplosiveObject::~CExplosiveObject()
{
    Unbind();
    m_model.Clear();
    XString::AnsiString resource(m_resource.c_str());
    const char* model = resource;
    App* app = static_cast<App*>(WindowApp::m_instance);
    if (app->m_visualObjectManager != NULL) {
        app->m_visualObjectManager->Release(model);
    }
    delete m_animation;
}

void CExplosiveObject::LoadModel(const char* const& resource, const XString& root)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_model = app->m_visualObjectManager->Create(resource, XString::AnsiString(root.c_str()));
    if (m_model == NULL) {
        return;
    }
    m_model.setPickingEnable(false);
    m_pickable = Group(m_model.find(SwerveHelper::Id(XString::AnsiString(m_pickableName.c_str()))));
    if (m_pickable != NULL) {
        m_pickable.setPickingEnable(true);
    }
    m_animation = new CDH_Animation(m_model);
    m_animation->addController(XString("barrel_blast"));
    m_intersection = RayIntersection(CSwerve::GetInstance()->m_swerve);
}

void CExplosiveObject::SetToPos(Group& point)
{
    CSwerveGame* game = CSwerveGame::m_pInstance;
    SwerveHelper::Transform transform = SwerveHelper::NewTransform();
    if (m_model != NULL && point.getTransformTo(game->m_world, transform)) {
        m_model.setTransform(transform);
        Mat4x4 matrix;
        m_position = SwerveHelper::SetMatrix(matrix, transform).Translation();
    }
}

void CExplosiveObject::Bind()
{
    CSwerveGame* game = CSwerveGame::m_pInstance;
    if (m_model != NULL) {
        m_parent = Group(game->m_world.find(DGHelper::getSwerveID("worldTransform")));
        if (m_parent != NULL) {
            m_parent.addChild(m_model);
        }
    }
}

void CExplosiveObject::Unbind()
{
    if (m_parent != NULL) {
        m_parent.removeChild(m_model);
        m_parent.Clear();
    }
}

const XString& CExplosiveObject::GetName()
{
    return m_name;
}

bool CExplosiveObject::Pick(float x, float y, float distance, const Camera& camera)
{
    if (m_pickable == NULL || m_intersection == NULL || camera == NULL) {
        return false;
    }
    WindowApp* app = WindowApp::m_instance;
    if (!m_pickable.pick(-1, x / app->m_screenWidth, y / app->m_screenHeight, camera, m_intersection)) {
        return false;
    }
    return m_intersection.getDistance() < distance;
}

void CExplosiveObject::Tick(int elapsed)
{
    if (m_animation == NULL || m_animationComplete) {
        return;
    }
    int time = CSwerveGame::m_pInstance->m_missionElapsedMs;
    m_animation->animate(time);
    if (m_pendingExplosion && !m_exploded) {
        m_explosionDelay -= elapsed;
        if (m_explosionDelay <= 0) {
            Explode();
        }
    }
    if (m_exploded && m_animation->isAnimationComplete(time)) {
        m_animationComplete = true;
        if (m_removeAfterExplosion) {
            Unbind();
        }
    }
}

void CExplosiveObject::Explode()
{
    if (m_animation == NULL || m_exploded) {
        return;
    }
    CSwerveGame* game = CSwerveGame::m_pInstance;
    m_animation->setCurrentController(game->m_missionElapsedMs, 0, false);
    m_exploded = true;
    App* app = static_cast<App*>(WindowApp::m_instance);
    app->m_soundManager->PlaySound("IDM_BARREL_EXPLOSION", false, false);
    game->OnExplosionOccured(this);
    game->m_peaceTime = false;
    game->m_missionAlert = true;
}

void CExplosiveObject::ReceivedDamage(int damage, int delayMs)
{
    if (damage > 0) {
        m_hp -= damage;
        if (m_hp <= 0) {
            if (!m_pendingExplosion) {
                m_explosionDelay = delayMs;
            }
            m_pendingExplosion = true;
        }
    }
}

bool CExplosiveObject::CanBeHandled()
{
    return !m_animationComplete;
}

typedef char AssertExplosiveObjectSize[sizeof(CExplosiveObject) == 0x48 ? 1 : -1];
