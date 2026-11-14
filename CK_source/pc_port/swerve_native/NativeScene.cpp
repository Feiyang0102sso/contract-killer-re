// Native Swerve scene graph slots: Object3D, Transformable, Node, Group, World, Mesh, SkinnedMesh,
// Camera, Light, Sprite3D, Appearance, Texture2D, Background, CompositingMode, Material,
// VertexBuffer, VertexArray, IndexBuffer and the animation classes.
// Each slot is filled by the original function it stands for (SwerveSlots.h); the signatures follow
// the game's calls (tws.h wrappers and the CKMethod call sites in CK_source/src).
#include "NativeRuntime.h"
#include "M3gCkExtensions.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <vector>

namespace ck::native {
namespace {

/** The m3gcore handle of `object` as the m3gcore type the call expects. */
template<class Handle>
Handle H(const void* object)
{
    return reinterpret_cast<Handle>(HandleOf(object));
}

bool IsNodeClass(ClassId id)
{
    switch (id) {
    case ClassCamera:
    case ClassGroup:
    case ClassWorld:
    case ClassLight:
    case ClassMesh:
    case ClassMorphingMesh:
    case ClassSkinnedMesh:
    case ClassSprite3D:
        return true;
    default:
        return false;
    }
}

// ================= Object3D =================

int GetUserID(Object* self, int* userID)
{
    *userID = m3gGetUserID(self->handle);
    return TakeError();
}

int SetUserID(Object* self, int userID)
{
    m3gSetUserID(self->handle, userID);
    return TakeError();
}

int GetAnimationTrackCount(Object* self, int* count)
{
    *count = m3gGetAnimationTrackCount(self->handle);
    return TakeError();
}

int GetAnimationTrack(Object* self, int index, void** track)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetAnimationTrack(self->handle, index)), track);
    return TakeError();
}

int AddAnimationTrack(Object* self, Object* track)
{
    m3gAddAnimationTrack(self->handle, H<M3GAnimationTrack>(track));
    return TakeError();
}

int RemoveAnimationTrack(Object* self, Object* track)
{
    m3gRemoveAnimationTrack(self->handle, H<M3GAnimationTrack>(track));
    return TakeError();
}

int Animate(Object* self, int worldTime, int* validity)
{
    const M3Gint result = m3gAnimate(self->handle, worldTime);
    if (validity != nullptr) {
        *validity = result;
    }
    return TakeError();
}

int Find(Object* self, int userID, void** found)
{
    WrapInto(m3gFind(self->handle, userID), found);
    return TakeError();
}

/** The objects this one references; the first `capacity` go to `references` (new references). */
int GetReferences(Object* self, unsigned int capacity, void** references, int* count)
{
    const M3Gint total = m3gGetReferences(self->handle, nullptr, 0);
    if (count != nullptr) {
        *count = total;
    }
    if (references == nullptr || capacity == 0) {
        return TakeError();
    }
    std::vector<M3Gulong> handles(static_cast<size_t>(total) + 1);
    m3gGetReferences(self->handle, handles.data(), total);
    for (unsigned int index = 0; index < capacity; index++) {
        if (static_cast<int>(index) < total) {
            WrapInto(reinterpret_cast<M3GObject>(static_cast<uintptr_t>(handles[index])), &references[index]);
        } else {
            references[index] = nullptr;
        }
    }
    return TakeError();
}

/** User parameter `id` from the M3G file (kept by the loader, see NativeLoader.cpp). */
int GetUserParameter(Object* self, int id, int bufferSize, char* buffer, int* length)
{
    const int found = FindUserParameter(self->handle, id, bufferSize, buffer);
    if (found < 0) {
        if (length != nullptr) {
            *length = 0;
        }
        return ErrorInvalidParameter;
    }
    if (length != nullptr) {
        *length = found;
    }
    return ErrorNone;
}

int Duplicate(Object* self, void** copy)
{
    // m3gcore records (original, clone) pairs of every duplicated node to fix up alignment references.
    int referenceSlots = 2;
    if (IsNodeClass(self->objectClass->id)) {
        referenceSlots = 2 * m3gGetSubtreeSize(H<M3GNode>(self)) + 2;
    }
    std::vector<M3Gulong> references(static_cast<size_t>(referenceSlots));
    const M3GObject clone = m3gDuplicate(self->handle, references.data());
    WrapInto(clone, copy);
    return TakeError();
}

// ================= Transformable =================

int GetTranslation(Object* self, float* translation)
{
    m3gGetTranslation(H<M3GTransformable>(self), translation);
    return TakeError();
}

int SetTranslation(Object* self, const float* translation)
{
    m3gSetTranslation(H<M3GTransformable>(self), translation[0], translation[1], translation[2]);
    return TakeError();
}

int GetScale(Object* self, float* scale)
{
    m3gGetScale(H<M3GTransformable>(self), scale);
    return TakeError();
}

int SetScale(Object* self, const float* scale)
{
    m3gSetScale(H<M3GTransformable>(self), scale[0], scale[1], scale[2]);
    return TakeError();
}

/** Orientation as angle (degrees) and axis x, y, z. */
int GetOrientation(Object* self, float* angleAxis)
{
    m3gGetOrientation(H<M3GTransformable>(self), angleAxis);
    return TakeError();
}

int SetOrientation(Object* self, const float* angleAxis)
{
    m3gSetOrientation(H<M3GTransformable>(self), angleAxis[0], angleAxis[1], angleAxis[2], angleAxis[3]);
    return TakeError();
}

