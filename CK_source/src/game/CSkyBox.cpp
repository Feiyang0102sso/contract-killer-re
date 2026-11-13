#include "CSkyBox.h"

#include "CSwerve.h"
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
    m_skyWorld = World(Object3D(
        (IQueryInterface*)CSwerve::GetInstance()->LoadM3GResource("BIN_SKYBOX")));
    if (m_skyWorld == NULL) {
        return;
    }
    m_skyWorld.setPickingEnable(false);
    if (world == NULL) {
        return;
    }
    Mesh sky(m_skyWorld.find(DGHelper::getSwerveID("SkyBox")));
    if (sky == NULL) {
        return;
    }
    sky.setPickingEnable(false);
    Mesh materials(world.find(DGHelper::getSwerveID("SkyBoxMaterialDummy")));
    if (materials == NULL) {
        // Original adds the plain copy without checking it.
        world.addChild(SwerveHelper::Node(sky.duplicate()));
        return;
    }
    Appearance face0 = materials.getAppearance(0);
    Appearance face1 = materials.getAppearance(1);
    Appearance face2 = materials.getAppearance(2);
    Appearance face3 = materials.getAppearance(3);
    Appearance face4 = materials.getAppearance(4);
    Appearance face5 = materials.getAppearance(5);
    if (face0 != NULL && face1 != NULL && face2 != NULL && face3 != NULL &&
        face4 != NULL && face5 != NULL) {
        DisableZBuffer(face0);
        DisableZBuffer(face1);
        DisableZBuffer(face2);
        DisableZBuffer(face3);
        DisableZBuffer(face4);
        DisableZBuffer(face5);
        sky.setAppearance(0, face0);
        sky.setAppearance(1, face1);
        sky.setAppearance(2, face2);
        sky.setAppearance(3, face3);
        sky.setAppearance(4, face4);
        sky.setAppearance(5, face5);
        world.addChild(SwerveHelper::Node(sky.duplicate()));
        // The rendered sky is the copy now living in the game world.
        m_skyMesh = Mesh(world.find(DGHelper::getSwerveID("SkyBox")));
        materials.setRenderingEnable(false);
    }
}

void CSkyBox::DisableZBuffer(Appearance appearance)
{
    CompositingMode mode = appearance.getCompositingMode();
    if (mode == NULL) {
        mode = SwerveHelper::NewCompositingMode();
        mode.setDepthTestEnable(false);
        mode.setDepthWriteEnable(false);
        appearance.setCompositingMode(mode);
    } else {
        // Original fetches the mode again for each call.
        appearance.getCompositingMode().setDepthWriteEnable(false);
        appearance.getCompositingMode().setDepthTestEnable(false);
    }
}

void CSkyBox::Update()
{
    if (m_skyMesh == NULL) {
        return;
    }
    // Original: no camera check, and the sky first gets an identity transform.
    Camera camera = m_world.getActiveCamera();
    Transform cameraTransform(CSwerve::GetInstance()->m_swerve);
    camera.getTransformTo(m_world, cameraTransform);
    Transform skyTransform(CSwerve::GetInstance()->m_swerve);
    m_skyMesh.setTransform(skyTransform);
    float cameraMatrix[16];
    cameraTransform.get(cameraMatrix);
    float rotation[4];
    rotation[0] = -90.0f;
    rotation[1] = 1.0f;
    rotation[2] = 0.0f;
    rotation[3] = 0.0f;
    skyTransform.postRotate(rotation);
    float matrix[16];
    skyTransform.get(matrix);
    matrix[3] = cameraMatrix[3];
    matrix[7] = cameraMatrix[7] - 5.0f;
    matrix[11] = cameraMatrix[11] + 5.0f;
    skyTransform.set(matrix);
    m_skyMesh.setTransform(skyTransform);
    cameraTransform.Clear();
    skyTransform.Clear();
}

void CSkyBox::Render(const Graphics3D& graphics)
{
    if (m_skyMesh != NULL) {
        m_skyMesh.setRenderingEnable(true);
        Group root(m_world.find(DGHelper::getSwerveID("RootNode")));
        root.setRenderingEnable(false);
        graphics.render(m_world);
        m_skyMesh.setRenderingEnable(false);
        root.setRenderingEnable(true);
    } else {
        graphics.render(m_skyWorld);
    }
}

typedef char AssertSkyBoxSize[sizeof(CSkyBox) == 0xc ? 1 : -1];
