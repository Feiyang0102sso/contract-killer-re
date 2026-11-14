// What the backend tests (M3gBackendTest.cpp, M3gRenderTest.cpp) share: slot calls that work on an
// ARM object and on a native one, the M3G files of the archives, and the two Swerve runtimes.
#pragma once

#include "BigArchive.h"
#include "../CK_source/pc_port/swerve_compat/ArmCall.h"
#include "../CK_source/pc_port/swerve_native/SwerveSlots.h"
#include <filesystem>
#include <string>
#include <vector>

struct ISwerve;

namespace ck::test {

namespace slot = ck::native::slot;

// Swerve class ids (CreateObject types and QueryInterface ids, NativeRuntime.h).
constexpr int ClassCamera = 5;
constexpr int ClassGraphics3D = 8;
constexpr int ClassGroup = 9;
constexpr int ClassLight = 13;
constexpr int ClassLoader = 14;
constexpr int ClassMesh = 16;
constexpr int ClassNode = 18;
constexpr int ClassRayIntersection = 21;
constexpr int ClassSkinnedMesh = 22;
constexpr int ClassSprite3D = 23;
constexpr int ClassTransform = 26;
constexpr int ClassTransformable = 27;
constexpr int ClassWorld = 31;

/** A Swerve object of either backend (first word: its slot table). */
typedef void* Handle;

/** Call slot `index` of `object` (ARM or native). */
template<class Result, class... Arguments>
Result Call(const void* object, int index, Arguments... arguments)
{
    return ck::arm::Method<Result>(object, static_cast<size_t>(index), arguments...);
}

void Release(Handle object);

/** CreateObject of the runtime (no self argument); NULL on failure. */
Handle CreateObject(ISwerve* swerve, int type);

/** QueryInterface for `classId`; the returned reference is released at once (only "is it?" is asked). */
bool Supports(Handle object, int classId);

/** Load `data` like CSwerve::LoadM3GRoot: a loader of the runtime, loadBuffer, then every root. */
int LoadWith(ISwerve* runtime, const std::vector<uint8_t>& data, std::vector<Handle>& roots);

void ReleaseRoots(std::vector<Handle>& roots);

/** One M3G file of an archive. */
struct M3gFile {
    const wchar_t* archive;
    BigEntry entry;
};

/** The M3G resources of BountyHunter.big and vga.big, read once. */
std::vector<M3gFile> M3gFiles();

/** Test and message name of a file, e.g. "BountyHunter_m3g_0031a2f0". */
std::string FileName(const M3gFile& file);

/** The bytes of a file (empty on error). */
std::vector<uint8_t> ReadM3gFile(const M3gFile& file);

/** The game's runtime (native with Engine=m3g); NULL before the game started. */
ISwerve* NativeRuntime();

/**
 * One original ARM runtime for the whole run, created and initialized on first use (needs the game
 * started, see GameHarness, and Engine=m3g so that the game's runtime is the native one).
 * NULL when that is not possible; the reason is logged.
 */
ISwerve* OriginalRuntime();

} // namespace ck::test
