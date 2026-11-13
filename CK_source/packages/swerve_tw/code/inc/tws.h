// PROVISIONAL (created by b057x): header-only Swerve C++ wrapper (swerve_tw), no owning unit
// tws.h - typed wrappers of the Swerve scene graph objects; only what src/general uses.
#ifndef TWS_H
#define TWS_H

#include "twb_queryinterface.h"
#include "SwerveApi.h"  // added by b048
#include "CSwerve.h"  // added by b047x (Object3D::operator==)
#include "CSwerveGraphics3D.h"  // added by fx2 (Graphics3D handle)

/** Error policy that asserts on Swerve errors. */
class CSwerveAssertingErrorHandler;


/** Dispatch a slot established from CK ARMv7 call sites. */
template <class Method>
Method CKMethod(IQueryInterface* object, unsigned int offset)
{
    return reinterpret_cast<Method>((*reinterpret_cast<void***>(object))[offset / 4]);
}

// Interfaces of the Swerve runtime objects. The runtime ships without headers: the vtable slots are  // added by b051x
// laid out from the call sites in src/ext/SwerveHelper.cpp, the names follow the M3G API (guesses);  // added by b051x
// VirtualSlotXX are slots not used by the game. Every method returns an error code (0: success).  // added by b051x
class IObject3DInterface;  // added by b051x
class ITransformInterface;  // added by b051x
class ITransformableInterface;  // added by b051x
class INodeInterface;  // added by b051x
class IGroupInterface;  // added by b051x
class IMeshInterface;  // added by b051x
class ISkinnedMeshInterface;  // added by b051x
class IAppearanceInterface;  // added by b051x
class ITexture2DInterface;  // added by b051x
class IVertexBufferInterface;  // added by b051x
class IVertexArrayInterface;  // added by b051x
class IIndexBufferInterface;
class IAnimationControllerInterface;  // added by b051x
class IAnimationTrackInterface;  // added by b051x
class IKeyframeSequenceInterface;  // added by b051x
class IImage2DInterface;  // added by b048
class ICameraInterface;  // added by b048
class IRayIntersectionInterface;

/** RayIntersection interface proven by the original Body Pick call; the unknown 0x24/0x28 slots are kept. */
class IRayIntersectionInterface : public IQueryInterface {
public:
    virtual int getDistance(float* distance) = 0; // 0x0c
    virtual int getIntersected(INodeInterface** node) = 0; // 0x10
    virtual void VirtualSlot14() = 0;
    virtual int getNormalX(float* normal) = 0; // 0x18
    virtual int getNormalY(float* normal) = 0; // 0x1c
    virtual int getNormalZ(float* normal) = 0; // 0x20
    virtual void VirtualSlot24() = 0;
    virtual void VirtualSlot28() = 0;
    virtual int getRay(int count, float* originAndDirection) = 0; // 0x2c, original count6
};

/** Object3D: user id, animation tracks, references, duplication. */  // added by b051x
class IObject3DInterface : public IQueryInterface {  // added by b051x
public:  // added by b051x
    virtual int getUserID(int* userID) = 0;  // 0x0c  // added by b051x
    virtual int setUserID(int userID) = 0;  // 0x10  // added by b051x
    virtual int getAnimationTrackCount(int* count) = 0;  // 0x14  // added by b051x
    virtual void VirtualSlot18() = 0;  // added by b051x
    virtual void VirtualSlot1c() = 0;  // added by b051x
    virtual int getAnimationTrack(int index, IAnimationTrackInterface** track) = 0;  // 0x20  // added by b051x
    virtual void VirtualSlot24() = 0;  // added by b051x
    virtual int addAnimationTrack(IAnimationTrackInterface* track) = 0;  // 0x28  // added by b051x
    virtual void VirtualSlot2c() = 0;  // added by b051x
    virtual int removeAnimationTrack(IAnimationTrackInterface* track) = 0;  // 0x30  // added by b051x
    virtual int animate(int worldTime, int* validity) = 0;  // 0x34  // changed by b048 (was VirtualSlot34)
    virtual void VirtualSlot38() = 0;  // added by b051x
    virtual int find(int userID, IObject3DInterface** found) = 0;  // 0x3c  // added by b051x
    virtual void VirtualSlot40() = 0;  // added by b051x
    virtual void VirtualSlot44() = 0;  // added by b051x
    virtual int getReferences(unsigned int capacity, IObject3DInterface** references, int* count) = 0;  // 0x48  // added by b051x
    /** User parameter `id` copied into `buffer` (bufferSize bytes); its length goes to *length. */  // added by b048
    virtual int getUserParameter(int id, int bufferSize, char* buffer, int* length) = 0;  // 0x4c  // changed by b048 (was VirtualSlot4c)
    virtual void VirtualSlot50() = 0;  // added by b051x
    virtual void VirtualSlot54() = 0;  // added by b051x
    virtual void VirtualSlot58() = 0;  // added by b051x
    virtual void VirtualSlot5c() = 0;  // added by b051x
    virtual int duplicate(IObject3DInterface** copy) = 0;  // 0x60  // added by b051x
};  // added by b051x

/** Transform: 4x4 matrix (row major, 16 floats). */  // added by b051x
class ITransformInterface : public IQueryInterface {  // added by b051x
public:  // added by b051x
    virtual void VirtualSlot0c() = 0;  // added by b051x
    virtual int get(int count, float* matrix) = 0;  // 0x10  // added by b051x
    virtual int set(int count, const float* matrix) = 0;  // 0x14  // changed by b047x (was VirtualSlot14)
    virtual int multiply(ITransformInterface* transform) = 0;  // added by b051x (was VirtualSlot18; Ballistics uses 0x18)
    virtual int setIdentity() = 0;  // 0x1c  // changed by b048 (was VirtualSlot1c)
    virtual int invert() = 0;  // changed by b049 (was VirtualSlot20)  // added by b051x
    virtual int postMultiply(ITransformInterface* transform) = 0;  // 0x24  // changed by b055x (was VirtualSlot24)
    virtual int postRotateQuat(const float* quaternion) = 0;  // 0x28  // changed by b048 (was VirtualSlot28)
    virtual void VirtualSlot2c() = 0;  // added by b051x
    virtual int postScale(const float* scale) = 0;  // 0x30, Visual LoadModel  // added by b051x (was VirtualSlot30; named by b079e)
    virtual int postTranslate(const float* translation) = 0;  // 0x34  // changed by b048 (was VirtualSlot34)
    /** Transform the positions of `vertices` into `out` (outCount floats, 4 per vertex). */  // added by b048
    virtual int transform(IVertexArrayInterface* vertices, int outCount, float* out, bool w) = 0;  // 0x38  // changed by b048 (was VirtualSlot38)
    virtual int transform(int count, float* vectors) = 0;  // 0x3c  // added by b051x
};  // added by b051x

/** Transformable: translation (x, y, z), scale (x, y, z), orientation (angle, axis x, y, z), generic transform. */  // added by b051x
class ITransformableInterface : public IObject3DInterface {  // added by b051x
public:  // added by b051x
    virtual void VirtualSlot64() = 0;  // added by b051x
    virtual void VirtualSlot68() = 0;  // added by b051x
    virtual int getTranslation(float* translation) = 0;  // 0x6c  // changed by b047x (was VirtualSlot6c)
    virtual int setTranslation(const float* translation) = 0;  // 0x70  // added by b051x
    virtual int getScale(float* scale) = 0;  // 0x74  // changed by b047x (was VirtualSlot74)
    virtual int setScale(const float* scale) = 0;  // 0x78  // added by b051x
    virtual int getOrientation(float* angleAxis) = 0;  // 0x7c  // changed by b047x (was VirtualSlot7c)
    virtual int setOrientation(const float* angleAxis) = 0;  // 0x80  // added by b051x
    virtual int getOrientation84(float* orientation) = 0; // original VirtualSlot84; Body104d50 passes four float arguments, semantic name to be verified
    virtual void VirtualSlot88() = 0;  // added by b051x
    virtual int translate(const float* translation) = 0;  // added by b051x (was VirtualSlot8c; SwerveBallistics 0xeeb98)
    virtual void VirtualSlot90() = 0;  // added by b051x
    virtual void VirtualSlot94() = 0;  // added by b051x
    virtual int postRotate(const float* angleAxis) = 0;  // added by b051x (was VirtualSlot98; HandGrenade PostSimulate)
    virtual void VirtualSlot9c() = 0;  // added by b051x
    virtual void VirtualSlota0() = 0;  // added by b051x
    /** Composite transform (translation, orientation, scale, generic) into `transform`. */  // changed by b055x (was VirtualSlota4)
    virtual int getCompositeTransform(ITransformInterface* transform) = 0;  // 0xa4  // changed by b055x
    virtual void VirtualSlota8() = 0;  // added by b051x
    virtual int setTransform(ITransformInterface* transform) = 0;  // 0xac  // added by b051x
};  // added by b051x

/** Node. */  // added by b051x
class INodeInterface : public ITransformableInterface {  // added by b051x
public:  // added by b051x
    virtual void VirtualSlotb0() = 0;  // added by b051x
    virtual int getParent(INodeInterface** parent) = 0;  // 0xb4  // changed by b047x (was VirtualSlotb4; name guessed)
    virtual void VirtualSlotb8() = 0;  // added by b051x
    virtual int setAlphaFactor(float alpha) = 0;  // 0xbc  // changed by b047x (was VirtualSlotbc)
    virtual int getRenderingEnable(unsigned char* enabled) = 0; // original VirtualSlotc0, Body0x106b4a
    virtual int setRenderingEnable(bool enable) = 0;  // 0xc4  // changed by b047x (was VirtualSlotc4)
    virtual int getPickingEnable(unsigned char* enabled) = 0; // original VirtualSlotc8, Body0x106e64
    virtual int setPickingEnable(bool enable) = 0;  // 0xcc  // changed by b055x
    virtual void VirtualSlotd0() = 0;  // added by b051x
    virtual int setScope(int scope) = 0;  // 0xd4  // changed by b055x (was VirtualSlotd4)
    virtual void VirtualSlotd8() = 0;  // added by b051x
    virtual void VirtualSlotdc() = 0;  // added by b051x
    virtual void VirtualSlote0() = 0;  // added by b051x
    virtual void VirtualSlote4() = 0;  // added by b051x
    virtual int getTransformTo(INodeInterface* target, ITransformInterface* transform, bool* result) = 0;  // 0xe8  // added by b051x
};  // added by b051x

/** Group. */  // added by b051x
class IGroupInterface : public INodeInterface {  // added by b051x
public:  // added by b051x
    virtual void VirtualSlotec() = 0;  // added by b051x
    virtual void VirtualSlotf0() = 0;  // added by b051x
    virtual void VirtualSlotf4() = 0;  // added by b051x
    virtual void VirtualSlotf8() = 0;  // added by b051x
    virtual void VirtualSlotfc() = 0;  // added by b051x
    virtual void VirtualSlot100() = 0;  // added by b051x
    virtual void VirtualSlot104() = 0;  // added by b051x
    virtual void VirtualSlot108() = 0;  // added by b051x
    virtual void VirtualSlot10c() = 0;  // added by b051x
    virtual void VirtualSlot110() = 0;  // added by b051x
    virtual void VirtualSlot114() = 0;  // added by b051x
    virtual void VirtualSlot118() = 0;  // added by b051x
    virtual void VirtualSlot11c() = 0;  // added by b051x
    virtual void VirtualSlot120() = 0;  // added by b051x
    virtual int getChildCount(int* count) = 0;  // 0x124  // added by b051x
    virtual void VirtualSlot128() = 0;  // added by b051x
    virtual void VirtualSlot12c() = 0;  // added by b051x
    virtual void VirtualSlot130() = 0;  // added by b051x
    virtual void VirtualSlot134() = 0;  // added by b051x
    virtual void VirtualSlot138() = 0;  // added by b051x
    virtual void VirtualSlot13c() = 0;  // added by b051x
    virtual int getChild(int index, INodeInterface** child) = 0;  // 0x140  // added by b051x
    virtual int addChild(INodeInterface* child) = 0;  // 0x144  // added by b055x
    virtual int removeChild(INodeInterface* child) = 0;  // 0x148  // added by b055x
    // original VirtualSlot14c: a previously unobserved ray overload; Body0x106f9e proves the parameters.
    virtual int pick(int scope, const float* origin, const float* direction,
                     IQueryInterface* intersection, unsigned char* hit) = 0;
    virtual int pick(int scope, float x, float y, IQueryInterface* camera,
                     IQueryInterface* intersection, unsigned char* hit) = 0; // 0x150, Ballistics 0xf053c
};  // added by b051x

