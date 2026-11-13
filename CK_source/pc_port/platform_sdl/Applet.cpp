// Bridge SDL lifecycle and pointer events to the original Glu applet.
#include "Host.h"
#include "Config.h"
#include "DebugTools.h"
#include "Log.h"
#ifdef CK_TESTS
#include "../../../tests/TestProgram.h"
#include "../../../tests/tests_tools/TestTools.h"
#endif
#include <stdio.h>
#include <wchar.h>
#include <string>
#include <vector>
#include <filesystem>
#include "CApplet.h"
#include "CApp.h"
#include "ICCore.h"


CApplet* CApplet::m_pThis;
ICCore* CApplet::m_pCore;
CApp* CApplet::m_pApp;
short CApplet::m_UIScreenMainScreenWidth = ck::WindowWidth;
short CApplet::m_UIScreenMainScreenHeight = ck::WindowHeight;
void* CApplet::m_eaglLayer;

namespace ck {
SDL_Window* window;
SDL_GLContext context;
const wchar_t* assetPath;
const wchar_t* savePath;
bool running;
}

CApplet::CApplet()
    : m_tickRate(1), m_accelerometerFrequency(0), m_multipleTouchEnabled(0), m_gyroscope(nullptr)
{
    m_pThis = this;
}

CApplet::~CApplet()
{
    Destroy();
    m_pThis = nullptr;
}

unsigned char CApplet::Initialize()
{
    m_pCore = ICCore::CreateInstance();
    m_pApp = CApp::CreateInstance();
    return m_pCore != nullptr && m_pApp != nullptr && m_pApp->Initialize() == 0;
}

void CApplet::Destroy()
{
    if (m_pApp != nullptr) {
        m_pApp->Destroy();
        delete m_pApp;
        m_pApp = nullptr;
    }
    delete m_pCore;
    m_pCore = nullptr;
}

void CApplet::Start() { QueueSystemEvent(SystemEvent_Start, 0, 0); }
void CApplet::Stop()
{
    QueueSystemEvent(SystemEvent_Stop, 0, 0);
    HandleTimer();
}

void CApplet::HandleTimer()
{
    for (int index = 0; index < m_eventQueue.GetCount(); index++) {
        CSystemEventQueue::SystemEvent event = {};
        m_eventQueue.GetEvent(event, index);
        m_pApp->OnSystemEvent(event.type, event.param1, event.param2);
    }
    m_eventQueue.Clear();
    m_pApp->OnTimer();
}

void CApplet::ExitApp() { ck::running = false; }
void CApplet::QueueSystemEvent(unsigned int type, unsigned int first, unsigned int second)
{
    m_eventQueue.Queue(type, first, second);
}
void CApplet::SetAccelerometerFrequency(unsigned int frequency) { m_accelerometerFrequency = frequency; }
unsigned char CApplet::IsGyroscopeAvailable() const { return 0; }
void CApplet::SetGyroscopeFrequency(unsigned int) {}

namespace {

using namespace ck;

/**
 * True when the test build runs in the background: minimized to the taskbar (it can be closed from
 * there at any time), muted, no dialogs (tests/TestProgram.h). Always false in the game build.
 */
bool IsBackgroundRun()
{
#ifdef CK_TESTS
    return ck::test::RunInBackground();
#else
    return false;
#endif
}

void QueueTouch(CApplet& applet, unsigned int type, unsigned int id, float x, float y, unsigned int taps)
{
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(ck::window, &width, &height);
    if (width <= 0 || height <= 0) {
        return;
    }
    // Map from the game picture, which may have black bars around it (GamePictureRect).
    const SDL_Rect picture = GamePictureRect(width, height);
    const float pictureX = (x - picture.x) * ck::WindowWidth / picture.w;
    const float pictureY = (y - picture.y) * ck::WindowHeight / picture.h;
    const auto gameX = static_cast<unsigned int>(SDL_clamp(pictureX, 0.0f, ck::WindowWidth - 1.0f));
    const auto gameY = static_cast<unsigned int>(SDL_clamp(pictureY, 0.0f, ck::WindowHeight - 1.0f));
    applet.QueueSystemEvent(type, id, ((taps & 3) << 28) | gameX | (gameY << 14));
}

} // namespace

