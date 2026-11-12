// CStdUtil_Cocoa.h - iOS implementation of the standard utility services (time, wide strings, printf).
// Implemented in CStdUtil_Cocoa.cpp (time, compare, printf) and CStdUtil_Cocoa_mm.mm (case mapping via NSString).
#ifndef CSTDUTIL_COCOA_H
#define CSTDUTIL_COCOA_H

#include <wchar.h>

#include "CSingleton.h"

/** Broken-down local time (fields of struct tm the engine keeps). */
struct CStdTime {
    int m_second;       // 0x00
    int m_minute;       // 0x04
    int m_hour;         // 0x08
    int m_dayOfMonth;   // 0x0c
    int m_month;        // 0x10 (0..11)
    int m_year;         // 0x14 (years since 1900)
    int m_dayOfWeek;    // 0x18
    int m_isDst;        // 0x1c
};

/** Singleton holding the buffer LocalTime returns (0x28 bytes). */
class CStdTimeManager : public CSingleton {
public:
    CStdTimeManager()
        : CSingleton(0xad3a63d9)
    {
    }

    CStdTime m_time;    // 0x08
};

class CStdUtil_Cocoa {
public:
    /** Wall clock seconds since the epoch (gettimeofday tv_sec). */
    static long GetTimeSeconds();
    /** Milliseconds since the first call (mach_absolute_time based, monotonic). */
    static int GetUpTimeMS();
    /** Case-insensitive wide string compare (ASCII letters only); 0 when equal, -1 / 1 otherwise. */
    static int WcsICmp(const wchar_t* left, const wchar_t* right);
    /** vsprintf into `buffer`; `arguments` is the va_list. */
    static int VSPrintF(char* buffer, const char* format, void* arguments);
    /** Bounded vsprintf: writes at most `bufferSize - 1` characters plus the terminator. */
    static int VSPrintF_S(char* buffer, int bufferSize, const char* format, void* arguments);
    /** Wide vsprintf. */
    static int VSWPrintF(wchar_t* buffer, const wchar_t* format, void* arguments);
    /** Local time of `seconds` in the shared CStdTimeManager buffer; NULL when localtime fails. */
    static CStdTime* LocalTime(unsigned int seconds);
    /** Upper case of one character (NSString uppercaseString). */
    static wchar_t ToWUpper(wchar_t character);
    /** Lower case of one character (NSString lowercaseString). */
    static wchar_t ToWLower(wchar_t character);
};

#endif
