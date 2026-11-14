// Test: first check of the native M3G engine: load M3G files with m3gcore.
// Compiled only into the test program (vs/ck_tests.vcxproj).
#pragma once

namespace ck::m3g {

// Set to a folder of .m3g files to run the load test instead of the game; the process then exits.
constexpr const char* LoadTestVariable = "CK_M3G_LOADTEST";

/** Load every .m3g file in `folder` with m3gcore and print one line per file. Returns 0 if all loaded. */
int RunLoadTest(const char* folder);

} // namespace ck::m3g
