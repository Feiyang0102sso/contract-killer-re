#include "CExplosiveObject.h"

#include "App.h"
#include "CDH_Animation.h"
#include "CDH_SoundManager.h"
#include "CSwerve.h"
#include "CSwerveGame.h"
#include "CVisualObjectManager.h"
#include "CXmlHelper.h"
#include "DGHelper.h"

CExplosiveObject::CExplosiveObject(const TiXmlNode* config) : m_reserved20(0)
{
    m_animation = NULL;
    m_animationComplete = false;
    m_pendingExplosion = false;
    m_exploded = false;
    m_removeAfterExplosion = false;
    m_explosionDelay = 0;
    m_damage = CXmlHelper::GetAttributeValue(config, "damage").ToInt();
    m_damageRadius = CXmlHelper::GetAttributeValue(config, "damage_radius").ToInt();
    m_maxHp = CXmlHelper::GetAttributeValue(config, "hp").ToInt();
    m_hp = m_maxHp;
    m_name = CXmlHelper::GetAttributeValue(config, "name");
    m_pickableName = CXmlHelper::GetAttributeValue(config, "pickable_dummy_name");
    m_removeAfterExplosion = CXmlHelper::GetAttributeValue(config, "delete_after_exposion").ToBool();
    m_resource = CXmlHelper::GetAttributeValue(config, "modelRes");
    XString dummy = CXmlHelper::GetAttributeValue(config, "dummy_name");
    XString::AnsiString resource(m_resource.c_str());
    const char* model = resource;
    LoadModel(model, dummy);
}

CExplosiveObject::~CExplosiveObject()
{
    m_model.Clear();
    {
        CVisualObjectManager* visualObjects = static_cast<App*>(WindowApp::m_instance)->m_visualObjectManager;
        XString::AnsiString resource(m_resource.c_str());
        const char* model = resource;
        visualObjects->Release(model);
    }
    if (m_animation != NULL) {
        delete m_animation;
        m_animation = NULL;
    }
}

void CExplosiveObject::LoadModel(const char* const& resource, const XString& root)
{
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_model = app->m_visualObjectManager->Create(resource, XString::AnsiString(root.c_str()));
    m_model.setPickingEnable(false);
    m_pickable = Group(m_model.find(SwerveHelper::Id(XString::AnsiString(m_pickableName.c_str()))));
    if (m_pickable != NULL) {
        m_pickable.setPickingEnable(true);
    }
    m_animation = new CDH_Animation(m_model);
    m_animation->addController(XString("barrel_blast"));
}

void CExplosiveObject::SetToPos(Group& point)
{
    if (point != NULL) {
        SwerveHelper::Transform transform(CSwerve::GetInstance()->m_swerve);
        CSwerveGame* game = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame;
        {
            SwerveHelper::Transform transformForPoint(transform);
            bool positioned;
            typedef int (*GetTransformTo)(IQueryInterface*, INodeInterface*, ITransformInterface*, bool*);
            CKMethod<GetTransformTo>(point.m_interface, 0xa4)(
                point.m_interface, game->m_world.node(), transformForPoint.transformInterface(), &positioned);
        }
        m_model.setTransform(transform);
        float matrix[16];
        transform.get(matrix);
        m_position.m_x = matrix[3];
        m_position.m_y = matrix[7];
        m_position.m_z = matrix[11];
    }
}

void CExplosiveObject::Bind()
{
    CSwerveGame* game = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame;
    Group parent(game->m_world.find(DGHelper::getSwerveID("worldTransform")));
    parent.addChild(m_model);
}

void CExplosiveObject::Unbind()
{
    CSwerveGame* game = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame;
    Group parent(game->m_world.find(DGHelper::getSwerveID("worldTransform")));
    parent.removeChild(m_model);
}

const XString& CExplosiveObject::GetName()
{
    return m_name;
}

bool CExplosiveObject::Pick(float x, float y, float distance, const Camera& camera)
{
    DGHelper* helper = static_cast<App*>(WindowApp::m_instance)->m_helper;
    int width = helper->m_screenWidth;
    int height = helper->m_screenHeight;
    RayIntersection intersection(CSwerve::GetInstance()->m_swerve);
    unsigned char hit;
    unsigned char wasHit;
    {
        RayIntersection result(intersection);
        typedef int (*ScreenPick)(IQueryInterface*, int, float, float, IQueryInterface*,
                                  IQueryInterface*, unsigned char*);
        CKMethod<ScreenPick>(m_pickable.m_interface, 0xcc)(
            m_pickable.m_interface, -1, x / width, y / height, camera.m_interface,
            result.m_interface, &hit);
        wasHit = hit;
    }
    if (wasHit == 0) {
        return false;
    }
    return intersection.getDistance() < distance;
}

void CExplosiveObject::Tick(int elapsed)
{
    int time = static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_missionElapsedMs;
    m_animation->animate(time);
    if (m_pendingExplosion && !m_exploded) {
        m_explosionDelay = static_cast<unsigned int>(m_explosionDelay) - elapsed;
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
    App* app = static_cast<App*>(WindowApp::m_instance);
    m_animation->setCurrentController(app->m_helper->m_swerveGame->m_missionElapsedMs, 0, false);
    m_exploded = true;
    static_cast<App*>(WindowApp::m_instance)->m_soundManager->PlaySound("IDM_BARREL_EXPLOSION", false, false);
    static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->OnExplosionOccured(this);
    static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_peaceTime = false;
    static_cast<App*>(WindowApp::m_instance)->m_helper->m_swerveGame->m_missionAlert = true;
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