int Translate(Object* self, const float* translation)
{
    m3gTranslate(H<M3GTransformable>(self), translation[0], translation[1], translation[2]);
    return TakeError();
}

int PostRotate(Object* self, const float* angleAxis)
{
    m3gPostRotate(H<M3GTransformable>(self), angleAxis[0], angleAxis[1], angleAxis[2], angleAxis[3]);
    return TakeError();
}

/**
 * transformable_getTransform: only the generic matrix (M3G getTransform). tws.h calls this slot
 * getCompositeTransform; the game reads it, changes it and writes it back with setTransform
 * (CDH_Weapon, dgCamera), so the composite would apply translation and orientation twice.
 */
int GetTransform(Object* self, Object* transform)
{
    m3gGetTransform(H<M3GTransformable>(self), MatrixOf(transform));
    return TakeError();
}

int GetCompositeTransform(Object* self, Object* transform)
{
    m3gGetCompositeTransform(H<M3GTransformable>(self), MatrixOf(transform));
    return TakeError();
}

/**
 * M3G setTransform: the generic matrix after translation, orientation and scale.
 * m3gcore demands a node transform whose bottom row is exactly (0, 0, 0, 1). The game passes
 * products of rotations whose bottom row is off by ~1e-9, which Swerve accepts, so a bottom row
 * within BottomRowTolerance is made exact first.
 */
int SetTransform(Object* self, Object* transform)
{
    constexpr float BottomRowTolerance = 1e-4f;
    M3GTransformable transformable = H<M3GTransformable>(self);
    if (transform == nullptr) {
        m3gSetTransform(transformable, nullptr);
        return TakeError();
    }
    float rows[16];
    m3gGetMatrixRows(MatrixOf(transform), rows);
    bool nearUnity = fabsf(rows[12]) < BottomRowTolerance && fabsf(rows[13]) < BottomRowTolerance &&
        fabsf(rows[14]) < BottomRowTolerance && fabsf(rows[15] - 1.0f) < BottomRowTolerance;
    M3GMatrix matrix;
    if (nearUnity) {
        rows[12] = 0.0f;
        rows[13] = 0.0f;
        rows[14] = 0.0f;
        rows[15] = 1.0f;
    }
    m3gSetMatrixRows(&matrix, rows);
    m3gSetTransform(transformable, &matrix);
    const int error = TakeError();
    static int logged = 0;
    if (error != ErrorNone && logged < 3) {
        logged++;
        fprintf(stderr, "Swerve native: setTransform rejected, bottom row %g %g %g %g\n", rows[12], rows[13], rows[14], rows[15]);
    }
    return error;
}

// ================= Node =================

int GetParent(Object* self, void** parent)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetParent(H<M3GNode>(self))), parent);
    return TakeError();
}

int GetAlphaFactor(Object* self, float* alpha)
{
    *alpha = m3gGetAlphaFactor(H<M3GNode>(self));
    return TakeError();
}

int SetAlphaFactor(Object* self, float alpha)
{
    m3gSetAlphaFactor(H<M3GNode>(self), alpha);
    return TakeError();
}

int IsRenderingEnabled(Object* self, unsigned char* enabled)
{
    *enabled = static_cast<unsigned char>(m3gIsEnabled(H<M3GNode>(self), M3G_SETGET_RENDERING) != 0);
    return TakeError();
}

int SetRenderingEnable(Object* self, bool enable)
{
    m3gEnable(H<M3GNode>(self), M3G_SETGET_RENDERING, enable);
    return TakeError();
}

int IsPickingEnabled(Object* self, unsigned char* enabled)
{
    *enabled = static_cast<unsigned char>(m3gIsEnabled(H<M3GNode>(self), M3G_SETGET_PICKING) != 0);
    return TakeError();
}

int SetPickingEnable(Object* self, bool enable)
{
    m3gEnable(H<M3GNode>(self), M3G_SETGET_PICKING, enable);
    return TakeError();
}

int GetScope(Object* self, int* scope)
{
    *scope = m3gGetScope(H<M3GNode>(self));
    return TakeError();
}

int SetScope(Object* self, int scope)
{
    m3gSetScope(H<M3GNode>(self), scope);
    return TakeError();
}

/** Transform from this node to `target` into `transform`; *result says whether a path exists. */
int GetTransformTo(Object* self, Object* target, Object* transform, bool* result)
{
    const M3Gbool found = m3gGetTransformTo(H<M3GNode>(self), H<M3GNode>(target), MatrixOf(transform));
    if (result != nullptr) {
        *result = found != 0;
    }
    return TakeError();
}

int SetAlignment(Object* self, Object* zReference, int zTarget, Object* yReference, int yTarget)
{
    m3gSetAlignment(H<M3GNode>(self), H<M3GNode>(zReference), zTarget, H<M3GNode>(yReference), yTarget);
    return TakeError();
}

int Align(Object* self, Object* reference)
{
    m3gAlignNode(H<M3GNode>(self), H<M3GNode>(reference));
    return TakeError();
}

// ================= Group, World =================

int GetChildCount(Object* self, int* count)
{
    *count = m3gGetChildCount(H<M3GGroup>(self));
    return TakeError();
}

int GetChild(Object* self, int index, void** child)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetChild(H<M3GGroup>(self), index)), child);
    return TakeError();
}

int AddChild(Object* self, Object* child)
{
    m3gAddChild(H<M3GGroup>(self), H<M3GNode>(child));
    return TakeError();
}

