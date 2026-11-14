// Render test of the two Swerve backends: every M3G file of BountyHunter.big and vga.big is drawn by
// the original ARM Swerve core and by the native m3gcore runtime into the same offscreen framebuffer,
// and the pictures are compared pixel by pixel.
//
// A file whose root is a World is drawn through its own active camera. Any other model is hung into a
// placeholder World (CK has no fixed survival scene to test in): a camera in front of the model's
// bounding box and an ambient plus a directional light. Every scene is drawn from two camera poses,
// the second turned by PoseTurnDegrees, so that a difference which only appears when the view moves
// (draw order changing with what the frustum culls) shows up too.
//
// Set CK_RENDER_SAVE=1 to save every picture; otherwise only the differing ones are saved, as
// render_compare\<file>_pose<n>.bmp next to the executable: original | native | difference.
#include "SwerveBackends.h"
#include "GameHarness.h"
#include "../CK_source/pc_port/swerve_native/NativeRuntime.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <SDL3/SDL.h>
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include <SwerveApi.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <filesystem>
#include <string>
#include <vector>

namespace ck::test {
namespace {

// Size of the offscreen picture (the game's 4:3).
constexpr int PictureWidth = 320;
constexpr int PictureHeight = 240;
// A pixel differs when a colour channel differs by more than this.
constexpr int PixelTolerance = 48;
// A picture fails when more than this share of its pixels differs.
constexpr double FailingShare = 0.02;
// A picture is saved when more than this share differs (or with CK_RENDER_SAVE).
constexpr double SavedShare = 0.002;
constexpr const char* SaveAllVariable = "CK_RENDER_SAVE";
constexpr const wchar_t* SaveFolder = L"render_compare";
// Camera poses: as placed, then turned about the camera's Y axis.
constexpr int PoseCount = 2;
constexpr float PoseTurnDegrees = 1.5f;
// Flicker check of Worlds: frames with the camera turned by a tiny angle each time.
constexpr int StabilitySteps = 6;
constexpr float StabilityTurnDegrees = 0.02f;
// Allowed extra changing share per step for the native picture (besides twice the original's).
constexpr double StabilitySlack = 0.002;
// World time of animate() before drawing.
constexpr int DrawTime = 0;
// Placeholder scene: camera distance in bounding radii, field of view and lights.
constexpr float CameraDistanceRadii = 2.2f;
constexpr float FieldOfViewDegrees = 60.0f;
constexpr int LightAmbient = 128;      // Light.AMBIENT
constexpr int LightDirectional = 129;  // Light.DIRECTIONAL
constexpr int AmbientColor = 0x606060;
constexpr int DirectionalColor = 0xffffff;
constexpr int BytesPerPixel = 4;

/** The offscreen framebuffer both backends draw into. */
struct Target {
    GLuint framebuffer;
    GLuint color;
    GLuint depth;
};

Target target = {};

bool CreateTarget()
{
    if (target.framebuffer != 0) {
        return true;
    }
    glGenFramebuffersOES(1, &target.framebuffer);
    glGenRenderbuffersOES(1, &target.color);
    glGenRenderbuffersOES(1, &target.depth);
    GLint previous = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_OES, &previous);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, target.framebuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, target.color);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_RGBA8_OES, PictureWidth, PictureHeight);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, target.color);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, target.depth);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT24_OES, PictureWidth, PictureHeight);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, target.depth);
    const GLenum status = glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, static_cast<GLuint>(previous));
    if (status != GL_FRAMEBUFFER_COMPLETE_OES) {
        fprintf(stderr, "Render test: offscreen framebuffer incomplete (0x%x)\n", status);
        return false;
    }
    return true;
}

/** Both runtimes and a Graphics3D of each. */
struct Renderers {
    ISwerve* native;
    ISwerve* original;
    Handle nativeGraphics;
    Handle originalGraphics;
    bool ready;
};

Renderers renderers = {};

/** Draw `world` with `graphics` into the offscreen target; returns the RGBA rows (bottom row first). */
std::vector<uint8_t> Draw(Handle graphics, Handle world)
{
    GLint previous = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_OES, &previous);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, target.framebuffer);
    glViewport(0, 0, PictureWidth, PictureHeight);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepthf(1.0f);
    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // As CSwerveGraphics3D::BindTarget: the bound framebuffer, no pixels, the whole target as viewport.
    Call<int>(graphics, slot::graphics3d::setCroppedBackBufferData, PictureWidth, PictureHeight, 0, 0, 0, 0,
              PictureWidth, PictureHeight);
    Call<void>(graphics, slot::graphics3d::renderWorld, world);
    glFinish();
    std::vector<uint8_t> pixels(static_cast<size_t>(PictureWidth) * PictureHeight * BytesPerPixel);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, PictureWidth, PictureHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, static_cast<GLuint>(previous));
    return pixels;
}