namespace ck {

void RunFrame(CApplet& applet, LoopInput& input)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
            ck::running = false;
            break;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            if (input.pointerDown) {
                applet.QueueSystemEvent(TouchCancelled, 1, 0);
            }
            input.pointerDown = false;
            SetFireHeld(applet, false);
            break;
        case SDL_EVENT_KEY_DOWN:
            if (event.key.repeat) {
                break;
            }
#ifdef CK_TESTS
            // Test keys for automated checks (tests/tests_tools/TestTools.h).
            if (ck::test::HandleTestKey(applet, event.key.key, true)) {
                break;
            }
#endif
            if (event.key.key == SDLK_F9 && ck::config.debugMode) {
                // Debug: dump the next game frame.
                ck::RequestFrameDump();
            } else {
                HandleKeyDown(applet, event.key.key);
                HandleCheatKey(event.key.key);
            }
            break;
        case SDL_EVENT_KEY_UP:
#ifdef CK_TESTS
            ck::test::HandleTestKey(applet, event.key.key, false);
#endif
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            AdjustScopeZoom(event.wheel.y);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button != FireButton) {
                HandleMouseButtonDown(applet, event.button.button);
            } else if (input.mouseCaptured) {
                SetFireHeld(applet, true);
            } else if (!SkipMovie()) {
                // A click during the intro movie only skips it; any other click is a touch.
                input.pointerDown = true;
                QueueTouch(applet, TouchBegan, 1, event.button.x, event.button.y, event.button.clicks);
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button != FireButton) {
                break;
            }
            SetFireHeld(applet, false);
            if (input.pointerDown) {
                input.pointerDown = false;
                QueueTouch(applet, TouchEnded, 1, event.button.x, event.button.y, event.button.clicks);
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
            if (input.mouseCaptured) {
                input.lookX += event.motion.xrel;
                input.lookY += event.motion.yrel;
            } else if (input.pointerDown) {
                QueueTouch(applet, TouchMoved, 1, event.motion.x, event.motion.y, 0);
            }
            break;
        }
    }
    if (!ck::running) {
        return;
    }
    ApplyDesktopFireMode();
    // Capture the mouse exactly while the mission HUD is live; pause, dialogs and menus free it.
    const bool focused = (SDL_GetWindowFlags(ck::window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    const bool missionInput = IsMissionInputActive();
    if (missionInput != input.missionInput) {
        input.missionInput = missionInput;
        fprintf(stderr, "Mission HUD input %s at %llu ms\n", missionInput ? "on" : "off",
                static_cast<unsigned long long>(SDL_GetTicks()));
    }
    const bool wantCapture = focused && missionInput && !ScriptDrivesInput();
    if (wantCapture != input.mouseCaptured) {
        input.mouseCaptured = wantCapture;
        SDL_SetWindowRelativeMouseMode(ck::window, input.mouseCaptured);
        fprintf(stderr, "Mouse %s at %llu ms (focus %d)\n", input.mouseCaptured ? "captured" : "released",
                static_cast<unsigned long long>(SDL_GetTicks()), focused);
        input.lookX = 0.0f;
        input.lookY = 0.0f;
        if (input.mouseCaptured && input.pointerDown) {
            applet.QueueSystemEvent(TouchCancelled, 1, 0);
            input.pointerDown = false;
        }
        if (!input.mouseCaptured) {
            SetFireHeld(applet, false);
            EndViewDrag(applet);
        }
    }
    if (input.mouseCaptured) {
        TurnView(applet, input.lookX, input.lookY);
        input.lookX = 0.0f;
        input.lookY = 0.0f;
    }
    applet.HandleTimer();
#ifdef CK_TESTS
    ck::test::UpdateAutoPlay();
    ck::test::UpdatePickTest();
#endif
    UpdateDebugScript(applet);
}

int RunApplet()
{
    // The GL device requires the view's drawable layer (AppView set it on iOS); the SDL window stands in.
    CApplet::m_eaglLayer = ck::window;
    CApplet applet;
    if (!applet.Initialize()) {
        fprintf(stderr, "CK initialization failed. Check the asset directory and earlier errors.\n");
        return 1;
    }

    ck::running = true;
    applet.Start();
    // Menus: the cursor is free and the left button is the pointer (touch 1).
    // Mission: the mouse is captured, moving it turns the view and the left button fires.
    LoopInput input;
    while (ck::running) {
        RunFrame(applet, input);
        if (ck::running) {
            SDL_Delay(1);
        }
    }
    applet.Stop();
    return 0;
}

} // namespace ck

namespace {

using namespace ck;

/** Check every entry of RequiredFiles (AssetPaths.h); report each missing file and return false if any. */
bool CheckRequiredFiles(const std::filesystem::path& assetsFolder, const std::filesystem::path& executableFolder)
{
    std::string report;
    for (const RequiredFile& file : RequiredFiles) {
        std::filesystem::path folder = assetsFolder;
        if (file.location == FileLocation::ExecutableFolder) {
            folder = executableFolder;
        }
        const std::filesystem::path path = folder / file.name;
        if (std::filesystem::is_regular_file(path)) {
            continue;
        }
        const std::string line = "Missing file: " + path.u8string() + " (" + file.purpose + ")\n";
        fputs(line.c_str(), stderr);
        report += line;
    }
    if (report.empty()) {
        return true;
    }
    // The game has no console unless DebugMode is on, so also say it in a dialog.
    if (!IsBackgroundRun()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, ck::config.title.c_str(), report.c_str(), nullptr);
    }
    return false;
}

