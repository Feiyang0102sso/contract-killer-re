// SwerveHelper.cpp - CK's resource and model lookup paths.
#include "SwerveHelper.h"

#include <stdlib.h>
#include <string.h>
#include <OpenGLES/ES1/gl.h>
#include "CFileUtil.h"
#include "CFont.h"
#include "CDrawUtil.h"
#include "CSwerve.h"
#include "CStrWChar.h"
#include "DGHelper.h"
#include "ICFile.h"
#include "ICFileMgr.h"
#include "ICRenderSurface.h"
#include "WindowApp.h"
#include "xString.h"

typedef CSwerveAssertingErrorHandler ErrorHandler;
typedef ::SkinnedMesh SkinnedMesh;
typedef ::Appearance Appearance;
typedef ::Texture2D Texture2D;
typedef ::VertexBuffer VertexBuffer;
typedef ::VertexArray VertexArray;
typedef ::Transform Transform;
typedef ::Transformable Transformable;
typedef ::AnimationTrack AnimationTrack;
typedef ::AnimationController AnimationController;
typedef ::KeyframeSequence KeyframeSequence;

void SwerveHelper::CopyAnimationTracks_r(Object3D& source, const Object3D& target, Vector<int>& controllerIds,
                                         Vector<AnimationController>& controllers, int userIdOffset)
{
    if (source.m_interface == NULL || target.m_interface == NULL) {
        return;
    }

    // tracks of this object: duplicated, controllers duplicated once per user id
    int trackCount = source.getAnimationTrackCount();
    for (int trackIndex = 0; trackIndex < trackCount; trackIndex++) {
        AnimationTrack track = source.getAnimationTrack(trackIndex);
        if (track.m_interface != NULL) {
            AnimationController controller = track.getController();
            if (controller.m_interface != NULL) {
                int controllerId = controller.getUserID();
                if (controllerId != 0) {
                    AnimationTrack trackCopy(track.duplicate());
                    if (trackCopy == NULL) {
                        continue;
                    }
                    // The target is const, so add the track through a temporary handle.
                    Object3D(target).addAnimationTrack(trackCopy);
                    int index = controllerIds.indexOf(controllerId);
                    if (index == -1) {
                        AnimationController controllerCopy(controller.duplicate());
                        if (controllerCopy == NULL) {
                            Object3D(target).removeAnimationTrack(trackCopy);
                            continue;
                        }
                        controllerIds.addElement(controllerId);
                        controllerCopy.setUserID(userIdOffset + controllerId);
                        trackCopy.setController(controllerCopy);
                        controllers.addElement(controllerCopy);
                    } else {
                        trackCopy.setController(controllers.elementAt(index));
                    }
                }
            }
        }
    }

    // children (matched by user id, else by position), mesh textures and skeletons
    Group sourceGroup(source);
    Group targetGroup(target);
    if (sourceGroup.m_interface == NULL || targetGroup.m_interface == NULL) {
        return;
    }
    int childCount = sourceGroup.getChildCount();
    for (int childIndex = 0; childIndex < childCount; childIndex++) {
        Node sourceChild = sourceGroup.getChild(childIndex);
        if (sourceChild == NULL) {
            continue;
        }
        int childId = sourceChild.getUserID();
        Node targetChild;
        if (childId != 0) {
            targetChild = Node(FindChild(targetGroup, childId));
        }
        if (targetChild.m_interface == NULL) {
            targetChild = targetGroup.getChild(childIndex);
        }
        if (sourceChild.m_interface == NULL || targetChild.m_interface == NULL) {
            continue;
        }

        Group sourceChildGroup(sourceChild);
        Group targetChildGroup(targetChild);
        if (sourceChildGroup.m_interface != NULL && targetChildGroup.m_interface != NULL) {
            CopyAnimationTracks_r(sourceChildGroup, targetChildGroup, controllerIds, controllers, userIdOffset);
        }

        Mesh sourceMesh(sourceChild);
        Mesh targetMesh(targetChild);
        if (sourceMesh.m_interface == NULL || targetMesh.m_interface == NULL) {
            continue;
        }
        int submeshCount = sourceMesh.getSubmeshCount();
        for (int submesh = 0; submesh < submeshCount; submesh++) {
            Appearance sourceAppearance = sourceMesh.getAppearance(submesh);
            Appearance targetAppearance = targetMesh.getAppearance(submesh);
            if (sourceAppearance.m_interface != NULL && targetAppearance.m_interface != NULL) {
                Texture2D sourceTexture = sourceAppearance.getTexture(0);
                Texture2D targetTexture = targetAppearance.getTexture(0);
                if (sourceTexture.m_interface != NULL && targetTexture.m_interface != NULL) {
                    CopyAnimationTracks_r(sourceTexture, targetTexture, controllerIds, controllers, userIdOffset);
                }
            }
        }

        SkinnedMesh sourceSkinnedMesh(sourceMesh);
        SkinnedMesh targetSkinnedMesh(targetMesh);
        if (sourceSkinnedMesh.m_interface != NULL && targetSkinnedMesh.m_interface != NULL) {
            Group sourceSkeleton = sourceSkinnedMesh.getSkeleton();
            Group targetSkeleton = targetSkinnedMesh.getSkeleton();
            if (sourceSkeleton.m_interface != NULL && targetSkeleton.m_interface != NULL) {
                CopyAnimationTracks_r(sourceSkeleton, targetSkeleton, controllerIds, controllers, userIdOffset);
            }
        }
    }
}

