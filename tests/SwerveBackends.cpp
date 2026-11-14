// Shared parts of the backend tests, see SwerveBackends.h.
#include "SwerveBackends.h"
#include "../CK_source/pc_port/swerve_compat/ArmBackend.h"
#include "../CK_source/pc_port/platform_sdl/Config.h"
#include "../CK_source/pc_port/platform_sdl/Host.h"
// The PC copy of the Swerve API (ARM-aware calls) must come before any game header that
// includes "SwerveApi.h" next to itself (packages/swerve/code/inc, the iOS original).
#include <SwerveApi.h>
#include <stdio.h>
#include "CSwerve.h"

namespace ck::test {
namespace {

// The archives whose M3G files are compared (in the asset folder).
const wchar_t* const Archives[] = { L"BountyHunter.big", L"vga.big" };

std::filesystem::path ArchivePath(const wchar_t* archive)
{
    return std::filesystem::path(ck::assetPath) / archive;
}

} // namespace

void Release(Handle object)
{
    Call<void>(object, slot::Release);
}

Handle CreateObject(ISwerve* swerve, int type)
{
    void* object = nullptr;
    swerve->m_functions->CreateObject(type, &object);
    return object;
}

bool Supports(Handle object, int classId)
{
    void* found = nullptr;
    const int error = Call<int>(object, slot::QueryInterface, classId, &found);
    if (error != 0 || found == nullptr) {
        return false;
    }
    Release(found);
    return true;
}

int LoadWith(ISwerve* runtime, const std::vector<uint8_t>& data, std::vector<Handle>& roots)
{
    Handle loader = CreateObject(runtime, ClassLoader);
    if (loader == nullptr) {
        return -1;
    }
    int error = Call<int>(loader, slot::loader::loadBuffer, static_cast<unsigned int>(data.size()), data.data(), 0);
    if (error == 0) {
        int count = 0;
        Call<int>(loader, slot::loader::getRootCount, &count);
        for (int index = 0; index < count; index++) {
            Handle root = nullptr;
            Call<int>(loader, slot::loader::getRoot, index, &root);
            if (root != nullptr) {
                roots.push_back(root);
            }
        }
    }
    Release(loader);
    return error;
}

void ReleaseRoots(std::vector<Handle>& roots)
{
    for (Handle root : roots) {
        Release(root);
    }
    roots.clear();
}

std::vector<M3gFile> M3gFiles()
{
    static std::vector<M3gFile> files;
    static bool scanned = false;
    if (scanned) {
        return files;
    }
    scanned = true;
    for (const wchar_t* name : Archives) {
        BigArchive archive;
        if (!archive.Open(ArchivePath(name))) {
            fprintf(stderr, "Swerve backend tests: %ls not found\n", name);
            continue;
        }
        size_t found = 0;
        for (const BigEntry& entry : archive.Entries()) {
            if (IsM3gData(archive.Read(entry))) {
                M3gFile file;
                file.archive = name;
                file.entry = entry;
                files.push_back(file);
                found++;
            }
        }
        fprintf(stderr, "Swerve backend tests: %zu M3G files in %ls\n", found, name);
    }
    return files;
}

std::string FileName(const M3gFile& file)
{
    std::string archive;
    for (const wchar_t* character = file.archive; *character != L'\0' && *character != L'.'; character++) {
        archive += static_cast<char>(*character);
    }
    return archive + "_" + EntryName(file.entry);
}

std::vector<uint8_t> ReadM3gFile(const M3gFile& file)
{
    BigArchive archive;
    if (!archive.Open(ArchivePath(file.archive))) {
        return {};
    }
    return archive.Read(file.entry);
}

ISwerve* NativeRuntime()
{
    if (CSwerve::GetInstance() == nullptr) {
        return nullptr;
    }
    return CSwerve::GetInstance()->m_swerve;
}

ISwerve* OriginalRuntime()
{
    static ISwerve* original = nullptr;
    static bool tried = false;
    if (tried) {
        return original;
    }
    tried = true;
    if (ck::config.engine != ck::EngineM3g || NativeRuntime() == nullptr) {
        fprintf(stderr, "Swerve backend tests: need Engine=m3g in BountyHunter.cfg and a started game\n");
        return nullptr;
    }
    ISwerve* created = nullptr;
    const int createError = ck::arm::CreateSwerve(&created);
    fprintf(stderr, "Swerve backend tests: ARM swvNew returned %d\n", createError);
    if (createError != 0 || created == nullptr) {
        return nullptr;
    }
    const int initializeError = created->m_functions->Initialize(created, 0, 0);
    fprintf(stderr, "Swerve backend tests: ARM initialize returned %d\n", initializeError);
    if (initializeError != 0) {
        return nullptr;
    }
    original = created;
    return original;
}

} // namespace ck::test
