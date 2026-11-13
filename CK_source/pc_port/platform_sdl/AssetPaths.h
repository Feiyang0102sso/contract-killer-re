// PC resource layout: the single place that names every folder and file the game needs on disk.
// Startup checks the whole RequiredFiles table and reports each missing file by name.
#pragma once

namespace ck {

/** Where a required file lives. */
enum class FileLocation {
    AssetsFolder,     // copied by the player from the original game package
    ExecutableFolder, // placed next to BountyHunter.exe by the build
};

struct RequiredFile {
    const wchar_t* name;
    FileLocation location;
    const char* purpose; // shown in the error message when the file is missing
};

// Both folders sit next to BountyHunter.exe unless the command line overrides them:
//   BountyHunter.exe [assets folder] [saves folder]
constexpr wchar_t AssetsFolderName[] = L"assets";
constexpr wchar_t SavesFolderName[] = L"saves";

// The Swerve image is assembled from swerve_core.s by the build (scripts/make_swerve_image.py),
// so it stays with the executable and does not move when the assets folder is overridden.
constexpr wchar_t SwerveImageName[] = L"swerve-arm.bin";

constexpr RequiredFile RequiredFiles[] = {
    { L"BountyHunter.big", FileLocation::AssetsFolder, "game data archive" },
    { L"xga.big", FileLocation::AssetsFolder, "1024x768 graphics archive" },
    { L"vga.big", FileLocation::AssetsFolder, "960x640 graphics archive, also used at 1024x768" },
    { L"en.big", FileLocation::AssetsFolder, "English text archive" },
    { L"res_map.dat", FileLocation::AssetsFolder, "resource name map" },
    { L"glu_logo_landscape.m4v", FileLocation::AssetsFolder, "Glu logo movie" },
    { L"ck_logo_1024x768.m4v", FileLocation::AssetsFolder, "Contract Killer logo movie" },
    { SwerveImageName, FileLocation::ExecutableFolder, "Swerve 3D engine image, assembled by the build" },
};

} // namespace ck