SwerveHelper::Object3D SwerveHelper::FindChild(Group group, int userID)
{
    int childCount = group.getChildCount();
    for (int index = 0; index < childCount; index++) {
        Object3D child = group.getChild(index);
        if (child != NULL && child.getUserID() == userID) {
            return child;
        }
    }
    return Object3D();
}

void SwerveHelper::SeparateAnimationAppearances_r(Object3D object)
{
    if (object.m_interface == NULL) {
        return;
    }

    // every submesh gets its own copy of its appearance and of its first two textures
    Mesh mesh(object);
    if (mesh.m_interface != NULL) {
        int submeshCount = mesh.getSubmeshCount();
        for (int submesh = 0; submesh < submeshCount; submesh++) {
            Appearance appearance = mesh.getAppearance(submesh);
            if (appearance.m_interface == NULL) {
                continue;
            }
            appearance = Appearance(appearance.duplicate());
            int textureIndex = 0;
            Texture2D texture = appearance.getTexture(0);
            while (texture.m_interface != NULL) {
                appearance.setTexture(textureIndex, Texture2D(texture.duplicate()));
                if (textureIndex == 1) {
                    break;
                }
                textureIndex++;
                texture = appearance.getTexture(textureIndex);
            }
            mesh.setAppearance(submesh, appearance);
        }
    }

    unsigned int referenceCount = object.getReferences(0, NULL);
    if (referenceCount != 0) {
        Object3D* references = new Object3D[referenceCount];
        object.getReferences(referenceCount, references);
        for (int index = 0; index < (int)referenceCount; index++) {
            SeparateAnimationAppearances_r(references[index]);
        }
        delete[] references;
    }
}

void SwerveHelper::RemoveAnimationTracks_r(Object3D object)
{
    if (object.m_interface == NULL) {
        return;
    }

    int trackCount = object.getAnimationTrackCount();
    if (trackCount > 0) {
        Vector<AnimationTrack> tracks;
        for (int index = 0; index < trackCount; index++) {
            AnimationTrack track = object.getAnimationTrack(index);
            if (track.m_interface != NULL) {
                tracks.addElement(track);
            }
        }
        for (int index = 0; index < tracks.size(); index++) {
            object.removeAnimationTrack(tracks[index]);
        }
    }

    unsigned int referenceCount = object.getReferences(0, NULL);
    if (referenceCount != 0) {
        Object3D* references = new Object3D[referenceCount];
        object.getReferences(referenceCount, references);
        for (int index = 0; index < (int)referenceCount; index++) {
            RemoveAnimationTracks_r(references[index]);
        }
        delete[] references;
    }
}