/** Mesh. */  // added by b051x
class IMeshInterface : public INodeInterface {  // added by b051x
public:  // added by b051x
    virtual void VirtualSlotec() = 0;  // added by b051x
    virtual void VirtualSlotf0() = 0;  // added by b051x
    virtual void VirtualSlotf4() = 0;  // added by b051x
    virtual void VirtualSlotf8() = 0;  // added by b051x
    virtual void VirtualSlotfc() = 0;  // added by b051x
    virtual void VirtualSlot100() = 0;  // added by b051x
    virtual void VirtualSlot104() = 0;  // added by b051x
    virtual void VirtualSlot108() = 0;  // added by b051x
    virtual void VirtualSlot10c() = 0;  // added by b051x
    virtual void VirtualSlot110() = 0;  // added by b051x
    virtual void VirtualSlot114() = 0;  // added by b051x
    virtual void VirtualSlot118() = 0;  // added by b051x
    virtual void VirtualSlot11c() = 0;  // added by b051x
    virtual void VirtualSlot120() = 0;  // added by b051x
    virtual void VirtualSlot124() = 0;  // added by b051x
    virtual void VirtualSlot128() = 0;  // added by b051x
    virtual void VirtualSlot12c() = 0;  // added by b051x
    virtual int getSubmeshCount(int* count) = 0;  // 0x130  // added by b051x
    virtual int getVertexBuffer(IVertexBufferInterface** vertexBuffer) = 0;  // 0x134  // added by b051x
    virtual void VirtualSlot138() = 0;  // added by b051x
    virtual void VirtualSlot13c() = 0;  // added by b051x
    virtual int getTriangleIndexBuffer(int index, IIndexBufferInterface** indexBuffer) = 0; // added by b051x (was VirtualSlot140; CCollision 0xf2486)
    virtual int getAppearance(int index, IAppearanceInterface** appearance) = 0;  // 0x144  // added by b051x
    /** Index buffer of submesh `index` (name guessed from the M3G Mesh API). */  // changed by b055x (was VirtualSlot148)
    // Historical name at 0x148 retained for existing callers; CCollision reads triangle indices at 0x140.
    virtual int getIndexBuffer(int index, IObject3DInterface** indexBuffer) = 0;  // 0x148  // changed by b055x
    virtual int setAppearance(int index, IAppearanceInterface* appearance) = 0;  // 0x14c  // added by b051x
};  // added by b051x

/** SkinnedMesh. */  // added by b051x
class ISkinnedMeshInterface : public IMeshInterface {  // added by b051x
public:  // added by b051x
    virtual void VirtualSlot150() = 0;  // added by b051x
    virtual void VirtualSlot154() = 0;  // added by b051x
    virtual void VirtualSlot158() = 0;  // added by b051x
    virtual void VirtualSlot15c() = 0;  // added by b051x
    virtual void VirtualSlot160() = 0;  // added by b051x
    virtual void VirtualSlot164() = 0;  // added by b051x
    virtual void VirtualSlot168() = 0;  // added by b051x
    virtual void VirtualSlot16c() = 0;  // added by b051x
    virtual void VirtualSlot170() = 0;  // added by b051x
    virtual void VirtualSlot174() = 0;  // added by b051x
    virtual void VirtualSlot178() = 0;  // added by b051x
    virtual int getSkeleton(IGroupInterface** skeleton) = 0;  // 0x17c  // added by b051x
};  // added by b051x

/** Appearance. */  // added by b051x
class IAppearanceInterface : public IObject3DInterface {  // added by b051x
public:  // added by b051x
    virtual void VirtualSlot64() = 0;  // added by b051x
    virtual void VirtualSlot68() = 0;  // added by b051x
    virtual void VirtualSlot6c() = 0;  // added by b051x
    virtual void VirtualSlot70() = 0;  // added by b051x
    virtual int getCompositingMode(IObject3DInterface** mode) = 0;  // 0x74  // changed by b055x
    virtual int setCompositingMode(IObject3DInterface* mode) = 0;  // 0x78  // changed by b055x
    virtual void VirtualSlot7c() = 0;  // added by b051x
    virtual void VirtualSlot80() = 0;  // added by b051x
    virtual void VirtualSlot84() = 0;  // added by b051x
    virtual void VirtualSlot88() = 0;  // added by b051x
    virtual int getMaterial(IObject3DInterface** material) = 0;  // 0x8c  // changed by b055x
    virtual void VirtualSlot90() = 0;  // added by b051x
    virtual int getFog(IObject3DInterface** fog) = 0; // original ffc92, was VirtualSlot94.  // added by b051x
    virtual int setFog(IObject3DInterface* fog) = 0; // original ffe52, was VirtualSlot98.  // added by b051x
    virtual void VirtualSlot9c() = 0;  // added by b051x
    virtual void VirtualSlota0() = 0;  // added by b051x
    virtual int getTexture(int index, ITexture2DInterface** texture) = 0;  // 0xa4  // added by b051x
    virtual int setTexture(int index, ITexture2DInterface* texture) = 0;  // 0xa8  // added by b051x
};  // added by b051x

/** Texture2D (no method used yet). */  // added by b051x
class ITexture2DInterface : public ITransformableInterface {  // added by b051x
public:  // added by b051x
    virtual void VirtualSlotb0() = 0;  // added by b048
    virtual void VirtualSlotb4() = 0;  // added by b048
    virtual void VirtualSlotb8() = 0;  // added by b048
    virtual void VirtualSlotbc() = 0;  // added by b048
    virtual void VirtualSlotc0() = 0;  // added by b048
    virtual void VirtualSlotc4() = 0;  // added by b048
    virtual void VirtualSlotc8() = 0;  // added by b048
    virtual void VirtualSlotcc() = 0;  // added by b048
    virtual void VirtualSlotd0() = 0;  // added by b048
    virtual void VirtualSlotd4() = 0;  // added by b048
    virtual int setImage(IImage2DInterface* image) = 0;  // 0xd8  // added by b048
};  // added by b051x

/** Image2D (no method used yet). */  // added by b048
class IImage2DInterface : public IObject3DInterface {  // added by b048
public:  // added by b048
};  // added by b048

/** Camera: projection parameters (fovy, aspect ratio, near, far). */  // added by b048
class ICameraInterface : public INodeInterface {  // added by b048
public:  // added by b048
    virtual int setAlignment(INodeInterface* zReference, int zTarget, INodeInterface* yReference, int yTarget) = 0;  // changed by b049 (was VirtualSlotec)  // added by b048
    virtual void VirtualSlotf0() = 0;  // added by b048
    virtual void VirtualSlotf4() = 0;  // added by b048
    virtual int align(INodeInterface* reference) = 0;  // changed by b049 (was VirtualSlotf8)  // added by b048
    virtual void VirtualSlotfc() = 0;  // added by b048
    virtual void VirtualSlot100() = 0;  // added by b048
    virtual void VirtualSlot104() = 0;  // added by b048
    virtual void VirtualSlot108() = 0;  // added by b048
    virtual void VirtualSlot10c() = 0;  // added by b048
    virtual void VirtualSlot110() = 0;  // added by b048
    virtual void VirtualSlot114() = 0;  // added by b048
    virtual void VirtualSlot118() = 0;  // added by b048
    virtual void VirtualSlot11c() = 0;  // added by b048
    virtual void VirtualSlot120() = 0;  // added by b048
    virtual int getProjection(int count, float* params, int* projectionType) = 0;  // 0x124  // added by b048
    virtual int getProjection(ITransformInterface* transform, int* projectionType) = 0;  // changed by b049 (was VirtualSlot128)  // added by b048
    virtual void VirtualSlot12c() = 0;  // added by b048
    virtual int setParallel(float fovy, float aspectRatio, float nearClip, float farClip) = 0;  // 0x130  // changed by b047x (was VirtualSlot130)
    virtual int setPerspective(float fovy, float aspectRatio, float nearClip, float farClip) = 0;  // 0x134  // added by b048
};  // added by b048

class IBackgroundInterface;  // added by b055x

/** World: a group with an active camera and a background (names follow the M3G API). */  // added by b055x
class IWorldInterface : public IGroupInterface {  // added by b055x
public:  // added by b055x
    // b116: slots 0x14c / 0x150 are IGroupInterface's two pick() overloads (added there later); the
    // placeholders VirtualSlot14c / VirtualSlot150 that stood here duplicated them and pushed every
    // World method 8 bytes down (setActiveCamera called slot 0x168, a NULL entry: crash entering a level).
    virtual void VirtualSlot154() = 0;  // added by b055x
    virtual void VirtualSlot158() = 0;  // added by b055x
    virtual int getActiveCamera(ICameraInterface** camera) = 0;  // 0x15c  // changed by b055x
    virtual int setActiveCamera(ICameraInterface* camera) = 0;  // 0x160  // added by b055x
    virtual int getBackground(IBackgroundInterface** background) = 0;  // 0x164  // added by b055x
};  // added by b055x

/** Background (only the color clear switch is used). */  // added by b055x
class IBackgroundInterface : public IObject3DInterface {  // added by b055x
public:  // added by b055x
    virtual void VirtualSlot64() = 0;  // added by b055x
    virtual void VirtualSlot68() = 0;  // added by b055x
    virtual void VirtualSlot6c() = 0;  // added by b055x
    virtual void VirtualSlot70() = 0;  // added by b055x
    virtual void VirtualSlot74() = 0;  // added by b055x
    virtual void VirtualSlot78() = 0;  // added by b055x
    virtual void VirtualSlot7c() = 0;  // added by b055x
    virtual void VirtualSlot80() = 0;  // added by b055x
    virtual int getCropX(int* value) = 0;  // changed by b049 (was VirtualSlot84)  // added by b055x
    virtual int getCropY(int* value) = 0;  // changed by b049 (was VirtualSlot88)  // added by b055x
    virtual int getCropWidth(int* value) = 0;  // changed by b049 (was VirtualSlot8c)  // added by b055x
    virtual int getCropHeight(int* value) = 0;  // changed by b049 (was VirtualSlot90)  // added by b055x
    virtual void VirtualSlot94() = 0;  // added by b055x
    virtual int setColorClearEnable(bool enable) = 0;  // 0x98  // added by b055x
    virtual void VirtualSlot9c() = 0; // Real slot gap not yet used in Background.
    virtual void VirtualSlota0() = 0;
    virtual void VirtualSlota4() = 0;
    virtual int setDepthClearEnable(bool enable) = 0; // The original Game constructor reads slot a8 at f8bec and calls it at f8bf2.
};  // added by b055x

