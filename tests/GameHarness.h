// Drives the real game inside the test program: one applet for the whole run, advanced frame by
// frame by the tests. The window exists already (Applet.cpp creates it before RunTestProgram).
// CK version: start, frames and taps. Reaching a mission from the map (CKZ's ReachMap /
// EnterSurvival) is not connected to CK yet.
#pragma once

#include <SDL3/SDL.h>

class CApplet;

namespace ck::test {

// Frame pacing of the tests: the window is minimized, so vertical sync does not limit the frame
// rate any more; the harness waits instead (an unlimited frame rate once crashed the GPU driver).
constexpr Uint64 TestFrameNs = 1000000000ull / 60;

/** A condition the harness waits for; checked once per frame. */
typedef bool (*Condition)();

class GameHarness {
public:
    /** The running game; started on first use. Fails the calling test when the game cannot start. */
    static GameHarness* Get();

    /** Run `count` frames. */
    void RunFrames(int count);
    /** Run frames for `seconds` of wall clock time. */
    void RunSeconds(double seconds);
    /** Run frames until `condition` holds; false when `seconds` pass first. */
    bool RunUntil(Condition condition, double seconds);

    /** Tap the screen at game pixels `x`, `y` (a touch that begins and ends one frame later). */
    void Tap(int x, int y);

    CApplet& Applet();

private:
    GameHarness() = default;
    bool Start();
    void Frame();

    CApplet* m_applet = nullptr;
    void* m_input = nullptr; // ck::LoopInput, kept opaque to keep Host.h out of the tests
};

} // namespace ck::test
