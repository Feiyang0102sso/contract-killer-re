// DGWorld.cpp - the 3D world of a level and its hit effects.
#include "DGWorld.h"

#include "App.h"
#include "CSwerve.h"
#include "CSwerveGame.h"
#include "CStdUtil_Cocoa.h"
#include "DGHelper.h"
#include "MathLib.h"
#include "SwerveHelper.h"
#include "dgCamera.h"

namespace {
typedef ::World World;
typedef ::Node Node;
typedef ::Camera Camera;

/** Rows of the visibility table start at camera angle -45 degrees. */
const int VISIBILITY_ANGLE_OFFSET = 45;
}

DGWorld::DGWorld(World world, const char*)
    : m_world(world), m_field04(false), m_visibility(NULL), m_nodes(NULL), m_nodeCount(0),
      m_rayIntersection(CSwerve::GetInstance()->m_swerve)
{
    loadHitEffects();
}

void DGWorld::loadHitEffects()
{
    World ground = App::LoadObject3D("BIN_M3G_HIT_EFFECT_GROUND");
    World dust = App::LoadObject3D("BIN_M3G_HIT_EFFECT_DUST");
    World water = App::LoadObject3D("BIN_M3G_HIT_EFFECT_WATER");
    World blast = App::LoadObject3D("BIN_M3G_HIT_EFFECT_BLAST");
    if (ground != NULL) {
        m_effectNodes[EFFECT_GROUND] = Node(ground.find(SwerveHelper::Id("rootTransform")));
    }
    if (dust != NULL) {
        m_effectNodes[EFFECT_DUST] = Node(dust.find(SwerveHelper::Id("dummy_dustTransform")));
    }
    if (water != NULL) {
        m_effectNodes[EFFECT_WATER] = Node(water.find(SwerveHelper::Id("rootTransform")));
    }
    if (blast != NULL) {
        m_effectNodes[EFFECT_BLAST] = Node(blast.find(SwerveHelper::Id("dummy_blastTransform")));
    }
    m_effectDurations[EFFECT_GROUND] = 0x683;
    m_effectDurations[EFFECT_DUST] = 600;
    m_effectDurations[EFFECT_WATER] = 0x683;
    m_effectDurations[EFFECT_BLAST] = 0x535;
}

DGWorld::~DGWorld()
{
    if (m_nodes != NULL) {
        delete[] m_nodes;
        m_nodes = NULL;
    }
    if (m_visibility != NULL) {
        delete[] m_visibility;
        m_visibility = NULL;
    }
}

void DGWorld::update()
{
    CStdUtil_Cocoa::GetUpTimeMS();
    if (m_nodeCount < 1) {
        return;
    }
    DGCamera* camera = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->m_camera;
    int row = (int)camera->m_field0c + VISIBILITY_ANGLE_OFFSET;
    int rowStart = m_nodeCount * row;
    for (int index = 0; index < m_nodeCount; ++index) {
        m_nodes[index].setRenderingEnable(m_visibility[rowStart + index] & 1);
    }
}

void DGWorld::shot(int x, int y)
{
    if (m_world == NULL || m_rayIntersection == NULL || m_effectNodes[EFFECT_DUST] == NULL) {
        return;
    }
    Camera camera = m_world.getActiveCamera();
    if (camera == NULL) {
        return;
    }
    WindowApp* app = WindowApp::m_instance;
    float relativeX = static_cast<float>(x) / app->m_screenWidth;
    float relativeY = static_cast<float>(y) / app->m_screenHeight;
    if (!m_world.pick(-1, relativeX, relativeY, camera, m_rayIntersection)) {
        return;
    }
    float distance = m_rayIntersection.getDistance();
    if (distance == 0.0f) {
        return;
    }
    m_rayIntersection.getRay(m_pickRay);
    vec3 origin(m_pickRay[0], m_pickRay[1], m_pickRay[2]);
    vec3 direction(m_pickRay[3], m_pickRay[4], m_pickRay[5]);
    vec3 position = origin + direction * distance;
    SwerveHelper::Transform transform = SwerveHelper::NewTransform();
    transform.setIdentity();
    transform.postTranslate(&position.m_x);
    float rotation[4] = {-MathLib::ArcTan(direction.m_y, direction.m_x), 0.0f, 0.0f, 1.0f};
    transform.postRotate(rotation);

    // The CK shot path selects dust even when the picked mesh has a surfaceType parameter.
    EffectInstance effect = { EFFECT_DUST, transform, 0 };
    m_effects.addElement(effect);
}

void DGWorld::paintHitEffects(Graphics3D& graphics)
{
    for (int index = m_effects.size() - 1; index >= 0; --index) {
        EffectInstance& effect = m_effects[index];
        effect.m_time += WindowApp::m_instance->m_updateFrameTime;
        if (effect.m_time >= m_effectDurations[effect.m_type]) {
            m_effects.removeElementAt(index);
        } else {
            // The original renders without a null check on the effect node.
            // fps: original calls an out-of-line Graphics3D::render(const Node&, const Transform&) const
            // (weak, 32 bytes); tws.h only has the inline interface-pointer overload.
            Node& node = m_effectNodes[effect.m_type];
            node.animate(effect.m_time);
            graphics.render(node, effect.m_transform);
        }
    }
}

void DGWorld::AddBlast(const vec3& position, const vec3& normal, float scale)
{
    if (m_effectNodes[EFFECT_BLAST] == NULL) {
        return;
    }
    vec3 center = position + normal * 0.1f;
    SwerveHelper::Transform transform = SwerveHelper::NewTransform();
    transform.setIdentity();
    transform.postTranslate(&center.m_x);
    float dimensions[3] = {scale, scale, scale};
    transform.postScale(dimensions);
    EffectInstance effect = { EFFECT_BLAST, transform, 0 };
    m_effects.addElement(effect);
}

typedef char AssertDGWorldSize[sizeof(DGWorld) == 0x70 ? 1 : -1];
