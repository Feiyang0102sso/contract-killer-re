// Native Swerve renderer (IGraphics3D of SwerveApi.h plus the IGraphics3DRenderer slots of tws.h).
//
// CK's CSwerveGraphics3D::BindTarget binds the game's render surface itself (an FBO or the window),
// then calls graphics3d_setCroppedBackBufferData with the surface size and the viewport; its
// ReleaseTarget never tells the engine. So the renderer only records the target here, and every
// render call binds m3gcore to the current framebuffer, draws and releases it again.
//
// The game's 2D renderer shares the GL context. m3gcore sets every state it needs for each draw but
// leaves its state behind, so the game's GL state is saved before and restored after each render.
#include "NativeRuntime.h"
#include "CkEgl.h"
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include <stdio.h>

extern "C" {
// Desktop OpenGL 1.1 calls (opengl32) outside the GLES headers.
void GL_APIENTRY glPushAttrib(GLbitfield mask);
void GL_APIENTRY glPopAttrib(void);
void GL_APIENTRY glPushClientAttrib(GLbitfield mask);
void GL_APIENTRY glPopClientAttrib(void);
}

namespace ck::native {
namespace {

constexpr GLbitfield AllAttributeBits = 0xffffffff;
constexpr GLenum ArrayBufferBinding = 0x8894;
constexpr GLenum ElementArrayBufferBinding = 0x8895;
constexpr GLenum VertexArrayBinding = 0x85b5;
constexpr GLenum TextureMatrixMode = 0x1702;
// m3gcore uses two texture units (M3G_NUM_TEXTURE_UNITS).
constexpr int SavedTextureUnits = 2;

/** Viewport rectangle in target coordinates (origin top left, as M3G and the game use it). */
struct Viewport {
    int x;
    int y;
    int width;
    int height;
};

struct Graphics3DObject : Object {
    M3GRenderContext context;
    EGLSurface surface;
    // Target recorded by setCroppedBackBufferData; 0 x 0 before the first one.
    int targetWidth;
    int targetHeight;
    Viewport viewport;
    int rotation;
    // GL state of the game, restored after each render
    GLint savedArrayBuffer;
    GLint savedElementBuffer;
    GLint savedVertexArray;
    GLint savedActiveTexture;
    GLint savedClientActiveTexture;
};

Graphics3DObject* AsGraphics(Object* object)
{
    return static_cast<Graphics3DObject*>(object);
}

void DestroyGraphics(Object* object)
{
    Graphics3DObject* graphics = AsGraphics(object);
    if (graphics->context != nullptr) {
        m3gDeleteObject(reinterpret_cast<M3GObject>(graphics->context));
    }
    if (graphics->surface != EGL_NO_SURFACE) {
        egl::DestroySurface(graphics->surface);
    }
    delete graphics;
}

/** Save the game's GL state and give m3gcore a clean one (no buffer objects, its own defaults). */
void SaveGameState(Graphics3DObject* graphics)
{
    glGetIntegerv(ArrayBufferBinding, &graphics->savedArrayBuffer);
    glGetIntegerv(ElementArrayBufferBinding, &graphics->savedElementBuffer);
    glGetIntegerv(VertexArrayBinding, &graphics->savedVertexArray);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &graphics->savedActiveTexture);
    glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &graphics->savedClientActiveTexture);
    glPushAttrib(AllAttributeBits);
    glPushClientAttrib(AllAttributeBits);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    for (int unit = 0; unit < SavedTextureUnits; unit++) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glMatrixMode(TextureMatrixMode);
        glPushMatrix();
    }
    glActiveTexture(GL_TEXTURE0);
    glMatrixMode(GL_MODELVIEW);

    // m3gcore draws from client memory, never from buffer objects.
    glBindVertexArrayOES(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // m3gcore's m3gSetGLDefaults, which it only runs once when it believes it made a new context.
    static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glEnable(GL_NORMALIZE);
    glEnable(GL_SCISSOR_TEST);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, black);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // A Background clears depth with whatever clear value is set; m3gcore only sets it without one.
    glClearDepthf(1.0f);
}

void RestoreGameState(Graphics3DObject* graphics)
{
    for (int unit = SavedTextureUnits - 1; unit >= 0; unit--) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glMatrixMode(TextureMatrixMode);
        glPopMatrix();
    }
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopClientAttrib();
    glPopAttrib();
    glActiveTexture(static_cast<GLenum>(graphics->savedActiveTexture));
    glClientActiveTexture(static_cast<GLenum>(graphics->savedClientActiveTexture));
    glBindVertexArrayOES(static_cast<GLuint>(graphics->savedVertexArray));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(graphics->savedArrayBuffer));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(graphics->savedElementBuffer));
}

/** Bind m3gcore to the current framebuffer with the recorded viewport; false when nothing is bound. */
bool BeginRender(Graphics3DObject* graphics)
{
    if (graphics->targetWidth <= 0 || graphics->targetHeight <= 0) {
        return false;
    }
    SaveGameState(graphics);
    TakeError();
    m3gBindEGLSurfaceTarget(graphics->context, static_cast<M3GEGLSurface>(reinterpret_cast<uintptr_t>(graphics->surface)));
    if (TakeError() != ErrorNone) {
        RestoreGameState(graphics);
        return false;
    }
    const Viewport& viewport = graphics->viewport;
    m3gSetViewport(graphics->context, viewport.x, viewport.y, viewport.width, viewport.height);
    return true;
}

