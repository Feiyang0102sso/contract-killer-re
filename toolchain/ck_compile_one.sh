#!/bin/bash
# Compile one CK source file with the fixed toolchain (called by the make rules of build_ck.sh).
# Usage: ck_compile_one.sh <relative source> <object base name>
#
# This file is the only place the compile flags are defined; build_ck.sh hashes it, so any change
# here rebuilds everything. Writes /work/ck_build/obj/<base>.o (via a temporary file, so an
# interrupted compile never leaves a half-written object that make would treat as up to date),
# /work/ck_build/dep/<base>.d and /work/ck_build/log/<base>.log.
#
# Toolchain: the same one as CKZ (determined there, re-checked on CK 2026-11-10): Xcode 4.2.1's
# Apple clang-211.12 and its external cctools `as` (cctools-809), both x86_64 Mach-O programs run
# through maloader. Only armv7 is built; the original's armv6 slice is out of scope.
set -uo pipefail

RELATIVE=$1
BASE=$2

ROOT=/Applications/buildAgent/work/1365410bc639d1a3
SDK=/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.0.sdk
LOADER=/work/maloader_src/maloader/ld-mac
CLANG=/work/xcode421/packages/clang/usr/bin/clang
# External ARM assembler: inside IT blocks it encodes "add Rd, Rd, Rm" with low registers as T1 (0x18xx)
# like the original, where clang's integrated assembler picks T2 (0x44xx)
AS=/work/xcode421/packages/DeveloperToolsCLI/usr/libexec/as/arm/as
BUILD=/work/ck_build
OUT=$BUILD/obj
DEPS=$BUILD/dep
LOGS=$BUILD/log
PREFIX_HEADER=$ROOT/BountyHunter_Prefix.pch

# Same flags as CKZ except the deployment target: CK 1.5.2 has LC_VERSION_MIN_IPHONEOS 3.0
# (code generation is identical to 4.0, checked on Lua/TinyXML).
# -fvisibility=hidden: Xcode "Symbols Hidden by Default"; original Glu symbols are private extern (N_PEXT)
COMMON_FLAGS="-arch armv7 -miphoneos-version-min=3.0 -isysroot $SDK -no-integrated-as -Os -g -fno-strict-aliasing -fvisibility=hidden"
CXX_FLAGS="-x c++ -fno-rtti -fno-exceptions"
OBJCXX_FLAGS="-x objective-c++ -fno-rtti"
OBJC_FLAGS="-x objective-c"
C_FLAGS="-x c"

case "$RELATIVE" in
    *.cpp) LANGUAGE="$CXX_FLAGS" ;;
    *.mm)  LANGUAGE="$OBJCXX_FLAGS" ;;
    *.m)   LANGUAGE="$OBJC_FLAGS" ;;
    *)     LANGUAGE="$C_FLAGS" ;;
esac

# The TinyXML code units use libc / new[] directly, without the Glu NP prefix header (as in CKZ).
PREFIX_FLAGS="-include $PREFIX_HEADER"
case "$RELATIVE" in
    src/tinyxml/tinystr.cpp|src/tinyxml/tinyxml.cpp|src/tinyxml/tinyxmlparser.cpp|src/tinyxml/tinyxmlerror.cpp)
        PREFIX_FLAGS="" ;;
esac

# Xcode header maps make every project header visible by name: -I for each header directory
# (computed once per build by build_ck.sh)
INCLUDE_FLAGS=$(cat "$BUILD/include_flags")

TEMPORARY="$OUT/$BASE.o.tmp"
ASSEMBLY="$OUT/$BASE.s.tmp"
cd "$ROOT" || exit 1
# The driver would exec itself for -cc1, which maloader cannot do: ask it for the -cc1 command line
# with -### and run that line through maloader directly. cc1 writes assembly (-S); the external `as`
# then assembles it exactly as the driver would ("as -arch armv7 -o <object> <assembly>").
CC1_LINE=$($LOADER $CLANG $COMMON_FLAGS $LANGUAGE $PREFIX_FLAGS $INCLUDE_FLAGS -MMD -MF "$DEPS/$BASE.d" -MT "$OUT/$BASE.o" \
        -S "$ROOT/$RELATIVE" -o "$ASSEMBLY" -### 2>&1 | grep '"-cc1"')
COMPILED=0
if [ -n "$CC1_LINE" ] && eval "$LOADER $CC1_LINE" > "$LOGS/$BASE.log" 2>&1; then
    rm -f "$TEMPORARY"
    # maloader prints a harmless "sys_errlist/sys_nerr: undefined symbol" note for `as`; keep the rest
    $LOADER $AS -arch armv7 -o "$TEMPORARY" "$ASSEMBLY" 2>&1 | grep -v "sys_errlist\|sys_nerr" >> "$LOGS/$BASE.log"
    if [ -s "$TEMPORARY" ]; then
        COMPILED=1
    fi
fi
rm -f "$ASSEMBLY"
if [ "$COMPILED" = 1 ]; then
    mv -f "$TEMPORARY" "$OUT/$BASE.o"
    echo "ok   $RELATIVE"
else
    rm -f "$TEMPORARY"
    echo "FAIL $RELATIVE (log $LOGS/$BASE.log)"
    exit 1
fi
