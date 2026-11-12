// CSkyBox.h - sky box of a 3D scene: either a separate world (BIN_SKYBOX) or its mesh moved into the
// scene world and rendered with the camera's rotation only.
#ifndef CSKYBOX_H
#define CSKYBOX_H

#include "tws.h"

/** Sky box (0x0c bytes). */
class CSkyBox {
public:
    CSkyBox();
    ~CSkyBox();

    /** Load BIN_SKYBOX for `world`. */
    void Init(World& world);
    /** Switch depth test and depth writes off for `appearance` (a compositing mode is created when missing). */
    void DisableZBuffer(Appearance appearance);
    /** Keep the sky mesh around the active camera. */
    void Update();
    void Render(const Graphics3D& graphics);

    World m_skyWorld;     // 0x00 BIN_SKYBOX (NULL once its mesh moved)
    World m_world;        // 0x04 the scene world
    Mesh m_skyMesh;       // 0x08 sky mesh inside m_world
};

#endif