/** Bounding box of the meshes under `root` in root coordinates, from the native runtime. */
bool MeasureModel(Handle root, Handle transform, Handle node, float* low, float* high, bool& found)
{
    if (Supports(node, ClassMesh)) {
        int reachable = 0;
        Call<int>(transform, slot::transform::setIdentity);
        Call<int>(node, slot::node::getTransformTo, root, transform, reinterpret_cast<bool*>(&reachable));
        float matrix[16];
        Call<int>(transform, slot::transform::get, 16, matrix);
        Handle vertexBuffer = nullptr;
        Call<int>(node, slot::mesh::getVertexBuffer, &vertexBuffer);
        if (vertexBuffer != nullptr) {
            float scaleBias[4] = {};
            Handle positions = nullptr;
            Call<int>(vertexBuffer, slot::vertexbuffer::getPositions, 4, scaleBias, &positions);
            if (positions != nullptr) {
                int count = 0;
                Call<int>(positions, slot::vertexarray::getVertexCount, &count);
                std::vector<short> values(static_cast<size_t>(count) * 3);
                Call<int>(positions, slot::vertexarray::get16, 0, count, count * 3, values.data());
                for (int vertex = 0; vertex < count; vertex++) {
                    float local[3];
                    for (int axis = 0; axis < 3; axis++) {
                        local[axis] = values[static_cast<size_t>(vertex) * 3 + axis] * scaleBias[0] + scaleBias[1 + axis];
                    }
                    for (int axis = 0; axis < 3; axis++) {
                        const int row = axis * 4;
                        const float value = matrix[row] * local[0] + matrix[row + 1] * local[1] +
                                            matrix[row + 2] * local[2] + matrix[row + 3];
                        if (!found || value < low[axis]) {
                            low[axis] = value;
                        }
                        if (!found || value > high[axis]) {
                            high[axis] = value;
                        }
                    }
                    found = true;
                }
                Release(positions);
            }
            Release(vertexBuffer);
        }
    }
    if (Supports(node, ClassGroup)) {
        int childCount = 0;
        Call<int>(node, slot::group::getChildCount, &childCount);
        for (int index = 0; index < childCount; index++) {
            Handle child = nullptr;
            Call<int>(node, slot::group::getChild, index, &child);
            if (child != nullptr) {
                MeasureModel(root, transform, child, low, high, found);
                Release(child);
            }
        }
    }
    return found;
}

/** Where the placeholder camera goes for a model: centre and bounding radius. */
struct Framing {
    float center[3];
    float radius;
};

Handle CreateLight(ISwerve* runtime, int mode, int color)
{
    Handle light = CreateObject(runtime, ClassLight);
    Call<int>(light, slot::light::setMode, mode);
    Call<int>(light, slot::light::setColor, color);
    Call<int>(light, slot::light::setIntensity, 1.0f);
    return light;
}

/**
 * The scene of a root object: the World itself, or a placeholder World holding the model.
 * Returns a new reference to the World (NULL when the root cannot be drawn); `camera` gets a new
 * reference to its active camera.
 */
Handle MakeScene(ISwerve* runtime, Handle root, const Framing& framing, Handle& camera)
{
    camera = nullptr;
    if (Supports(root, ClassWorld)) {
        Call<int>(root, slot::world::getActiveCamera, &camera);
        if (camera == nullptr) {
            return nullptr;
        }
        Call<void>(root, slot::AddRef);
        return root;
    }
    if (!Supports(root, ClassNode)) {
        return nullptr;
    }
    Handle world = CreateObject(runtime, ClassWorld);
    Call<int>(world, slot::group::addChild, root);
    camera = CreateObject(runtime, ClassCamera);
    const float nearClip = framing.radius * 0.05f;
    const float farClip = framing.radius * 10.0f;
    Call<int>(camera, slot::camera::setPerspective, FieldOfViewDegrees,
              static_cast<float>(PictureWidth) / PictureHeight, nearClip, farClip);
    const float position[3] = { framing.center[0], framing.center[1],
                                framing.center[2] + framing.radius * CameraDistanceRadii };
    Call<int>(camera, slot::transformable::setTranslation, position);
    Call<int>(world, slot::group::addChild, camera);
    Call<int>(world, slot::world::setActiveCamera, camera);
    // An ambient light and a directional one shining along the view (-Z), as a light on the camera.
    Handle ambient = CreateLight(runtime, LightAmbient, AmbientColor);
    Handle directional = CreateLight(runtime, LightDirectional, DirectionalColor);
    Call<int>(world, slot::group::addChild, ambient);
    Call<int>(world, slot::group::addChild, directional);
    Release(ambient);
    Release(directional);
    return world;
}