/** Swerve also removes nodes m3gcore flags as skeleton bones (duplicated heads, see M3gCkCore.c). */
int RemoveChild(Object* self, Object* child)
{
    ckM3gRemoveChild(H<M3GGroup>(self), H<M3GNode>(child));
    return TakeError();
}

// ---- picking ----
//
// m3gcore renders the skeleton of a SkinnedMesh but never picks it (m3gSkinnedMeshRayIntersect only
// tests the skinned triangles). Swerve picks it: hit zones hanging under bones must be hit. The pick
// slots therefore run m3gcore's pick and then pick every reachable skeleton themselves, keeping the
// nearest hit (found in CKZ, whose engine picks the same way).

constexpr int PickResultFloats = 15;
constexpr int PickRayFloats = 6;

/** Pick ray as two points (start x, y, z, end x, y, z) in the coordinates of the picked group. */
struct PickSegment {
    M3Gfloat points[PickRayFloats];
};

/** Collect the skeleton groups of the SkinnedMeshes under `node` reachable through pickable nodes. */
void CollectPickableSkeletons(M3GNode node, std::vector<M3GGroup>& skeletons)
{
    if (!m3gIsEnabled(node, M3G_SETGET_PICKING)) {
        return;
    }
    const M3GClass nodeClass = m3gGetClass(reinterpret_cast<M3GObject>(node));
    if (nodeClass == M3G_CLASS_SKINNED_MESH) {
        const M3GGroup skeleton = m3gGetSkeleton(reinterpret_cast<M3GSkinnedMesh>(node));
        if (skeleton != nullptr && m3gIsEnabled(reinterpret_cast<M3GNode>(skeleton), M3G_SETGET_PICKING)) {
            skeletons.push_back(skeleton);
            // A skeleton may hold SkinnedMeshes of its own.
            CollectPickableSkeletons(reinterpret_cast<M3GNode>(skeleton), skeletons);
        }
        return;
    }
    if (nodeClass != M3G_CLASS_GROUP && nodeClass != M3G_CLASS_WORLD) {
        return;
    }
    const M3GGroup group = reinterpret_cast<M3GGroup>(node);
    const M3Gint childCount = m3gGetChildCount(group);
    for (M3Gint index = 0; index < childCount; index++) {
        CollectPickableSkeletons(m3gGetChild(group, index), skeletons);
    }
}

/** Transform a point (x, y, z) by `matrix` with the perspective divide. */
void TransformPoint(const M3GMatrix* matrix, const M3Gfloat* point, M3Gfloat* out)
{
    M3GVec4 vector;
    vector.x = point[0];
    vector.y = point[1];
    vector.z = point[2];
    vector.w = 1.0f;
    m3gTransformVec4(matrix, &vector);
    out[0] = vector.x / vector.w;
    out[1] = vector.y / vector.w;
    out[2] = vector.z / vector.w;
}

/**
 * Pick the skeletons under `root` with `segment` (root coordinates) and replace `node` / `result`
 * when a skeleton hit is nearer. Distances compare directly: m3gcore's distance is the fraction of
 * the segment, which an affine change of coordinates keeps.
 */
void PickSkeletons(M3GGroup root, int scope, const PickSegment& segment, M3GNode& node, M3Gfloat* result,
                   M3GCamera camera = nullptr, float x = 0.0f, float y = 0.0f)
{
    std::vector<M3GGroup> skeletons;
    // The root itself must be pickable, as in m3gcore's own pick path test.
    if (!m3gIsEnabled(reinterpret_cast<M3GNode>(root), M3G_SETGET_PICKING)) {
        return;
    }
    const M3Gint childCount = m3gGetChildCount(root);
    for (M3Gint index = 0; index < childCount; index++) {
        CollectPickableSkeletons(m3gGetChild(root, index), skeletons);
    }
    for (M3GGroup skeleton : skeletons) {
        M3GMatrix rootFromSkeleton;
        if (!m3gGetTransformTo(reinterpret_cast<M3GNode>(skeleton), reinterpret_cast<M3GNode>(root), &rootFromSkeleton)) {
            continue;
        }
        M3GMatrix skeletonFromRoot = rootFromSkeleton;
        if (!m3gInvertMatrix(&skeletonFromRoot)) {
            continue;
        }
        M3Gfloat start[3];
        M3Gfloat end[3];
        TransformPoint(&skeletonFromRoot, &segment.points[0], start);
        TransformPoint(&skeletonFromRoot, &segment.points[3], end);
        // m3gPick3D takes origin and direction (it adds them itself).
        M3Gfloat localRay[PickRayFloats] = {
            start[0], start[1], start[2],
            end[0] - start[0], end[1] - start[1], end[2] - start[2],
        };
        M3Gfloat localResult[PickResultFloats] = {};
        M3GNode skeletonHit = nullptr;
        if (camera != nullptr) {
            // A camera pick picks the skeleton through the camera too: m3gcore only picks
            // Sprite3Ds in 2D picks, and Swerve hits sprites hanging on bones.
            // Its distance is the same near-to-far fraction as the root's.
            skeletonHit = m3gPick2D(skeleton, scope, x, y, camera, localResult);
        } else {
            skeletonHit = m3gPick3D(skeleton, scope, localRay, localResult);
        }
        if (skeletonHit == nullptr) {
            continue;
        }
        if (node != nullptr && localResult[0] >= result[0]) {
            continue;
        }
        node = skeletonHit;
        // Distance, submesh, texture coordinates and normal belong to the hit; the ray stays the root's.
        for (int index = 0; index < 9; index++) {
            result[index] = localResult[index];
        }
        result[9] = segment.points[0];
        result[10] = segment.points[1];
        result[11] = segment.points[2];
        result[12] = segment.points[3] - segment.points[0];
        result[13] = segment.points[4] - segment.points[1];
        result[14] = segment.points[5] - segment.points[2];
    }
}

