// ICDebug.h - portable debug output; the body is the platform inline header ICDebug_inl.h.
// (created by ggn for the swerve port layer; ported from CKZ without the namespaces)
// CK callers (ssstdlib.o, swvpngload.o) see only this declaration: they pass the message to an
// out-of-line call, and the single weak copy lives in CDebug_Cocoa_mm.o (N_SOL ICDebug_inl.h).
#ifndef ICDEBUG_H
#define ICDEBUG_H

class ICDebug {
public:
    /** printf-style debug message (body in ICDebug_inl.h). */
    static void LogMessage(const char* format, ...);
};

#endif