/** Undo MakeScene's placeholder: the model leaves the placeholder World so the roots stay unchanged. */
void DropScene(Handle world, Handle root, Handle camera)
{
    if (world != root) {
        Call<int>(world, slot::group::removeChild, root);
    }
    Release(camera);
    Release(world);
}

void WriteLittleEndian(std::vector<uint8_t>& out, uint32_t value, int bytes)
{
    for (int index = 0; index < bytes; index++) {
        out.push_back(static_cast<uint8_t>(value >> (8 * index)));
    }
}

/** original | native | difference as a 24-bit BMP (rows bottom-up like GL). */
void SavePictures(const std::string& name, const std::vector<uint8_t>& original, const std::vector<uint8_t>& native)
{
    std::filesystem::create_directories(SaveFolder);
    const int width = PictureWidth * 3;
    const int rowBytes = (width * 3 + 3) & ~3;
    std::vector<uint8_t> file;
    file.push_back('B');
    file.push_back('M');
    WriteLittleEndian(file, static_cast<uint32_t>(54 + rowBytes * PictureHeight), 4);
    WriteLittleEndian(file, 0, 4);
    WriteLittleEndian(file, 54, 4);
    WriteLittleEndian(file, 40, 4);
    WriteLittleEndian(file, static_cast<uint32_t>(width), 4);
    WriteLittleEndian(file, static_cast<uint32_t>(PictureHeight), 4);
    WriteLittleEndian(file, 1, 2);
    WriteLittleEndian(file, 24, 2);
    WriteLittleEndian(file, 0, 4);
    WriteLittleEndian(file, static_cast<uint32_t>(rowBytes * PictureHeight), 4);
    WriteLittleEndian(file, 2835, 4);
    WriteLittleEndian(file, 2835, 4);
    WriteLittleEndian(file, 0, 4);
    WriteLittleEndian(file, 0, 4);
    for (int y = 0; y < PictureHeight; y++) {
        std::vector<uint8_t> row;
        for (int panel = 0; panel < 3; panel++) {
            for (int x = 0; x < PictureWidth; x++) {
                const size_t offset = (static_cast<size_t>(y) * PictureWidth + x) * BytesPerPixel;
                uint8_t rgb[3];
                for (int channel = 0; channel < 3; channel++) {
                    if (panel == 0) {
                        rgb[channel] = original[offset + channel];
                    } else if (panel == 1) {
                        rgb[channel] = native[offset + channel];
                    } else {
                        const int difference = abs(original[offset + channel] - native[offset + channel]);
                        rgb[channel] = static_cast<uint8_t>(difference);
                    }
                }
                row.push_back(rgb[2]);
                row.push_back(rgb[1]);
                row.push_back(rgb[0]);
            }
        }
        while (static_cast<int>(row.size()) < rowBytes) {
            row.push_back(0);
        }
        file.insert(file.end(), row.begin(), row.end());
    }
    const std::filesystem::path path = std::filesystem::path(SaveFolder) / (name + ".bmp");
    FILE* stream = _wfopen(path.c_str(), L"wb");
    if (stream != nullptr) {
        fwrite(file.data(), 1, file.size(), stream);
        fclose(stream);
    }
}

