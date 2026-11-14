// Drives the real game inside the test program, see GameHarness.h.
#include "GameHarness.h"
#include "../CK_source/pc_port/platform_sdl/Host.h"
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <SwerveApi.h>
#include "CApplet.h"

namespace ck::test {
namespace {

GameHarness* instance = nullptr;
bool startFailed = false;

/** Encode a touch position like the platform layer does (x in bits 0-13, y in bits 14-27). */
unsigned int TouchPosition(int x, int y)
{
    return static_cast<unsigned int>(x) | (static_cast<unsigned int>(y) << 14);
}

} // namespace

GameHarness* GameHarness::Get()
{
    if (instance != nullptr) {
        return instance;
    }
    if (startFailed) {
        ADD_FAILURE() << "the game could not start (see an earlier test)";
        return nullptr;
    }
    GameHarness* harness = new GameHarness();
    if (!harness->Start()) {
        startFailed = true;
        delete harness;
        ADD_FAILURE() << "the game could not start; check BountyHunter_tests.log";
        return nullptr;
    }
    instance = harness;
    return instance;
}

bool GameHarness::Start()
{
    fprintf(stderr, "Test harness: starting the game\n");
    // The GL device requires the view's drawable layer (RunApplet does the same).
    CApplet::m_eaglLayer = ck::window;
    m_applet = new CApplet();
    if (!m_applet->Initialize()) {
        fprintf(stderr, "Test harness: CApplet::Initialize failed\n");
        return false;
    }
    m_input = new ck::LoopInput();
    ck::running = true;
    m_applet->Start();
    return true;
}

void GameHarness::Frame()
{
    const Uint64 start = SDL_GetTicksNS();
    ck::RunFrame(*m_applet, *static_cast<ck::LoopInput*>(m_input));
    if (!ck::running) {
        // The window was closed from the taskbar (or the game quit): stop the whole run here.
        fprintf(stderr, "Test harness: the game window was closed, stopping the test run\n");
        fflush(stderr);
        _exit(3);
    }
    const Uint64 elapsed = SDL_GetTicksNS() - start;
    if (elapsed < TestFrameNs) {
        SDL_DelayPrecise(TestFrameNs - elapsed);
    }
}

void GameHarness::RunFrames(int count)
{
    for (int index = 0; index < count; index++) {
        Frame();
    }
}

void GameHarness::RunSeconds(double seconds)
{
    const Uint64 end = SDL_GetTicksNS() + static_cast<Uint64>(seconds * 1e9);
    while (SDL_GetTicksNS() < end) {
        Frame();
    }
}

bool GameHarness::RunUntil(Condition condition, double seconds)
{
    const Uint64 end = SDL_GetTicksNS() + static_cast<Uint64>(seconds * 1e9);
    while (SDL_GetTicksNS() < end) {
        if (condition()) {
            return true;
        }
        Frame();
    }
    return condition();
}

void GameHarness::Tap(int x, int y)
{
    const unsigned int position = TouchPosition(x, y);
    m_applet->QueueSystemEvent(ck::TouchBegan, 1, position | (1u << 28));
    Frame();
    m_applet->QueueSystemEvent(ck::TouchEnded, 1, position);
    Frame();
}

CApplet& GameHarness::Applet()
{
    return *m_applet;
}

} // namespace ck::test
