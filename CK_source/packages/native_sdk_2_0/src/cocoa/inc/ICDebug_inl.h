// ICDebug_inl.h - iOS inline bodies of ICDebug (created by ggn).
// CK's weak copy (CDebug_Cocoa_mm.o, 20 bytes) only starts and ends the argument list:
// the shipping build logs nothing.
#ifndef ICDEBUG_INL_H
#define ICDEBUG_INL_H

#include <stdarg.h>
#include "ICDebug.h"

inline void ICDebug::LogMessage(const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    va_end(arguments);
}

#endif