/** Share of pixels whose colour differs by more than PixelTolerance; `lit` counts non-black pixels. */
double DifferingShare(const std::vector<uint8_t>& original, const std::vector<uint8_t>& native, int& lit)
{
    int differing = 0;
    lit = 0;
    const int pixelCount = PictureWidth * PictureHeight;
    for (int pixel = 0; pixel < pixelCount; pixel++) {
        int largest = 0;
        bool anyLit = false;
        for (int channel = 0; channel < 3; channel++) {
            const size_t offset = static_cast<size_t>(pixel) * BytesPerPixel + channel;
            const int difference = abs(original[offset] - native[offset]);
            if (difference > largest) {
                largest = difference;
            }
            if (original[offset] != 0 || native[offset] != 0) {
                anyLit = true;
            }
        }
        if (largest > PixelTolerance) {
            differing++;
        }
        if (anyLit) {
            lit++;
        }
    }
    return static_cast<double>(differing) / pixelCount;
}

/** Name of a root's pictures: file and root index. */
std::string RootName(const std::string& file, size_t rootIndex)
{
    return file + "_root" + std::to_string(rootIndex);
}

/**
 * Flicker check of a World: StabilitySteps frames, the camera turned by StabilityTurnDegrees each
 * time (far less than a pixel at the centre). For each backend the share of pixels that change
 * between consecutive frames is summed; a backend whose picture jumps where the other's stays put
 * flickers (draw order or depth fights that depend on what the frustum lets through).
 */
void MeasureStability(const std::string& name, Handle originalWorld, Handle nativeWorld, Handle originalCamera,
                      Handle nativeCamera)
{
    std::vector<uint8_t> previousOriginal = Draw(renderers.originalGraphics, originalWorld);
    std::vector<uint8_t> previousNative = Draw(renderers.nativeGraphics, nativeWorld);
    double originalChange = 0.0;
    double nativeChange = 0.0;
    for (int step = 0; step < StabilitySteps; step++) {
        const float turn[4] = { StabilityTurnDegrees, 0.0f, 1.0f, 0.0f };
        Call<int>(originalCamera, slot::transformable::postRotate, turn);
        Call<int>(nativeCamera, slot::transformable::postRotate, turn);
        const std::vector<uint8_t> nativePicture = Draw(renderers.nativeGraphics, nativeWorld);
        const std::vector<uint8_t> originalPicture = Draw(renderers.originalGraphics, originalWorld);
        int lit = 0;
        originalChange += DifferingShare(previousOriginal, originalPicture, lit);
        nativeChange += DifferingShare(previousNative, nativePicture, lit);
        previousOriginal = originalPicture;
        previousNative = nativePicture;
    }
    fprintf(stderr, "Render test: %s stability, pixels changing per step: original %.3f%%, native %.3f%%\n",
            name.c_str(), originalChange * 100.0 / StabilitySteps, nativeChange * 100.0 / StabilitySteps);
    EXPECT_LE(nativeChange, originalChange * 2.0 + StabilitySlack * StabilitySteps)
        << name << ": the native picture changes far more than the original while the camera barely moves";
}

class M3gRender : public testing::TestWithParam<M3gFile> {
protected:
    static void SetUpTestSuite()
    {
        if (renderers.ready) {
            return;
        }
        GameHarness* harness = GameHarness::Get();
        if (harness == nullptr) {
            return;
        }
        harness->RunFrames(3);
        renderers.original = OriginalRuntime();
        renderers.native = NativeRuntime();
        if (renderers.original == nullptr || renderers.native == nullptr || !CreateTarget()) {
            return;
        }
        renderers.nativeGraphics = CreateObject(renderers.native, ClassGraphics3D);
        renderers.originalGraphics = CreateObject(renderers.original, ClassGraphics3D);
        renderers.ready = renderers.nativeGraphics != nullptr && renderers.originalGraphics != nullptr;
    }
};