void SwerveHelper::CopyAnimationsTracks(Group source, const Group& target, int userIdOffset)
{
    SeparateAnimationAppearances_r(target);
    RemoveAnimationTracks_r(target);
    Vector<int> controllerIds;
    Vector<AnimationController> controllers;
    CopyAnimationTracks_r(source, target, controllerIds, controllers, userIdOffset);
}

SwerveHelper::Group SwerveHelper::FullObjectDuplicate(Group group, int userIdOffset)
{
    if (group == NULL) {
        return Group();
    }
    Group copy(group.duplicate());
    CopyAnimationsTracks(group, copy, userIdOffset);
    return copy;
}

int SwerveHelper::Id(const char* name)
{
    // same hash as DGHelper::getSwerveID, written out here (the original inlines it)
    int length = strlen(name);
    unsigned int hash = 0;
    while (length > 0) {
        --length;
        hash = hash * 0x19660dU + static_cast<unsigned char>(name[length]) + 0x3c6ef35fU;
    }
    unsigned int sign = static_cast<int>(hash) >> 31;
    return ((hash ^ sign) - sign) & 0x3fffffff;
}

SwerveHelper::Object3D SwerveHelper::Find(Object3D root, const char* name)
{
    IQueryInterface* found = NULL;
    if (root != NULL) {
        // CK Object3D's Find slot is 0x28 in the original Swerve vtable.
        typedef int (*FindObject)(IQueryInterface*, int, IQueryInterface**);
        FindObject find = reinterpret_cast<FindObject>((*reinterpret_cast<void***>(root.m_interface))[0x28 / 4]);
        find(root.m_interface, Id(name), &found);
    }
    return Object3D(found);
}

SwerveHelper::Object3D SwerveHelper::LoadObject3D(const char* name)
{
    return Object3D(reinterpret_cast<IQueryInterface*>(CSwerve::GetInstance()->LoadM3GResource(name)));
}

bool SwerveHelper::PickWithAngle(Group& group, Camera& camera,
                                 float angle, float axisX, float axisY, float axisZ,
                                 RayIntersection intersection)
{
    // Original: no handle checks, the transform result is ignored and the rotation always applied.
    Transform transform = NewTransform();
    camera.getTransformTo(group, transform);
    float rotation[4] = {angle, axisX, axisY, axisZ};
    transform.postRotate(rotation);
    float matrix[16];
    transform.get(matrix);
    float origin[3] = {matrix[3], matrix[7], matrix[11]};
    float direction[3] = {-matrix[2], -matrix[6], -matrix[10]};
    return group.pick(-1, origin, direction, intersection) != 0;
}

SwerveHelper::Transform SwerveHelper::NewTransform()
{
    return Transform(CSwerve::GetInstance()->m_swerve);
}

void SwerveHelper::DrawToArray(unsigned char** pixels, int* width, int* height,
                               ICRenderSurface* target, const XString& text, CFont* font)
{
    *pixels = NULL;
    *width = 0;
    *height = 0;
    ICRenderSurface* previous = ICGraphics::GetInstance()->GetTargetSurface();
    if (previous == NULL) {
        return;
    }
    target->Bind();
    unsigned int targetWidth = 0;
    unsigned int targetHeight = 0;
    target->GetWidthAndHeight(targetWidth, targetHeight);
    ICGraphics2d* graphics = ICGraphics2d::GetInstance();
    graphics->RenderBegin(1);
    graphics->Scale(0x10000, 0x10000);
    CDrawUtil::Fill(0, 0, targetWidth, targetHeight, 0xffff00ffu, ICGraphics2d::Primitive_Rectangles);
    font->PaintText(text.c_str(), -1, 0, 0, -1, -1);
    graphics->RenderEnd();
    target->UpdateTextureWithTarget();

    unsigned char* buffer = NULL;
    int pitch = 0;
    target->Lock(&buffer, pitch, ICRenderSurface::LockType_ReadWrite, NULL);
    int glyphWidth = font->MeasureTextWidth(text.c_str(), -1, -1, NULL);
    int glyphHeight = font->GetFontHeight();
    if (buffer != NULL && glyphWidth > 0 && glyphHeight > 0 &&
        static_cast<unsigned int>(glyphWidth) <= targetWidth && static_cast<unsigned int>(glyphHeight) <= targetHeight) {
        unsigned char* image = new unsigned char[glyphWidth * glyphHeight * 4];
        unsigned char* destination = image;
        // OpenGL reads bottom-to-top, while Sprite3D consumes top-to-bottom rows.
        // The surface lock exposes that order through its signed pitch.
        for (int row = 0; row < glyphHeight; ++row) {
            const unsigned char* source = buffer + row * pitch;
            for (int column = 0; column < glyphWidth; ++column) {
                unsigned int color;
                memcpy(&color, source, sizeof(color));
                if ((color & 0xff00ff) == 0xff00ff) {
                    color = 0;
                }
                memcpy(destination, &color, sizeof(color));
                source += sizeof(color);
                destination += sizeof(color);
            }
        }
        *pixels = image;
        *width = glyphWidth;
        *height = glyphHeight;
    }
    target->Unlock();
    previous->Bind();
}

