// DGWorld.cpp - the 3D world of a level and its hit effects.
#include "DGWorld.h"

#include "App.h"
#include "CDH_Cheats.h"
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
    m_effectNodes[EFFECT_GROUND] = Node(ground.find(SwerveHelper::Id("rootTransform")));
    m_effectNodes[EFFECT_DUST] = Node(dust.find(SwerveHelper::Id("dummy_dustTransform")));
    m_effectNodes[EFFECT_WATER] = Node(water.find(SwerveHelper::Id("rootTransform")));
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
        bool visible = false;
        if (m_visibility[rowStart + index] & 1) {
            visible = CDH_Cheats::CHEAT_SHOW_TREES;
        }
        m_nodes[index].setRenderingEnable(visible);
    }
}

void DGWorld::shot(int x, int y)
{
    Camera camera = m_world.getActiveCamera();
    WindowApp* app = WindowApp::m_instance;
    IQueryInterface* cameraHandle = camera.m_interface;
    IQueryInterface* intersection = m_rayIntersection.m_interface;
    int width = app->m_screenWidth;
    int height = app->m_screenHeight;
    QueryInterface::AddReference(intersection);
    float relativeX = static_cast<float>(x) / width;
    float relativeY = static_cast<float>(y) / height;
    unsigned char hit;
    typedef int (*Pick)(IQueryInterface*, int, float, float, IQueryInterface*, IQueryInterface*, unsigned char*);
    CKMethod<Pick>(m_world.m_interface, 0xcc)(m_world.m_interface, -1, relativeX, relativeY,
                                             cameraHandle, intersection, &hit);
    unsigned char picked = hit;
    if (intersection != NULL) {
        intersection->Release();
    }
    if (picked == 0) {
        return;
    }
    float distance = m_rayIntersection.getDistance();
    if (distance == 0.0f) {
        return;
    }
    m_rayIntersection.getRay(m_pickRay);
    SwerveHelper::Transform transform = SwerveHelper::NewTransform();
    transform.setIdentity();
    vec3 origin(m_pickRay[0], m_pickRay[1], m_pickRay[2]);
    vec3 direction(m_pickRay[3], m_pickRay[4], m_pickRay[5]);
    vec3 position = origin + direction * distance;
    transform.postTranslate(&position.m_x);
    float rotation[4] = {-MathLib::ArcTan(-m_pickRay[3], -m_pickRay[4]), 0.0f, 0.0f, 1.0f};
    transform.postRotate(rotation);

    Node parent = m_rayIntersection.getIntersected().getParent();
    parent.getUserID();
    int surfaceType = -1;
    static_cast<App*>(WindowApp::m_instance)->m_helper->getUserParameter(parent, "surfaceType", &surfaceType);

    // The CK shot path selects dust even when the picked mesh has a surfaceType parameter.
    surfaceType = EFFECT_DUST;
    EffectInstance effect = { surfaceType, transform, 0 };
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

typedef char AssertDGWorldSize[sizeof(DGWorld) == 0x70 ? 1 : -1];