TEST_P(M3gRender, SamePicture)
{
    ASSERT_TRUE(renderers.ready) << "both Swerve backends and an offscreen framebuffer are needed";
    const std::string file = FileName(GetParam());
    const std::vector<uint8_t> data = ReadM3gFile(GetParam());
    ASSERT_TRUE(IsM3gData(data));
    std::vector<Handle> originalRoots;
    std::vector<Handle> nativeRoots;
    ASSERT_EQ(LoadWith(renderers.original, data, originalRoots), 0) << file;
    ASSERT_EQ(LoadWith(renderers.native, data, nativeRoots), 0) << file;
    ASSERT_EQ(originalRoots.size(), nativeRoots.size()) << file;
    static const bool saveAll = SDL_getenv(SaveAllVariable) != nullptr;

    for (size_t rootIndex = 0; rootIndex < nativeRoots.size(); rootIndex++) {
        // Framing from the native model (both backends load the same geometry, M3gBackendTest).
        Framing framing = {};
        float low[3] = {};
        float high[3] = {};
        bool found = false;
        Handle transform = CreateObject(renderers.native, ClassTransform);
        MeasureModel(nativeRoots[rootIndex], transform, nativeRoots[rootIndex], low, high, found);
        Release(transform);
        float radius = 0.0f;
        for (int axis = 0; axis < 3; axis++) {
            framing.center[axis] = (low[axis] + high[axis]) * 0.5f;
            radius += (high[axis] - low[axis]) * (high[axis] - low[axis]);
        }
        framing.radius = sqrtf(radius) * 0.5f;
        const bool isWorld = Supports(nativeRoots[rootIndex], ClassWorld);
        if (!isWorld && (!found || framing.radius <= 0.0f)) {
            fprintf(stderr, "Render test: %s root %zu has no geometry, skipped\n", file.c_str(), rootIndex);
            continue;
        }
        Handle originalCamera = nullptr;
        Handle nativeCamera = nullptr;
        Handle originalWorld = MakeScene(renderers.original, originalRoots[rootIndex], framing, originalCamera);
        Handle nativeWorld = MakeScene(renderers.native, nativeRoots[rootIndex], framing, nativeCamera);
        if (originalWorld == nullptr || nativeWorld == nullptr) {
            fprintf(stderr, "Render test: %s root %zu cannot be drawn (no camera or not a node), skipped\n",
                    file.c_str(), rootIndex);
            continue;
        }
        int validity = 0;
        Call<int>(originalWorld, slot::object3d::animate, DrawTime, &validity);
        Call<int>(nativeWorld, slot::object3d::animate, DrawTime, &validity);
        for (int pose = 0; pose < PoseCount; pose++) {
            if (pose > 0) {
                const float turn[4] = { PoseTurnDegrees, 0.0f, 1.0f, 0.0f };
                Call<int>(originalCamera, slot::transformable::postRotate, turn);
                Call<int>(nativeCamera, slot::transformable::postRotate, turn);
            }
            // Native first: the original engine leaves its GL state behind, the native one restores it.
            const std::vector<uint8_t> nativePicture = Draw(renderers.nativeGraphics, nativeWorld);
            const std::vector<uint8_t> originalPicture = Draw(renderers.originalGraphics, originalWorld);
            int lit = 0;
            const double share = DifferingShare(originalPicture, nativePicture, lit);
            const std::string name = file + "_root" + std::to_string(rootIndex) + "_pose" + std::to_string(pose);
            fprintf(stderr, "Render test: %s %s, %.2f%% pixels differ, %d lit\n", name.c_str(),
                    isWorld ? "world" : "model", share * 100.0, lit);
            if (saveAll || share > SavedShare) {
                SavePictures(name, originalPicture, nativePicture);
            }
            EXPECT_LE(share, FailingShare) << name << ": " << share * 100.0 << "% of the pixels differ (see "
                                           << "render_compare\\" << name << ".bmp)";
        }
        if (isWorld) {
            MeasureStability(RootName(file, rootIndex), originalWorld, nativeWorld, originalCamera, nativeCamera);
        }
        DropScene(originalWorld, originalRoots[rootIndex], originalCamera);
        DropScene(nativeWorld, nativeRoots[rootIndex], nativeCamera);
    }
    ReleaseRoots(originalRoots);
    ReleaseRoots(nativeRoots);
}

/** Every Mesh / Sprite3D under `node` (new references), in traversal order. */
void CollectDrawables(Handle node, std::vector<Handle>& drawables)
{
    if (Supports(node, ClassMesh) || Supports(node, ClassSprite3D)) {
        Call<void>(node, slot::AddRef);
        drawables.push_back(node);
    }
    if (Supports(node, ClassGroup)) {
        int childCount = 0;
        Call<int>(node, slot::group::getChildCount, &childCount);
        for (int index = 0; index < childCount; index++) {
            Handle child = nullptr;
            Call<int>(node, slot::group::getChild, index, &child);
            if (child != nullptr) {
                CollectDrawables(child, drawables);
                Release(child);
            }
        }
    }
}

void SetRendering(Handle node, bool enable)
{
    Call<int>(node, slot::node::setRenderingEnable, enable);
}