SwerveHelper::Node SwerveHelper::CreateSprite(const unsigned char* pixels, int width, int height)
{
    // Original: no failure checks; picking stays at its default.
    IQueryInterface* imageObject = NULL;
    CSwerve::GetInstance()->m_swerve->m_functions->CreateObject(10, reinterpret_cast<void**>(&imageObject));
    Object3D image(imageObject);
    typedef int (*SetPixels)(IQueryInterface*, int, int, int, int, const unsigned char*);
    CKMethod<SetPixels>(image.m_interface, 0x4c)(image.m_interface, 100, width, height, width * height * 4, pixels);
    IQueryInterface* appearanceObject = NULL;
    CSwerve::GetInstance()->m_swerve->m_functions->CreateObject(3, reinterpret_cast<void**>(&appearanceObject));
    Appearance appearance(appearanceObject);
    IQueryInterface* materialObject = NULL;
    CSwerve::GetInstance()->m_swerve->m_functions->CreateObject(0xf, reinterpret_cast<void**>(&materialObject));
    Object3D material(materialObject);
    typedef int (*SetColor)(IQueryInterface*, int, unsigned int);
    CKMethod<SetColor>(material.m_interface, 0x60)(material.m_interface, 0x400, 0x64c8c800u);
    CKMethod<SetColor>(material.m_interface, 0x60)(material.m_interface, 0x1000, 0x64c8c800u);
    CKMethod<SetColor>(material.m_interface, 0x60)(material.m_interface, 0x2000, 0x64c8c800u);
    CKMethod<SetColor>(material.m_interface, 0x60)(material.m_interface, 0x800, 0x64c8c800u);
    typedef int (*SetMaterial)(IQueryInterface*, IQueryInterface*);
    CKMethod<SetMaterial>(appearance.m_interface, 0x58)(appearance.m_interface, material.m_interface);
    CompositingMode mode = NewCompositingMode();
    typedef int (*SetBlend)(IQueryInterface*, int);
    CKMethod<SetBlend>(mode.m_interface, 0x58)(mode.m_interface, 0x40);
    appearance.setCompositingMode(mode);
    IQueryInterface* spriteObject = NULL;
    CSwerve::GetInstance()->m_swerve->m_functions->CreateObject(0x17, reinterpret_cast<void**>(&spriteObject));
    Node sprite(spriteObject);
    typedef int (*InitializeSprite)(IQueryInterface*, bool, IQueryInterface*, IQueryInterface*);
    CKMethod<InitializeSprite>(sprite.m_interface, 0xb8)(sprite.m_interface, true, image.m_interface,
                                                         appearance.m_interface);
    sprite.setRenderingEnable(true);
    return sprite;
}

vec3 SwerveHelper::GetTranslation(const Transform& transform)
{
    float matrix[16];
    transform.get(matrix);
    return vec3(matrix[3], matrix[7], matrix[11]);
}

Mat4x4& SwerveHelper::SetMatrix(Mat4x4& matrix, const Transform& transform)
{
    transform.get(matrix.ToFloatPtr());
    return matrix;
}