/** Light: mode, color (0xAARRGGBB), intensity. */  // added by b055x
class ILightInterface : public INodeInterface {  // added by b055x
public:  // added by b055x
    virtual void VirtualSlotec() = 0;  // added by b055x
    virtual void VirtualSlotf0() = 0;  // added by b055x
    virtual void VirtualSlotf4() = 0;  // added by b055x
    virtual void VirtualSlotf8() = 0;  // added by b055x
    virtual void VirtualSlotfc() = 0;  // added by b055x
    virtual void VirtualSlot100() = 0;  // added by b055x
    virtual void VirtualSlot104() = 0;  // added by b055x
    virtual void VirtualSlot108() = 0;  // added by b055x
    virtual void VirtualSlot10c() = 0;  // added by b055x
    virtual void VirtualSlot110() = 0;  // added by b055x
    virtual void VirtualSlot114() = 0;  // added by b055x
    virtual void VirtualSlot118() = 0;  // added by b055x
    virtual void VirtualSlot11c() = 0;  // added by b055x
    virtual void VirtualSlot120() = 0;  // added by b055x
    virtual void VirtualSlot124() = 0;  // added by b055x
    virtual int setMode(int mode) = 0;  // 0x128  // added by b055x
    virtual void VirtualSlot12c() = 0;  // added by b055x
    virtual int setColor(int color) = 0;  // 0x130  // added by b055x
    virtual void VirtualSlot134() = 0;  // added by b055x
    virtual int setIntensity(float intensity) = 0;  // 0x138  // added by b055x
};  // added by b055x

/** VertexBuffer: positions with scale (scaleBias[0]) and bias (scaleBias[1..3]). */  // added by b051x
/** CompositingMode (only the depth switches are used). */  // added by b055x
class ICompositingModeInterface : public IObject3DInterface {  // added by b055x
public:  // added by b055x
    virtual void VirtualSlot64() = 0;  // added by b055x
    virtual void VirtualSlot68() = 0;  // added by b055x
    virtual void VirtualSlot6c() = 0;  // added by b055x
    virtual void VirtualSlot70() = 0;  // added by b055x
    virtual void VirtualSlot74() = 0;  // added by b055x
    virtual void VirtualSlot78() = 0;  // added by b055x
    virtual void VirtualSlot7c() = 0;  // added by b055x
    virtual void VirtualSlot80() = 0;  // added by b055x
    virtual void VirtualSlot84() = 0;  // added by b055x
    virtual void VirtualSlot88() = 0;  // added by b055x
    virtual void VirtualSlot8c() = 0;  // added by b055x
    virtual void VirtualSlot90() = 0;  // added by b055x
    virtual void VirtualSlot94() = 0;  // added by b055x
    virtual void VirtualSlot98() = 0;  // added by b055x
    virtual void VirtualSlot9c() = 0;  // added by b055x
    virtual void VirtualSlota0() = 0;  // added by b055x
    virtual void VirtualSlota4() = 0;  // added by b055x
    virtual int setDepthTestEnable(bool enable) = 0;  // 0xa8  // added by b055x
    virtual void VirtualSlotac() = 0;  // added by b055x
    virtual int setDepthWriteEnable(bool enable) = 0;  // 0xb0  // added by b055x
};  // added by b055x
  // added by b055x
/** Material (only getColor is used). */  // added by b055x
class IMaterialInterface : public IObject3DInterface {  // added by b055x
public:  // added by b055x
    virtual void VirtualSlot64() = 0;  // added by b055x
    virtual void VirtualSlot68() = 0;  // added by b055x
    virtual void VirtualSlot6c() = 0;  // added by b055x
    virtual void VirtualSlot70() = 0;  // added by b055x
    virtual void VirtualSlot74() = 0;  // added by b055x
    virtual void VirtualSlot78() = 0;  // added by b055x
    /** Color of `target` (AMBIENT 0x400, DIFFUSE 0x800, ...) as 0xAARRGGBB. */  // added by b055x
    virtual int getColor(int target, int* color) = 0;  // 0x7c  // added by b055x
};  // added by b055x
class IVertexBufferInterface : public IObject3DInterface {  // added by b051x
public:  // added by b051x
    virtual void VirtualSlot64() = 0;  // added by b051x
    virtual void VirtualSlot68() = 0;  // added by b051x
    virtual void VirtualSlot6c() = 0;  // added by b051x
    virtual void VirtualSlot70() = 0;  // added by b051x
    virtual void VirtualSlot74() = 0;  // added by b051x
    virtual void VirtualSlot78() = 0;  // added by b051x
    virtual void VirtualSlot7c() = 0;  // added by b051x
    virtual void VirtualSlot80() = 0;  // added by b051x
    virtual void VirtualSlot84() = 0;  // added by b051x
    virtual void VirtualSlot88() = 0;  // added by b051x
    virtual void VirtualSlot8c() = 0;  // added by b051x
    virtual void VirtualSlot90() = 0;  // added by b051x
    virtual int getVertexCount(int* count) = 0;  // 0x94  // changed by b048 (was VirtualSlot94)
    virtual void VirtualSlot98() = 0;  // added by b051x
    virtual void VirtualSlot9c() = 0;  // added by b051x
    virtual void VirtualSlota0() = 0;  // added by b051x
    virtual int getPositions(int scaleBiasCount, float* scaleBias, IVertexArrayInterface** positions) = 0;  // 0xa4  // added by b051x
};  // added by b051x

/** VertexArray (16 bit components). */  // added by b051x
class IVertexArrayInterface : public IObject3DInterface {  // added by b051x
public:  // added by b051x
    virtual void VirtualSlot64() = 0;  // added by b051x
    virtual void VirtualSlot68() = 0;  // added by b051x
    virtual void VirtualSlot6c() = 0;  // added by b051x
    virtual int getVertexCount(int* count) = 0;  // 0x70  // added by b051x
    virtual void VirtualSlot74() = 0;  // added by b051x
    virtual void VirtualSlot78() = 0;  // added by b051x
    virtual int get(int firstVertex, int vertexCount, int valueCount, short* values) = 0;  // 0x7c  // added by b051x
};  // added by b051x

/** Triangle index stream, observed in CCollision CollectMeshInfo/BuildMesh. */
class IIndexBufferInterface : public IObject3DInterface {
public:
    virtual void VirtualSlot64() = 0;
    virtual void VirtualSlot68() = 0;
    virtual void VirtualSlot6c() = 0;
    virtual void VirtualSlot70() = 0;
    virtual void VirtualSlot74() = 0;
    virtual void VirtualSlot78() = 0;
    virtual void VirtualSlot7c() = 0;
    virtual int getIndexCount(int* count) = 0; // 0x80
    virtual int getIndices(int count, unsigned int* indices) = 0; // 0x84
};

/** AnimationController (times in milliseconds). */  // added by b051x
class IAnimationControllerInterface : public IObject3DInterface {  // added by b051x
public:  // added by b051x
    virtual void VirtualSlot64() = 0;  // added by b051x
    virtual void VirtualSlot68() = 0;  // added by b051x
    virtual int getWeight(float* weight) = 0;  // 0x6c  // added by b051x
    virtual int setWeight(float weight) = 0;  // 0x70  // added by b051x
    virtual void VirtualSlot74() = 0;  // added by b051x
    virtual void VirtualSlot78() = 0;  // added by b051x
    virtual int getSpeed(float* speed) = 0;  // 0x7c  // added by b051x
    virtual void VirtualSlot80() = 0;  // added by b051x
    virtual void VirtualSlot84() = 0;  // added by b051x
    virtual void VirtualSlot88() = 0;  // added by b051x
    virtual int getPosition(int worldTime, float* sequenceTime) = 0;  // 0x8c  // added by b051x
    virtual int setPosition(float sequenceTime, int worldTime) = 0;  // 0x90  // added by b051x
    virtual void VirtualSlot94() = 0;  // added by b051x
    virtual int setSpeed(float speed, int worldTime) = 0;  // 0x98  // added by b051x
};  // added by b051x

/** AnimationTrack. */  // added by b051x
class IAnimationTrackInterface : public IObject3DInterface {  // added by b051x
public:  // added by b051x
    virtual void VirtualSlot64() = 0;  // added by b051x
    virtual void VirtualSlot68() = 0;  // added by b051x
    virtual void VirtualSlot6c() = 0;  // added by b051x
    virtual void VirtualSlot70() = 0;  // added by b051x
    virtual int getController(IAnimationControllerInterface** controller) = 0;  // 0x74  // added by b051x
    virtual int setController(IAnimationControllerInterface* controller) = 0;  // 0x78  // added by b051x
    virtual int getKeyframeSequence(IKeyframeSequenceInterface** sequence) = 0;  // 0x7c  // added by b051x
};  // added by b051x

/** KeyframeSequence. */  // added by b051x
class IKeyframeSequenceInterface : public IObject3DInterface {  // added by b051x
public:  // added by b051x
    virtual void VirtualSlot64() = 0;  // added by b051x
    virtual void VirtualSlot68() = 0;  // added by b051x
    virtual void VirtualSlot6c() = 0;  // added by b051x
    virtual void VirtualSlot70() = 0;  // added by b051x
    virtual int getDuration(int* duration) = 0;  // 0x74  // added by b051x
    virtual void VirtualSlot78() = 0;  // added by b051x
    virtual int getRepeatMode(int* repeatMode) = 0;  // 0x7c  // added by b051x
};  // added by b051x

class AnimationTrack;  // added by b051x
class Image2D;  // added by b048
class Background;  // added by b055x
class Camera;  // added by b055x
class RayIntersection;
class CompositingMode;  // added by b055x
class Material;  // added by b055x


/** Any scene graph object. */
class Object3D : public QueryInterface {
public:
    using QueryInterface::operator=;

    Object3D()  // added by b051x
    {  // added by b051x
    }  // added by b051x

    /** Take over the reference held by `object`. */  // added by b051x
    explicit Object3D(IQueryInterface* object)  // added by b051x
        : QueryInterface(object)  // added by b051x
    {  // added by b051x
    }  // added by b051x

    int getUserID() const  // added by b051x
    {  // added by b051x
        int userID;  // added by b051x
        typedef int (*GetUserId)(IQueryInterface*, int*);
        CKMethod<GetUserId>(this->m_interface, 0x0c)(this->m_interface, &userID);
        return userID;  // added by b051x
    }  // added by b051x

    void setUserID(int userID)  // added by b051x
    {  // added by b051x
        typedef int (*SetUserId)(IQueryInterface*, int);
        CKMethod<SetUserId>(this->m_interface, 0x10)(this->m_interface, userID);
    }  // added by b051x

    int getAnimationTrackCount() const  // added by b051x
    {  // added by b051x
        int count;  // added by b051x
        typedef int (*GetCount)(IQueryInterface*, int*);
        CKMethod<GetCount>(this->m_interface, 0x14)(this->m_interface, &count);
        return count;  // added by b051x
    }  // added by b051x

    AnimationTrack getAnimationTrack(int index) const;  // body after the classes (non-template wrappers)

    void addAnimationTrack(const AnimationTrack& track);  // body after the classes (non-template wrappers)

    void removeAnimationTrack(const AnimationTrack& track);  // body after the classes (non-template wrappers)

    /** The descendant with user id `userID` (NULL handle when there is none). */  // added by b051x
    Object3D find(int userID) const;  // body after the classes (non-template wrappers)

    /**  // added by b051x
     * Objects directly referenced by this one: the count; the first `capacity` of them are stored in  // added by b051x
     * `references` when it is not NULL.  // added by b051x
     */  // added by b051x
    unsigned int getReferences(unsigned int capacity, Object3D* references);  // added by b051x

    /** Copy user parameter `id` into `buffer` (bufferSize bytes); returns its length. */  // added by b048
    int getUserParameter(int id, int bufferSize, char* buffer) const  // added by b048
    {  // added by b048
        int length;  // added by b048; original leaves the output uninitialised
        typedef int (*GetUserParameter)(IQueryInterface*, int, int, char*, int*);
        CKMethod<GetUserParameter>(this->m_interface, 0x34)(this->m_interface, id, bufferSize, buffer, &length);
        return length;  // added by b048
    }  // added by b048

