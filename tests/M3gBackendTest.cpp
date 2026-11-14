// Differential test of the two Swerve backends: every M3G file of BountyHunter.big and vga.big is
// loaded with the native m3gcore runtime and with the original ARM Swerve core, and both scene
// graphs are compared through the object slots the game calls: node classes, user ids, transforms,
// world matrices, rendering and picking switches, mesh data (the game's collision mesh,
// CCollision::BuildMesh, is built from it), camera projections, the scene after animate() at several
// times, and camera / ray picks.
//
// Every call goes through ck::arm::Method with the slot numbers of SwerveSlots.h (generated from
// the original engine's vtables), so the same code drives an ARM object and a native one; a wrong
// slot shows up as a difference. The original Swerve is the reference: a difference is a bug of the
// native backend unless it is explained in tests/README.md.
#include "SwerveBackends.h"
#include "GameHarness.h"
#include <gtest/gtest.h>
#include <SDL3/SDL.h>
#include <math.h>
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <map>
#include <string>
#include <vector>

namespace ck::test {
namespace {

// The classes recorded for every object, in this order (one bit each).
const int RecordedClasses[] = { ClassNode, ClassGroup, ClassWorld, ClassMesh, ClassSkinnedMesh, ClassCamera,
                                ClassLight, ClassSprite3D, ClassTransformable };

// Solo phase (how the game plays animations, CDH_Animation): each of the first SoloControllers
// controllers alone at weight 1, compared SoloTimes ms after it was started.
constexpr size_t SoloControllers = 4;
const int SoloTimes[] = { 95, 455, 1305 };
// World time at which the solo phase starts (after the AnimationTimes).
constexpr int SoloStartTime = 10000;
// Sequence time the solo controller starts at: the fraction tells whether the sequence time is
// sampled as a float, rounded or truncated to whole milliseconds.
constexpr float SoloSequenceStart = 0.6f;
// World times (ms) at which the animated scene is compared.
const int AnimationTimes[] = { 0, 150, 700, 2300, 6100 };
// Picks: a grid of PickGrid x PickGrid normalized screen points through the active camera.
constexpr int PickGrid = 9;
// User ids are recorded as id / IdSplit and id % IdSplit, each exact in a float.
constexpr int IdSplit = 100000;
// Two floats agree when |a - b| <= Tolerance * max(1, |a|, |b|).
constexpr float Tolerance = 2e-3f;
// Mismatches reported per file; the rest is only counted.
constexpr int ReportedMismatches = 12;
// Set CK_DIFF_ALL=1 to report every mismatching record (for investigating one file).
constexpr const char* ReportAllVariable = "CK_DIFF_ALL";
// Triangle indices shown in full per submesh (all of them go into a checksum).
constexpr size_t IndicesShown = 18;
// Largest number of vertices whose positions are compared one by one (the rest by checksum).
constexpr int MaxVertexComponents = 1 << 20;
constexpr float DegreesToRadians = 3.14159265358979f / 180.0f;

/** One recorded value of a scene: where it was read, what, the call's error code and the numbers. */
struct Record {
    std::string path;   // e.g. "/1#1234/2#5/9#77"
    std::string field;  // e.g. "translation"
    int error;
    std::vector<float> values;
    bool quaternion;    // q and -q are the same orientation
};

/** All records of one backend's scene, in walk order. */
typedef std::vector<Record> Snapshot;

/** A Swerve runtime, the per-backend objects the walk needs, and its name for messages. */
struct Backend {
    const char* name;
    ISwerve* swerve;
    Handle transform;
    Handle intersection;
    int worldTime; // time of the last animate(), for the controllers' sequence positions
};

/** A boolean the engine writes through a pointer: read as a whole word, whatever it stores. */
int ReadFlag(Handle object, int index, int& error)
{
    int value = 0;
    error = Call<int>(object, index, reinterpret_cast<unsigned char*>(&value));
    return value & 0xff;
}

void Add(Snapshot& snapshot, const std::string& path, const char* field, int error, std::vector<float> values,
         bool quaternion = false)
{
    Record record;
    record.path = path;
    record.field = field;
    record.error = error;
    record.values = std::move(values);
    record.quaternion = quaternion;
    snapshot.push_back(std::move(record));
}

/** FNV-1a over raw bytes: exact comparison of large integer arrays. */
float Checksum(const void* data, size_t size)
{
    const auto* bytes = static_cast<const uint8_t*>(data);
    uint32_t hash = 2166136261u;
    for (size_t index = 0; index < size; index++) {
        hash ^= bytes[index];
        hash *= 16777619u;
    }
    // Stored as two exact halves (a float holds 24 bits exactly).
    return static_cast<float>(hash >> 16) * 65536.0f + static_cast<float>(hash & 0xffff);
}

std::vector<float> ReadMatrix(Handle transform)
{
    std::vector<float> matrix(16, 0.0f);
    Call<int>(transform, slot::transform::get, 16, matrix.data());
    return matrix;
}

void SetIdentity(Handle transform)
{
    Call<int>(transform, slot::transform::setIdentity);
}

void WalkObject(Backend& backend, Handle object, Handle root, const std::string& parentPath, Snapshot& snapshot,
                int depth);

/** Mesh data: what CCollision::CollectMeshInfo / BuildMesh read, plus the appearances' presence. */
void RecordMesh(Handle mesh, const std::string& path, Snapshot& snapshot)
{
    int submeshCount = 0;
    const int submeshError = Call<int>(mesh, slot::mesh::getSubmeshCount, &submeshCount);
    Add(snapshot, path, "submeshCount", submeshError, { static_cast<float>(submeshCount) });
    Handle vertexBuffer = nullptr;
    const int bufferError = Call<int>(mesh, slot::mesh::getVertexBuffer, &vertexBuffer);
    Add(snapshot, path, "vertexBuffer", bufferError, { static_cast<float>(vertexBuffer != nullptr) });
    if (vertexBuffer != nullptr) {
        int vertexCount = 0;
        const int countError = Call<int>(vertexBuffer, slot::vertexbuffer::getVertexCount, &vertexCount);
        Add(snapshot, path, "vertexCount", countError, { static_cast<float>(vertexCount) });
        float scaleBias[4] = {};
        Handle positions = nullptr;
        const int positionsError = Call<int>(vertexBuffer, slot::vertexbuffer::getPositions, 4, scaleBias, &positions);
        Add(snapshot, path, "positionScaleBias", positionsError,
            { scaleBias[0], scaleBias[1], scaleBias[2], scaleBias[3] });
        if (positions != nullptr) {
            int arrayCount = 0;
            const int arrayCountError = Call<int>(positions, slot::vertexarray::getVertexCount, &arrayCount);
            Add(snapshot, path, "positionCount", arrayCountError, { static_cast<float>(arrayCount) });
            if (arrayCount > 0 && arrayCount * 3 <= MaxVertexComponents) {
                std::vector<short> components(static_cast<size_t>(arrayCount) * 3);
                const int getError = Call<int>(positions, slot::vertexarray::get16, 0, arrayCount, arrayCount * 3,
                                               components.data());
                Add(snapshot, path, "positionValues", getError,
                    { Checksum(components.data(), components.size() * sizeof(short)) });
            }
            Release(positions);
        }
        Release(vertexBuffer);
    }
    for (int submesh = 0; submesh < submeshCount; submesh++) {
        const std::string submeshPath = path + "/submesh" + std::to_string(submesh);
        Handle indexBuffer = nullptr;
        const int indexError = Call<int>(mesh, slot::mesh::getIndexBuffer, submesh, &indexBuffer);
        Add(snapshot, submeshPath, "indexBuffer", indexError, { static_cast<float>(indexBuffer != nullptr) });
        if (indexBuffer != nullptr) {
            int indexCount = 0;
            const int indexCountError = Call<int>(indexBuffer, slot::indexbuffer::getIndexCount, &indexCount);
            Add(snapshot, submeshPath, "indexCount", indexCountError, { static_cast<float>(indexCount) });
            if (indexCount > 0) {
                std::vector<unsigned int> indices(static_cast<size_t>(indexCount));
                const int indicesError = Call<int>(indexBuffer, slot::indexbuffer::getIndices, indexCount, indices.data());
                Add(snapshot, submeshPath, "indices", indicesError,
                    { Checksum(indices.data(), indices.size() * sizeof(unsigned int)) });
                // The first triangles in full, to show how a difference looks.
                std::vector<float> head;
                for (size_t index = 0; index < indices.size() && index < IndicesShown; index++) {
                    head.push_back(static_cast<float>(indices[index]));
                }
                Add(snapshot, submeshPath, "firstIndices", indicesError, head);
            }
            Release(indexBuffer);
        }
        Handle appearance = nullptr;
        const int appearanceError = Call<int>(mesh, slot::mesh::getAppearance, submesh, &appearance);
        Add(snapshot, submeshPath, "appearance", appearanceError, { static_cast<float>(appearance != nullptr) });
        if (appearance != nullptr) {
            Release(appearance);
        }
    }
}

/** Orientation angle (degrees) and axis as a quaternion x, y, z, w (one value per orientation). */
std::vector<float> AngleAxisToQuaternion(const float* angleAxis)
{
    const float length = sqrtf(angleAxis[1] * angleAxis[1] + angleAxis[2] * angleAxis[2] + angleAxis[3] * angleAxis[3]);
    const float halfAngle = angleAxis[0] * DegreesToRadians * 0.5f;
    float sine = sinf(halfAngle);
    if (length > 0.0f) {
        sine /= length;
    } else {
        sine = 0.0f;
    }
    return { angleAxis[1] * sine, angleAxis[2] * sine, angleAxis[3] * sine, cosf(halfAngle) };
}

/** Transformable state: TRS, orientation, the generic transform (the game's "composite") and the composite. */
void RecordTransformable(Backend& backend, Handle transformable, const std::string& path, Snapshot& snapshot)
{
    float translation[3] = {};
    const int translationError = Call<int>(transformable, slot::transformable::getTranslation, translation);
    Add(snapshot, path, "translation", translationError, { translation[0], translation[1], translation[2] });
    float scale[3] = {};
    const int scaleError = Call<int>(transformable, slot::transformable::getScale, scale);
    Add(snapshot, path, "scale", scaleError, { scale[0], scale[1], scale[2] });
    float angleAxis[4] = {};
    const int orientationError = Call<int>(transformable, slot::transformable::getOrientation, angleAxis);
    Add(snapshot, path, "orientation", orientationError, AngleAxisToQuaternion(angleAxis), true);
    SetIdentity(backend.transform);
    const int transformError = Call<int>(transformable, slot::transformable::getTransform, backend.transform);
    Add(snapshot, path, "transform", transformError, ReadMatrix(backend.transform));
    SetIdentity(backend.transform);
    const int compositeError = Call<int>(transformable, slot::transformable::getCompositeTransform, backend.transform);
    Add(snapshot, path, "compositeTransform", compositeError, ReadMatrix(backend.transform));
}

/** Node state: switches and the transform to the scene root. */
void RecordNode(Backend& backend, Handle node, Handle root, const std::string& path, Snapshot& snapshot)
{
    int renderingError = 0;
    const int rendering = ReadFlag(node, slot::node::isRenderingEnabled, renderingError);
    Add(snapshot, path, "renderingEnable", renderingError, { static_cast<float>(rendering != 0) });
    int pickingError = 0;
    const int picking = ReadFlag(node, slot::node::isPickingEnabled, pickingError);
    Add(snapshot, path, "pickingEnable", pickingError, { static_cast<float>(picking != 0) });
    float alpha = 0.0f;
    const int alphaError = Call<int>(node, slot::node::getAlphaFactor, &alpha);
    Add(snapshot, path, "alphaFactor", alphaError, { alpha });
    SetIdentity(backend.transform);
    int reachable = 0;
    const int toRootError = Call<int>(node, slot::node::getTransformTo, root, backend.transform,
                                      reinterpret_cast<bool*>(&reachable));
    std::vector<float> matrix = ReadMatrix(backend.transform);
    matrix.push_back(static_cast<float>((reachable & 0xff) != 0));
    Add(snapshot, path, "transformToRoot", toRootError, matrix);
}

void RecordCamera(Handle camera, const std::string& path, Snapshot& snapshot)
{
    float parameters[4] = {};
    int projectionType = 0;
    const int error = Call<int>(camera, slot::camera::getProjectionParams, 4, parameters, &projectionType);
    Add(snapshot, path, "projection", error,
        { parameters[0], parameters[1], parameters[2], parameters[3], static_cast<float>(projectionType) });
}

void WalkChildren(Backend& backend, Handle group, Handle root, const std::string& path, Snapshot& snapshot, int depth)
{
    int childCount = 0;
    const int countError = Call<int>(group, slot::group::getChildCount, &childCount);
    Add(snapshot, path, "childCount", countError, { static_cast<float>(childCount) });
    for (int index = 0; index < childCount; index++) {
        Handle child = nullptr;
        Call<int>(group, slot::group::getChild, index, &child);
        if (child == nullptr) {
            Add(snapshot, path, "missingChild", 0, { static_cast<float>(index) });
            continue;
        }
        WalkObject(backend, child, root, path, snapshot, depth + 1);
        Release(child);
    }
}

/**
 * One animation track as numbers: track user id, controller user id, weight, speed, sequence
 * position now, sequence user id, duration, repeat mode (-1 where a part is missing).
 */
std::vector<float> DescribeTrack(Backend& backend, Handle object, int index)
{
    std::vector<float> values(8, -1.0f);
    Handle track = nullptr;
    Call<int>(object, slot::object3d::getAnimationTrack, index, &track);
    if (track == nullptr) {
        return values;
    }
    int trackId = 0;
    Call<int>(track, slot::object3d::getUserID, &trackId);
    values[0] = static_cast<float>(trackId);
    Handle controller = nullptr;
    Call<int>(track, slot::animationtrack::getController, &controller);
    if (controller != nullptr) {
        int controllerId = 0;
        float weight = 0.0f;
        float speed = 0.0f;
        float position = 0.0f;
        Call<int>(controller, slot::object3d::getUserID, &controllerId);
        Call<int>(controller, slot::animationcontroller::getWeight, &weight);
        Call<int>(controller, slot::animationcontroller::getSpeed, &speed);
        Call<int>(controller, slot::animationcontroller::getPosition, backend.worldTime, &position);
        values[1] = static_cast<float>(controllerId);
        values[2] = weight;
        values[3] = speed;
        values[4] = position;
        Release(controller);
    }
    Handle sequence = nullptr;
    Call<int>(track, slot::animationtrack::getKeyframeSequence, &sequence);
    if (sequence != nullptr) {
        int sequenceId = 0;
        int duration = 0;
        int repeatMode = 0;
        Call<int>(sequence, slot::object3d::getUserID, &sequenceId);
        Call<int>(sequence, slot::keyframesequence::getDuration, &duration);
        Call<int>(sequence, slot::keyframesequence::getRepeatMode, &repeatMode);
        values[5] = static_cast<float>(sequenceId);
        values[6] = static_cast<float>(duration);
        values[7] = static_cast<float>(repeatMode);
        Release(sequence);
    }
    Release(track);
    return values;
}

/** Sort order of track descriptions: plain lexicographic order of the numbers. */
bool TrackComesFirst(const std::vector<float>& left, const std::vector<float>& right)
{
    return left < right;
}

/**
 * The animation tracks of an object as a set. Their order is not compared: m3gcore keeps them
 * sorted by target property (for blending), Swerve in another order, and every place the game
 * walks tracks by index only looks for a controller or the longest duration (CDH_Animation,
 * SwerveHelper), so the order cannot change the game.
 */
void RecordAnimationTracks(Backend& backend, Handle object, int trackCount, const std::string& path, Snapshot& snapshot)
{
    // The getters' error codes are not recorded: CK's getters leave r0 unset (see CompareSnapshots).
    std::vector<std::vector<float>> tracks;
    for (int index = 0; index < trackCount; index++) {
        tracks.push_back(DescribeTrack(backend, object, index));
    }
    std::sort(tracks.begin(), tracks.end(), TrackComesFirst);
    for (size_t index = 0; index < tracks.size(); index++) {
        Add(snapshot, path + "/track" + std::to_string(index), "track", 0, tracks[index]);
    }
}

/** Bit mask of RecordedClasses the object supports. */
unsigned int ClassMask(Handle object)
{
    unsigned int classMask = 0;
    for (size_t index = 0; index < sizeof(RecordedClasses) / sizeof(RecordedClasses[0]); index++) {
        if (Supports(object, RecordedClasses[index])) {
            classMask |= 1u << index;
        }
    }
    return classMask;
}

void WalkObject(Backend& backend, Handle object, Handle root, const std::string& parentPath, Snapshot& snapshot, int depth)
{
    int userId = 0;
    const int userIdError = Call<int>(object, slot::object3d::getUserID, &userId);
    const unsigned int classMask = ClassMask(object);
    const std::string path = parentPath + "/" + std::to_string(classMask) + "#" + std::to_string(userId);
    Add(snapshot, path, "userID", userIdError, { static_cast<float>(userId) });
    Add(snapshot, path, "classMask", 0, { static_cast<float>(classMask) });
    int trackCount = 0;
    const int trackError = Call<int>(object, slot::object3d::getAnimationTrackCount, &trackCount);
    Add(snapshot, path, "animationTracks", trackError, { static_cast<float>(trackCount) });
    RecordAnimationTracks(backend, object, trackCount, path, snapshot);
    if (depth > 64) {
        Add(snapshot, path, "tooDeep", 0, {});
        return;
    }
    if (Supports(object, ClassTransformable)) {
        RecordTransformable(backend, object, path, snapshot);
    }
    if (Supports(object, ClassNode)) {
        RecordNode(backend, object, root, path, snapshot);
    }
    if (Supports(object, ClassCamera)) {
        RecordCamera(object, path, snapshot);
    }
    if (Supports(object, ClassMesh)) {
        RecordMesh(object, path, snapshot);
    }
    if (Supports(object, ClassSkinnedMesh)) {
        Handle skeleton = nullptr;
        const int skeletonError = Call<int>(object, slot::skinnedmesh::getSkeleton, &skeleton);
        Add(snapshot, path, "skeleton", skeletonError, { static_cast<float>(skeleton != nullptr) });
        if (skeleton != nullptr) {
            WalkObject(backend, skeleton, root, path + "/skeleton", snapshot, depth + 1);
            Release(skeleton);
        }
    }
    if (Supports(object, ClassGroup)) {
        WalkChildren(backend, object, root, path, snapshot, depth);
    }
}

/** Everything a pick result tells the game: hit, distance, picked node id, normal and ray. */
void RecordIntersection(Backend& backend, int error, int hit, const std::string& path, Snapshot& snapshot)
{
    std::vector<float> values;
    values.push_back(static_cast<float>(hit != 0));
    if (hit != 0) {
        float distance = 0.0f;
        Call<int>(backend.intersection, slot::rayintersection::getDistance, &distance);
        int pickedId = 0;
        unsigned int pickedClasses = 0;
        Handle picked = nullptr;
        Call<int>(backend.intersection, slot::rayintersection::getIntersected, &picked);
        if (picked != nullptr) {
            Call<int>(picked, slot::object3d::getUserID, &pickedId);
            pickedClasses = ClassMask(picked);
            Release(picked);
        }
        float normal[3] = {};
        Call<int>(backend.intersection, slot::rayintersection::getNormalX, &normal[0]);
        Call<int>(backend.intersection, slot::rayintersection::getNormalY, &normal[1]);
        Call<int>(backend.intersection, slot::rayintersection::getNormalZ, &normal[2]);
        float ray[6] = {};
        Call<int>(backend.intersection, slot::rayintersection::getRay, 6, ray);
        values.push_back(distance);
        // The user id in two exact parts (a float holds 24 bits), then the picked node's classes.
        values.push_back(static_cast<float>(pickedId / IdSplit));
        values.push_back(static_cast<float>(pickedId % IdSplit));
        values.push_back(static_cast<float>(pickedClasses));
        for (float component : normal) {
            values.push_back(component);
        }
        for (float component : ray) {
            values.push_back(component);
        }
    }
    Add(snapshot, path, "pick", error, values);
}

/**
 * Picks of a world: through the active camera on a screen grid (group_pickCamera, the game's shots
 * and hit zone picks) and as rays from the camera position (group_pickNode, SwerveHelper::RayPick).
 */
void RecordPicks(Backend& backend, Handle world, const std::string& path, Snapshot& snapshot)
{
    if (!Supports(world, ClassWorld)) {
        return;
    }
    Handle camera = nullptr;
    Call<int>(world, slot::world::getActiveCamera, &camera);
    if (camera == nullptr) {
        return;
    }
    // Camera axes in world space for the rays.
    SetIdentity(backend.transform);
    int reachable = 0;
    Call<int>(camera, slot::node::getTransformTo, world, backend.transform, reinterpret_cast<bool*>(&reachable));
    const std::vector<float> cameraMatrix = ReadMatrix(backend.transform);
    const float origin[3] = { cameraMatrix[3], cameraMatrix[7], cameraMatrix[11] };
    for (int row = 0; row < PickGrid; row++) {
        for (int column = 0; column < PickGrid; column++) {
            const float x = (column + 0.5f) / PickGrid;
            const float y = (row + 0.5f) / PickGrid;
            const std::string pickPath = path + "/pick(" + std::to_string(column) + "," + std::to_string(row) + ")";
            int hit = 0;
            const int error = Call<int>(world, slot::group::pickCamera, -1, x, y, camera, backend.intersection,
                                        reinterpret_cast<unsigned char*>(&hit));
            RecordIntersection(backend, error, hit & 0xff, pickPath + "camera", snapshot);
            // Ray: forward (-Z of the camera) tilted by the grid position.
            const float side = x - 0.5f;
            const float up = 0.5f - y;
            float direction[3];
            for (int axis = 0; axis < 3; axis++) {
                const int row4 = axis * 4;
                direction[axis] = -cameraMatrix[row4 + 2] + side * cameraMatrix[row4] + up * cameraMatrix[row4 + 1];
            }
            int rayHit = 0;
            const int rayError = Call<int>(world, slot::group::pickNode, -1, static_cast<const float*>(origin),
                                           static_cast<const float*>(direction), backend.intersection,
                                           reinterpret_cast<unsigned char*>(&rayHit));
            RecordIntersection(backend, rayError, rayHit & 0xff, pickPath + "ray", snapshot);
        }
    }
    Release(camera);
}

/** Everything compared for one root object at one moment (the picks only when `withPicks`). */
void SnapshotRoot(Backend& backend, Handle root, const std::string& label, Snapshot& snapshot, bool withPicks = true)
{
    Handle rootNode = nullptr;
    if (Supports(root, ClassNode)) {
        rootNode = root;
    }
    WalkObject(backend, root, rootNode, label, snapshot, 0);
    if (withPicks) {
        RecordPicks(backend, root, label, snapshot);
    }
}

/** The animation controllers of a scene by user id (nodes and skeletons; id 0 is skipped). */
typedef std::map<int, Handle> Controllers;

void CollectControllers(Handle object, Controllers& controllers, int depth)
{
    if (depth > 64) {
        return;
    }
    int trackCount = 0;
    Call<int>(object, slot::object3d::getAnimationTrackCount, &trackCount);
    for (int index = 0; index < trackCount; index++) {
        Handle track = nullptr;
        Call<int>(object, slot::object3d::getAnimationTrack, index, &track);
        if (track == nullptr) {
            continue;
        }
        Handle controller = nullptr;
        Call<int>(track, slot::animationtrack::getController, &controller);
        Release(track);
        if (controller == nullptr) {
            continue;
        }
        int id = 0;
        Call<int>(controller, slot::object3d::getUserID, &id);
        if (id == 0 || controllers.count(id) != 0) {
            Release(controller);
            continue;
        }
        // Keeps the reference until ReleaseControllers.
        controllers[id] = controller;
    }
    if (Supports(object, ClassSkinnedMesh)) {
        Handle skeleton = nullptr;
        Call<int>(object, slot::skinnedmesh::getSkeleton, &skeleton);
        if (skeleton != nullptr) {
            CollectControllers(skeleton, controllers, depth + 1);
            Release(skeleton);
        }
    }
    if (Supports(object, ClassGroup)) {
        int childCount = 0;
        Call<int>(object, slot::group::getChildCount, &childCount);
        for (int index = 0; index < childCount; index++) {
            Handle child = nullptr;
            Call<int>(object, slot::group::getChild, index, &child);
            if (child != nullptr) {
                CollectControllers(child, controllers, depth + 1);
                Release(child);
            }
        }
    }
}

void ReleaseControllers(Controllers& controllers)
{
    for (auto& entry : controllers) {
        Release(entry.second);
    }
    controllers.clear();
}

/** Weight 1 for `solo`, 0 for every other controller; `solo` starts its sequence at `worldTime`. */
void SoloController(Controllers& controllers, int solo, int worldTime)
{
    for (auto& entry : controllers) {
        if (entry.first == solo) {
            Call<int>(entry.second, slot::animationcontroller::setWeight, 1.0f);
            Call<int>(entry.second, slot::animationcontroller::setPosition, SoloSequenceStart, worldTime);
        } else {
            Call<int>(entry.second, slot::animationcontroller::setWeight, 0.0f);
        }
    }
}

int Animate(Handle object, int worldTime)
{
    int validity = 0;
    return Call<int>(object, slot::object3d::animate, worldTime, &validity);
}

bool FloatsAgree(float left, float right)
{
    if (isnan(left) && isnan(right)) {
        return true;
    }
    float scale = 1.0f;
    if (fabsf(left) > scale) {
        scale = fabsf(left);
    }
    if (fabsf(right) > scale) {
        scale = fabsf(right);
    }
    return fabsf(left - right) <= Tolerance * scale;
}

// Pick records: hit, distance, id / IdSplit, id % IdSplit, classes, normal (3), ray (6).
constexpr size_t PickDistance = 1;
constexpr size_t PickIdHigh = 2;
constexpr size_t PickIdLow = 3;
constexpr size_t PickClasses = 4;
constexpr size_t PickNormal = 5;
constexpr size_t PickRay = 8;
// Skinning precision: the original skins with slightly other numbers, so the face normal of a
// skinned hit may turn by about a degree. Normals agree within this per component (about 2 deg).
constexpr float NormalTolerance = 0.035f;
// Coplanar faces at the same distance (decals on walls) are hit in an order that depends on the
// last bit of each backend's arithmetic; such ties are counted, not failed.
constexpr float TieTolerance = 2e-4f;
// Ties seen in the current test (reset per file).
int pickTies = 0;

/** Pick records: the hit and its ray strictly, the normal within NormalTolerance, ties counted. */
bool PicksAgree(const Record& expected, const Record& actual)
{
    if (expected.values.size() != actual.values.size()) {
        return false;
    }
    if (expected.values.size() <= 1) {
        return expected.values == actual.values;
    }
    for (size_t index = PickRay; index < expected.values.size(); index++) {
        if (!FloatsAgree(expected.values[index], actual.values[index])) {
            return false;
        }
    }
    const float expectedDistance = expected.values[PickDistance];
    const float actualDistance = actual.values[PickDistance];
    const bool sameNode = expected.values[PickIdHigh] == actual.values[PickIdHigh] &&
                          expected.values[PickIdLow] == actual.values[PickIdLow] &&
                          expected.values[PickClasses] == actual.values[PickClasses];
    if (!sameNode) {
        const float scale = fabsf(expectedDistance) + 1e-6f;
        if (fabsf(expectedDistance - actualDistance) <= TieTolerance * scale) {
            pickTies++;
            return true;
        }
        return false;
    }
    if (!FloatsAgree(expectedDistance, actualDistance)) {
        return false;
    }
    for (size_t index = PickNormal; index < PickRay; index++) {
        if (fabsf(expected.values[index] - actual.values[index]) > NormalTolerance) {
            return false;
        }
    }
    return true;
}

bool ValuesAgree(const Record& expected, const Record& actual)
{
    if (expected.field == "pick") {
        return PicksAgree(expected, actual);
    }
    if (expected.values.size() != actual.values.size()) {
        return false;
    }
    bool same = true;
    bool negated = expected.quaternion;
    for (size_t index = 0; index < expected.values.size(); index++) {
        if (!FloatsAgree(expected.values[index], actual.values[index])) {
            same = false;
        }
        if (!FloatsAgree(expected.values[index], -actual.values[index])) {
            negated = false;
        }
    }
    return same || negated;
}

std::string FormatValues(const std::vector<float>& values)
{
    std::string text;
    for (size_t index = 0; index < values.size(); index++) {
        char number[32];
        snprintf(number, sizeof(number), "%.6g", values[index]);
        if (index > 0) {
            text += " ";
        }
        text += number;
    }
    return text;
}

/**
 * Compare the native snapshot with the original one. Records are matched in walk order; a
 * structural difference (another path) stops the comparison because the walks diverged.
 * Returns the number of mismatching records.
 */
int CompareSnapshots(const std::string& file, const Snapshot& original, const Snapshot& native)
{
    int mismatches = 0;
    const size_t count = std::min(original.size(), native.size());
    for (size_t index = 0; index < count; index++) {
        const Record& expected = original[index];
        const Record& actual = native[index];
        if (expected.path != actual.path || expected.field != actual.field) {
            ADD_FAILURE() << file << ": scene graphs diverge at record " << index << "\n  original " << expected.path
                          << " " << expected.field << "\n  native   " << actual.path << " " << actual.field;
            return mismatches + 1;
        }
        // Error codes are shown but not compared: CK's simple getters (object3d_getUserID,
        // node_isPickingEnabled, ...) return without setting r0, so the original's code is whatever
        // the register held; the game never reads them.
        if (ValuesAgree(expected, actual)) {
            continue;
        }
        mismatches++;
        if (mismatches <= ReportedMismatches || SDL_getenv(ReportAllVariable) != nullptr) {
            ADD_FAILURE() << file << ": " << expected.path << " " << expected.field << "\n  original (error " << expected.error
                          << "): " << FormatValues(expected.values) << "\n  native   (error " << actual.error
                          << "): " << FormatValues(actual.values);
        }
    }
    if (original.size() != native.size()) {
        ADD_FAILURE() << file << ": " << original.size() << " original records, " << native.size() << " native records";
        mismatches++;
    }
    if (mismatches > ReportedMismatches) {
        ADD_FAILURE() << file << ": " << mismatches << " mismatching records in total";
    }
    return mismatches;
}

/** Both backends for the whole suite: the game's native runtime and one original ARM runtime. */
struct Backends {
    Backend native;
    Backend original;
    bool ready;
};

Backends backends = {};

Backend MakeBackend(const char* name, ISwerve* swerve)
{
    Backend backend = {};
    backend.name = name;
    backend.swerve = swerve;
    backend.transform = CreateObject(swerve, ClassTransform);
    backend.intersection = CreateObject(swerve, ClassRayIntersection);
    return backend;
}

class M3gBackends : public testing::TestWithParam<M3gFile> {
protected:
    static void SetUpTestSuite()
    {
        if (backends.ready) {
            return;
        }
        GameHarness* harness = GameHarness::Get();
        if (harness == nullptr) {
            return;
        }
        // CSwerve exists once the applet is initialized; a few frames settle the start-up.
        harness->RunFrames(3);
        ISwerve* original = OriginalRuntime();
        if (original == nullptr) {
            return;
        }
        backends.native = MakeBackend("native", NativeRuntime());
        backends.original = MakeBackend("original", original);
        backends.ready = true;
    }
};

/** Solo phase: every controller alone, as the game plays an animation. Returns the mismatches. */
int CompareSoloAnimations(const std::string& file, const std::string& label, Handle originalRoot, Handle nativeRoot)
{
    Controllers originalControllers;
    Controllers nativeControllers;
    CollectControllers(originalRoot, originalControllers, 0);
    CollectControllers(nativeRoot, nativeControllers, 0);
    int mismatches = 0;
    if (originalControllers.size() != nativeControllers.size()) {
        ADD_FAILURE() << file << ": " << originalControllers.size() << " original controllers, " << nativeControllers.size()
                      << " native controllers";
        mismatches++;
    }
    size_t soloCount = 0;
    for (auto& entry : originalControllers) {
        if (soloCount == SoloControllers) {
            break;
        }
        if (nativeControllers.count(entry.first) == 0) {
            continue;
        }
        soloCount++;
        const int start = SoloStartTime + static_cast<int>(soloCount) * 10000;
        SoloController(originalControllers, entry.first, start);
        SoloController(nativeControllers, entry.first, start);
        for (int offset : SoloTimes) {
            const int time = start + offset;
            Animate(originalRoot, time);
            Animate(nativeRoot, time);
            backends.original.worldTime = time;
            backends.native.worldTime = time;
            Snapshot originalSnapshot;
            Snapshot nativeSnapshot;
            const std::string soloLabel = label + "@solo" + std::to_string(entry.first) + "+" + std::to_string(offset);
            SnapshotRoot(backends.original, originalRoot, soloLabel, originalSnapshot, false);
            SnapshotRoot(backends.native, nativeRoot, soloLabel, nativeSnapshot, false);
            mismatches += CompareSnapshots(file + " (solo " + std::to_string(entry.first) + " +" + std::to_string(offset) + ")",
                                           originalSnapshot, nativeSnapshot);
        }
    }
    ReleaseControllers(originalControllers);
    ReleaseControllers(nativeControllers);
    return mismatches;
}

TEST_P(M3gBackends, SameSceneGraph)
{
    ASSERT_TRUE(backends.ready) << "both Swerve backends are needed (Engine=m3g, swerve-arm.bin present)";
    const M3gFile& param = GetParam();
    const std::string file = FileName(param);
    const std::vector<uint8_t> data = ReadM3gFile(param);
    ASSERT_TRUE(IsM3gData(data));

    pickTies = 0;
    std::vector<Handle> originalRoots;
    std::vector<Handle> nativeRoots;
    const int originalError = LoadWith(backends.original.swerve, data, originalRoots);
    const int nativeError = LoadWith(backends.native.swerve, data, nativeRoots);
    ASSERT_EQ(originalError, 0) << file << ": the original Swerve does not load it";
    ASSERT_EQ(nativeError, 0) << file << ": the native runtime does not load it";
    ASSERT_EQ(originalRoots.size(), nativeRoots.size()) << file << ": root object count";

    int mismatches = 0;
    for (size_t rootIndex = 0; rootIndex < originalRoots.size(); rootIndex++) {
        const std::string label = "root" + std::to_string(rootIndex);
        // As loaded.
        backends.original.worldTime = 0;
        backends.native.worldTime = 0;
        Snapshot originalSnapshot;
        Snapshot nativeSnapshot;
        SnapshotRoot(backends.original, originalRoots[rootIndex], label, originalSnapshot);
        SnapshotRoot(backends.native, nativeRoots[rootIndex], label, nativeSnapshot);
        mismatches += CompareSnapshots(file + " (loaded)", originalSnapshot, nativeSnapshot);
        // After animate() at increasing world times, like the game's per-frame animation.
        for (int time : AnimationTimes) {
            backends.original.worldTime = time;
            backends.native.worldTime = time;
            const int originalAnimateError = Animate(originalRoots[rootIndex], time);
            const int nativeAnimateError = Animate(nativeRoots[rootIndex], time);
            EXPECT_EQ(originalAnimateError == 0, nativeAnimateError == 0) << file << ": animate(" << time << ") error";
            Snapshot originalAnimated;
            Snapshot nativeAnimated;
            const std::string animatedLabel = label + "@" + std::to_string(time);
            SnapshotRoot(backends.original, originalRoots[rootIndex], animatedLabel, originalAnimated);
            SnapshotRoot(backends.native, nativeRoots[rootIndex], animatedLabel, nativeAnimated);
            mismatches += CompareSnapshots(file + " (animate " + std::to_string(time) + ")", originalAnimated, nativeAnimated);
        }
    }
    ReleaseRoots(originalRoots);
    ReleaseRoots(nativeRoots);

    // Solo phase on freshly loaded scenes: a property whose controllers all have weight 0 keeps its
    // last value, so the scenes animated above would carry their differences into it.
    ASSERT_EQ(LoadWith(backends.original.swerve, data, originalRoots), 0);
    ASSERT_EQ(LoadWith(backends.native.swerve, data, nativeRoots), 0);
    for (size_t rootIndex = 0; rootIndex < originalRoots.size() && rootIndex < nativeRoots.size(); rootIndex++) {
        const std::string label = "root" + std::to_string(rootIndex);
        mismatches += CompareSoloAnimations(file, label, originalRoots[rootIndex], nativeRoots[rootIndex]);
    }
    ReleaseRoots(originalRoots);
    ReleaseRoots(nativeRoots);
    fprintf(stderr, "M3G backend test: %s %zu bytes, %d mismatching records, %d coplanar pick ties\n", file.c_str(),
            data.size(), mismatches, pickTies);
}

std::string ParamName(const testing::TestParamInfo<M3gFile>& info)
{
    return FileName(info.param);
}

INSTANTIATE_TEST_SUITE_P(CK, M3gBackends, testing::ValuesIn(M3gFiles()), ParamName);

} // namespace
} // namespace ck::test