/**
 * The pick segment of viewport point (x, y) of `camera` in `group` coordinates, from the near to the
 * far clipping plane, computed as m3gPick2D does. False when the camera is not connected to `group`.
 */
bool CameraPickSegment(M3GGroup group, float x, float y, M3GCamera camera, PickSegment& segment)
{
    M3GMatrix inverseProjection;
    m3gGetProjectionAsMatrix(camera, &inverseProjection);
    if (!m3gInvertMatrix(&inverseProjection)) {
        return false;
    }
    M3GMatrix groupFromCamera;
    if (!m3gGetTransformTo(reinterpret_cast<M3GNode>(camera), reinterpret_cast<M3GNode>(group), &groupFromCamera)) {
        return false;
    }
    const M3Gfloat nearClip[3] = { 2.0f * x - 1.0f, 1.0f - 2.0f * y, -1.0f };
    const M3Gfloat farClip[3] = { 2.0f * x - 1.0f, 1.0f - 2.0f * y, 1.0f };
    M3Gfloat nearCamera[3];
    M3Gfloat farCamera[3];
    TransformPoint(&inverseProjection, nearClip, nearCamera);
    TransformPoint(&inverseProjection, farClip, farCamera);
    TransformPoint(&groupFromCamera, nearCamera, &segment.points[0]);
    TransformPoint(&groupFromCamera, farCamera, &segment.points[3]);
    return true;
}

/** Store a pick in the game's RayIntersection and hit flag. */
void StorePick(Object* ray, unsigned char* hit, M3GNode node, const M3Gfloat* result)
{
    if (ray != nullptr) {
        SetPickResult(ray, node, result);
    }
    if (hit != nullptr) {
        *hit = static_cast<unsigned char>(node != nullptr);
    }
}

/** group_pickNode: pick along a ray from `origin` in `direction` (SwerveHelper::RayPick). */
int PickRay(Object* self, int scope, const float* origin, const float* direction, Object* ray, unsigned char* hit)
{
    // The skeleton picks take the ray as two points: origin and origin + direction.
    PickSegment segment;
    for (int axis = 0; axis < 3; axis++) {
        segment.points[axis] = origin[axis];
        segment.points[3 + axis] = origin[axis] + direction[axis];
    }
    // m3gPick3D takes origin and direction and adds them itself (m3g_group.c).
    M3Gfloat originAndDirection[PickRayFloats];
    for (int axis = 0; axis < 3; axis++) {
        originAndDirection[axis] = origin[axis];
        originAndDirection[3 + axis] = direction[axis];
    }
    M3Gfloat result[PickResultFloats] = {};
    M3GNode node = m3gPick3D(H<M3GGroup>(self), scope, originAndDirection, result);
    int error = TakeError();
    if (error == ErrorNone) {
        PickSkeletons(H<M3GGroup>(self), scope, segment, node, result);
        error = TakeError();
    }
    StorePick(ray, hit, node, result);
    return error;
}

/** group_pickCamera: pick through viewport point (x, y) of `camera` (aiming, CUnitBody hit zones). */
int PickCamera(Object* self, int scope, float x, float y, Object* camera, Object* ray, unsigned char* hit)
{
    M3Gfloat result[PickResultFloats] = {};
    M3GNode node = m3gPick2D(H<M3GGroup>(self), scope, x, y, H<M3GCamera>(camera), result);
    int error = TakeError();
    PickSegment segment;
    if (error == ErrorNone && CameraPickSegment(H<M3GGroup>(self), x, y, H<M3GCamera>(camera), segment)) {
        PickSkeletons(H<M3GGroup>(self), scope, segment, node, result, H<M3GCamera>(camera), x, y);
        error = TakeError();
    }
    StorePick(ray, hit, node, result);
    return error;
}

int GetActiveCamera(Object* self, void** camera)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetActiveCamera(H<M3GWorld>(self))), camera);
    return TakeError();
}

int SetActiveCamera(Object* self, Object* camera)
{
    m3gSetActiveCamera(H<M3GWorld>(self), H<M3GCamera>(camera));
    return TakeError();
}

int GetBackground(Object* self, void** background)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetBackground(H<M3GWorld>(self))), background);
    return TakeError();
}

int SetBackground(Object* self, Object* background)
{
    m3gSetBackground(H<M3GWorld>(self), H<M3GBackground>(background));
    return TakeError();
}

// ================= Mesh, SkinnedMesh =================

int GetSubmeshCount(Object* self, int* count)
{
    *count = m3gGetSubmeshCount(H<M3GMesh>(self));
    return TakeError();
}

int GetVertexBuffer(Object* self, void** vertexBuffer)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetVertexBuffer(H<M3GMesh>(self))), vertexBuffer);
    return TakeError();
}

int GetIndexBuffer(Object* self, int index, void** indexBuffer)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetIndexBuffer(H<M3GMesh>(self), index)), indexBuffer);
    return TakeError();
}

int GetAppearance(Object* self, int index, void** appearance)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetAppearance(H<M3GMesh>(self), index)), appearance);
    return TakeError();
}

int SetAppearance(Object* self, int index, Object* appearance)
{
    m3gSetAppearance(H<M3GMesh>(self), index, H<M3GAppearance>(appearance));
    return TakeError();
}