/** Effective alpha factor of `node` (its own times its parents'). */
float EffectiveAlpha(Handle node)
{
    float alpha = 1.0f;
    Handle current = node;
    Call<void>(current, slot::AddRef);
    while (current != nullptr) {
        float own = 1.0f;
        Call<int>(current, slot::node::getAlphaFactor, &own);
        alpha *= own;
        Handle parent = nullptr;
        Call<int>(current, slot::node::getParent, &parent);
        Release(current);
        current = parent;
    }
    return alpha;
}

/** Facts of one appearance (native object). */
std::string DescribeOneAppearance(Handle appearance)
{
    if (appearance == nullptr) {
        return "no appearance";
    }
    M3GAppearance handle = reinterpret_cast<M3GAppearance>(ck::native::HandleOf(appearance));
    char text[256];
    int blending = -1;
    float threshold = -1.0f;
    int depthTest = -1;
    int depthWrite = -1;
    M3GCompositingMode mode = m3gGetCompositingMode(handle);
    if (mode != nullptr) {
        blending = m3gGetBlending(mode);
        threshold = m3gGetAlphaThreshold(mode);
        depthTest = m3gIsDepthTestEnabled(mode);
        depthWrite = m3gIsDepthWriteEnabled(mode);
    }
    int imageFormat = -1;
    M3GTexture texture = m3gGetTexture(handle, 0);
    if (texture != nullptr) {
        imageFormat = m3gGetFormat(m3gGetTextureImage(texture));
    }
    int culling = -1;
    int shading = -1;
    int twoSided = -1;
    int localCamera = -1;
    M3GPolygonMode polygonMode = m3gGetPolygonMode(handle);
    if (polygonMode != nullptr) {
        culling = m3gGetCulling(polygonMode);
        shading = m3gGetShading(polygonMode);
        twoSided = m3gIsTwoSidedLightingEnabled(polygonMode);
        localCamera = m3gIsLocalCameraLightingEnabled(polygonMode);
    }
    int colorTracking = -1;
    M3GMaterial material = m3gGetMaterial(handle);
    if (material != nullptr) {
        colorTracking = m3gIsVertexColorTrackingEnabled(material);
    }
    snprintf(text, sizeof(text),
             "layer %d blending %d threshold %g depthTest %d depthWrite %d image 0x%x culling %d shading %d twoSided %d"
             " localCamera %d material %d (tracking %d)",
             m3gGetLayer(handle), blending, threshold, depthTest, depthWrite, imageFormat, culling, shading, twoSided,
             localCamera, material != nullptr, colorTracking);
    return text;
}

/** Appearance facts of every submesh of a mesh, or of a sprite (native object). */
std::string DescribeAppearance(Handle mesh)
{
    if (!Supports(mesh, ClassMesh)) {
        Handle appearance = nullptr;
        Call<int>(mesh, slot::sprite3d::getAppearance, &appearance);
        std::string text = "sprite: " + DescribeOneAppearance(appearance);
        if (appearance != nullptr) {
            Release(appearance);
        }
        return text;
    }
    int submeshCount = 0;
    Call<int>(mesh, slot::mesh::getSubmeshCount, &submeshCount);
    std::string text;
    for (int submesh = 0; submesh < submeshCount; submesh++) {
        Handle appearance = nullptr;
        Call<int>(mesh, slot::mesh::getAppearance, submesh, &appearance);
        text += "\n      submesh " + std::to_string(submesh) + ": " + DescribeOneAppearance(appearance);
        if (appearance != nullptr) {
            Release(appearance);
        }
    }
    return text;
}

/** One mesh drawn alone: share of differing pixels, its index and lit pixels. */
struct IsolateResult {
    double share;
    size_t index;
    int lit;
};

bool DiffersMore(const IsolateResult& left, const IsolateResult& right)
{
    return left.share > right.share;
}

/**
 * Diagnostics (CK_RENDER_ISOLATE=<file name, e.g. vga_m3g_06ff4eb4>): every mesh of the file's World
 * drawn alone with both backends; the meshes whose pictures differ most are listed with their
 * appearance, to find which object a difference of the whole picture comes from.
 */