CompositingMode SwerveHelper::NewCompositingMode()
{
    ISwerve* runtime = CSwerve::GetInstance()->m_swerve;
    IQueryInterface* mode = NULL;
    runtime->m_functions->CreateObject(6, reinterpret_cast<void**>(&mode));
    return CompositingMode(mode);
}

SwerveHelper::Object3D SwerveHelper::LoadObject3D(const wchar_t* fileName, int index)
{
    CStrWChar path;
    CFileUtil::GetApplicationPathForFile(path, fileName);
    ICFileMgr* files = ICFileMgr::GetInstance();
    ICFile* file = files->Open(path.m_string, ICFileMgr::AccessRead);
    if (file == NULL) {
        return Object3D();
    }
    file->Seek(0, ICFile::SeekEnd);
    const unsigned int size = file->Tell();
    file->Seek(0, ICFile::SeekSet);
    unsigned char* data = static_cast<unsigned char*>(malloc(size));
    IObject3D* model = NULL;
    if (data != NULL && file->Read(data, size) == size) {
        ISwerve* runtime = CSwerve::GetInstance()->m_swerve;
        ILoader* loader = NULL;
        if (runtime->m_functions->CreateObject(0xe, reinterpret_cast<void**>(&loader)) == 0) {
            if (loader->m_functions->Load(loader, size, data, 0) == 0) {
                int count = 0;
                if (loader->m_functions->GetObjectCount(loader, &count) == 0 && index >= 0 && index < count) {
                    loader->m_functions->GetObject(loader, index, &model);
                }
            }
            loader->m_functions->Release(loader);
        }
    }
    free(data);
    files->Close(file);
    return Object3D(reinterpret_cast<IQueryInterface*>(model));
}

SwerveAnimations::Animation::~Animation()
{
}

bool SwerveAnimations::Animation::IsPlaying() const
{
    if (m_stopTime != 0) {
        return false;
    }
    return m_controller.getWeight() != 0.0f;
}

void SwerveAnimations::Animation::Start(float delaySec, float speed, float fadeSec, float startWeight,
                                        float endWeight)
{
    int delayMS = (int)(delaySec * 1000.0f);
    int fadeMS = (int)(fadeSec * 1000.0f);
    int startTime = m_owner->TimeMS() + delayMS;
    StartAtTimeMS(startTime, speed, startTime + fadeMS, startWeight, endWeight);
}

void SwerveAnimations::Animation::StartAtTimeMS(int startTime, float speed, int endTime, float startWeight,
                                                float endWeight)
{
    m_paused = false;
    m_stopTime = 0;
    m_fadeStartTime = startTime;
    m_fadeEndTime = endTime;
    float weight = endWeight;
    if (endTime > startTime) {
        weight = startWeight;
        if (weight < 1.0e-5f) {
            weight = 1.0e-5f;
        }
    }
    m_fadeStartWeight = weight;
    m_fadeEndWeight = endWeight;
    m_weight = weight;
    m_controller.setWeight(weight);

    // backwards from the end, forwards from the start
    float position;
    if (speed >= 0.0f) {
        position = 0.0f;
    } else {
        position = (float)m_durationMS;
    }
    m_controller.setPosition(position, startTime);
    m_controller.setSpeed(speed, startTime);

    if (m_channelId < 0) {
        return;
    }
    // the other channels of the group stop
    for (Animation* other = m_owner->m_animations; other != NULL; other = other->m_next) {
        if (other != this && other->m_groupId == m_groupId && other->m_channelId >= 0 &&
            other->m_channelId != m_channelId && other->IsPlaying()) {
            other->Stop();
        }
    }
    if (m_blendable) {
        m_owner->RegisterBlending(this);
    }
}

