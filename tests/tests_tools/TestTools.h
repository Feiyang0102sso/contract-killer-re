// Tools for driving the game by hand or from scripts, compiled only into the test program
// (vs/ck_tests.vcxproj, BountyHunter_tests.exe --play). The automated assertions are the gtest
// suites in tests/. Applet.cpp calls these hooks under CK_TESTS.
//
// CK version: F8 (ARM profiler, see ArmProfiler.h) works. CKZ's auto play into a mission and its
// head pick self-test are not connected to CK yet; their hooks do nothing.
#pragma once

#include <SDL3/SDL.h>

class CApplet;

namespace ck::test {

/** Call once per frame; the head pick self-test (not connected to CK yet). */
void UpdatePickTest();

/** Call once per frame; the auto play mission (not connected to CK yet). */
void UpdateAutoPlay();

/** Handle a test key press or release; true when the key was a test key. */
bool HandleTestKey(CApplet& applet, SDL_Keycode key, bool down);

} // namespace ck::test
