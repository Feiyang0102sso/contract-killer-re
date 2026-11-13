// Debug output of the game itself, switched on in BountyHunter.cfg (Config.h).
// Tools for automated checks live in tests/ and are only compiled into the test build.
#pragma once

#include <SDL3/SDL.h>

class CApplet;
class CSwerveGame;

namespace ck {

/** The running mission once its scene is ready, else nullptr (not connected to CK yet). */
CSwerveGame* ReadyGame();
/**
 * Call once per frame: runs the debug script of the environment variables CK_DUMP_FRAMES
 * (seconds at which a frame is dumped, as F9), CK_TAPS (taps "seconds@x,y" in game pixels) and
 * CK_INPUT (desktop input "seconds@key:R", "@look:dx,dy", "@fire:ms", "@wheel:n", "@rmb").
 */
void UpdateDebugScript(CApplet& applet);
/** True when CK_INPUT drives the controls: the mouse is then never captured (it stays the user's). */
bool ScriptDrivesInput();

} // namespace ck
