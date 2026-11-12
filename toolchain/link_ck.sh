#!/bin/bash
# Link the CK objects into the armv7 Mach-O executable BountyHunter.
#
# Needs an ld64 that runs on Linux: Xcode 4.2.1's ld (ld64-127.2, x86_64 Mach-O) through maloader,
# both prepared by setup_distro.sh. Override with LD64 (the ld binary) and LD64_LOADER (maloader's
# ld-mac, may be empty for a native Linux ld64).
# Output: /work/ck_build/link/BountyHunter, copied to $CK_OUTPUT_DIR (default: <repo>/_output).
#
# The command line is the one clang-211.12's driver produces for
#   clang -arch armv7 -miphoneos-version-min=3.0 -isysroot $SDK -dead_strip ... -lstdc++
# (checked with -###: for a 3.0 target it links crt1.o instead of crt1.3.1.o and adds libclang_rt.ios.a),
# plus the frameworks/dylibs of the original LC_LOAD_DYLIB / LC_LOAD_WEAK_DYLIB commands, in the
# original order. Only armv7 is linked; the original's armv6 slice is out of scope.
set -euo pipefail

TOOLCHAIN=$(cd "$(dirname "$0")" && pwd)
REPO=$(dirname "$TOOLCHAIN")
# ld lives next to ../lib/libLTO.dylib in this merged directory (setup_distro.sh)
LD64=${LD64:-/work/ck_build/ld64_root/usr/bin/ld}
LD64_LOADER=${LD64_LOADER-/work/maloader_src/maloader/ld-mac}
SDK=/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.0.sdk
CLANG_RUNTIME=/work/xcode421/packages/clang/usr/lib/clang/3.0/lib/darwin/libclang_rt.ios.a
OBJECTS=/work/ck_build/obj
OUT_DIR=/work/ck_build/link
OUTPUT=$OUT_DIR/BountyHunter
# lookup.c of libvorbis is only #included by lsp.c (same as CKZ)
EXCLUDED_OBJECTS="lookup.o"
# original object order (N_OSO order of the STABS debug map)
LINK_ORDER=$TOOLCHAIN/link_order.txt
# final products are copied here after every successful link
FINAL_DIR=${CK_OUTPUT_DIR:-$REPO/_output}

# Original dynamic libraries of the armv7 slice, in LC_LOAD_DYLIB order
LIBRARIES=(
    "framework MessageUI"
    "weak_framework AVFoundation"
    "framework AudioToolbox"
    "framework StoreKit"
    "framework AddressBook"
    "weak_framework CFNetwork"
    "weak_framework CoreGraphics"
    "weak_framework CoreLocation"
    "framework Foundation"
    "framework MobileCoreServices"
    "framework QuartzCore"
    "framework SystemConfiguration"
    "weak_framework UIKit"
    "framework OpenGLES"
    "framework CoreFoundation"
    "weak_framework MediaPlayer"
    "framework OpenAL"
    "library sqlite3"
    "weak_framework CoreTelephony"
    "framework Security"
    "framework AddressBookUI"
    "library z.1"
    "weak_framework GameKit"
    "weak_framework EventKit"
    "weak_framework EventKitUI"
    "weak_framework iAd"
    "weak_library System"
    "weak_framework CoreMotion"
    "weak_framework MapKit"
)

mkdir -p "$OUT_DIR"
ARGUMENTS=(-dynamic -arch armv7 -dead_strip -iphoneos_version_min 3.0.0 -syslibroot "$SDK" -o "$OUTPUT" -lcrt1.o)

# objects: first in the original link order, then the rest (objects the original only had inside
# prebuilt libraries, stubs, ...) in name order
declare -A ADDED
OBJECT_COUNT=0
ORDERED_COUNT=0
add_object() {
    local name=$1
    local excluded
    for excluded in $EXCLUDED_OBJECTS; do
        if [ "$name" = "$excluded" ]; then
            return
        fi
    done
    if [ -n "${ADDED[$name]:-}" ] || [ ! -f "$OBJECTS/$name" ]; then
        return
    fi
    ADDED[$name]=1
    ARGUMENTS+=("$OBJECTS/$name")
    OBJECT_COUNT=$((OBJECT_COUNT + 1))
}
while read -r name; do
    case "$name" in
        ""|"#"*) continue ;;
    esac
    add_object "$name"
done < "$LINK_ORDER"
ORDERED_COUNT=$OBJECT_COUNT
for path in "$OBJECTS"/*.o; do
    add_object "$(basename "$path")"
done
echo "objects in original order: $ORDERED_COUNT, appended: $((OBJECT_COUNT - ORDERED_COUNT))"

# frameworks and dylibs
for entry in "${LIBRARIES[@]}"; do
    kind=${entry%% *}
    name=${entry#* }
    case "$kind" in
        framework)      ARGUMENTS+=(-framework "$name") ;;
        weak_framework) ARGUMENTS+=(-weak_framework "$name") ;;
        library)        ARGUMENTS+=("-l$name") ;;
        weak_library)   ARGUMENTS+=("-weak-l$name") ;;
    esac
done
# the driver's default libraries (C++ runtime, libSystem, libgcc_s, clang runtime) and libobjc used by
# the ObjC code; libSystem is already listed above as weak, ld ignores the repeat
ARGUMENTS+=(-lstdc++ -lSystem -lgcc_s.1 -lobjc "$CLANG_RUNTIME")

echo "link: $OBJECT_COUNT objects -> $OUTPUT"
printf '%s\n' "${ARGUMENTS[@]}" > "$OUT_DIR/link.args"
STATUS=0
$LD64_LOADER "$LD64" "${ARGUMENTS[@]}" > "$OUT_DIR/link.log" 2>&1 || STATUS=$?
echo "ld64 exit $STATUS (log $OUT_DIR/link.log)"
if [ $STATUS -eq 0 ]; then
    mkdir -p "$FINAL_DIR"
    cp -f "$OUTPUT" "$OUT_DIR/link.args" "$OUT_DIR/link.log" "$FINAL_DIR/"
    (cd "$FINAL_DIR" && sha256sum BountyHunter > BountyHunter.sha256)
    echo "copied to $FINAL_DIR: $(cat "$FINAL_DIR/BountyHunter.sha256")"
else
    # while sources are missing the link fails with undefined symbols; show how many
    undefined=$(grep -c '^  "' "$OUT_DIR/link.log" || true)
    echo "undefined symbols: $undefined"
fi
exit $STATUS