int GetSkeleton(Object* self, void** skeleton)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetSkeleton(H<M3GSkinnedMesh>(self))), skeleton);
    return TakeError();
}

// ================= Camera =================

/** Projection parameters (fovy or height, aspect ratio, near, far); *projectionType gets the M3G type. */
int GetProjectionParams(Object* self, int count, float* params, int* projectionType)
{
    M3Gfloat values[4] = {};
    const M3Gint type = m3gGetProjectionAsParams(H<M3GCamera>(self), values);
    if (params != nullptr) {
        int copied = count;
        if (copied > 4) {
            copied = 4;
        }
        for (int index = 0; index < copied; index++) {
            params[index] = values[index];
        }
    }
    if (projectionType != nullptr) {
        *projectionType = type;
    }
    return TakeError();
}

int GetProjectionTransform(Object* self, Object* transform, int* projectionType)
{
    const M3Gint type = m3gGetProjectionAsMatrix(H<M3GCamera>(self), MatrixOf(transform));
    if (projectionType != nullptr) {
        *projectionType = type;
    }
    return TakeError();
}

int SetParallel(Object* self, float height, float aspectRatio, float nearClip, float farClip)
{
    m3gSetParallel(H<M3GCamera>(self), height, aspectRatio, nearClip, farClip);
    return TakeError();
}

int SetPerspective(Object* self, float fovy, float aspectRatio, float nearClip, float farClip)
{
    m3gSetPerspective(H<M3GCamera>(self), fovy, aspectRatio, nearClip, farClip);
    return TakeError();
}

// ================= Light =================

int SetLightMode(Object* self, int mode)
{
    m3gSetLightMode(H<M3GLight>(self), mode);
    return TakeError();
}

int SetLightColor(Object* self, int color)
{
    m3gSetLightColor(H<M3GLight>(self), static_cast<M3Guint>(color));
    return TakeError();
}

int SetLightIntensity(Object* self, float intensity)
{
    m3gSetIntensity(H<M3GLight>(self), intensity);
    return TakeError();
}

// ================= Sprite3D =================

/** sprite3d_initialize(scaled, image, appearance) of a sprite made by CreateObject (SwerveHelper). */
int InitializeSprite(Object* self, bool scaled, Object* image, Object* appearance)
{
    if (self->handle != nullptr) {
        return ErrorInvalidState;
    }
    const M3GSprite sprite = m3gCreateSprite(Interface(), scaled, H<M3GImage>(image), H<M3GAppearance>(appearance));
    const int error = TakeError();
    if (sprite == nullptr) {
        if (error == ErrorNone) {
            return ErrorGeneral;
        }
        return error;
    }
    AttachHandle(self, reinterpret_cast<M3GObject>(sprite));
    return error;
}

int GetSpriteAppearance(Object* self, void** appearance)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetSpriteAppearance(H<M3GSprite>(self))), appearance);
    return TakeError();
}

int SetSpriteAppearance(Object* self, Object* appearance)
{
    m3gSetSpriteAppearance(H<M3GSprite>(self), H<M3GAppearance>(appearance));
    return TakeError();
}

int GetSpriteImage(Object* self, void** image)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetSpriteImage(H<M3GSprite>(self))), image);
    return TakeError();
}

int SetSpriteImage(Object* self, Object* image)
{
    m3gSetSpriteImage(H<M3GSprite>(self), H<M3GImage>(image));
    return TakeError();
}

// ================= Appearance =================

int GetLayer(Object* self, int* layer)
{
    *layer = m3gGetLayer(H<M3GAppearance>(self));
    return TakeError();
}

int SetLayer(Object* self, int layer)
{
    m3gSetLayer(H<M3GAppearance>(self), layer);
    return TakeError();
}

int GetMaterial(Object* self, void** material)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetMaterial(H<M3GAppearance>(self))), material);
    return TakeError();
}

int SetMaterial(Object* self, Object* material)
{
    m3gSetMaterial(H<M3GAppearance>(self), H<M3GMaterial>(material));
    return TakeError();
}

int GetCompositingMode(Object* self, void** mode)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetCompositingMode(H<M3GAppearance>(self))), mode);
    return TakeError();
}

int SetCompositingMode(Object* self, Object* mode)
{
    m3gSetCompositingMode(H<M3GAppearance>(self), H<M3GCompositingMode>(mode));
    return TakeError();
}

int GetFog(Object* self, void** fog)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetFog(H<M3GAppearance>(self))), fog);
    return TakeError();
}

int SetFog(Object* self, Object* fog)
{
    m3gSetFog(H<M3GAppearance>(self), H<M3GFog>(fog));
    return TakeError();
}

int GetTexture(Object* self, int index, void** texture)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetTexture(H<M3GAppearance>(self), index)), texture);
    return TakeError();
}

int SetTexture(Object* self, int index, Object* texture)
{
    m3gSetTexture(H<M3GAppearance>(self), index, H<M3GTexture>(texture));
    return TakeError();
}

// ================= Texture2D =================

int GetTextureImage(Object* self, void** image)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetTextureImage(H<M3GTexture>(self))), image);
    return TakeError();
}

int SetTextureImage(Object* self, Object* image)
{
    m3gSetTextureImage(H<M3GTexture>(self), H<M3GImage>(image));
    return TakeError();
}

// ================= Background =================

int GetCropX(Object* self, int* value)
{
    *value = m3gGetBgCrop(H<M3GBackground>(self), M3G_GET_CROPX);
    return TakeError();
}

int GetCropY(Object* self, int* value)
{
    *value = m3gGetBgCrop(H<M3GBackground>(self), M3G_GET_CROPY);
    return TakeError();
}

