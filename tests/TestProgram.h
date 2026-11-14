// Entry of the test program BountyHunter_tests.exe (vs/ck_tests.vcxproj). It shares the game's
// folder, assets, saves and BountyHunter.cfg, and writes BountyHunter_tests.log.
//
//   BountyHunter_tests.exe [gtest options]   run the automated tests (GoogleTest), see tests/README.md
//   BountyHunter_tests.exe --play            play the game with the tools of tests_tools/TestTools.h
//   CK_M3G_LOADTEST=<folder>                 load every .m3g of the folder with m3gcore (tests_tools/M3gLoadTest.h)
//
// The automated tests run in the background: the window starts minimized (its taskbar button
// shows the run and closes it at any time) and the sound goes to OpenAL Soft's null device.
// --play runs in the foreground unless CK_HIDDEN is set, which gives it the same background mode.
#pragma once

namespace ck::test {

// Option that runs the game with the test tools instead of the automated tests.
constexpr const wchar_t* PlayOption = L"--play";
// Environment variable: background mode (minimized, muted, no dialogs) for --play too.
constexpr const char* HiddenVariable = "CK_HIDDEN";
// Environment variable: mute --play without minimizing it.
constexpr const char* MuteVariable = "CK_MUTE";
// Environment variable: overrides [graph] Engine of BountyHunter.cfg ("m3g" or "swerve").
constexpr const char* EngineVariable = "CK_ENGINE";

/** Decide the run mode from the arguments and mute the sound if needed; call before SDL starts. */
void PrepareTestProcess(int argc, wchar_t** argv);

/** True when the window starts minimized and no dialog may be shown. */
bool RunInBackground();

/** Run the mode chosen by PrepareTestProcess once the window exists. Returns the exit code. */
int RunTestProgram(int argc, wchar_t** argv);

} // namespace ck::test