    Object3D duplicate() const  // added by b051x
    {  // added by b051x
        IObject3DInterface*copy;  // added by b051x
        typedef int (*Duplicate)(IQueryInterface*, IObject3DInterface**);
        CKMethod<Duplicate>(this->m_interface, 0x48)(this->m_interface, &copy);
        return Object3D(copy);  // added by b051x
    }  // added by b051x

    /** Animate this object and its descendants to world time `worldTime` (ms); returns the validity. */  // added by b055x
    int animate(int worldTime)  // added by b055x
    {  // added by b055x
        int validity;  // added by b055x; original leaves the output uninitialised
        typedef int (*Animate)(IQueryInterface*, int, int*);
        CKMethod<Animate>(this->m_interface, 0x24)(this->m_interface, worldTime, &validity);
        return validity;  // added by b055x
    }  // added by b055x

    /** Same runtime object (compared through the runtime's object identity). */  // added by b047x
    int operator==(const Object3D& other) const;  // added by b047x; changed by b055x: int (CDH_Animation tests the result with cbz, a bool gives cmp #1)
    int operator!=(const Object3D& other) const;  // added by b048

protected:  // added by b051x
    IObject3DInterface* object3D() const  // added by b051x
    {  // added by b051x
        return static_cast<IObject3DInterface*>(this->m_interface);  // added by b051x
    }  // added by b051x

};


/** Renderer object behind Graphics3D; CK draws nodes at 0x58 and worlds at 0x5c. */
class IGraphics3DRenderer {
public:
    virtual void VirtualSlot00() = 0;
    virtual void VirtualSlot04() = 0;
    virtual void VirtualSlot08() = 0;
    virtual void VirtualSlot0c() = 0;
    virtual void VirtualSlot10() = 0;
    virtual void VirtualSlot14() = 0;
    virtual void VirtualSlot18() = 0;
    virtual void VirtualSlot1c() = 0;
    virtual void VirtualSlot20() = 0;
    virtual void VirtualSlot24() = 0;
    virtual void VirtualSlot28() = 0;
    virtual void VirtualSlot2c() = 0;
    virtual void VirtualSlot30() = 0;
    virtual void VirtualSlot34() = 0;  // 0x34 is _graphics3d_setCroppedBackBufferData  // changed by fx2
    virtual void VirtualSlot38() = 0;
    virtual void VirtualSlot3c() = 0;
    virtual void VirtualSlot40() = 0;
    virtual void VirtualSlot44() = 0;
    virtual int SetViewport(int x, int y, int width, int height) = 0;  // 0x48
    virtual void VirtualSlot4c() = 0;
    virtual void VirtualSlot50() = 0;
    virtual void VirtualSlot54() = 0;
    virtual void RenderNode(IQueryInterface* node, IQueryInterface* transform) = 0;  // 0x58
    virtual void Render(IQueryInterface* world) = 0;
};

/** The renderer: a plain handle on CSwerve's CSwerveGraphics3D (no reference counting, built on the stack  // changed by fx2
 *  by Window::RunPaint3D). Replaces the former Graphics3DState view of the same object. */  // changed by fx2
class World;
class Node;
class Transform;

class Graphics3D {
public:
    static Graphics3D getInstance()
    {
        return Graphics3D(CSwerve::GetInstance()->m_graphics3D);
    }

    explicit Graphics3D(CSwerveGraphics3D* graphics)  // added by fx2
    {  // added by fx2
        m_graphics = graphics;  // added by fx2
    }  // added by fx2

    // The original returns this wrapper through a caller-provided object.
    Graphics3D(const Graphics3D& other)
        : m_graphics(other.m_graphics)
    {
    }

    /** The runtime renderer seen through its C++ interface (the IGraphics3D C object, cf. tws_loader.h). */  // added by fx2
    IGraphics3DRenderer* renderer() const  // added by fx2
    {  // added by fx2
        return (IGraphics3DRenderer*)m_graphics->m_graphics3D;  // added by fx2
    }  // added by fx2

    /** True while a render surface or a mutable image is bound. */  // added by fx2
    bool hasTarget() const  // added by fx2
    {  // added by fx2
        return m_graphics->m_target != NULL || m_graphics->m_boundImage != NULL;  // added by fx2
    }  // added by fx2

    /** Viewport inside the bound target (renderer slot 0x48). */  // added by fx2
    void setViewport(int x, int y, int width, int height)  // added by fx2
    {  // added by fx2
        renderer()->SetViewport(x, y, width, height);  // added by fx2
    }  // added by fx2

    /** Render `world` when a target is bound (original: weak, not inlined; body after the classes). */  // added by b055x
    void render(const World& world) const;
    /** Render a node in a supplied transform (Ballistics, renderer slot 0x58; weak, not inlined). */
    void render(const Node& node, const Transform& transform) const;
    CSwerveGraphics3D* m_graphics;   // 0x00  // changed by fx2 (was Graphics3DState* m_state)
};

inline unsigned int Object3D::getReferences(unsigned int capacity, Object3D* references)  // added by b051x
{  // added by b051x
    int count = 0;  // added by b051x
    IObject3DInterface** found = NULL;  // added by b051x
    if (capacity != 0) {  // added by b051x
        found = new IObject3DInterface*[capacity];  // added by b051x
    }  // added by b051x
    typedef int (*GetReferences)(IQueryInterface*, unsigned int, IObject3DInterface**, int*);
    CKMethod<GetReferences>(this->m_interface, 0x30)(this->m_interface, capacity, found, &count);
    if (references != NULL && capacity > 0 && found != NULL) {  // added by b051x
        for (unsigned int index = 0; index < capacity; index++) {  // added by b051x
            references[index] = Object3D(found[index]);  // added by b051x
        }  // added by b051x
        delete[] found;  // added by b051x
    }  // added by b051x
    return count;  // added by b051x
}  // added by b051x

/** 4x4 matrix (not an Object3D). */  // added by b051x
class Transform : public QueryInterface {  // added by b051x
public:  // added by b051x
    Transform()  // added by b051x
    {  // added by b051x
    }  // added by b051x

    explicit Transform(IQueryInterface* object)  // added by b051x
        : QueryInterface(object)  // added by b051x
    {  // added by b051x
    }  // added by b051x

    /** New identity transform created by `runtime`. */  // added by b048
    explicit Transform(ISwerve* runtime)  // added by b048
    {  // added by b048
        runtime->m_functions->CreateObject(0x1a, (void**)&this->m_interface);  // added by b048
    }  // added by b048

    /** The 16 floats, row major. */  // added by b051x
    void get(float* matrix) const  // added by b051x
    {  // added by b051x
        transformInterface()->get(16, matrix);  // added by b051x
    }  // added by b051x

    /** Replace the 16 floats (row major). */  // added by b047x
    void set(const float* matrix)  // added by b047x
    {  // added by b047x
        transformInterface()->set(16, matrix);  // added by b047x
    }  // added by b047x

    /** Multiply by a rotation; `angleAxis`: angle (degrees), axis x, y, z. */  // added by b055x
    void postRotate(const float* angleAxis)  // added by b055x
    {  // added by b055x
        transformInterface()->postRotateQuat(angleAxis);  // added by b055x
    }  // added by b055x

    /** Multiply by a translation (x, y, z). */  // added by b055x
    void postTranslate(const float* translation)  // added by b055x
    {  // added by b055x
        transformInterface()->postTranslate(translation);  // added by b055x
    }  // added by b055x

    /** Multiply by scale (x, y, z), slot 0x30 in Visual LoadModel. */  // added by b079e
    void postScale(const float* scale)  // added by b079e
    {
        transformInterface()->postScale(scale);
    }

    /** this = `transform` * this (Transform interface slot 0x18). */
    void multiply(const Transform& transform)
    {
        transformInterface()->multiply(transform.transformInterface());
    }

    /** this = this * `transform`. */  // added by b055x
    void postMultiply(const Transform& transform)  // added by b055x
    {  // added by b055x
        transformInterface()->postMultiply(transform.transformInterface());  // added by b055x
    }  // added by b055x

    /** Transform the positions of `vertices` into `out` (outCount floats). */  // added by b048
    void transform(IVertexArrayInterface* vertices, int outCount, float* out, bool w) const  // added by b048
    {  // added by b048
        transformInterface()->transform(vertices, outCount, out, w);  // added by b048
    }  // added by b048

    /** Transform `count` floats in place (4 per vector: x, y, z, w). */  // added by b051x
    void transform(int count, float* vectors) const  // added by b051x
    {  // added by b051x
        transformInterface()->transform(count, vectors);  // added by b051x
    }  // added by b051x

    /** Reset / invert the scratch matrix (dgCamera.cpp slots 0x1c / 0x20). */
    void setIdentity()  // added by b049
    {
        transformInterface()->setIdentity();
    }
    void invert()  // added by b049
    {
        transformInterface()->invert();
    }

    ITransformInterface* transformInterface() const  // added by b051x
    {  // added by b051x
        return static_cast<ITransformInterface*>(this->m_interface);  // added by b051x
    }  // added by b051x
};  // added by b051x

class Transformable : public Object3D {  // added by b051x
public:  // added by b051x
    Transformable()  // added by b051x
    {  // added by b051x
    }  // added by b051x

    explicit Transformable(IQueryInterface* object)  // added by b051x
        : Object3D(object)  // added by b051x
    {  // added by b051x
    }  // added by b051x

    /** Interface 0x1b (Transformable) of `other`, NULL when unsupported. */  // added by b047x
    Transformable(const Object3D& other)  // added by b047x
    {  // added by b047x
        if (other.m_interface != NULL) {  // added by b047x
            other.m_interface->QueryInterface(0x1b, (void**)&this->m_interface);  // added by b047x
        }  // added by b047x
    }  // added by b047x

    void getTranslation(float* translation) const  // added by b047x
    {  // added by b047x
        typedef int (*GetTranslation)(IQueryInterface*, float*);
        CKMethod<GetTranslation>(this->m_interface, 0x4c)(this->m_interface, translation);
    }  // added by b047x

    void getScale(float* scale) const  // added by b047x
    {  // added by b047x
        typedef int (*GetScale)(IQueryInterface*, float*);
        CKMethod<GetScale>(this->m_interface, 0x54)(this->m_interface, scale);
    }  // added by b047x

    void getOrientation(float* angleAxis) const  // added by b047x
    {  // added by b047x
        typedef int (*GetOrientation)(IQueryInterface*, float*);
        CKMethod<GetOrientation>(this->m_interface, 0x5c)(this->m_interface, angleAxis);
    }  // added by b047x

    /** `translation`: x, y, z. */  // added by b051x
    void setTranslation(const float* translation)  // added by b051x
    {  // added by b051x
        typedef int (*SetTranslation)(IQueryInterface*, const float*);
        CKMethod<SetTranslation>(this->m_interface, 0x50)(this->m_interface, translation);
    }  // added by b051x
    /** The original Body keeps the +0x84 call whose output is unused; the concrete rotation representation is still to be proven. */
    void getOrientation84(float* orientation) const
    {
        transformable()->getOrientation84(orientation);
    }

    /** Add a translation through the original slot 0x8c. */
    void translate(const float* translation)
    {
        transformable()->translate(translation);
    }

    /** Add an orientation rotation through the original Transformable slot 0x98. */
    void postRotate(const float* angleAxis)
    {
        transformable()->postRotate(angleAxis);
    }

    /** `scale`: x, y, z. */  // added by b051x
    void setScale(const float* scale)  // added by b051x
    {  // added by b051x
        typedef int (*SetScale)(IQueryInterface*, const float*);
        CKMethod<SetScale>(this->m_interface, 0x58)(this->m_interface, scale);
    }  // added by b051x