void SwerveAnimations::RegisterBlending(Animation* animation)
{
    Blend* blend = m_blends;
    while (blend != NULL) {
        if (blend->m_groupId == animation->m_groupId && blend->m_channelId == animation->m_channelId) {
            break;
        }
        blend = blend->m_next;
    }
    if (blend == NULL) {
        blend = new Blend();
        blend->m_groupId = animation->m_groupId;
        blend->m_channelId = animation->m_channelId;
        blend->m_next = m_blends;
        m_blends = blend;
    }
    int index = blend->m_animations.indexOf(animation);
    if (index >= 0) {
        blend->m_animations.removeElementAt(index);
    }
    blend->m_animations.addElement(animation);
    blend->m_started = false;
}

void SwerveAnimations::Animation::SetPositionSec(float seconds)
{
    m_controller.setPosition(seconds * 1000.0f, m_owner->TimeMS());
}

void SwerveAnimations::Animation::Pause(int time)
{
    if (m_paused) {
        return;
    }
    if (!IsPlaying()) {
        return;
    }
    m_paused = true;
    m_pausedSpeed = m_controller.getSpeed();
    m_controller.setSpeed(0.0f, time);
}

void SwerveAnimations::Animation::Pause()
{
    Pause(m_owner->TimeMS());
}

SwerveAnimations::SwerveAnimations()
{
    m_animations = NULL;
    m_animationCount = 0;
    m_blends = NULL;
}

SwerveAnimations::~SwerveAnimations()
{
    Reset();
}

void SwerveAnimations::Reset()
{
    while (m_animations != NULL) {
        Animation* animation = m_animations;
        m_animations = animation->m_next;
        animation->m_next = NULL;
        delete animation;
        m_animationCount--;
    }
    while (m_blends != NULL) {
        Blend* blend = m_blends;
        m_blends = blend->m_next;
        blend->m_next = NULL;
        delete blend;
    }
    m_root.Clear();
}

SwerveAnimations::Animation* SwerveAnimations::RegisterAnimationTrack(const AnimationTrack& track)
{
    AnimationController controller = track.getController();
    if (controller == NULL) {
        return NULL;
    }
    int controllerId = controller.getUserID();
    if (controllerId == 0) {
        controller.setWeight(0.0f);
        return NULL;
    }

    Animation* animation = Anim(controllerId);
    if (animation == NULL) {
        animation = new Animation();
        animation->m_owner = this;
        animation->m_controller = controller;
        animation->m_next = m_animations;
        m_animations = animation;
        m_animationCount++;
        animation->Stop();
    }

    KeyframeSequence sequence = track.getKeyframeSequence();
    if (sequence == NULL) {
        return animation;
    }
    animation->m_looping = animation->m_looping || sequence.getRepeatMode() == KeyframeSequence::LOOP;
    int durationMS = animation->m_durationMS;
    int sequenceDuration = sequence.getDuration();
    if (durationMS > sequenceDuration) {
        sequenceDuration = durationMS;
    }
    animation->m_durationMS = sequenceDuration;
    animation->m_durationSec = (float)animation->m_durationMS * 0.001f;
    return animation;
}

SwerveAnimations::Animation* SwerveAnimations::Anim(int userID) const
{
    for (Animation* animation = m_animations; animation != NULL; animation = animation->m_next) {
        if (animation->m_controller.getUserID() == userID) {
            return animation;
        }
    }
    return NULL;
}

int SwerveAnimations::LoadFrom(Object3D root)
{
    Reset();
    m_root = root;
    LoadFrom_r(root);
    return m_animationCount;
}

void SwerveAnimations::LoadFrom_r(Object3D& object)
{
    AnimationTrack track(object);
    if (track.m_interface != NULL) {
        RegisterAnimationTrack(track);
    } else if (object.m_interface != NULL) {
        unsigned int referenceCount = object.getReferences(0, NULL);
        if (referenceCount != 0) {
            Object3D* references = new Object3D[referenceCount];
            object.getReferences(referenceCount, references);
            for (int index = 0; index < (int)referenceCount; index++) {
                LoadFrom_r(references[index]);
            }
            delete[] references;
        }
    }
}

void SwerveAnimations::StopAll()
{
    for (Animation* animation = m_animations; animation != NULL; animation = animation->m_next) {
        animation->Stop();
    }
}

int SwerveAnimations::TimeMS()
{
    return WindowApp::m_instance->m_updateTime;
}