/** Create the game window (WindowWidth / WindowHeight / FullScreen / Title of the config) and its GL context. */
bool CreateGameWindow()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_WindowFlags windowFlags = SDL_WINDOW_OPENGL;
    if (IsBackgroundRun()) {
        // Minimized, not hidden: the taskbar button shows the run and closes it. The game draws into
        // its own framebuffer, so nothing depends on the window being visible.
        windowFlags |= SDL_WINDOW_MINIMIZED;
    }
    int windowWidth = ck::config.windowWidth;
    int windowHeight = ck::config.windowHeight;
    if (ck::config.fullScreen) {
        // Borderless full screen at the desktop resolution; the window size is used when leaving it.
        windowFlags |= SDL_WINDOW_FULLSCREEN;
        fprintf(stderr, "Window: full screen\n");
    } else {
        // Use the configured window size, or the largest one of the same shape that fits the desktop.
        SDL_Rect desktop;
        if (SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &desktop)) {
            const int freeHeight = desktop.h - ck::TitleBarAllowance;
            if (windowWidth > desktop.w || windowHeight > freeHeight) {
                const float fitX = static_cast<float>(desktop.w) / windowWidth;
                const float fitY = static_cast<float>(freeHeight) / windowHeight;
                const float fit = SDL_min(fitX, fitY);
                windowWidth = static_cast<int>(windowWidth * fit);
                windowHeight = static_cast<int>(windowHeight * fit);
            }
            fprintf(stderr, "Window %dx%d (wanted %dx%d, desktop %dx%d)\n", windowWidth, windowHeight,
                    ck::config.windowWidth, ck::config.windowHeight, desktop.w, desktop.h);
        }
    }
    ck::window = SDL_CreateWindow(ck::config.title.c_str(), windowWidth, windowHeight, windowFlags);
    if (ck::window != nullptr) {
        if (!ck::config.fullScreen) {
            // SDL centres the picture, which can push the title bar of a tall window above the screen.
            int titleBarHeight = 0;
            int windowX = 0;
            int windowY = 0;
            SDL_GetWindowBordersSize(ck::window, &titleBarHeight, nullptr, nullptr, nullptr);
            SDL_GetWindowPosition(ck::window, &windowX, &windowY);
            if (windowY - titleBarHeight < 0) {
                SDL_SetWindowPosition(ck::window, windowX, titleBarHeight);
            }
        }
        ck::context = SDL_GL_CreateContext(ck::window);
    }
    if (ck::context == nullptr) {
        fprintf(stderr, "Could not create the OpenGL window: %s\n", SDL_GetError());
        SDL_DestroyWindow(ck::window);
        return false;
    }
    // VSync=0 shows the frame rate the game can reach without waiting for the display.
    if (ck::config.vSync) {
        SDL_GL_SetSwapInterval(1);
    } else {
        SDL_GL_SetSwapInterval(0);
    }
    ck::ConfigureDesktopGl();
    return true;
}

/** Everything after the log and the config: check the files, open the window, run the game. */
int RunGame(int argc, wchar_t** argv, const std::filesystem::path& binaryPath)
{
    namespace fs = std::filesystem;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    fs::path assets = binaryPath / ck::AssetsFolderName;
    fs::path saves = binaryPath / ck::SavesFolderName;
    // Optional arguments: asset folder, save folder. Options ("--...", e.g. of the test build) are not paths.
    std::vector<std::wstring> paths;
    for (int index = 1; index < argc; index++) {
        if (wcsncmp(argv[index], L"-", 1) != 0) {
            paths.push_back(argv[index]);
        }
    }
    if (paths.size() >= 1) {
        assets = fs::absolute(paths[0]);
    }
    if (paths.size() >= 2) {
        saves = fs::absolute(paths[1]);
    }
    const std::wstring assetsString = assets.generic_wstring();
    const std::wstring savesString = saves.generic_wstring();
    ck::assetPath = assetsString.c_str();
    ck::savePath = savesString.c_str();
    if (!CheckRequiredFiles(assets, binaryPath)) {
        SDL_Quit();
        return 1;
    }
    fs::create_directories(saves);
    if (!CreateGameWindow()) {
        SDL_Quit();
        return 1;
    }
    int result = 0;
#ifdef CK_TESTS
    // The test build runs the automated tests, or the game with the test tools (--play).
    result = ck::test::RunTestProgram(argc, argv);
#else
    result = RunApplet();
#endif
    SDL_GL_DestroyContext(ck::context);
    SDL_DestroyWindow(ck::window);
    SDL_Quit();
    return result;
}

} // namespace

int wmain(int argc, wchar_t** argv)
{
    // The log comes first so that it holds every line, including the config's own messages.
    ck::StartLog();
    ck::InstallCrashLog();
    const std::filesystem::path binaryPath = ck::ExecutableFolder();
    ck::LoadConfig(binaryPath / ck::ConfigFileName);
#ifdef CK_TESTS
    // Decide the background mode and mute the sound before SDL and OpenAL start.
    ck::test::PrepareTestProcess(argc, argv);
#endif
    if (ck::config.debugMode && !IsBackgroundRun()) {
        ck::OpenLogConsole();
    }
    const int result = RunGame(argc, argv, binaryPath);
    fprintf(stderr, "Exit code %d\n", result);
    ck::StopLogConsole();
    return result;
}