    /** `angleAxis`: angle (degrees), axis x, y, z. */  // added by b051x
    void setOrientation(const float* angleAxis)  // added by b051x
    {  // added by b051x
        typedef int (*SetOrientation)(IQueryInterface*, const float*);
        CKMethod<SetOrientation>(this->m_interface, 0x60)(this->m_interface, angleAxis);
    }  // added by b051x

    void setTransform(const Transform& transform)  // added by b051x
    {  // added by b051x
        // CK Transformable::setTransform is 0x7c.
        typedef int (*SetTransform)(IQueryInterface*, ITransformInterface*);
        CKMethod<SetTransform>(this->m_interface, 0x7c)(this->m_interface, transform.transformInterface());
    }  // added by b051x

    /** Fill `transform` with the composite transform of this object. */  // added by b055x
    void getCompositeTransform(Transform transform) const  // added by b055x
    {  // added by b055x
        // CK Transformable::getCompositeTransform is 0x74.
        typedef int (*GetTransform)(IQueryInterface*, ITransformInterface*);
        CKMethod<GetTransform>(this->m_interface, 0x74)(this->m_interface, transform.transformInterface());
    }  // added by b055x

protected:  // added by b051x
    ITransformableInterface* transformable() const  // added by b051x
    {  // added by b051x
        return static_cast<ITransformableInterface*>(this->m_interface);  // added by b051x
    }  // added by b051x
};  // added by b051x

class Node : public Transformable {  // added by b051x
public:  // added by b051x
    enum { InterfaceId = 0x12 };  // added by b051x

    Node()  // added by b051x
    {  // added by b051x
    }  // added by b051x

    explicit Node(IQueryInterface* object)  // added by b051x
        : Transformable(object)  // added by b051x
    {  // added by b051x
    }  // added by b051x

    Node(const Object3D& other)  // added by b051x
    {  // added by b051x
        // interface InterfaceId of `other` (stays NULL when unsupported)  // added by b051x
        if (other.m_interface != NULL) {  // added by b051x
            other.m_interface->QueryInterface(InterfaceId, (void**)&this->m_interface);  // added by b051x
        }  // added by b051x
    }  // added by b051x

    /** Fill `transform` with the transformation from this node to `target`; false when they are not connected. */  // added by b051x
    bool getTransformTo(const Node& target, Transform transform) const  // added by b051x
    {  // added by b051x
        bool result = false;  // added by b051x
        // CK Node::getTransformTo is 0xa4.
        typedef int (*GetTransformTo)(IQueryInterface*, INodeInterface*, ITransformInterface*, bool*);
        CKMethod<GetTransformTo>(this->m_interface, 0xa4)(this->m_interface, target.node(),
                                                         transform.transformInterface(), &result);
        return result;  // added by b051x
    }  // added by b051x

    /** The parent node (NULL handle when none). */  // added by b047x
    Node getParent() const  // added by b047x
    {  // added by b047x
        INodeInterface*parent;  // added by b047x
        typedef int (*GetParent)(IQueryInterface*, INodeInterface**);
        CKMethod<GetParent>(this->m_interface, 0x80)(this->m_interface, &parent);
        return Node(parent);  // added by b047x
    }  // added by b047x

    void setAlphaFactor(float alpha)  // added by b047x
    {  // added by b047x
        typedef int (*SetAlpha)(IQueryInterface*, float);
        CKMethod<SetAlpha>(this->m_interface, 0x88)(this->m_interface, alpha);
    }  // added by b047x

    void setRenderingEnable(bool enable)  // added by b047x
    {  // added by b047x
        typedef int (*SetEnabled)(IQueryInterface*, bool);
        CKMethod<SetEnabled>(this->m_interface, 0x90)(this->m_interface, enable);
    }  // added by b047x

    void setPickingEnable(bool enable)  // added by b055x
    {  // added by b055x
        typedef int (*SetEnabled)(IQueryInterface*, bool);
        CKMethod<SetEnabled>(this->m_interface, 0x98)(this->m_interface, enable);
    }  // added by b055x
    unsigned char getRenderingEnable() const
    {
        // The original caller keeps the full output byte and only normalizes it as non-zero at the final use.
        unsigned char enabled = 0;
        node()->getRenderingEnable(&enabled);
        return enabled;
    }
    unsigned char getPickingEnable() const
    {
        // The original caller keeps the full output byte and only normalizes it as non-zero at the final use.
        unsigned char enabled = 0;
        node()->getPickingEnable(&enabled);
        return enabled;
    }
    void setScope(int scope)  // added by b055x
    {  // added by b055x
        node()->setScope(scope);  // added by b055x
    }  // added by b055x

    INodeInterface* node() const  // added by b051x
    {  // added by b051x
        return static_cast<INodeInterface*>(this->m_interface);  // added by b051x
    }  // added by b051x
};  // added by b051x

/** Sprite3D (interface 0x17; only the Node part is used). */  // added by b047x
class Sprite3D : public Node {  // added by b047x
public:  // added by b047x
    enum { InterfaceId = 0x17 };  // added by b047x

    Sprite3D(const Object3D& other)  // added by b047x
    {  // added by b047x
        if (other.m_interface != NULL) {  // added by b047x
            other.m_interface->QueryInterface(InterfaceId, (void**)&this->m_interface);  // added by b047x
        }  // added by b047x
    }  // added by b047x
};  // added by b047x

class Group : public Node {  // added by b051x
public:  // added by b051x
    enum { InterfaceId = 9 };  // added by b051x


    Group()  // added by b051x
    {  // added by b051x
    }  // added by b051x

    explicit Group(IQueryInterface* object)  // added by b051x
        : Node(object)  // added by b051x
    {  // added by b051x
    }  // added by b051x

    Group(const Object3D& other)  // added by b051x
    {  // added by b051x
        // interface InterfaceId of `other` (stays NULL when unsupported)  // added by b051x
        if (other.m_interface != NULL) {  // added by b051x
            other.m_interface->QueryInterface(InterfaceId, (void**)&this->m_interface);  // added by b051x
        }  // added by b051x
    }  // added by b051x

    /** New group (type 9). The temporary output slot is uninitialized at 0x168736. */
    explicit Group(ISwerve* runtime)  // added by b049
        : Node(CreateGroup(runtime))
    {
    }
    /** Factory helper name inferred; only the inlined allocation is present in the image. */
    static IQueryInterface* CreateGroup(ISwerve* runtime)  // added by b049
    {
        IQueryInterface* object;
        runtime->m_functions->CreateObject(9, (void**)&object);
        return object;
    }

    int getChildCount() const  // added by b051x
    {  // added by b051x
        int count;  // added by b051x
        typedef int (*GetCount)(IQueryInterface*, int*);
        CKMethod<GetCount>(this->m_interface, 0xb8)(this->m_interface, &count);
        return count;  // added by b051x
    }  // added by b051x

    Node getChild(int index) const  // added by b051x
    {  // added by b051x
        INodeInterface*child;  // added by b051x
        typedef int (*GetChild)(IQueryInterface*, int, INodeInterface**);
        CKMethod<GetChild>(this->m_interface, 0xbc)(this->m_interface, index, &child);
        return Node(child);  // added by b051x
    }  // added by b051x

    /** Add a child node (any handle holding a node interface). */  // added by b055x
    void addChild(const QueryInterface& child)  // added by b055x
    {  // added by b055x
        typedef int (*SetChild)(IQueryInterface*, INodeInterface*);
        CKMethod<SetChild>(this->m_interface, 0xc0)(this->m_interface,
                                                  static_cast<INodeInterface*>(child.m_interface));
    }  // added by b055x

    /** Detach a node. */  // added by b079e
    void removeChild(const QueryInterface& child)  // added by b079e
    {
        typedef int (*SetChild)(IQueryInterface*, INodeInterface*);
        CKMethod<SetChild>(this->m_interface, 0xc4)(this->m_interface,
                                                  static_cast<INodeInterface*>(child.m_interface));
    }

    /** Screen pick with the original by-value intersection handle (retained during the call). */
    unsigned char pick(int scope, float x, float y, const Camera& camera,
              RayIntersection intersection) const;
    unsigned char pick(int scope, const float* origin, const float* direction,
              RayIntersection intersection) const;

protected:  // added by b051x
    IGroupInterface* group() const  // added by b051x
    {  // added by b051x
        return static_cast<IGroupInterface*>(this->m_interface);  // added by b051x
    }  // added by b051x
};  // added by b051x

/** Scene root (a group; moved after Group and derived from it by b055x, was Object3D). */
class World : public Group {  // changed by b055x (was Object3D)
public:
    using Object3D::operator=;
    enum { InterfaceId = 0x1f };  // added by b048

    World()  // added by b048
    {  // added by b048
    }  // added by b048

    World(const Object3D& other)  // added by b048
    {  // added by b048
        // interface InterfaceId of `other` (stays NULL when unsupported)  // added by b048
        if (other.m_interface != NULL) {  // added by b048
            other.m_interface->QueryInterface(InterfaceId, (void**)&this->m_interface);  // added by b048
        }  // added by b048
    }  // added by b048

    /** Remove a child node (addChild comes from Group). */  // added by b055x
    void removeChild(const QueryInterface& child)  // added by b055x
    {  // added by b055x
        world()->removeChild(static_cast<INodeInterface*>(child.m_interface));  // added by b055x
    }  // added by b055x

    void setActiveCamera(const QueryInterface& camera)  // added by b055x
    {  // added by b055x
        typedef int (*SetCamera)(IQueryInterface*, ICameraInterface*);
        CKMethod<SetCamera>(this->m_interface, 0xd4)(this->m_interface,
                                                   static_cast<ICameraInterface*>(camera.m_interface));
    }  // added by b055x

    /** The background (a new reference). */  // added by b055x
    Camera getActiveCamera() const;  // body after the classes (non-template wrappers)
    Background getBackground() const;  // body after the classes (non-template wrappers)

    IWorldInterface* world() const  // added by b055x
    {  // added by b055x
        return static_cast<IWorldInterface*>(this->m_interface);  // added by b055x
    }  // added by b055x
};

class Texture2D : public Transformable {  // added by b051x
public:  // added by b051x
    enum { InterfaceId = 0x19 };  // added by b051x

    Texture2D()  // added by b051x
    {  // added by b051x
    }  // added by b051x

    explicit Texture2D(IQueryInterface* object)  // added by b051x
        : Transformable(object)  // added by b051x
    {  // added by b051x
    }  // added by b051x

    Texture2D(const Object3D& other)  // added by b051x
    {  // added by b051x
        // interface InterfaceId of `other` (stays NULL when unsupported)  // added by b051x
        if (other.m_interface != NULL) {  // added by b051x
            other.m_interface->QueryInterface(InterfaceId, (void**)&this->m_interface);  // added by b051x
        }  // added by b051x
    }  // added by b051x

    void setImage(const Image2D& image);  // body after the classes (non-template wrappers)

    ITexture2DInterface* texture2D() const  // added by b051x
    {  // added by b051x
        return static_cast<ITexture2DInterface*>(this->m_interface);  // added by b051x
    }  // added by b051x
};  // added by b051x

/** Game only passes and releases the Fog-owned handle; unproven IIDs or Fog-specific methods are not added. */
class Fog : public Object3D {
public:
    Fog() {}
    explicit Fog(IQueryInterface* object) : Object3D(object) {}
};

class Appearance : public Object3D {  // added by b051x
public:  // added by b051x
    enum { InterfaceId = 3 };  // added by b051x

    Appearance()  // added by b051x
    {  // added by b051x
    }  // added by b051x

    explicit Appearance(IQueryInterface* object)  // added by b051x
        : Object3D(object)  // added by b051x
    {  // added by b051x
    }  // added by b051x