TEST(M3gRenderIsolate, DifferingMeshes)
{
    const char* wanted = SDL_getenv("CK_RENDER_ISOLATE");
    if (wanted == nullptr) {
        GTEST_SKIP() << "set CK_RENDER_ISOLATE=<file name>";
    }
    GameHarness* harness = GameHarness::Get();
    ASSERT_NE(harness, nullptr);
    harness->RunFrames(3);
    ISwerve* original = OriginalRuntime();
    ISwerve* native = NativeRuntime();
    ASSERT_TRUE(original != nullptr && native != nullptr && CreateTarget());
    Handle nativeGraphics = CreateObject(native, ClassGraphics3D);
    Handle originalGraphics = CreateObject(original, ClassGraphics3D);
    const M3gFile* file = nullptr;
    static std::vector<M3gFile> files = M3gFiles();
    for (const M3gFile& candidate : files) {
        if (FileName(candidate) == wanted) {
            file = &candidate;
        }
    }
    ASSERT_NE(file, nullptr) << wanted;
    const std::vector<uint8_t> data = ReadM3gFile(*file);
    std::vector<Handle> originalRoots;
    std::vector<Handle> nativeRoots;
    ASSERT_EQ(LoadWith(original, data, originalRoots), 0);
    ASSERT_EQ(LoadWith(native, data, nativeRoots), 0);
    ASSERT_TRUE(Supports(nativeRoots[0], ClassWorld));
    int validity = 0;
    Call<int>(originalRoots[0], slot::object3d::animate, DrawTime, &validity);
    Call<int>(nativeRoots[0], slot::object3d::animate, DrawTime, &validity);
    std::vector<Handle> originalMeshes;
    std::vector<Handle> nativeMeshes;
    CollectDrawables(originalRoots[0], originalMeshes);
    CollectDrawables(nativeRoots[0], nativeMeshes);
    ASSERT_EQ(originalMeshes.size(), nativeMeshes.size());
    // Remember the switches, then draw one mesh at a time.
    std::vector<int> enabled(nativeMeshes.size());
    for (size_t index = 0; index < nativeMeshes.size(); index++) {
        int value = 0;
        Call<int>(nativeMeshes[index], slot::node::isRenderingEnabled, reinterpret_cast<unsigned char*>(&value));
        enabled[index] = value & 0xff;
        SetRendering(originalMeshes[index], false);
        SetRendering(nativeMeshes[index], false);
    }
    std::vector<IsolateResult> results;
    for (size_t index = 0; index < nativeMeshes.size(); index++) {
        if (enabled[index] == 0) {
            continue;
        }
        SetRendering(originalMeshes[index], true);
        SetRendering(nativeMeshes[index], true);
        const std::vector<uint8_t> nativePicture = Draw(nativeGraphics, nativeRoots[0]);
        const std::vector<uint8_t> originalPicture = Draw(originalGraphics, originalRoots[0]);
        SetRendering(originalMeshes[index], false);
        SetRendering(nativeMeshes[index], false);
        int lit = 0;
        const double share = DifferingShare(originalPicture, nativePicture, lit);
        if (share > 0.0) {
            results.push_back({ share, index, lit });
            if (results.size() <= 40) {
                SavePictures(std::string(wanted) + "_mesh" + std::to_string(index), originalPicture, nativePicture);
            }
        }
    }
    std::sort(results.begin(), results.end(), DiffersMore);
    fprintf(stderr, "Render isolate: %s, %zu drawables, %zu differ alone\n", wanted, nativeMeshes.size(), results.size());
    for (size_t rank = 0; rank < results.size() && rank < 25; rank++) {
        const IsolateResult& result = results[rank];
        Handle mesh = nativeMeshes[result.index];
        int userId = 0;
        Call<int>(mesh, slot::object3d::getUserID, &userId);
        fprintf(stderr, "  mesh %zu id %d: %.2f%% differ, %d lit, alpha %g, %s\n", result.index, userId,
                result.share * 100.0, result.lit, EffectiveAlpha(mesh), DescribeAppearance(mesh).c_str());
    }
    for (Handle mesh : originalMeshes) {
        Release(mesh);
    }
    for (Handle mesh : nativeMeshes) {
        Release(mesh);
    }
    ReleaseRoots(originalRoots);
    ReleaseRoots(nativeRoots);
}

/**
 * Diagnostics (CK_RENDER_ACCUMULATE=<file name>): the meshes of the file's World switched on one by
 * one in traversal order, both backends drawn after each step. A step whose mesh looks the same alone
 * but raises the difference of the whole picture points at GL state one draw leaves to the next.
 */
