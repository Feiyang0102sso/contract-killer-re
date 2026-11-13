// Files (replaces CFileMgr_Cocoa.cpp / CFileMgr_Cocoa_mm.mm). Win32 wide paths let players install
// the port in a non-ASCII directory.
#include "Host.h"
#include <filesystem>
#include "CFile_Cocoa.h"
#include "ICFileMgr.h"

namespace fs = std::filesystem;

namespace {

class FileMgrSDL final : public ICFileMgr {
public:
    const wchar_t* GetApplicationPath() override { return ck::assetPath; }
    const wchar_t* GetApplicationDataPath() override { return ck::savePath; }
    const wchar_t* GetApplicationTempPath() override { return ck::savePath; }
    const wchar_t* GetApplicationCachePath() override { return ck::savePath; }
    const wchar_t* GetPathSeparator() override { return L"/"; }

    ICFile* Open(const wchar_t* path, eFileAccessMode mode) override
    {
        if (path == nullptr) {
            return nullptr;
        }
        const wchar_t* modes[] = { L"rb", L"wb", L"ab" };
        FILE* stream = _wfopen(path, modes[mode]);
        if (stream == nullptr) {
            return nullptr;
        }
        auto* file = new CFile_Cocoa;
        file->m_file = stream;
        return file;
    }

    void Close(ICFile* file) override { delete file; }

    unsigned char Exists(const wchar_t* path) override
    {
        return path != nullptr && fs::is_regular_file(path, m_error);
    }

    unsigned int Size(const wchar_t* path) override
    {
        if (path == nullptr) {
            return 0;
        }
        const auto size = fs::file_size(path, m_error);
        if (m_error) {
            return 0;
        }
        return static_cast<unsigned int>(size);
    }

    unsigned char Rename(const wchar_t* from, const wchar_t* to) override
    {
        if (from == nullptr || to == nullptr) {
            return 0;
        }
        fs::rename(from, to, m_error);
        return !m_error;
    }

    unsigned char Delete(const wchar_t* path) override
    {
        return path != nullptr && fs::remove(path, m_error);
    }

    unsigned char MkDir(const wchar_t* path) override
    {
        if (path == nullptr) {
            return 0;
        }
        fs::create_directories(path, m_error);
        return !m_error;
    }

    // As in CK 1.5.2: directories are never removed or emptied, and the free space is not measured.
    unsigned char RmDir(const wchar_t*) override { return 0; }
    unsigned char DeleteFilesInDir(const wchar_t*) override { return 0; }
    unsigned int GetFreeDiskSpace() override { return 0x7fffffff; }

private:
    std::error_code m_error;
};

} // namespace

ICFileMgr* ICFileMgr::CreateInstance() { return new FileMgrSDL; }