    Appearance(const Object3D& other)  // added by b051x
    {  // added by b051x
        // interface InterfaceId of `other` (stays NULL when unsupported)  // added by b051x
        if (other.m_interface != NULL) {  // added by b051x
            other.m_interface->QueryInterface(InterfaceId, (void**)&this->m_interface);  // added by b051x
        }  // added by b051x
    }  // added by b051x

    Texture2D getTexture(int index) const  // added by b051x
    {  // added by b051x
        ITexture2DInterface*texture;  // added by b051x
        typedef int (*GetTexture)(IQueryInterface*, int, ITexture2DInterface**);
        CKMethod<GetTexture>(this->m_interface, 0x74)(this->m_interface, index, &texture);
        return Texture2D(texture);  // added by b051x
    }  // added by b051x

    void setTexture(int index, const Texture2D& texture)  // added by b051x
    {  // added by b051x
        typedef int (*SetTexture)(IQueryInterface*, int, ITexture2DInterface*);
        CKMethod<SetTexture>(this->m_interface, 0x78)(this->m_interface, index, texture.texture2D());
    }  // added by b051x

    CompositingMode getCompositingMode() const;  // body after the classes (non-template wrappers)
    void setCompositingMode(const CompositingMode& mode);  // body after the classes (non-template wrappers)
    Material getMaterial() const;  // body after the classes (non-template wrappers)
    IAppearanceInterface* appearance() const  // added by b051x
    {  // added by b051x
        return static_cast<IAppearanceInterface*>(this->m_interface);  // added by b051x
    }  // added by b051x
    Fog getFog() const
    {
        // CK's SetRandomCamera calls the appearance vtable at 0x5c.
        IObject3DInterface* fog = NULL;
        typedef int (*GetFog)(IQueryInterface*, IObject3DInterface**);
        CKMethod<GetFog>(this->m_interface, 0x5c)(this->m_interface, &fog);
        return Fog(fog);
    }
    void setFog(const Fog& fog)
    {
        // CK's ApplyFog calls the adjacent setter at 0x60.
        typedef int (*SetFog)(IQueryInterface*, IObject3DInterface*);
        CKMethod<SetFog>(this->m_interface, 0x60)(this->m_interface,
                                                static_cast<IObject3DInterface*>(fog.m_interface));
    }
};  // added by b051x

class VertexArray : public Object3D {  // added by b051x
public:  // added by b051x
    VertexArray()  // added by b051x
    {  // added by b051x
    }  // added by b051x

    explicit VertexArray(IQueryInterface* object)  // added by b051x
        : Object3D(object)  // added by b051x
    {  // added by b051x
    }  // added by b051x

    int getVertexCount() const  // added by b051x
    {  // added by b051x
        int count;  // added by b051x
        // original slot 0x50 (CCollision::BuildMesh 0xd7f5c)
        typedef int (*GetCount)(IQueryInterface*, int*);
        CKMethod<GetCount>(this->m_interface, 0x50)(this->m_interface, &count);
        return count;  // added by b051x
    }  // added by b051x

    /** Copy the components of `vertexCount` vertices from `firstVertex` (`valueCount` shorts). */  // added by b051x
    void get(int firstVertex, int vertexCount, int valueCount, short* values) const  // added by b051x
    {  // added by b051x
        // original slot 0x5c (CCollision::BuildMesh 0xd7f84)
        typedef int (*Get)(IQueryInterface*, int, int, int, short*);
        CKMethod<Get>(this->m_interface, 0x5c)(this->m_interface, firstVertex, vertexCount, valueCount, values);
    }  // added by b051x

protected:  // added by b051x
    IVertexArrayInterface* vertexArray() const  // added by b051x
    {  // added by b051x
        return static_cast<IVertexArrayInterface*>(this->m_interface);  // added by b051x
    }  // added by b051x
};  // added by b051x

class VertexBuffer : public Object3D {  // added by b051x
public:  // added by b051x
    VertexBuffer()  // added by b051x
    {  // added by b051x
    }  // added by b051x

    explicit VertexBuffer(IQueryInterface* object)  // added by b051x
        : Object3D(object)  // added by b051x
    {  // added by b051x
    }  // added by b051x

    /** The positions; `scaleBias` receives the scale and the x, y, z bias. */  // added by b051x
    int getVertexCount() const  // added by b048
    {  // added by b048
        int count;  // added by b048
        // original slot 0x64 (FrustumCulling::GetBoundingSphere 0x14c316)
        typedef int (*GetCount)(IQueryInterface*, int*);
        CKMethod<GetCount>(this->m_interface, 0x64)(this->m_interface, &count);
        return count;  // added by b048
    }  // added by b048

    VertexArray getPositions(float* scaleBias) const  // added by b051x
    {  // added by b051x
        IVertexArrayInterface*positions;  // added by b051x
        // original slot 0x68 (FrustumCulling::GetBoundingSphere 0x14c32a)
        typedef int (*GetPositions)(IQueryInterface*, int, float*, IVertexArrayInterface**);
        CKMethod<GetPositions>(this->m_interface, 0x68)(this->m_interface, 4, scaleBias, &positions);
        return VertexArray(positions);  // added by b051x
    }  // added by b051x

protected:  // added by b051x
    IVertexBufferInterface* vertexBuffer() const  // added by b051x
    {  // added by b051x
        return static_cast<IVertexBufferInterface*>(this->m_interface);  // added by b051x
    }  // added by b051x
};  // added by b051x

/** IndexBuffer (no method used; a distinct type so that it is copied into Object3D parameters). */  // added by b055x
class IndexBuffer : public Object3D {  // added by b055x
public:  // added by b055x
    explicit IndexBuffer(IQueryInterface* object)  // added by b055x
        : Object3D(object)  // added by b055x
    {  // added by b055x
    }  // added by b055x

    int getIndexCount() const
    {
        int count;
        // original slot 0x4c (CCollision::CollectMeshInfo 0xd7d70)
        typedef int (*GetCount)(IQueryInterface*, int*);
        CKMethod<GetCount>(this->m_interface, 0x4c)(this->m_interface, &count);
        return count;
    }

    void getIndices(int count, unsigned int* indices) const
    {
        // original slot 0x50 (CCollision::BuildMesh)
        typedef int (*GetIndices)(IQueryInterface*, int, unsigned int*);
        CKMethod<GetIndices>(this->m_interface, 0x50)(this->m_interface, count, indices);
    }
};  // added by b055x

class Mesh : public Node {  // added by b051x
public:  // added by b051x
    enum { InterfaceId = 0x10 };  // added by b051x

    Mesh()  // added by b051x
    {  // added by b051x
    }  // added by b051x

    explicit Mesh(IQueryInterface* object)  // added by b051x
        : Node(object)  // added by b051x
    {  // added by b051x
    }  // added by b051x

    Mesh(const Object3D& other)  // added by b051x
    {  // added by b051x
        // interface InterfaceId of `other` (stays NULL when unsupported)  // added by b051x
        if (other.m_interface != NULL) {  // added by b051x
            other.m_interface->QueryInterface(InterfaceId, (void**)&this->m_interface);  // added by b051x
        }  // added by b051x
    }  // added by b051x

    int getSubmeshCount() const  // added by b051x
    {  // added by b051x
        int count;  // added by b051x
        typedef int (*GetCount)(IQueryInterface*, int*);
        CKMethod<GetCount>(this->m_interface, 0xc0)(this->m_interface, &count);
        return count;  // added by b051x
    }  // added by b051x

    VertexBuffer getVertexBuffer() const  // added by b051x
    {  // added by b051x
        IVertexBufferInterface*vertexBuffer;  // added by b051x
        // original slot 0xc4 (CCollision::CollectMeshInfo 0xd7cfa)
        typedef int (*GetVertexBuffer)(IQueryInterface*, IVertexBufferInterface**);
        CKMethod<GetVertexBuffer>(this->m_interface, 0xc4)(this->m_interface, &vertexBuffer);
        return VertexBuffer(vertexBuffer);  // added by b051x
    }  // added by b051x

    /** Index buffer of submesh `index`. */  // added by b055x
    IndexBuffer getTriangleIndexBuffer(int index) const
    {
        IIndexBufferInterface*indexBuffer;
        // original slot 0xc8 (CCollision::CollectMeshInfo 0xd7d5e)
        typedef int (*GetIndexBuffer)(IQueryInterface*, int, IIndexBufferInterface**);
        CKMethod<GetIndexBuffer>(this->m_interface, 0xc8)(this->m_interface, index, &indexBuffer);
        return IndexBuffer(indexBuffer);
    }

    IndexBuffer getIndexBuffer(int index) const  // added by b055x
    {  // added by b055x
        IObject3DInterface*indexBuffer;  // added by b055x
        mesh()->getIndexBuffer(index, &indexBuffer);  // added by b055x
        return IndexBuffer(indexBuffer);  // added by b055x
    }  // added by b055x

    Appearance getAppearance(int index) const  // added by b051x
    {  // added by b051x
        IAppearanceInterface*appearance;  // added by b051x
        typedef int (*GetAppearance)(IQueryInterface*, int, IAppearanceInterface**);
        CKMethod<GetAppearance>(this->m_interface, 0xcc)(this->m_interface, index, &appearance);
        return Appearance(appearance);  // added by b051x
    }  // added by b051x
    /** The original ApplyFog keeps the error code and repeatedly overwrites the original pointer without Release; do not change its lifetime with a value handle. */
    int getAppearance(int index, IAppearanceInterface** appearance) const
    {
        typedef int (*GetAppearance)(IQueryInterface*, int, IAppearanceInterface**);
        return CKMethod<GetAppearance>(this->m_interface, 0xcc)(this->m_interface, index, appearance);
    }

    void setAppearance(int index, const Appearance& appearance)  // added by b051x
    {  // added by b051x
        typedef int (*SetAppearance)(IQueryInterface*, int, IAppearanceInterface*);
        CKMethod<SetAppearance>(this->m_interface, 0xd0)(this->m_interface, index, appearance.appearance());
    }  // added by b051x

protected:  // added by b051x
    IMeshInterface* mesh() const  // added by b051x
    {  // added by b051x
        return static_cast<IMeshInterface*>(this->m_interface);  // added by b051x
    }  // added by b051x
};  // added by b051x

class SkinnedMesh : public Mesh {  // added by b051x
public:  // added by b051x
    enum { InterfaceId = 0x16 };  // added by b051x

    SkinnedMesh()  // added by b051x
    {  // added by b051x
    }  // added by b051x

    SkinnedMesh(const Object3D& other)  // added by b051x
    {  // added by b051x
        // interface InterfaceId of `other` (stays NULL when unsupported)  // added by b051x
        if (other.m_interface != NULL) {  // added by b051x
            other.m_interface->QueryInterface(InterfaceId, (void**)&this->m_interface);  // added by b051x
        }  // added by b051x
    }  // added by b051x

    Group getSkeleton() const  // added by b051x
    {  // added by b051x
        IGroupInterface*skeleton;  // added by b051x
        typedef int (*GetSkeleton)(IQueryInterface*, IGroupInterface**);
        CKMethod<GetSkeleton>(this->m_interface, 0xdc)(this->m_interface, &skeleton);
        return Group(skeleton);  // added by b051x
    }  // added by b051x

protected:  // added by b051x
    ISkinnedMeshInterface* skinnedMesh() const  // added by b051x
    {  // added by b051x
        return static_cast<ISkinnedMeshInterface*>(this->m_interface);  // added by b051x
    }  // added by b051x
};  // added by b051x

class KeyframeSequence : public Object3D {  // added by b051x
public:  // added by b051x
    /** getRepeatMode() values. */  // added by b051x
    enum {  // added by b051x
        CONSTANT = 192,  // added by b051x
        LOOP = 193  // added by b051x
    };  // added by b051x

