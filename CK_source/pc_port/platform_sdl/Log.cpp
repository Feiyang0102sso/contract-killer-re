// Log file and debug console, see Log.h.
// stdout and stderr are reopened on the log file, unbuffered, so every line is on disk at once and
// survives a crash. The console is fed by a thread that copies what the log file gains.
#include "Log.h"
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>

namespace ck {
namespace {

// How often the console thread looks for new log text.
constexpr DWORD ConsolePollMs = 50;

std::filesystem::path logPath;
HANDLE logReader = INVALID_HANDLE_VALUE;
HANDLE consoleWriter = INVALID_HANDLE_VALUE;
HANDLE consoleThread = nullptr;
volatile bool consoleRunning = false;

/** Copy everything the log file gained since the last call to the console. */
void CopyNewLogText()
{
    char buffer[4096];
    DWORD read = 0;
    while (ReadFile(logReader, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
        DWORD written = 0;
        WriteFile(consoleWriter, buffer, read, &written, nullptr);
    }
}

DWORD WINAPI FollowLog(void*)
{
    while (consoleRunning) {
        CopyNewLogText();
        Sleep(ConsolePollMs);
    }
    return 0;
}

// Frames of a crash call stack written to the log.
constexpr int CrashStackFrames = 32;

/** Write the crash and its call stack (function, file and line from the PDB) to the log. */
LONG WINAPI LogCrash(EXCEPTION_POINTERS* exception)
{
    const EXCEPTION_RECORD* record = exception->ExceptionRecord;
    fprintf(stderr, "Crash: exception %08lx at %p", record->ExceptionCode, record->ExceptionAddress);
    if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2) {
        fprintf(stderr, " (%s %08lx)", record->ExceptionInformation[0] != 0 ? "writing" : "reading",
                static_cast<unsigned long>(record->ExceptionInformation[1]));
    }
    fprintf(stderr, "\n");

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    SymInitialize(process, nullptr, TRUE);
    CONTEXT context = *exception->ContextRecord;
    STACKFRAME64 frame = {};
    frame.AddrPC.Offset = context.Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrStack.Mode = AddrModeFlat;
    for (int index = 0; index < CrashStackFrames; index++) {
        if (!StackWalk64(IMAGE_FILE_MACHINE_I386, process, thread, &frame, &context, nullptr,
                         SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
            break;
        }
        const DWORD64 address = frame.AddrPC.Offset;
        if (address == 0) {
            break;
        }
        char buffer[sizeof(SYMBOL_INFO) + 256] = {};
        auto* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 255;
        DWORD64 displacement = 0;
        const char* name = "?";
        if (SymFromAddr(process, address, &displacement, symbol)) {
            name = symbol->Name;
        }
        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(line);
        DWORD lineDisplacement = 0;
        if (SymGetLineFromAddr64(process, address, &lineDisplacement, &line)) {
            fprintf(stderr, "  #%02d %08llx %s (%s:%lu)\n", index, address, name, line.FileName, line.LineNumber);
        } else {
            fprintf(stderr, "  #%02d %08llx %s\n", index, address, name);
        }
    }
    SymCleanup(process);
    return EXCEPTION_CONTINUE_SEARCH;
}

} // namespace

void InstallCrashLog()
{
    SetUnhandledExceptionFilter(LogCrash);
}

std::filesystem::path ExecutableFolder()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
}

void StartLog()
{
    wchar_t executable[MAX_PATH];
    GetModuleFileNameW(nullptr, executable, MAX_PATH);
    logPath = std::filesystem::path(executable).replace_extension(L".log");
    // Empty the file, then append from both streams so their lines stay in order.
    FILE* file = _wfopen(logPath.c_str(), L"w");
    if (file != nullptr) {
        fclose(file);
    }
    _wfreopen(logPath.c_str(), L"a", stdout);
    _wfreopen(logPath.c_str(), L"a", stderr);
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    SYSTEMTIME now;
    GetLocalTime(&now);
    fprintf(stderr, "Log started %04d-%02d-%02d %02d:%02d:%02d\n", now.wYear, now.wMonth, now.wDay, now.wHour,
            now.wMinute, now.wSecond);
}

void OpenLogConsole()
{
    if (!AllocConsole()) {
        fprintf(stderr, "Could not open the debug console (Win32 error %lu)\n", GetLastError());
        return;
    }
    // The log holds UTF-8 file names.
    SetConsoleOutputCP(CP_UTF8);
    consoleWriter = CreateFileW(L"CONOUT$", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                OPEN_EXISTING, 0, nullptr);
    logReader = CreateFileW(logPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            nullptr, OPEN_EXISTING, 0, nullptr);
    if (consoleWriter == INVALID_HANDLE_VALUE || logReader == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Could not connect the debug console to the log (Win32 error %lu)\n", GetLastError());
        return;
    }
    consoleRunning = true;
    consoleThread = CreateThread(nullptr, 0, FollowLog, nullptr, 0, nullptr);
}

void StopLogConsole()
{
    if (consoleThread == nullptr) {
        return;
    }
    consoleRunning = false;
    WaitForSingleObject(consoleThread, INFINITE);
    CloseHandle(consoleThread);
    consoleThread = nullptr;
    CopyNewLogText();
}

} // namespace ck