int GetCropWidth(Object* self, int* value)
{
    *value = m3gGetBgCrop(H<M3GBackground>(self), M3G_GET_CROPWIDTH);
    return TakeError();
}

int GetCropHeight(Object* self, int* value)
{
    *value = m3gGetBgCrop(H<M3GBackground>(self), M3G_GET_CROPHEIGHT);
    return TakeError();
}

int IsColorClearEnabled(Object* self, unsigned char* enabled)
{
    *enabled = static_cast<unsigned char>(m3gIsBgEnabled(H<M3GBackground>(self), M3G_SETGET_COLORCLEAR) != 0);
    return TakeError();
}

int SetColorClearEnable(Object* self, bool enable)
{
    m3gSetBgEnable(H<M3GBackground>(self), M3G_SETGET_COLORCLEAR, enable);
    return TakeError();
}

int SetDepthClearEnable(Object* self, bool enable)
{
    m3gSetBgEnable(H<M3GBackground>(self), M3G_SETGET_DEPTHCLEAR, enable);
    return TakeError();
}

// ================= CompositingMode, Material =================

int SetBlending(Object* self, int mode)
{
    m3gSetBlending(H<M3GCompositingMode>(self), mode);
    return TakeError();
}

int SetDepthTestEnable(Object* self, bool enable)
{
    m3gEnableDepthTest(H<M3GCompositingMode>(self), enable);
    return TakeError();
}

int SetDepthWriteEnable(Object* self, bool enable)
{
    m3gEnableDepthWrite(H<M3GCompositingMode>(self), enable);
    return TakeError();
}

/** Color of `target` (Material.AMBIENT 0x400, DIFFUSE 0x800, ...) as 0xAARRGGBB. */
int GetMaterialColor(Object* self, int target, int* color)
{
    *color = static_cast<int>(m3gGetColor(H<M3GMaterial>(self), target));
    return TakeError();
}

int SetMaterialColor(Object* self, int target, unsigned int color)
{
    m3gSetColor(H<M3GMaterial>(self), target, color);
    return TakeError();
}

// ================= VertexBuffer, VertexArray, IndexBuffer =================

int GetBufferVertexCount(Object* self, int* count)
{
    *count = m3gGetVertexCount(H<M3GVertexBuffer>(self));
    return TakeError();
}

/** Position array; scaleBias gets the scale and the three bias values. */
int GetPositions(Object* self, int scaleBiasCount, float* scaleBias, void** positions)
{
    const M3GVertexArray array = m3gGetVertexArray(H<M3GVertexBuffer>(self), M3G_GET_POSITIONS, scaleBias, scaleBiasCount);
    WrapInto(reinterpret_cast<M3GObject>(array), positions);
    return TakeError();
}

int GetArrayVertexCount(Object* self, int* count)
{
    M3Gsizei vertexCount = 0;
    M3Gint size = 0;
    M3Gdatatype type = M3G_SHORT;
    M3Gsizei stride = 0;
    m3gGetVertexArrayParams(H<M3GVertexArray>(self), &vertexCount, &size, &type, &stride);
    *count = vertexCount;
    return TakeError();
}

/** Components of `vertexCount` vertices from `firstVertex` as 16 bit values (valueCount of them). */
int GetVertexValues(Object* self, int firstVertex, int vertexCount, int valueCount, short* values)
{
    m3gGetVertexArrayElements(H<M3GVertexArray>(self), firstVertex, vertexCount, valueCount, M3G_SHORT, values);
    return TakeError();
}

/** Triangle list index count of the strips (3 per triangle). */
int GetIndexCount(Object* self, int* count)
{
    *count = m3gGetBatchSize(H<M3GIndexBuffer>(self), 0);
    return TakeError();
}

int GetIndices(Object* self, int count, unsigned int* indices)
{
    const M3Gint size = m3gGetBatchSize(H<M3GIndexBuffer>(self), 0);
    if (count < size) {
        return ErrorInvalidParameter;
    }
    // Swerve's corner order of the odd strip triangles, see M3gCkCore.c.
    ckM3gGetSwerveTriangleIndices(H<M3GIndexBuffer>(self), reinterpret_cast<M3Gint*>(indices));
    return TakeError();
}

// ================= AnimationController, AnimationTrack, KeyframeSequence =================

int GetWeight(Object* self, float* weight)
{
    *weight = m3gGetWeight(H<M3GAnimationController>(self));
    return TakeError();
}

int SetWeight(Object* self, float weight)
{
    m3gSetWeight(H<M3GAnimationController>(self), weight);
    return TakeError();
}

int GetSpeed(Object* self, float* speed)
{
    *speed = m3gGetSpeed(H<M3GAnimationController>(self));
    return TakeError();
}

int GetPosition(Object* self, int worldTime, float* sequenceTime)
{
    *sequenceTime = m3gGetPosition(H<M3GAnimationController>(self), worldTime);
    return TakeError();
}

int SetPosition(Object* self, float sequenceTime, int worldTime)
{
    m3gSetPosition(H<M3GAnimationController>(self), sequenceTime, worldTime);
    return TakeError();
}

int SetSpeed(Object* self, float speed, int worldTime)
{
    m3gSetSpeed(H<M3GAnimationController>(self), speed, worldTime);
    return TakeError();
}

int GetController(Object* self, void** controller)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetController(H<M3GAnimationTrack>(self))), controller);
    return TakeError();
}