    KeyframeSequence()  // added by b051x
    {  // added by b051x
    }  // added by b051x

    explicit KeyframeSequence(IQueryInterface* object)  // added by b051x
        : Object3D(object)  // added by b051x
    {  // added by b051x
    }  // added by b051x

    /** Milliseconds. */  // added by b051x
    int getDuration() const  // added by b051x
    {  // added by b051x
        int duration;  // added by b051x
        typedef int (*GetDuration)(IQueryInterface*, int*);
        CKMethod<GetDuration>(this->m_interface, 0x50)(this->m_interface, &duration);  // added by b051x
        return duration;  // added by b051x
    }  // added by b051x

    int getRepeatMode() const  // added by b051x
    {  // added by b051x
        int repeatMode;  // added by b051x
        typedef int (*GetRepeatMode)(IQueryInterface*, int*);
        CKMethod<GetRepeatMode>(this->m_interface, 0x58)(this->m_interface, &repeatMode);  // added by b051x
        return repeatMode;  // added by b051x
    }  // added by b051x

protected:  // added by b051x
    IKeyframeSequenceInterface* keyframeSequence() const  // added by b051x
    {  // added by b051x
        return static_cast<IKeyframeSequenceInterface*>(this->m_interface);  // added by b051x
    }  // added by b051x
};  // added by b051x

class AnimationController : public Object3D {  // added by b051x
public:  // added by b051x
    enum { InterfaceId = 1 };  // added by b051x

    AnimationController()  // added by b051x
    {  // added by b051x
    }  // added by b051x

    explicit AnimationController(IQueryInterface* object)  // added by b051x
        : Object3D(object)  // added by b051x
    {  // added by b051x
    }  // added by b051x

    AnimationController(const Object3D& other)  // added by b051x
    {  // added by b051x
        // interface InterfaceId of `other` (stays NULL when unsupported)  // added by b051x
        if (other.m_interface != NULL) {  // added by b051x
            other.m_interface->QueryInterface(InterfaceId, (void**)&this->m_interface);  // added by b051x
        }  // added by b051x
    }  // added by b051x

    float getWeight() const  // added by b051x
    {  // added by b051x
        float weight;  // added by b051x
        typedef int (*GetWeight)(IQueryInterface*, float*);
        CKMethod<GetWeight>(this->m_interface, 0x4c)(this->m_interface, &weight);  // added by b051x
        return weight;  // added by b051x
    }  // added by b051x

    void setWeight(float weight)  // added by b051x
    {  // added by b051x
        typedef int (*SetWeight)(IQueryInterface*, float);
        CKMethod<SetWeight>(this->m_interface, 0x50)(this->m_interface, weight);  // added by b051x
    }  // added by b051x

    float getSpeed() const  // added by b051x
    {  // added by b051x
        float speed;  // added by b051x
        typedef int (*GetSpeed)(IQueryInterface*, float*);
        CKMethod<GetSpeed>(this->m_interface, 0x5c)(this->m_interface, &speed);  // added by b051x
        return speed;  // added by b051x
    }  // added by b051x

    /** Sequence time (ms) at world time `worldTime` (ms). */  // added by b051x
    float getPosition(int worldTime) const  // added by b051x
    {  // added by b051x
        float sequenceTime;  // added by b051x
        animationController()->getPosition(worldTime, &sequenceTime);  // added by b051x
        return sequenceTime;  // added by b051x
    }  // added by b051x

    void setPosition(float sequenceTime, int worldTime)  // added by b051x
    {  // added by b051x
        typedef int (*SetPosition)(IQueryInterface*, float, int);
        CKMethod<SetPosition>(this->m_interface, 0x6c)(this->m_interface, sequenceTime, worldTime);  // added by b051x
    }  // added by b051x

    void setSpeed(float speed, int worldTime)  // added by b051x
    {  // added by b051x
        typedef int (*SetSpeed)(IQueryInterface*, float, int);
        CKMethod<SetSpeed>(this->m_interface, 0x70)(this->m_interface, speed, worldTime);  // added by b051x
    }  // added by b051x

    IAnimationControllerInterface* animationController() const  // added by b051x
    {  // added by b051x
        return static_cast<IAnimationControllerInterface*>(this->m_interface);  // added by b051x
    }  // added by b051x
};  // added by b051x

class AnimationTrack : public Object3D {  // added by b051x
public:  // added by b051x
    enum { InterfaceId = 2 };  // added by b051x

    AnimationTrack()  // added by b051x
    {  // added by b051x
    }  // added by b051x

    explicit AnimationTrack(IQueryInterface* object)  // added by b051x
        : Object3D(object)  // added by b051x
    {  // added by b051x
    }  // added by b051x

    AnimationTrack(const Object3D& other)  // added by b051x
    {  // added by b051x
        // interface InterfaceId of `other` (stays NULL when unsupported)  // added by b051x
        if (other.m_interface != NULL) {  // added by b051x
            other.m_interface->QueryInterface(InterfaceId, (void**)&this->m_interface);  // added by b051x
        }  // added by b051x
    }  // added by b051x

    AnimationController getController() const  // added by b051x
    {  // added by b051x
        IAnimationControllerInterface*controller;  // added by b051x
        typedef int (*GetController)(IQueryInterface*, IAnimationControllerInterface**);
        CKMethod<GetController>(this->m_interface, 0x50)(this->m_interface, &controller);
        return AnimationController(controller);  // added by b051x
    }  // added by b051x

    void setController(const AnimationController& controller)  // added by b051x
    {  // added by b051x
        typedef int (*SetController)(IQueryInterface*, IAnimationControllerInterface*);
        CKMethod<SetController>(this->m_interface, 0x54)(this->m_interface, controller.animationController());
    }  // added by b051x

    KeyframeSequence getKeyframeSequence() const  // added by b051x
    {  // added by b051x
        IKeyframeSequenceInterface*sequence;  // added by b051x
        typedef int (*GetKeyframeSequence)(IQueryInterface*, IKeyframeSequenceInterface**);
        CKMethod<GetKeyframeSequence>(this->m_interface, 0x58)(this->m_interface, &sequence);  // added by b051x
        return KeyframeSequence(sequence);  // added by b051x
    }  // added by b051x

    IAnimationTrackInterface* animationTrack() const  // added by b051x
    {  // added by b051x
        return static_cast<IAnimationTrackInterface*>(this->m_interface);  // added by b051x
    }  // added by b051x
};  // added by b051x

class Background : public Object3D {  // added by b051x
public:  // added by b051x
    Background()  // added by b051x
    {  // added by b051x
    }  // added by b051x

    explicit Background(IQueryInterface* object)  // added by b051x
        : Object3D(object)  // added by b051x
    {  // added by b051x
    }  // added by b051x

    /** Background crop rectangle from dgCamera.cpp, slots 0x84..0x90. */
    int getCropX() const  // added by b049
    {
        int value;
        typedef int (*GetCrop)(IQueryInterface*, int*);
        CKMethod<GetCrop>(this->m_interface, 0x64)(this->m_interface, &value);
        return value;
    }

    /** Background crop rectangle from dgCamera.cpp, slots 0x84..0x90. */
    int getCropY() const  // added by b049
    {
        int value;
        typedef int (*GetCrop)(IQueryInterface*, int*);
        CKMethod<GetCrop>(this->m_interface, 0x68)(this->m_interface, &value);
        return value;
    }

    /** Background crop rectangle from dgCamera.cpp, slots 0x84..0x90. */
    int getCropWidth() const  // added by b049
    {
        int value;
        typedef int (*GetCrop)(IQueryInterface*, int*);
        CKMethod<GetCrop>(this->m_interface, 0x6c)(this->m_interface, &value);
        return value;
    }

    /** Background crop rectangle from dgCamera.cpp, slots 0x84..0x90. */
    int getCropHeight() const  // added by b049
    {
        int value;
        typedef int (*GetCrop)(IQueryInterface*, int*);
        CKMethod<GetCrop>(this->m_interface, 0x70)(this->m_interface, &value);
        return value;
    }

    void setColorClearEnable(bool enable)  // added by b055x
    {  // added by b055x
        typedef int (*SetColorClear)(IQueryInterface*, bool);
        CKMethod<SetColorClear>(this->m_interface, 0x78)(this->m_interface, enable);
    }  // added by b055x
    void setDepthClearEnable(bool enable)
    {
        static_cast<IBackgroundInterface*>(this->m_interface)->setDepthClearEnable(enable);
    }
};  // added by b051x

class Material : public Object3D {  // added by b055x
public:  // added by b055x
    enum { AMBIENT = 0x400, DIFFUSE = 0x800 };  // added by b055x
    explicit Material(IQueryInterface* object)  // added by b055x
        : Object3D(object)  // added by b055x
    {  // added by b055x
    }  // added by b055x
    int getColor(int target) const  // added by b055x
    {  // added by b055x
        int color;  // added by b055x
        static_cast<IMaterialInterface*>(this->m_interface)->getColor(target, &color);  // added by b055x
        return color;  // added by b055x
    }  // added by b055x
};  // added by b055x
class CompositingMode : public Object3D {  // added by b051x
public:  // added by b051x
    CompositingMode()  // added by b051x
    {  // added by b051x
    }  // added by b051x

    explicit CompositingMode(IQueryInterface* object)  // added by b051x
        : Object3D(object)  // added by b051x
    {  // added by b051x
    }  // added by b051x
    void setDepthTestEnable(bool enable)  // added by b055x
    {  // added by b055x
        typedef int (*SetDepthTest)(IQueryInterface*, bool);
        CKMethod<SetDepthTest>(this->m_interface, 0x70)(this->m_interface, enable);
    }  // added by b055x
    void setDepthWriteEnable(bool enable)  // added by b055x
    {  // added by b055x
        typedef int (*SetDepthWrite)(IQueryInterface*, bool);
        CKMethod<SetDepthWrite>(this->m_interface, 0x78)(this->m_interface, enable);
    }  // added by b055x
};  // added by b051x

/** Image. */  // added by b048
class Image2D : public Object3D {  // added by b048
public:  // added by b048
    enum { InterfaceId = 0xa };  // added by b048

    Image2D()  // added by b048
    {  // added by b048
    }  // added by b048

    Image2D(const Object3D& other)  // added by b048
    {  // added by b048
        // interface InterfaceId of `other` (stays NULL when unsupported)  // added by b048
        if (other.m_interface != NULL) {  // added by b048
            other.m_interface->QueryInterface(InterfaceId, (void**)&this->m_interface);  // added by b048
        }  // added by b048
    }  // added by b048
};  // added by b048

/** Camera. */  // added by b048
class Camera : public Node {  // added by b048
public:  // added by b048
    enum { InterfaceId = 5 };  // added by b048

    Camera()  // added by b048
    {  // added by b048
    }  // added by b048

    explicit Camera(IQueryInterface* object)  // added by b055x
        : Node(object)  // added by b055x
    {  // added by b055x
    }  // added by b055x
    Camera(const Object3D& other)  // added by b048
    {  // added by b048
        // interface InterfaceId of `other` (stays NULL when unsupported)  // added by b048
        if (other.m_interface != NULL) {  // added by b048
            other.m_interface->QueryInterface(InterfaceId, (void**)&this->m_interface);  // added by b048
        }  // added by b048
    }  // added by b048

    /** The 4 projection parameters (fovy, aspect ratio, near, far); returns the projection type. */  // added by b048
    int getProjection(float* params) const  // added by b048
    {  // added by b048
        int projectionType;  // added by b048
        typedef int (*GetProjection)(IQueryInterface*, int, float*, int*);
        CKMethod<GetProjection>(this->m_interface, 0xb8)(this->m_interface, 4, params, &projectionType);
        return projectionType;  // added by b048
    }  // added by b048

