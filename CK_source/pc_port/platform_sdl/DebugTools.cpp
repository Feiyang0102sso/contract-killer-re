// Debug script of the game itself (DebugTools.h): timed frame dumps, taps and desktop input from
// environment variables, so a run can be checked without anyone at the keyboard.
#include "DebugTools.h"
#include "Host.h"
#include "CApplet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

namespace ck {
namespace {

// Comma separated seconds after start at which a frame is dumped (as F9), e.g. "10,20,35".
constexpr char FrameDumpVariable[] = "CK_DUMP_FRAMES";
// Semicolon separated taps "seconds@x,y" in game pixels (1024 x 768), e.g. "12@512,700;20@100,200".
constexpr char TapVariable[] = "CK_TAPS";
// Semicolon separated desktop input "seconds@what", given to the controls as if the mouse were
// captured: key:<SDL key name>, look:<dx>,<dy> (mouse counts), fire:<held ms>, wheel:<notches>, rmb.
constexpr char InputVariable[] = "CK_INPUT";

struct ScriptedTap {
    Uint64 timeMs;
    unsigned int x;
    unsigned int y;
};

struct ScriptedInput {
    Uint64 timeMs;
    std::string what;
};

std::vector<Uint64> frameDumpTimes;
size_t nextFrameDump;
std::vector<ScriptedTap> taps;
size_t nextTap;
bool tapHeld;          // the touch of the current tap went down in the last frame
std::vector<ScriptedInput> inputs;
size_t nextInput;
Uint64 fireReleaseMs;  // 0: the scripted fire button is not held
bool scriptRead;

/** Split `text` at `separator`. */
std::vector<std::string> Split(const std::string& text, char separator)
{
    std::vector<std::string> parts;
    size_t start = 0;
    while (start < text.size()) {
        size_t end = text.find(separator, start);
        if (end == std::string::npos) {
            end = text.size();
        }
        parts.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    return parts;
}

/** Read the script variables once. */
void ReadScript()
{
    scriptRead = true;
    const char* dumps = getenv(FrameDumpVariable);
    if (dumps != nullptr) {
        for (const std::string& part : Split(dumps, ',')) {
            frameDumpTimes.push_back(static_cast<Uint64>(atof(part.c_str()) * 1000.0));
        }
        fprintf(stderr, "Debug script: %zu frame dumps\n", frameDumpTimes.size());
    }
    const char* tapText = getenv(TapVariable);
    if (tapText != nullptr) {
        for (const std::string& part : Split(tapText, ';')) {
            ScriptedTap tap = {};
            float seconds = 0.0f;
            if (sscanf(part.c_str(), "%f@%u,%u", &seconds, &tap.x, &tap.y) == 3) {
                tap.timeMs = static_cast<Uint64>(seconds * 1000.0f);
                taps.push_back(tap);
            }
        }
        fprintf(stderr, "Debug script: %zu taps\n", taps.size());
    }
    const char* inputText = getenv(InputVariable);
    if (inputText != nullptr) {
        for (const std::string& part : Split(inputText, ';')) {
            const size_t at = part.find('@');
            if (at == std::string::npos) {
                continue;
            }
            ScriptedInput input;
            input.timeMs = static_cast<Uint64>(atof(part.substr(0, at).c_str()) * 1000.0);
            input.what = part.substr(at + 1);
            inputs.push_back(input);
        }
        fprintf(stderr, "Debug script: %zu inputs\n", inputs.size());
    }
}

/** Queue a touch of the game at game pixel (x, y), as AppView did for a finger. */
void QueueGameTouch(CApplet& applet, unsigned int type, unsigned int x, unsigned int y)
{
    applet.QueueSystemEvent(type, 1, (1u << 28) | (x & 0x3fff) | ((y & 0x3fff) << 14));
}

/** Give one scripted input to the controls. */
void RunInput(CApplet& applet, const std::string& what, Uint64 now)
{
    fprintf(stderr, "Debug script: input %s\n", what.c_str());
    if (what.rfind("key:", 0) == 0) {
        // As a real key press: the controls and the cheat codes both see it.
        const SDL_Keycode key = SDL_GetKeyFromName(what.substr(4).c_str());
        HandleKeyDown(applet, key);
        HandleCheatKey(key);
    } else if (what.rfind("look:", 0) == 0) {
        float x = 0.0f;
        float y = 0.0f;
        sscanf(what.c_str() + 5, "%f,%f", &x, &y);
        TurnView(applet, x, y);
    } else if (what.rfind("fire:", 0) == 0) {
        SetFireHeld(applet, true);
        fireReleaseMs = now + static_cast<Uint64>(atoi(what.c_str() + 5));
    } else if (what.rfind("wheel:", 0) == 0) {
        AdjustScopeZoom(static_cast<float>(atof(what.c_str() + 6)));
    } else if (what == "rmb") {
        HandleMouseButtonDown(applet, SDL_BUTTON_RIGHT);
    }
}

} // namespace

// The unit count log of CKZ is not connected to CK.
CSwerveGame* ReadyGame() { return nullptr; }

bool ScriptDrivesInput()
{
    if (!scriptRead) {
        ReadScript();
    }
    return !inputs.empty();
}

void UpdateDebugScript(CApplet& applet)
{
    if (!scriptRead) {
        ReadScript();
    }
    const Uint64 now = SDL_GetTicks();
    if (nextFrameDump < frameDumpTimes.size() && now >= frameDumpTimes[nextFrameDump]) {
        nextFrameDump++;
        RequestFrameDump();
    }
    if (tapHeld) {
        // Lift the finger one frame after it went down.
        const ScriptedTap& tap = taps[nextTap - 1];
        QueueGameTouch(applet, TouchEnded, tap.x, tap.y);
        tapHeld = false;
    } else if (nextTap < taps.size() && now >= taps[nextTap].timeMs) {
        const ScriptedTap& tap = taps[nextTap];
        fprintf(stderr, "Debug script: tap %u,%u\n", tap.x, tap.y);
        QueueGameTouch(applet, TouchBegan, tap.x, tap.y);
        nextTap++;
        tapHeld = true;
    }
    if (fireReleaseMs != 0 && now >= fireReleaseMs) {
        SetFireHeld(applet, false);
        fireReleaseMs = 0;
    }
    while (nextInput < inputs.size() && now >= inputs[nextInput].timeMs) {
        RunInput(applet, inputs[nextInput].what, now);
        nextInput++;
    }
}

} // namespace ck