void EndRender(Graphics3DObject* graphics)
{
    m3gReleaseTarget(graphics->context);
    RestoreGameState(graphics);
    TakeError();
}

/**
 * graphics3d_setCroppedBackBufferData(width, height, pixels, pitch, x, y, cropWidth, cropHeight):
 * CK passes no pixels, meaning the framebuffer the game has bound; x, y, cropWidth, cropHeight is
 * the viewport.
 */
int SetCroppedBackBufferData(Object* self, unsigned int targetWidth, unsigned int targetHeight, int pixels, int pitch,
                             int x, int y, int width, int height)
{
    Graphics3DObject* graphics = AsGraphics(self);
    static bool logged = false;
    if (!logged) {
        logged = true;
        fprintf(stderr, "Swerve native: first target %ux%u viewport %d,%d %dx%d (pixels %d, pitch %d)\n",
                targetWidth, targetHeight, x, y, width, height, pixels, pitch);
    }
    if (pixels != 0) {
        fprintf(stderr, "Swerve native: back buffer pixels are not supported\n");
        return ErrorInvalidParameter;
    }
    if (graphics->surface == EGL_NO_SURFACE) {
        graphics->surface = egl::CreateSurface(static_cast<EGLint>(targetWidth), static_cast<EGLint>(targetHeight));
    } else {
        egl::ResizeSurface(graphics->surface, static_cast<EGLint>(targetWidth), static_cast<EGLint>(targetHeight));
    }
    graphics->targetWidth = static_cast<int>(targetWidth);
    graphics->targetHeight = static_cast<int>(targetHeight);
    graphics->viewport.x = x;
    graphics->viewport.y = y;
    graphics->viewport.width = width;
    graphics->viewport.height = height;
    return ErrorNone;
}

/** graphics3d_setBackBufferImage2D: image targets; CK only ever unbinds (NULL), see CSwerveGraphics3D::ReleaseTarget. */
int SetBackBufferImage2D(Object*, Object* image)
{
    if (image != nullptr) {
        fprintf(stderr, "Swerve native: Image2D render targets are not supported\n");
        return ErrorClassNotSupported;
    }
    return ErrorNone;
}

/** graphics3d_setHints: CSwerveGraphics3D::BindTarget only calls it for clear = 0 or flags != 0. */
int SetHints(Object*, int clear, int flags)
{
    static bool logged = false;
    if (!logged) {
        logged = true;
        fprintf(stderr, "Swerve native: setHints(%d, 0x%x) ignored\n", clear, flags);
    }
    return ErrorNone;
}

int SetViewport(Object* self, int x, int y, int width, int height)
{
    Graphics3DObject* graphics = AsGraphics(self);
    graphics->viewport.x = x;
    graphics->viewport.y = y;
    graphics->viewport.width = width;
    graphics->viewport.height = height;
    return ErrorNone;
}

/** Render `node` with `transform` using the camera and lights of the last rendered world. */
void RenderNode(Object* self, Object* node, Object* transform)
{
    Graphics3DObject* graphics = AsGraphics(self);
    if (node == nullptr || !BeginRender(graphics)) {
        return;
    }
    m3gRenderNode(graphics->context, reinterpret_cast<M3GNode>(node->handle), MatrixOf(transform));
    EndRender(graphics);
}

void RenderWorld(Object* self, Object* world)
{
    Graphics3DObject* graphics = AsGraphics(self);
    if (world == nullptr || !BeginRender(graphics)) {
        return;
    }
    m3gRenderWorld(graphics->context, reinterpret_cast<M3GWorld>(world->handle));
    EndRender(graphics);
}

/** graphics3d_setOrientation: only an upright display is supported (the PC port never rotates). */
int SetOrientation(Object* self, int degrees)
{
    Graphics3DObject* graphics = AsGraphics(self);
    if (degrees != 0 && degrees != graphics->rotation) {
        fprintf(stderr, "Swerve native: display rotation %d is not supported\n", degrees);
    }
    graphics->rotation = degrees;
    return ErrorNone;
}

} // namespace

Object* CreateGraphics3D()
{
    Graphics3DObject* graphics = new Graphics3DObject();
    InitObject(graphics, ClassGraphics3D);
    graphics->context = m3gCreateContext(Interface());
    graphics->surface = EGL_NO_SURFACE;
    graphics->targetWidth = 0;
    graphics->targetHeight = 0;
    graphics->viewport = Viewport{ 0, 0, 0, 0 };
    graphics->rotation = 0;
    if (graphics->context == nullptr) {
        fprintf(stderr, "Swerve native: m3gCreateContext failed\n");
    }
    return graphics;
}

void FillGraphics3DSlots(ObjectClass& objectClass)
{
    FillQueryInterfaceSlots(objectClass);
    objectClass.destroy = DestroyGraphics;
    SetSlot(objectClass, slot::graphics3d::setCroppedBackBufferData, &SetCroppedBackBufferData);
    SetSlot(objectClass, slot::graphics3d::setBackBufferImage2D, &SetBackBufferImage2D);
    SetSlot(objectClass, slot::graphics3d::setHints, &SetHints);
    SetSlot(objectClass, slot::graphics3d::setViewport, &SetViewport);
    SetSlot(objectClass, slot::graphics3d::renderNode, &RenderNode);
    SetSlot(objectClass, slot::graphics3d::renderWorld, &RenderWorld);
    SetSlot(objectClass, slot::graphics3d::setOrientation, &SetOrientation);
}

} // namespace ck::native