    void setPerspective(float fovy, float aspectRatio, float nearClip, float farClip)  // added by b048
    {  // added by b048
        typedef int (*SetPerspective)(IQueryInterface*, float, float, float, float);
        CKMethod<SetPerspective>(this->m_interface, 0xc8)(this->m_interface, fovy, aspectRatio, nearClip, farClip);
    }  // added by b048

    void setParallel(float fovy, float aspectRatio, float nearClip, float farClip)  // added by b047x
    {  // added by b047x
        typedef int (*SetParallel)(IQueryInterface*, float, float, float, float);
        CKMethod<SetParallel>(this->m_interface, 0xc4)(this->m_interface, fovy, aspectRatio, nearClip, farClip);
    }  // added by b047x

    /** Projection matrix, with the same by-value handle lifetime as the runtime wrapper. */
    int getProjection(Transform transform) const  // added by b049
    {
        int projectionType;
        typedef int (*GetProjection)(IQueryInterface*, ITransformInterface*, int*);
        CKMethod<GetProjection>(this->m_interface, 0xbc)(this->m_interface,
                                                       transform.transformInterface(), &projectionType);
        return projectionType;
    }
    void setAlignment(const Node& zReference, int zTarget,
                      const Node& yReference, int yTarget)  // added by b049
    {
        camera()->setAlignment(zReference.node(), zTarget, yReference.node(), yTarget);
    }
    void align(const Node& reference)  // added by b049
    {
        camera()->align(reference.node());
    }

    ICameraInterface* camera() const  // added by b048
    {  // added by b048
        return static_cast<ICameraInterface*>(this->m_interface);  // added by b048
    }  // added by b048
};  // added by b048

/** Light (interface 0xd). */  // added by b055x
class Light : public Node {  // added by b055x
public:  // added by b055x
    enum { InterfaceId = 0xd };  // added by b055x
    /** setMode() values. */  // added by b055x
    enum { AMBIENT = 128 };  // added by b055x

    Light()  // added by b055x
    {  // added by b055x
    }  // added by b055x

    /** Take over the reference held by `object`. */  // added by b055x
    explicit Light(IQueryInterface* object)  // added by b055x
        : Node(object)  // added by b055x
    {  // added by b055x
    }  // added by b055x

    Light(const Object3D& other)  // added by b055x
    {  // added by b055x
        // interface InterfaceId of `other` (stays NULL when unsupported)  // added by b055x
        if (other.m_interface != NULL) {  // added by b055x
            other.m_interface->QueryInterface(InterfaceId, (void**)&this->m_interface);  // added by b055x
        }  // added by b055x
    }  // added by b055x

    /** New light created by `runtime` (the handle takes over the new reference). */  // added by b055x
    static Light Create(ISwerve* runtime)  // added by b055x
    {  // added by b055x
        IQueryInterface* object;  // added by b055x
        runtime->m_functions->CreateObject(InterfaceId, (void**)&object);  // added by b055x
        return Light(object);  // added by b055x
    }  // added by b055x

    void setMode(int mode)  // added by b055x
    {  // added by b055x
        light()->setMode(mode);  // added by b055x
    }  // added by b055x

    void setColor(int color)  // added by b055x
    {  // added by b055x
        light()->setColor(color);  // added by b055x
    }  // added by b055x

    void setIntensity(float intensity)  // added by b055x
    {  // added by b055x
        light()->setIntensity(intensity);  // added by b055x
    }  // added by b055x

    ILightInterface* light() const  // added by b055x
    {  // added by b055x
        return static_cast<ILightInterface*>(this->m_interface);  // added by b055x
    }  // added by b055x
};  // added by b055x

/** Result of a pick (no method used yet). */  // added by b048
class RayIntersection : public QueryInterface {  // added by b048
public:  // added by b048
    RayIntersection() {} // Null handle from the original Body constructor/cleanup; does not call CreateObject
    /** New ray intersection created by `runtime`. */  // added by b048
    explicit RayIntersection(ISwerve* runtime)  // added by b048
    {  // added by b048
        runtime->m_functions->CreateObject(0x15, (void**)&this->m_interface);  // added by b048
    }  // added by b048
    void getRay(float* originAndDirection) const
    {
        intersection()->getRay(6, originAndDirection);
    }
    float getDistance() const
    {
        float distance;
        intersection()->getDistance(&distance);
        return distance;
    }
    Node getIntersected() const
    {
        INodeInterface*node;
        intersection()->getIntersected(&node);
        return Node(node);
    }
    float getNormalX() const
    {
        float normal;
        intersection()->getNormalX(&normal);
        return normal;
    }
    float getNormalY() const
    {
        float normal;
        intersection()->getNormalY(&normal);
        return normal;
    }
    float getNormalZ() const
    {
        float normal;
        intersection()->getNormalZ(&normal);
        return normal;
    }
    IRayIntersectionInterface* intersection() const
    {
        return static_cast<IRayIntersectionInterface*>(this->m_interface);
    }
};  // added by b048

inline unsigned char Group::pick(int scope, float x, float y, const Camera& camera,
                                     RayIntersection intersection) const
{
    // The output is a raw 8-bit flag; a non-canonical byte must not be truncated to its lowest bit early.
    unsigned char hit = 0;
    typedef int (*ScreenPick)(IQueryInterface*, int, float, float, IQueryInterface*, IQueryInterface*, unsigned char*);
    CKMethod<ScreenPick>(this->m_interface, 0xcc)(this->m_interface, scope, x, y,
                                                camera.m_interface, intersection.m_interface, &hit);
    return hit;
}

inline unsigned char Group::pick(int scope, const float* origin, const float* direction,
                                     RayIntersection intersection) const
{
    // The output is a raw 8-bit flag; a non-canonical byte must not be truncated to its lowest bit early.
    unsigned char hit = 0;
    typedef int (*RayPick)(IQueryInterface*, int, const float*, const float*, IQueryInterface*, unsigned char*);
    CKMethod<RayPick>(this->m_interface, 0xc8)(this->m_interface, scope, origin, direction,
                                             intersection.m_interface, &hit);
    return hit;
}

inline __attribute__((noinline)) int Object3D::operator==(const Object3D& other) const  // added by b047x; changed by b055x (was bool)
{  // added by b047x
    int identity = 0;  // added by b047x
    if (this->m_interface != NULL) {  // added by b047x
        identity = CSwerve::GetInstance()->m_swerve->m_functions->GetObjectIdentity(this->m_interface);  // added by b047x
    }  // added by b047x
    int otherIdentity = 0;  // added by b047x
    if (other.m_interface != NULL) {  // added by b047x
        otherIdentity = CSwerve::GetInstance()->m_swerve->m_functions->GetObjectIdentity(other.m_interface);  // added by b047x
    }  // added by b047x
    return identity == otherIdentity;  // added by b047x
}

inline __attribute__((noinline)) int Object3D::operator!=(const Object3D& other) const  // added by b048
{  // added by b048
    // The former b048 thin implementation was return !(*this == other); original 101810..1018e8 queries the two identities independently.
    // b067h restored the complete original method; must not rely on the out-of-line/inline choice of == to replace this method.
    int identity = 0;
    if (this->m_interface != NULL) {
        identity = CSwerve::GetInstance()->m_swerve->m_functions->GetObjectIdentity(this->m_interface);
    }
    int otherIdentity = 0;
    if (other.m_interface != NULL) {
        otherIdentity = CSwerve::GetInstance()->m_swerve->m_functions->GetObjectIdentity(other.m_interface);
    }
    return identity != otherIdentity;  // added by b048; b067h restored the original standalone NE branch.
}  // added by b048
  // added by b047x


// Bodies that need classes defined later (moved when the wrappers stopped being templates).
inline AnimationTrack Object3D::getAnimationTrack(int index) const
{  // added by b051x
    IAnimationTrackInterface*track;  // added by b051x
    typedef int (*GetTrack)(IQueryInterface*, int, IAnimationTrackInterface**);
    CKMethod<GetTrack>(this->m_interface, 0x18)(this->m_interface, index, &track);
    return AnimationTrack(track);  // added by b051x
}  // added by b051x

inline void Object3D::addAnimationTrack(const AnimationTrack& track)
{  // added by b051x
    typedef int (*SetTrack)(IQueryInterface*, IAnimationTrackInterface*);
    CKMethod<SetTrack>(this->m_interface, 0x1c)(this->m_interface, track.animationTrack());
}  // added by b051x

inline void Object3D::removeAnimationTrack(const AnimationTrack& track)
{  // added by b051x
    typedef int (*SetTrack)(IQueryInterface*, IAnimationTrackInterface*);
    CKMethod<SetTrack>(this->m_interface, 0x20)(this->m_interface, track.animationTrack());
}  // added by b051x

inline Object3D Object3D::find(int userID) const
{  // added by b051x
    IObject3DInterface*found;  // added by b051x
    // CK World::find uses 0x28 in setCamera and the original location loader.
    typedef int (*Find)(IQueryInterface*, int, IObject3DInterface**);
    CKMethod<Find>(this->m_interface, 0x28)(this->m_interface, userID, &found);
    return Object3D(found);  // added by b051x
}  // added by b051x

inline Camera World::getActiveCamera() const
{  // added by b055x
    ICameraInterface*camera;  // added by b055x
    typedef int (*GetCamera)(IQueryInterface*, ICameraInterface**);
    CKMethod<GetCamera>(this->m_interface, 0xd0)(this->m_interface, &camera);
    return Camera((IQueryInterface*)camera);  // added by b055x
}  // added by b055x

inline Background World::getBackground() const
{  // added by b055x
    IBackgroundInterface*background;  // added by b055x
    typedef int (*GetBackground)(IQueryInterface*, IBackgroundInterface**);
    CKMethod<GetBackground>(this->m_interface, 0xd8)(this->m_interface, &background);
    return Background(background);  // added by b055x
}  // added by b055x

inline void Texture2D::setImage(const Image2D& image)
{  // added by b048
    texture2D()->setImage(static_cast<IImage2DInterface*>(image.m_interface));  // added by b048
}  // added by b048

inline CompositingMode Appearance::getCompositingMode() const
{  // added by b055x
    IObject3DInterface*mode;  // added by b055x
    typedef int (*GetMode)(IQueryInterface*, IObject3DInterface**);
    CKMethod<GetMode>(this->m_interface, 0x64)(this->m_interface, &mode);
    return CompositingMode(mode);  // added by b055x
}  // added by b055x

inline void Appearance::setCompositingMode(const CompositingMode& mode)
{  // added by b055x
    typedef int (*SetMode)(IQueryInterface*, IQueryInterface*);
    CKMethod<SetMode>(this->m_interface, 0x68)(this->m_interface, mode.m_interface);
}  // added by b055x

inline Material Appearance::getMaterial() const
{  // added by b055x
    IObject3DInterface*material;  // added by b055x
    appearance()->getMaterial(&material);  // added by b055x
    return Material(material);  // added by b055x
}  // added by b055x

// Graphics3D::render: out-of-line weak functions in the original (32 bytes each, called with bl).
inline __attribute__((noinline)) void Graphics3D::render(const World& world) const
{
    // the original reads the world handle before testing for a target
    IQueryInterface* handle = world.m_interface;
    if (hasTarget()) {
        renderer()->Render(handle);
    }
}

inline __attribute__((noinline)) void Graphics3D::render(const Node& node, const Transform& transform) const
{
    // the original reads both handles before testing for a target
    IQueryInterface* nodeHandle = node.m_interface;
    IQueryInterface* transformHandle = transform.m_interface;
    if (hasTarget()) {
        renderer()->RenderNode(nodeHandle, transformHandle);
    }
}

#endif
