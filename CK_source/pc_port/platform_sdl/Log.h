// Log file: everything written to stdout and stderr goes to <executable name>.log next to the
// executable (BountyHunter.log), with or without the debug console. The console (DebugMode=1) only
// shows what the log file receives, so both always hold the same text.
#pragma once

#include <filesystem>

namespace ck {

/** Folder of the executable. */
std::filesystem::path ExecutableFolder();

/** Start a new log file and send stdout and stderr to it; call first thing at startup. */
void StartLog();

/** Write the call stack of a crash to the log (symbols from the PDB next to the executable). */
void InstallCrashLog();

/** Open a console window that follows the log file (DebugMode=1). */
void OpenLogConsole();

/** Show the last log lines in the console and stop following; call before the process ends. */
void StopLogConsole();

} // namespace ck