int SetController(Object* self, Object* controller)
{
    m3gSetController(H<M3GAnimationTrack>(self), H<M3GAnimationController>(controller));
    return TakeError();
}

int GetKeyframeSequence(Object* self, void** sequence)
{
    WrapInto(reinterpret_cast<M3GObject>(m3gGetSequence(H<M3GAnimationTrack>(self))), sequence);
    return TakeError();
}

int GetDuration(Object* self, int* duration)
{
    *duration = m3gGetDuration(H<M3GKeyframeSequence>(self));
    return TakeError();
}

int GetRepeatMode(Object* self, int* repeatMode)
{
    *repeatMode = m3gGetRepeatMode(H<M3GKeyframeSequence>(self));
    return TakeError();
}

} // namespace

Object* CreateSprite3D()
{
    Object* sprite = new Object();
    InitObject(sprite, ClassSprite3D);
    return sprite;
}

void FillObject3DSlots(ObjectClass& objectClass)
{
    FillQueryInterfaceSlots(objectClass);
    SetSlot(objectClass, slot::object3d::getUserID, &GetUserID);
    SetSlot(objectClass, slot::object3d::setUserID, &SetUserID);
    SetSlot(objectClass, slot::object3d::getAnimationTrackCount, &GetAnimationTrackCount);
    SetSlot(objectClass, slot::object3d::getAnimationTrack, &GetAnimationTrack);
    SetSlot(objectClass, slot::object3d::addAnimationTrack, &AddAnimationTrack);
    SetSlot(objectClass, slot::object3d::removeAnimationTrack, &RemoveAnimationTrack);
    SetSlot(objectClass, slot::object3d::animate, &Animate);
    SetSlot(objectClass, slot::object3d::find, &Find);
    SetSlot(objectClass, slot::object3d::getReferences, &GetReferences);
    SetSlot(objectClass, slot::object3d::getUserParameter, &GetUserParameter);
    SetSlot(objectClass, slot::object3d::duplicate, &Duplicate);
}

void FillTransformableSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, slot::transformable::getTranslation, &GetTranslation);
    SetSlot(objectClass, slot::transformable::setTranslation, &SetTranslation);
    SetSlot(objectClass, slot::transformable::getScale, &GetScale);
    SetSlot(objectClass, slot::transformable::setScale, &SetScale);
    SetSlot(objectClass, slot::transformable::getOrientation, &GetOrientation);
    SetSlot(objectClass, slot::transformable::setOrientation, &SetOrientation);
    SetSlot(objectClass, slot::transformable::translate, &Translate);
    SetSlot(objectClass, slot::transformable::postRotate, &PostRotate);
    SetSlot(objectClass, slot::transformable::getTransform, &GetTransform);
    SetSlot(objectClass, slot::transformable::getCompositeTransform, &GetCompositeTransform);
    SetSlot(objectClass, slot::transformable::setTransform, &SetTransform);
}

void FillNodeSlots(ObjectClass& objectClass)
{
    FillTransformableSlots(objectClass);
    SetSlot(objectClass, slot::node::getParent, &GetParent);
    SetSlot(objectClass, slot::node::getAlphaFactor, &GetAlphaFactor);
    SetSlot(objectClass, slot::node::setAlphaFactor, &SetAlphaFactor);
    SetSlot(objectClass, slot::node::isRenderingEnabled, &IsRenderingEnabled);
    SetSlot(objectClass, slot::node::setRenderingEnable, &SetRenderingEnable);
    SetSlot(objectClass, slot::node::isPickingEnabled, &IsPickingEnabled);
    SetSlot(objectClass, slot::node::setPickingEnable, &SetPickingEnable);
    SetSlot(objectClass, slot::node::getScope, &GetScope);
    SetSlot(objectClass, slot::node::setScope, &SetScope);
    SetSlot(objectClass, slot::node::getTransformTo, &GetTransformTo);
    SetSlot(objectClass, slot::node::setAlignment, &SetAlignment);
    SetSlot(objectClass, slot::node::align, &Align);
}

void FillGroupSlots(ObjectClass& objectClass)
{
    FillNodeSlots(objectClass);
    SetSlot(objectClass, slot::group::getChildCount, &GetChildCount);
    SetSlot(objectClass, slot::group::getChild, &GetChild);
    SetSlot(objectClass, slot::group::addChild, &AddChild);
    SetSlot(objectClass, slot::group::removeChild, &RemoveChild);
    SetSlot(objectClass, slot::group::pickNode, &PickRay);
    SetSlot(objectClass, slot::group::pickCamera, &PickCamera);
}

void FillWorldSlots(ObjectClass& objectClass)
{
    FillGroupSlots(objectClass);
    SetSlot(objectClass, slot::world::getActiveCamera, &GetActiveCamera);
    SetSlot(objectClass, slot::world::setActiveCamera, &SetActiveCamera);
    SetSlot(objectClass, slot::world::getBackground, &GetBackground);
    SetSlot(objectClass, slot::world::setBackground, &SetBackground);
}

void FillMeshSlots(ObjectClass& objectClass)
{
    FillNodeSlots(objectClass);
    SetSlot(objectClass, slot::mesh::getSubmeshCount, &GetSubmeshCount);
    SetSlot(objectClass, slot::mesh::getVertexBuffer, &GetVertexBuffer);
    SetSlot(objectClass, slot::mesh::getIndexBuffer, &GetIndexBuffer);
    SetSlot(objectClass, slot::mesh::getAppearance, &GetAppearance);
    SetSlot(objectClass, slot::mesh::setAppearance, &SetAppearance);
}

