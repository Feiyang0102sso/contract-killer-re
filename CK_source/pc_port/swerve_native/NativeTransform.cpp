// Native Swerve Transform (a 4x4 matrix) and RayIntersection (a pick result). Neither is an
// m3gcore object: m3gcore takes matrices as M3GMatrix values and returns pick results as floats.
#include "NativeRuntime.h"
#include <stdio.h>
#include <string.h>

namespace ck::native {
namespace {

constexpr int MatrixFloats = 16;
constexpr int PickResultFloats = 15;

TransformObject* AsTransform(const void* object)
{
    return static_cast<TransformObject*>(const_cast<void*>(object));
}

void DestroyTransform(Object* object)
{
    delete static_cast<TransformObject*>(object);
}

// ================= Transform =================

/** The 16 floats, row major. */
int TransformGet(Object* self, int count, float* matrix)
{
    if (count < MatrixFloats) {
        return ErrorInvalidParameter;
    }
    m3gGetMatrixRows(&AsTransform(self)->matrix, matrix);
    return ErrorNone;
}

int TransformSet(Object* self, int count, const float* matrix)
{
    if (count < MatrixFloats) {
        return ErrorInvalidParameter;
    }
    m3gSetMatrixRows(&AsTransform(self)->matrix, matrix);
    return ErrorNone;
}

/**
 * transform_setTransform is M3G's Transform.set(Transform): this = other (a copy, not a product).
 * tws.h names it `multiply` (CK: Ballistics). In CKZ, whose engine has the same slot, a product
 * squared the particle scale and drew screen-filling triangles; the copy was probed on the ARM backend.
 */
int TransformCopy(Object* self, Object* other)
{
    AsTransform(self)->matrix = *MatrixOf(other);
    return ErrorNone;
}

int TransformSetIdentity(Object* self)
{
    m3gIdentityMatrix(&AsTransform(self)->matrix);
    return ErrorNone;
}

int TransformInvert(Object* self)
{
    if (!m3gInvertMatrix(&AsTransform(self)->matrix)) {
        return ErrorGeneral; // singular matrix (M3G ArithmeticException)
    }
    return ErrorNone;
}

/** this = this * other. */
int TransformPostMultiply(Object* self, Object* other)
{
    m3gPostMultiplyMatrix(&AsTransform(self)->matrix, MatrixOf(other));
    return ErrorNone;
}

/** Rotation given as angle (degrees) and axis x, y, z. */
int TransformPostRotate(Object* self, const float* angleAxis)
{
    m3gPostRotateMatrix(&AsTransform(self)->matrix, angleAxis[0], angleAxis[1], angleAxis[2], angleAxis[3]);
    return ErrorNone;
}

/** Rotation given as quaternion x, y, z, w. */
int TransformPostRotateQuat(Object* self, const float* quaternion)
{
    M3GQuat quat;
    m3gSetQuat(&quat, quaternion);
    m3gPostRotateMatrixQuat(&AsTransform(self)->matrix, &quat);
    return ErrorNone;
}

int TransformPostScale(Object* self, const float* scale)
{
    m3gPostScaleMatrix(&AsTransform(self)->matrix, scale[0], scale[1], scale[2]);
    return ErrorNone;
}

int TransformPostTranslate(Object* self, const float* translation)
{
    m3gPostTranslateMatrix(&AsTransform(self)->matrix, translation[0], translation[1], translation[2]);
    return ErrorNone;
}

/** Transform the positions of `vertices` into `out` (4 floats per vertex). */
int TransformVertexArray(Object* self, Object* vertices, int outCount, float* out, bool w)
{
    m3gTransformArray(reinterpret_cast<M3GVertexArray>(vertices->handle), &AsTransform(self)->matrix, out, outCount, w);
    return TakeError();
}

/** Transform `count` floats in place, 4 per vector (x, y, z, w). */
int TransformVectors(Object* self, int count, float* vectors)
{
    for (int offset = 0; offset + 4 <= count; offset += 4) {
        M3GVec4 vector;
        vector.x = vectors[offset];
        vector.y = vectors[offset + 1];
        vector.z = vectors[offset + 2];
        vector.w = vectors[offset + 3];
        m3gTransformVec4(&AsTransform(self)->matrix, &vector);
        vectors[offset] = vector.x;
        vectors[offset + 1] = vector.y;
        vectors[offset + 2] = vector.z;
        vectors[offset + 3] = vector.w;
    }
    return ErrorNone;
}

// ================= RayIntersection =================

/** Result of the last pick: the node (one m3gcore reference) and m3gcore's 15 result floats. */
struct RayObject : Object {
    M3GNode node;
    M3Gfloat result[PickResultFloats];
};

RayObject* AsRay(const void* object)
{
    return static_cast<RayObject*>(const_cast<void*>(object));
}

void DestroyRay(Object* object)
{
    RayObject* ray = AsRay(object);
    if (ray->node != nullptr) {
        m3gDeleteRef(reinterpret_cast<M3GObject>(ray->node));
    }
    delete ray;
}

int RayGetDistance(Object* self, float* distance)
{
    *distance = AsRay(self)->result[0];
    return ErrorNone;
}

int RayGetIntersected(Object* self, void** node)
{
    WrapInto(reinterpret_cast<M3GObject>(AsRay(self)->node), node);
    return ErrorNone;
}

int RayGetSubmeshIndex(Object* self, int* index)
{
    *index = static_cast<int>(AsRay(self)->result[1]);
    return ErrorNone;
}

/** Texture coordinate S of texture unit `unit` (0 or 1) at the hit. */
int RayGetTextureS(Object* self, int unit, float* s)
{
    if (unit < 0 || unit > 1) {
        return ErrorInvalidParameter;
    }
    *s = AsRay(self)->result[2 + unit];
    return ErrorNone;
}

/** Texture coordinate T of texture unit `unit` (0 or 1) at the hit. */
int RayGetTextureT(Object* self, int unit, float* t)
{
    if (unit < 0 || unit > 1) {
        return ErrorInvalidParameter;
    }
    *t = AsRay(self)->result[4 + unit];
    return ErrorNone;
}

int RayGetNormalX(Object* self, float* normal)
{
    *normal = AsRay(self)->result[6];
    return ErrorNone;
}

int RayGetNormalY(Object* self, float* normal)
{
    *normal = AsRay(self)->result[7];
    return ErrorNone;
}

int RayGetNormalZ(Object* self, float* normal)
{
    *normal = AsRay(self)->result[8];
    return ErrorNone;
}

/** Ray origin x, y, z and direction x, y, z. */
int RayGetRay(Object* self, int count, float* originAndDirection)
{
    int copied = count;
    if (copied > 6) {
        copied = 6;
    }
    for (int index = 0; index < copied; index++) {
        originAndDirection[index] = AsRay(self)->result[9 + index];
    }
    return ErrorNone;
}

} // namespace

M3GMatrix* MatrixOf(const void* transform)
{
    if (transform == nullptr) {
        return nullptr;
    }
    return &AsTransform(transform)->matrix;
}

Object* CreateTransform()
{
    TransformObject* transform = new TransformObject();
    InitObject(transform, ClassTransform);
    m3gIdentityMatrix(&transform->matrix);
    return transform;
}

Object* CreateRayIntersection()
{
    RayObject* ray = new RayObject();
    InitObject(ray, ClassRayIntersection);
    ray->node = nullptr;
    memset(ray->result, 0, sizeof(ray->result));
    return ray;
}

void SetPickResult(void* rayIntersection, M3GNode node, const M3Gfloat* result)
{
    RayObject* ray = AsRay(rayIntersection);
    if (node != nullptr) {
        m3gAddRef(reinterpret_cast<M3GObject>(node));
    }
    if (ray->node != nullptr) {
        m3gDeleteRef(reinterpret_cast<M3GObject>(ray->node));
    }
    ray->node = node;
    memcpy(ray->result, result, sizeof(ray->result));
}

void FillTransformSlots(ObjectClass& objectClass)
{
    FillQueryInterfaceSlots(objectClass);
    objectClass.destroy = DestroyTransform;
    SetSlot(objectClass, slot::transform::get, &TransformGet);
    SetSlot(objectClass, slot::transform::setMatrix, &TransformSet);
    SetSlot(objectClass, slot::transform::setTransform, &TransformCopy);
    SetSlot(objectClass, slot::transform::setIdentity, &TransformSetIdentity);
    SetSlot(objectClass, slot::transform::invert, &TransformInvert);
    SetSlot(objectClass, slot::transform::postMultiply, &TransformPostMultiply);
    SetSlot(objectClass, slot::transform::postRotate, &TransformPostRotate);
    SetSlot(objectClass, slot::transform::postRotateQuat, &TransformPostRotateQuat);
    SetSlot(objectClass, slot::transform::postScale, &TransformPostScale);
    SetSlot(objectClass, slot::transform::postTranslate, &TransformPostTranslate);
    SetSlot(objectClass, slot::transform::transform, &TransformVertexArray);
    SetSlot(objectClass, slot::transform::transformPoints, &TransformVectors);
}

void FillRayIntersectionSlots(ObjectClass& objectClass)
{
    FillQueryInterfaceSlots(objectClass);
    objectClass.destroy = DestroyRay;
    SetSlot(objectClass, slot::rayintersection::getDistance, &RayGetDistance);
    SetSlot(objectClass, slot::rayintersection::getIntersected, &RayGetIntersected);
    SetSlot(objectClass, slot::rayintersection::getSubmeshIndex, &RayGetSubmeshIndex);
    SetSlot(objectClass, slot::rayintersection::getTextureS, &RayGetTextureS);
    SetSlot(objectClass, slot::rayintersection::getTextureT, &RayGetTextureT);
    SetSlot(objectClass, slot::rayintersection::getNormalX, &RayGetNormalX);
    SetSlot(objectClass, slot::rayintersection::getNormalY, &RayGetNormalY);
    SetSlot(objectClass, slot::rayintersection::getNormalZ, &RayGetNormalZ);
    SetSlot(objectClass, slot::rayintersection::getRay, &RayGetRay);
}

} // namespace ck::native
