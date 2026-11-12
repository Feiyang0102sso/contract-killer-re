#include "CSkyBox.h"

#include "App.h"
#include "DGHelper.h"
#include "SwerveHelper.h"
#include "AdBannerSizes.h"

namespace {
typedef ::Appearance Appearance;
typedef ::Camera Camera;
typedef ::CompositingMode CompositingMode;
typedef ::Mesh Mesh;
typedef ::World World;
}

/** Global camera handle of this unit (never assigned in CK; only its destructor runs at exit). */
Camera cam;

CSkyBox::CSkyBox()
{
}

CSkyBox::~CSkyBox()
{
    m_skyWorld.Clear();
    m_skyMesh.Clear();
}

void CSkyBox::Init(World& world)
{
    m_world = world;
    m_skyWorld = World(App::LoadObject3D("BIN_SKYBOX"));
    if (m_skyWorld == NULL) {
        return;
    }
    m_skyWorld.setPickingEnable(false);
    if (world == NULL) {
        return;
    }
    int skyId = DGHelper::getSwerveID("SkyBox");
    Mesh sky(m_skyWorld.find(skyId));
    if (sky == NULL) {
        return;
    }
    sky.setPickingEnable(false);
    Mesh materials(world.find(DGHelper::getSwerveID("SkyBoxMaterialDummy")));
    if (materials == NULL) {
        SwerveHelper::Node copy(sky.duplicate());
        if (copy != NULL) {
            world.addChild(copy);
        }
        return;
    }

    if (materials.getSubmeshCount() < 6 || sky.getSubmeshCount() < 6) {
        return;
    }
    Appearance faces[6];
    for (int index = 0; index < 6; ++index) {
        faces[index] = materials.getAppearance(index);
        if (faces[index] == NULL) {
            return;
        }
    }
    for (int index = 0; index < 6; ++index) {
        DisableZBuffer(faces[index]);
        sky.setAppearance(index, faces[index]);
    }
    Mesh copy(sky.duplicate());
    if (copy != NULL) {
        world.addChild(copy);
        m_skyMesh = copy;
    }
    materials.setRenderingEnable(false);
}

void CSkyBox::DisableZBuffer(Appearance appearance)
{
    CompositingMode mode = appearance.getCompositingMode();
    if (mode == NULL) {
        mode = SwerveHelper::NewCompositingMode();
        if (mode == NULL) {
            return;
        }
        mode.setDepthTestEnable(false);
        mode.setDepthWriteEnable(false);
        appearance.setCompositingMode(mode);
    } else {
        mode.setDepthWriteEnable(false);
        mode.setDepthTestEnable(false);
    }
}

void CSkyBox::Update()
{
    if (m_skyMesh == NULL) {
        return;
    }
    Camera camera = m_world.getActiveCamera();
    if (camera == NULL) {
        return;
    }
    SwerveHelper::Transform cameraTransform = SwerveHelper::NewTransform();
    if (!camera.getTransformTo(m_world, cameraTransform)) {
        return;
    }
    float cameraMatrix[16];
    cameraTransform.get(cameraMatrix);
    SwerveHelper::Transform skyTransform = SwerveHelper::NewTransform();
    const float rotation[4] = {-90.0f, 1.0f, 0.0f, 0.0f};
    skyTransform.postRotate(rotation);
    float matrix[16];
    skyTransform.get(matrix);
    matrix[3] = cameraMatrix[3];
    matrix[7] = cameraMatrix[7] - 5.0f;
    matrix[11] = cameraMatrix[11] + 5.0f;
    skyTransform.set(matrix);
    m_skyMesh.setTransform(skyTransform);
}

void CSkyBox::Render(const Graphics3D& graphics)
{
    if (m_skyMesh == NULL) {
        if (m_skyWorld != NULL) {
            App::RenderWorld(graphics, m_skyWorld);
        }
        return;
    }
    SwerveHelper::Node root(m_world.find(DGHelper::getSwerveID("RootNode")));
    if (root == NULL) {
        return;
    }
    m_skyMesh.setRenderingEnable(true);
    root.setRenderingEnable(false);
    App::RenderWorld(graphics, m_world);
    m_skyMesh.setRenderingEnable(false);
    root.setRenderingEnable(true);
}

typedef char AssertSkyBoxSize[sizeof(CSkyBox) == 0xc ? 1 : -1];