void FillSkinnedMeshSlots(ObjectClass& objectClass)
{
    FillMeshSlots(objectClass);
    SetSlot(objectClass, slot::skinnedmesh::getSkeleton, &GetSkeleton);
}

void FillCameraSlots(ObjectClass& objectClass)
{
    FillNodeSlots(objectClass);
    SetSlot(objectClass, slot::camera::getProjectionParams, &GetProjectionParams);
    SetSlot(objectClass, slot::camera::getProjectionTransform, &GetProjectionTransform);
    SetSlot(objectClass, slot::camera::setParallel, &SetParallel);
    SetSlot(objectClass, slot::camera::setPerspective, &SetPerspective);
}

void FillLightSlots(ObjectClass& objectClass)
{
    FillNodeSlots(objectClass);
    SetSlot(objectClass, slot::light::setMode, &SetLightMode);
    SetSlot(objectClass, slot::light::setColor, &SetLightColor);
    SetSlot(objectClass, slot::light::setIntensity, &SetLightIntensity);
}

void FillSpriteSlots(ObjectClass& objectClass)
{
    FillNodeSlots(objectClass);
    SetSlot(objectClass, slot::sprite3d::initialize, &InitializeSprite);
    SetSlot(objectClass, slot::sprite3d::getAppearance, &GetSpriteAppearance);
    SetSlot(objectClass, slot::sprite3d::setAppearance, &SetSpriteAppearance);
    SetSlot(objectClass, slot::sprite3d::getImage, &GetSpriteImage);
    SetSlot(objectClass, slot::sprite3d::setImage, &SetSpriteImage);
}

void FillAppearanceSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, slot::appearance::getLayer, &GetLayer);
    SetSlot(objectClass, slot::appearance::setLayer, &SetLayer);
    SetSlot(objectClass, slot::appearance::getMaterial, &GetMaterial);
    SetSlot(objectClass, slot::appearance::setMaterial, &SetMaterial);
    SetSlot(objectClass, slot::appearance::getFog, &GetFog);
    SetSlot(objectClass, slot::appearance::setFog, &SetFog);
    SetSlot(objectClass, slot::appearance::getCompositingMode, &GetCompositingMode);
    SetSlot(objectClass, slot::appearance::setCompositingMode, &SetCompositingMode);
    SetSlot(objectClass, slot::appearance::getTexture, &GetTexture);
    SetSlot(objectClass, slot::appearance::setTexture, &SetTexture);
}

void FillTextureSlots(ObjectClass& objectClass)
{
    FillTransformableSlots(objectClass);
    SetSlot(objectClass, slot::texture2d::getImage, &GetTextureImage);
    SetSlot(objectClass, slot::texture2d::setImage, &SetTextureImage);
}

void FillBackgroundSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, slot::background::getCropX, &GetCropX);
    SetSlot(objectClass, slot::background::getCropY, &GetCropY);
    SetSlot(objectClass, slot::background::getCropWidth, &GetCropWidth);
    SetSlot(objectClass, slot::background::getCropHeight, &GetCropHeight);
    SetSlot(objectClass, slot::background::isColorClearEnabled, &IsColorClearEnabled);
    SetSlot(objectClass, slot::background::setColorClearEnable, &SetColorClearEnable);
    SetSlot(objectClass, slot::background::setDepthClearEnable, &SetDepthClearEnable);
}

void FillCompositingModeSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, slot::compositingmode::setBlending, &SetBlending);
    SetSlot(objectClass, slot::compositingmode::setDepthTestEnable, &SetDepthTestEnable);
    SetSlot(objectClass, slot::compositingmode::setDepthWriteEnable, &SetDepthWriteEnable);
}

void FillMaterialSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, slot::material::getColor, &GetMaterialColor);
    SetSlot(objectClass, slot::material::setColor, &SetMaterialColor);
}

void FillVertexBufferSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, slot::vertexbuffer::getVertexCount, &GetBufferVertexCount);
    SetSlot(objectClass, slot::vertexbuffer::getPositions, &GetPositions);
}

void FillVertexArraySlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, slot::vertexarray::getVertexCount, &GetArrayVertexCount);
    SetSlot(objectClass, slot::vertexarray::get16, &GetVertexValues);
}

void FillIndexBufferSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, slot::indexbuffer::getIndexCount, &GetIndexCount);
    SetSlot(objectClass, slot::indexbuffer::getIndices, &GetIndices);
}

void FillAnimationControllerSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, slot::animationcontroller::getWeight, &GetWeight);
    SetSlot(objectClass, slot::animationcontroller::setWeight, &SetWeight);
    SetSlot(objectClass, slot::animationcontroller::getSpeed, &GetSpeed);
    SetSlot(objectClass, slot::animationcontroller::getPosition, &GetPosition);
    SetSlot(objectClass, slot::animationcontroller::setPosition, &SetPosition);
    SetSlot(objectClass, slot::animationcontroller::setSpeed, &SetSpeed);
}

void FillAnimationTrackSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, slot::animationtrack::getController, &GetController);
    SetSlot(objectClass, slot::animationtrack::setController, &SetController);
    SetSlot(objectClass, slot::animationtrack::getKeyframeSequence, &GetKeyframeSequence);
}

void FillKeyframeSequenceSlots(ObjectClass& objectClass)
{
    FillObject3DSlots(objectClass);
    SetSlot(objectClass, slot::keyframesequence::getDuration, &GetDuration);
    SetSlot(objectClass, slot::keyframesequence::getRepeatMode, &GetRepeatMode);
}

} // namespace ck::native