TEST(M3gRenderIsolate, AccumulatingMeshes)
{
    const char* wanted = SDL_getenv("CK_RENDER_ACCUMULATE");
    if (wanted == nullptr) {
        GTEST_SKIP() << "set CK_RENDER_ACCUMULATE=<file name>";
    }
    GameHarness* harness = GameHarness::Get();
    ASSERT_NE(harness, nullptr);
    harness->RunFrames(3);
    ISwerve* original = OriginalRuntime();
    ISwerve* native = NativeRuntime();
    ASSERT_TRUE(original != nullptr && native != nullptr && CreateTarget());
    Handle nativeGraphics = CreateObject(native, ClassGraphics3D);
    Handle originalGraphics = CreateObject(original, ClassGraphics3D);
    const M3gFile* file = nullptr;
    static std::vector<M3gFile> files = M3gFiles();
    for (const M3gFile& candidate : files) {
        if (FileName(candidate) == wanted) {
            file = &candidate;
        }
    }
    ASSERT_NE(file, nullptr) << wanted;
    const std::vector<uint8_t> data = ReadM3gFile(*file);
    std::vector<Handle> originalRoots;
    std::vector<Handle> nativeRoots;
    ASSERT_EQ(LoadWith(original, data, originalRoots), 0);
    ASSERT_EQ(LoadWith(native, data, nativeRoots), 0);
    int validity = 0;
    Call<int>(originalRoots[0], slot::object3d::animate, DrawTime, &validity);
    Call<int>(nativeRoots[0], slot::object3d::animate, DrawTime, &validity);
    std::vector<Handle> originalMeshes;
    std::vector<Handle> nativeMeshes;
    CollectDrawables(originalRoots[0], originalMeshes);
    CollectDrawables(nativeRoots[0], nativeMeshes);
    ASSERT_EQ(originalMeshes.size(), nativeMeshes.size());
    std::vector<int> enabled(nativeMeshes.size());
    for (size_t index = 0; index < nativeMeshes.size(); index++) {
        int value = 0;
        Call<int>(nativeMeshes[index], slot::node::isRenderingEnabled, reinterpret_cast<unsigned char*>(&value));
        enabled[index] = value & 0xff;
        SetRendering(originalMeshes[index], false);
        SetRendering(nativeMeshes[index], false);
    }
    double previousShare = 0.0;
    for (size_t index = 0; index < nativeMeshes.size(); index++) {
        if (enabled[index] == 0) {
            continue;
        }
        SetRendering(originalMeshes[index], true);
        SetRendering(nativeMeshes[index], true);
        const std::vector<uint8_t> nativePicture = Draw(nativeGraphics, nativeRoots[0]);
        const std::vector<uint8_t> originalPicture = Draw(originalGraphics, originalRoots[0]);
        int lit = 0;
        const double share = DifferingShare(originalPicture, nativePicture, lit);
        static const bool listAll = SDL_getenv("CK_RENDER_LIST") != nullptr;
        if (listAll) {
            int userId = 0;
            Call<int>(nativeMeshes[index], slot::object3d::getUserID, &userId);
            fprintf(stderr, "Render list: mesh %zu id %d: %.3f%%, %s\n", index, userId, share * 100.0,
                    DescribeAppearance(nativeMeshes[index]).c_str());
        }
        if (share - previousShare > 0.0005) {
            int userId = 0;
            Call<int>(nativeMeshes[index], slot::object3d::getUserID, &userId);
            fprintf(stderr, "Render accumulate: step mesh %zu id %d: %.3f%% -> %.3f%%, %s\n", index, userId,
                    previousShare * 100.0, share * 100.0, DescribeAppearance(nativeMeshes[index]).c_str());
            SavePictures(std::string(wanted) + "_step" + std::to_string(index), originalPicture, nativePicture);
        }
        previousShare = share;
    }
    fprintf(stderr, "Render accumulate: %s done, final %.3f%%\n", wanted, previousShare * 100.0);
    for (Handle mesh : originalMeshes) {
        Release(mesh);
    }
    for (Handle mesh : nativeMeshes) {
        Release(mesh);
    }
    ReleaseRoots(originalRoots);
    ReleaseRoots(nativeRoots);
}

std::string ParamName(const testing::TestParamInfo<M3gFile>& info)
{
    return FileName(info.param);
}

INSTANTIATE_TEST_SUITE_P(CK, M3gRender, testing::ValuesIn(M3gFiles()), ParamName);

} // namespace
} // namespace ck::test
